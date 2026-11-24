// Ribbon3D GPU - GPU device setup and rendering for OpenCL, CUDA and Metal.
//
// The CPU builds the packet (mesh, projected triangles, tile bins, lights); the GPU runs the
// per-pixel kernel (raster + supersampling + shading + compositing) on AE's GPU worlds.
// Frameworks this build does not support are left to AE, which then calls the CPU path.
#include "Ribbon3D_GPU.h"

#include <cstring>
#include <mutex>
#include <vector>

#include "Ribbon3D_Suite.h"

#ifdef RB_HAS_OPENCL
    #define CL_TARGET_OPENCL_VERSION 120
    #ifdef __APPLE__
        #include <OpenCL/cl.h>
    #else
        #include <CL/cl.h>
    #endif
    #include "RibbonKernelSource.h"
#endif

#ifdef RB_HAS_CUDA
    #include <cuda_runtime.h>
extern "C" int RibbonRender_CUDA(const void* P, const void* tris, const void* verts, const void* tileStart,
                                 const void* tileTris, const void* lights, const void* tex, const void* src, void* dst,
                                 int width, int height, void* stream);
#endif

#ifdef RB_HAS_METAL
    #include "Ribbon3D_Metal.h"
#endif

namespace {

// Per-device state; the AE handle stores a pointer to it.
struct GPUState {
    PF_GPU_Framework framework = PF_GPU_Framework_NONE;
    std::mutex lock;  // OpenCL kernel arguments are not thread safe (multi-frame rendering)
#ifdef RB_HAS_OPENCL
    cl_program program = nullptr;
    cl_kernel kernel = nullptr;
#endif
    void* metalPipeline = nullptr;
};

GPUState* StateFromHandle(const void* gpu_data) {
    PF_Handle h = static_cast<PF_Handle>(const_cast<void*>(gpu_data));
    return (h && *h) ? *reinterpret_cast<GPUState**>(*h) : nullptr;
}

#ifdef RB_HAS_OPENCL
bool SetupOpenCL(const PF_GPUDeviceInfo& info, GPUState& st) {
    cl_context ctx = static_cast<cl_context>(info.contextPV);
    cl_device_id device = static_cast<cl_device_id>(info.devicePV);
    std::vector<size_t> sizes;
    for (int i = 0; i < kRibbonKernelCLCount; i++) sizes.push_back(std::strlen(kRibbonKernelCL[i]));
    cl_int res = CL_SUCCESS;
    st.program = clCreateProgramWithSource(ctx, cl_uint(kRibbonKernelCLCount), const_cast<const char**>(kRibbonKernelCL),
                                           sizes.data(), &res);
    if (res != CL_SUCCESS || !st.program) return false;
    if (clBuildProgram(st.program, 1, &device, "-cl-single-precision-constant -cl-mad-enable", nullptr, nullptr) != CL_SUCCESS)
        return false;
    st.kernel = clCreateKernel(st.program, "RibbonRender", &res);
    return res == CL_SUCCESS && st.kernel;
}

void ReleaseOpenCL(GPUState& st) {
    if (st.kernel) clReleaseKernel(st.kernel);
    if (st.program) clReleaseProgram(st.program);
    st.kernel = nullptr;
    st.program = nullptr;
}

PF_Err RenderOpenCL(const PF_GPUDeviceInfo& info, GPUState& st, const rb::Packet& pk, void* src, void* dst, void* tex) {
    cl_context ctx = static_cast<cl_context>(info.contextPV);
    cl_command_queue queue = static_cast<cl_command_queue>(info.command_queuePV);
    cl_int res = CL_SUCCESS;
    auto upload = [&](const void* data, size_t bytes) {
        cl_int e = CL_SUCCESS;
        cl_mem m = clCreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, bytes, const_cast<void*>(data), &e);
        if (e != CL_SUCCESS) res = e;
        return m;
    };
    cl_mem bufs[6] = {
        upload(&pk.params, sizeof(RK_Params)),
        upload(pk.tris.data(), pk.tris.size() * sizeof(RK_Tri)),
        upload(pk.verts.data(), pk.verts.size() * sizeof(RK_Vertex)),
        upload(pk.tileStart.data(), pk.tileStart.size() * sizeof(int)),
        upload(pk.tileTris.data(), pk.tileTris.size() * sizeof(int)),
        upload(pk.lights.data(), pk.lights.size() * sizeof(RK_Light)),
    };
    cl_mem texMem = static_cast<cl_mem>(tex), srcMem = static_cast<cl_mem>(src), dstMem = static_cast<cl_mem>(dst);
    if (res == CL_SUCCESS) {
        std::lock_guard<std::mutex> guard(st.lock);
        for (cl_uint i = 0; i < 6 && res == CL_SUCCESS; i++) res = clSetKernelArg(st.kernel, i, sizeof(cl_mem), &bufs[i]);
        if (res == CL_SUCCESS) res = clSetKernelArg(st.kernel, 6, sizeof(cl_mem), &texMem);
        if (res == CL_SUCCESS) res = clSetKernelArg(st.kernel, 7, sizeof(cl_mem), &srcMem);
        if (res == CL_SUCCESS) res = clSetKernelArg(st.kernel, 8, sizeof(cl_mem), &dstMem);
        size_t local[2] = {16, 16};
        size_t global[2] = {size_t((pk.params.width + 15) / 16) * 16, size_t((pk.params.height + 15) / 16) * 16};
        if (res == CL_SUCCESS) res = clEnqueueNDRangeKernel(queue, st.kernel, 2, nullptr, global, local, 0, nullptr, nullptr);
    }
    for (cl_mem m : bufs)
        if (m) clReleaseMemObject(m);  // released after the kernel completes
    return res == CL_SUCCESS ? PF_Err_NONE : PF_Err_INTERNAL_STRUCT_DAMAGED;
}
#endif

