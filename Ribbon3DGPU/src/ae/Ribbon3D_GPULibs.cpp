#include "Ribbon3D_GPULibs.h"

#if defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

namespace rbgpu {
namespace {

void* OpenLib(const char* const* names) {
    for (const char* const* n = names; *n; n++) {
#if defined(_WIN32)
        HMODULE h = LoadLibraryA(*n);
        if (h) return reinterpret_cast<void*>(h);
#else
        void* h = dlopen(*n, RTLD_NOW | RTLD_LOCAL);
        if (h) return h;
#endif
    }
    return nullptr;
}

void* Sym(void* lib, const char* name) {
#if defined(_WIN32)
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(lib), name));
#else
    return dlsym(lib, name);
#endif
}

template <typename F>
bool Bind(void* lib, F& fn, const char* name) {
    fn = reinterpret_cast<F>(Sym(lib, name));
    return fn != nullptr;
}

OpenCL* LoadOpenCL() {
    static const char* const names[] = {
#if defined(_WIN32)
        "OpenCL.dll",
#elif defined(__APPLE__)
        "/System/Library/Frameworks/OpenCL.framework/OpenCL",
#else
        "libOpenCL.so.1", "libOpenCL.so",
#endif
        nullptr};
    void* lib = OpenLib(names);
    if (!lib) return nullptr;
    static OpenCL cl;
    bool ok = Bind(lib, cl.CreateProgramWithSource, "clCreateProgramWithSource") && Bind(lib, cl.BuildProgram, "clBuildProgram") &&
              Bind(lib, cl.CreateKernel, "clCreateKernel") && Bind(lib, cl.ReleaseKernel, "clReleaseKernel") &&
              Bind(lib, cl.ReleaseProgram, "clReleaseProgram") && Bind(lib, cl.CreateBuffer, "clCreateBuffer") &&
              Bind(lib, cl.ReleaseMemObject, "clReleaseMemObject") && Bind(lib, cl.SetKernelArg, "clSetKernelArg") &&
              Bind(lib, cl.EnqueueNDRangeKernel, "clEnqueueNDRangeKernel");
    return ok ? &cl : nullptr;
}

CudaDriver* LoadCuda() {
    static const char* const names[] = {
#if defined(_WIN32)
        "nvcuda.dll",
#elif defined(__APPLE__)
        "/usr/local/cuda/lib/libcuda.dylib",
#else
        "libcuda.so.1", "libcuda.so",
#endif
        nullptr};
    void* lib = OpenLib(names);
    if (!lib) return nullptr;
    static CudaDriver cu;
    bool ok = Bind(lib, cu.CtxPushCurrent, "cuCtxPushCurrent_v2") && Bind(lib, cu.CtxPopCurrent, "cuCtxPopCurrent_v2") &&
              Bind(lib, cu.CtxSynchronize, "cuCtxSynchronize") && Bind(lib, cu.ModuleLoadData, "cuModuleLoadData") &&
              Bind(lib, cu.ModuleUnload, "cuModuleUnload") && Bind(lib, cu.ModuleGetFunction, "cuModuleGetFunction") &&
              Bind(lib, cu.MemAlloc, "cuMemAlloc_v2") && Bind(lib, cu.MemFree, "cuMemFree_v2") &&
              Bind(lib, cu.MemcpyHtoD, "cuMemcpyHtoD_v2") && Bind(lib, cu.LaunchKernel, "cuLaunchKernel");
    return ok ? &cu : nullptr;
}

}  // namespace

const OpenCL* openCL() {
    static const OpenCL* p = LoadOpenCL();  // C++11 magic statics: thread-safe, loaded once
    return p;
}

const CudaDriver* cudaDriver() {
    static const CudaDriver* p = LoadCuda();
    return p;
}

}  // namespace rbgpu
