// Ribbon3D GPU - GPU device setup and rendering for OpenCL, CUDA and Metal.
//
// The CPU builds the packet (mesh, projected triangles, tile bins, lights); the GPU runs the
// per-pixel kernel (raster + supersampling + shading + compositing) on AE's GPU worlds.
//
// OpenCL and CUDA are bound at run time (Ribbon3D_GPULibs): the plugin loads on any machine and
// needs no SDK to build. OpenCL compiles the embedded kernel source, CUDA loads embedded PTX
// through the driver API. Frameworks that are unavailable are left to AE, which then calls the
// CPU path of this effect.
#include "Ribbon3D_GPU.h"

#include <cstring>
#include <mutex>
#include <vector>

#include "Ribbon3D_GPULibs.h"
#include "Ribbon3D_Suite.h"

#ifdef RB_HAS_OPENCL
    #include "RibbonKernelSource.h"
#endif
#ifdef RB_HAS_CUDA
    #include "kernel/RibbonKernelPTX.h"
#endif
#ifdef RB_HAS_METAL
    #include "Ribbon3D_Metal.h"
#endif

using namespace rbgpu;

namespace {

// Per-device state; the AE handle stores a pointer to it.
struct GPUState {
    PF_GPU_Framework framework = PF_GPU_Framework_NONE;
    std::mutex lock;  // OpenCL kernel arguments are not thread safe (multi-frame rendering)
    cl_program program = nullptr;
    cl_kernel kernel = nullptr;
    CUcontext cuContext = nullptr;
    CUmodule cuModule = nullptr;
    CUfunction cuFunction = nullptr;
    void* metalPipeline = nullptr;
};

GPUState* StateFromHandle(const void* gpu_data) {
    PF_Handle h = static_cast<PF_Handle>(const_cast<void*>(gpu_data));
    return (h && *h) ? *reinterpret_cast<GPUState**>(*h) : nullptr;
}

// The six packet arrays, in kernel argument order.
struct PacketArrays {
    const void* data[6];
    size_t bytes[6];
    explicit PacketArrays(const rb::Packet& pk) {
        data[0] = &pk.params; bytes[0] = sizeof(RK_Params);
        data[1] = pk.tris.data(); bytes[1] = pk.tris.size() * sizeof(RK_Tri);
        data[2] = pk.verts.data(); bytes[2] = pk.verts.size() * sizeof(RK_Vertex);
        data[3] = pk.tileStart.data(); bytes[3] = pk.tileStart.size() * sizeof(int);
        data[4] = pk.tileTris.data(); bytes[4] = pk.tileTris.size() * sizeof(int);
        data[5] = pk.lights.data(); bytes[5] = pk.lights.size() * sizeof(RK_Light);
    }
};

// ---------------------------------------------------------------- OpenCL
#ifdef RB_HAS_OPENCL
bool SetupOpenCL(const PF_GPUDeviceInfo& info, GPUState& st) {
    const OpenCL* cl = openCL();
    if (!cl) return false;
    cl_context ctx = static_cast<cl_context>(info.contextPV);
    cl_device_id device = static_cast<cl_device_id>(info.devicePV);
    std::vector<size_t> sizes;
    for (int i = 0; i < kRibbonKernelCLCount; i++) sizes.push_back(std::strlen(kRibbonKernelCL[i]));
    cl_int res = CL_SUCCESS;
    st.program = cl->CreateProgramWithSource(ctx, cl_uint(kRibbonKernelCLCount), const_cast<const char**>(kRibbonKernelCL),
                                             sizes.data(), &res);
    if (res != CL_SUCCESS || !st.program) return false;
    if (cl->BuildProgram(st.program, 1, &device, "-cl-single-precision-constant -cl-mad-enable", nullptr, nullptr) != CL_SUCCESS)
        return false;
    st.kernel = cl->CreateKernel(st.program, "RibbonRender", &res);
    return res == CL_SUCCESS && st.kernel;
}

void ReleaseOpenCL(GPUState& st) {
    const OpenCL* cl = openCL();
    if (cl && st.kernel) cl->ReleaseKernel(st.kernel);
    if (cl && st.program) cl->ReleaseProgram(st.program);
    st.kernel = nullptr;
    st.program = nullptr;
}

PF_Err RenderOpenCL(const PF_GPUDeviceInfo& info, GPUState& st, const rb::Packet& pk, void* src, void* dst, void* tex) {
    const OpenCL* cl = openCL();
    if (!cl || !st.kernel) return PF_Err_INTERNAL_STRUCT_DAMAGED;
    cl_context ctx = static_cast<cl_context>(info.contextPV);
    cl_command_queue queue = static_cast<cl_command_queue>(info.command_queuePV);
    PacketArrays arr(pk);
    cl_int res = CL_SUCCESS;
    cl_mem bufs[6] = {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
    for (int i = 0; i < 6 && res == CL_SUCCESS; i++)
        bufs[i] = cl->CreateBuffer(ctx, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, arr.bytes[i], const_cast<void*>(arr.data[i]), &res);
    cl_mem texMem = static_cast<cl_mem>(tex), srcMem = static_cast<cl_mem>(src), dstMem = static_cast<cl_mem>(dst);
    if (res == CL_SUCCESS) {
        std::lock_guard<std::mutex> guard(st.lock);
        for (cl_uint i = 0; i < 6 && res == CL_SUCCESS; i++) res = cl->SetKernelArg(st.kernel, i, sizeof(cl_mem), &bufs[i]);
        if (res == CL_SUCCESS) res = cl->SetKernelArg(st.kernel, 6, sizeof(cl_mem), &texMem);
        if (res == CL_SUCCESS) res = cl->SetKernelArg(st.kernel, 7, sizeof(cl_mem), &srcMem);
        if (res == CL_SUCCESS) res = cl->SetKernelArg(st.kernel, 8, sizeof(cl_mem), &dstMem);
        size_t local[2] = {16, 16};
        size_t global[2] = {size_t((pk.params.width + 15) / 16) * 16, size_t((pk.params.height + 15) / 16) * 16};
        if (res == CL_SUCCESS) res = cl->EnqueueNDRangeKernel(queue, st.kernel, 2, nullptr, global, local, 0, nullptr, nullptr);
    }
    for (cl_mem m : bufs)
        if (m) cl->ReleaseMemObject(m);  // freed by OpenCL once the kernel has finished
    return res == CL_SUCCESS ? PF_Err_NONE : PF_Err_INTERNAL_STRUCT_DAMAGED;
}
#endif

// ---------------------------------------------------------------- CUDA (driver API + embedded PTX)
#ifdef RB_HAS_CUDA
// Makes AE's context current for the scope of the object.
struct CudaScope {
    const CudaDriver* cu;
    bool pushed;
    CudaScope(const CudaDriver* d, CUcontext ctx) : cu(d), pushed(ctx && d->CtxPushCurrent(ctx) == CUDA_SUCCESS) {}
    ~CudaScope() {
        CUcontext dummy = nullptr;
        if (pushed) cu->CtxPopCurrent(&dummy);
    }
};

bool SetupCUDA(const PF_GPUDeviceInfo& info, GPUState& st) {
    const CudaDriver* cu = cudaDriver();
    if (!cu) return false;
    st.cuContext = static_cast<CUcontext>(info.contextPV);
    CudaScope scope(cu, st.cuContext);
    if (cu->ModuleLoadData(&st.cuModule, kRibbonKernelPTX) != CUDA_SUCCESS) return false;  // JIT for this GPU
    return cu->ModuleGetFunction(&st.cuFunction, st.cuModule, "RibbonRender") == CUDA_SUCCESS;
}

void ReleaseCUDA(GPUState& st) {
    const CudaDriver* cu = cudaDriver();
    if (cu && st.cuModule) {
        CudaScope scope(cu, st.cuContext);
        cu->ModuleUnload(st.cuModule);
    }
    st.cuModule = nullptr;
    st.cuFunction = nullptr;
}

PF_Err RenderCUDA(GPUState& st, const rb::Packet& pk, void* src, void* dst, void* tex) {
    const CudaDriver* cu = cudaDriver();
    if (!cu || !st.cuFunction) return PF_Err_INTERNAL_STRUCT_DAMAGED;
    CudaScope scope(cu, st.cuContext);
    PacketArrays arr(pk);
    CUdeviceptr dev[6] = {0, 0, 0, 0, 0, 0};
    CUresult r = CUDA_SUCCESS;
    for (int i = 0; i < 6 && r == CUDA_SUCCESS; i++) {
        r = cu->MemAlloc(&dev[i], arr.bytes[i]);
        if (r == CUDA_SUCCESS) r = cu->MemcpyHtoD(dev[i], arr.data[i], arr.bytes[i]);
    }
    // AE's GPU worlds are device pointers in the same context.
    CUdeviceptr texP = CUdeviceptr(reinterpret_cast<uintptr_t>(tex));
    CUdeviceptr srcP = CUdeviceptr(reinterpret_cast<uintptr_t>(src));
    CUdeviceptr dstP = CUdeviceptr(reinterpret_cast<uintptr_t>(dst));
    void* args[9] = {&dev[0], &dev[1], &dev[2], &dev[3], &dev[4], &dev[5], &texP, &srcP, &dstP};
    if (r == CUDA_SUCCESS) {
        unsigned gx = unsigned((pk.params.width + 15) / 16), gy = unsigned((pk.params.height + 15) / 16);
        r = cu->LaunchKernel(st.cuFunction, gx, gy, 1, 16, 16, 1, 0, nullptr, args, nullptr);
    }
    if (r == CUDA_SUCCESS) r = cu->CtxSynchronize();
    for (CUdeviceptr p : dev)
        if (p) cu->MemFree(p);
    return r == CUDA_SUCCESS ? PF_Err_NONE : PF_Err_INTERNAL_STRUCT_DAMAGED;
}
#endif

}  // namespace

bool RibbonGPU_Supports(PF_GPU_Framework fw) {
#ifdef RB_HAS_OPENCL
    if (fw == PF_GPU_Framework_OPENCL) return openCL() != nullptr;
#endif
#ifdef RB_HAS_CUDA
    if (fw == PF_GPU_Framework_CUDA) return cudaDriver() != nullptr;
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
    if (fw == PF_GPU_Framework_CUDA) {
        ok = SetupCUDA(info, *st);
        if (!ok) ReleaseCUDA(*st);
    }
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
#ifdef RB_HAS_CUDA
        ReleaseCUDA(*st);
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
        case PF_GPU_Framework_CUDA: return RenderCUDA(*st, pk, srcMem, dstMem, texMem);
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