#ifdef RB_HAS_CUDA
PF_Err RenderCUDA(const rb::Packet& pk, void* src, void* dst, void* tex) {
    const void* host[6] = {&pk.params, pk.tris.data(), pk.verts.data(), pk.tileStart.data(), pk.tileTris.data(), pk.lights.data()};
    const size_t bytes[6] = {sizeof(RK_Params), pk.tris.size() * sizeof(RK_Tri), pk.verts.size() * sizeof(RK_Vertex),
                             pk.tileStart.size() * sizeof(int), pk.tileTris.size() * sizeof(int), pk.lights.size() * sizeof(RK_Light)};
    void* dev[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    cudaError_t e = cudaSuccess;
    for (int i = 0; i < 6 && e == cudaSuccess; i++) {
        e = cudaMalloc(&dev[i], bytes[i]);
        if (e == cudaSuccess) e = cudaMemcpy(dev[i], host[i], bytes[i], cudaMemcpyHostToDevice);
    }
    if (e == cudaSuccess)
        e = cudaError_t(RibbonRender_CUDA(dev[0], dev[1], dev[2], dev[3], dev[4], dev[5], tex, src, dst, pk.params.width,
                                          pk.params.height, nullptr));
    for (void* p : dev)
        if (p) cudaFree(p);  // waits for the kernel
    return e == cudaSuccess ? PF_Err_NONE : PF_Err_INTERNAL_STRUCT_DAMAGED;
}
#endif

}  // namespace

bool RibbonGPU_Supports(PF_GPU_Framework fw) {
#ifdef RB_HAS_OPENCL
    if (fw == PF_GPU_Framework_OPENCL) return true;
#endif
#ifdef RB_HAS_CUDA
    if (fw == PF_GPU_Framework_CUDA) return true;
#endif
#ifdef RB_HAS_METAL
    if (fw == PF_GPU_Framework_METAL) return true;
#endif
    (void)fw;
    return false;
}

PF_Err RibbonGPU_DeviceSetup(PF_InData* in_data, PF_OutData* out_data, PF_GPUDeviceSetupExtra* extra) {
    const PF_GPU_Framework fw = extra->input->what_gpu;
    if (!RibbonGPU_Supports(fw)) return PF_Err_NONE;  // no flag: AE renders this effect on the CPU
    Suite<PF_HandleSuite1> handles(in_data, kPFHandleSuite, kPFHandleSuiteVersion1);
    Suite<PF_GPUDeviceSuite1> gpu(in_data, kPFGPUDeviceSuite, kPFGPUDeviceSuiteVersion1);
    if (!handles || !gpu) return PF_Err_NONE;
    PF_GPUDeviceInfo info;
    AEFX_CLR_STRUCT(info);
    if (gpu->GetDeviceInfo(in_data->effect_ref, extra->input->device_index, &info)) return PF_Err_NONE;

    GPUState* st = new GPUState();
    st->framework = fw;
    bool ok = false;
#ifdef RB_HAS_CUDA
    if (fw == PF_GPU_Framework_CUDA) ok = true;  // kernel is linked into the plugin
#endif
#ifdef RB_HAS_OPENCL
    if (fw == PF_GPU_Framework_OPENCL) {
        ok = SetupOpenCL(info, *st);
        if (!ok) ReleaseOpenCL(*st);
    }
#endif
#ifdef RB_HAS_METAL
    if (fw == PF_GPU_Framework_METAL) ok = RibbonMetal_Setup(info.devicePV, &st->metalPipeline);
#endif
    if (!ok) {
        delete st;
        return PF_Err_NONE;
    }
    PF_Handle h = handles->host_new_handle(sizeof(GPUState*));
    if (!h) {
        delete st;
        return PF_Err_OUT_OF_MEMORY;
    }
    *reinterpret_cast<GPUState**>(*h) = st;
    extra->output->gpu_data = h;
    out_data->out_flags2 = PF_OutFlag2_SUPPORTS_GPU_RENDER_F32;
    return PF_Err_NONE;
}

PF_Err RibbonGPU_DeviceSetdown(PF_InData* in_data, PF_OutData* out_data, PF_GPUDeviceSetdownExtra* extra) {
    (void)out_data;
    PF_Handle h = static_cast<PF_Handle>(const_cast<void*>(extra->input->gpu_data));
    GPUState* st = StateFromHandle(extra->input->gpu_data);
    if (st) {
#ifdef RB_HAS_OPENCL
        ReleaseOpenCL(*st);
#endif
#ifdef RB_HAS_METAL
        RibbonMetal_Release(st->metalPipeline);
#endif
        delete st;
    }
    if (h) {
        Suite<PF_HandleSuite1> handles(in_data, kPFHandleSuite, kPFHandleSuiteVersion1);
        if (handles) handles->host_dispose_handle(h);
    }
    return PF_Err_NONE;
}

PF_Err RibbonGPU_Render(PF_InData* in_data, PF_SmartRenderExtra* extra, rb::Packet& pk, int srcX, int srcY,
                        PF_EffectWorld* src, PF_EffectWorld* dst, PF_EffectWorld* tex) {
    GPUState* st = StateFromHandle(extra->input->gpu_data);
    Suite<PF_GPUDeviceSuite1> gpu(in_data, kPFGPUDeviceSuite, kPFGPUDeviceSuiteVersion1);
    if (!st || !gpu || !dst) return PF_Err_INTERNAL_STRUCT_DAMAGED;
    PF_GPUDeviceInfo info;
    AEFX_CLR_STRUCT(info);
    PF_Err err = gpu->GetDeviceInfo(in_data->effect_ref, extra->input->device_index, &info);
    void *srcMem = nullptr, *dstMem = nullptr, *texMem = nullptr;
    if (!err) err = gpu->GetGPUWorldData(in_data->effect_ref, dst, &dstMem);
    if (!err && src) err = gpu->GetGPUWorldData(in_data->effect_ref, src, &srcMem);
    if (!err && tex) err = gpu->GetGPUWorldData(in_data->effect_ref, tex, &texMem);
    if (err) return err;

    // GPU worlds are 32-bit float BGRA; pitches in pixels.
    const int bpp = int(sizeof(PF_PixelFloat));
    if (srcMem) rb::setSource(pk, srcX, srcY, src->width, src->height, int(src->rowbytes / bpp));
    else rb::setSource(pk, 0, 0, 0, 0, 1);
    pk.params.dstPitch = int(dst->rowbytes / bpp);
    if (texMem) rb::setTexture(pk, tex->width, tex->height, int(tex->rowbytes / bpp));
    // Unused inputs still need a valid buffer: the kernel never reads them (srcW = 0 / no texture).
    if (!srcMem) srcMem = dstMem;
    if (!texMem) texMem = dstMem;

    switch (st->framework) {
#ifdef RB_HAS_OPENCL
        case PF_GPU_Framework_OPENCL: return RenderOpenCL(info, *st, pk, srcMem, dstMem, texMem);
#endif
#ifdef RB_HAS_CUDA
        case PF_GPU_Framework_CUDA: return RenderCUDA(pk, srcMem, dstMem, texMem);
#endif
#ifdef RB_HAS_METAL
        case PF_GPU_Framework_METAL:
            return RibbonMetal_Render(st->metalPipeline, info.devicePV, info.command_queuePV, pk, srcMem, dstMem, texMem)
                       ? PF_Err_NONE
                       : PF_Err_INTERNAL_STRUCT_DAMAGED;
#endif
        default: return PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
}
