// Ribbon3D GPU - run-time bindings to OpenCL and the CUDA driver API.
//
// The plugin does not link against OpenCL or CUDA: the system libraries are loaded on first use
// (OpenCL.dll / nvcuda.dll on Windows, libOpenCL.so.1 / libcuda.so.1 on Linux, the OpenCL
// framework on macOS). A machine without them simply gets the CPU (or Metal) path, and building
// needs neither an OpenCL SDK nor the CUDA toolkit: the CUDA kernel ships as embedded PTX.
#pragma once

#include <cstddef>
#include <cstdint>

#if defined(_WIN32)
    #define RB_GPU_API __stdcall
#else
    #define RB_GPU_API
#endif

namespace rbgpu {

// ---------------------------------------------------------------- OpenCL (subset, OpenCL 1.2)
typedef int32_t cl_int;
typedef uint32_t cl_uint;
typedef uint64_t cl_ulong;
typedef cl_ulong cl_mem_flags;
typedef struct _cl_context* cl_context;
typedef struct _cl_device_id* cl_device_id;
typedef struct _cl_command_queue* cl_command_queue;
typedef struct _cl_program* cl_program;
typedef struct _cl_kernel* cl_kernel;
typedef struct _cl_mem* cl_mem;
typedef struct _cl_event* cl_event;

enum : cl_int { CL_SUCCESS = 0 };
enum : cl_mem_flags { CL_MEM_READ_ONLY = 1 << 2, CL_MEM_COPY_HOST_PTR = 1 << 5 };

struct OpenCL {
    cl_program(RB_GPU_API* CreateProgramWithSource)(cl_context, cl_uint, const char**, const size_t*, cl_int*);
    cl_int(RB_GPU_API* BuildProgram)(cl_program, cl_uint, const cl_device_id*, const char*, void (RB_GPU_API*)(cl_program, void*), void*);
    cl_kernel(RB_GPU_API* CreateKernel)(cl_program, const char*, cl_int*);
    cl_int(RB_GPU_API* ReleaseKernel)(cl_kernel);
    cl_int(RB_GPU_API* ReleaseProgram)(cl_program);
    cl_mem(RB_GPU_API* CreateBuffer)(cl_context, cl_mem_flags, size_t, void*, cl_int*);
    cl_int(RB_GPU_API* ReleaseMemObject)(cl_mem);
    cl_int(RB_GPU_API* SetKernelArg)(cl_kernel, cl_uint, size_t, const void*);
    cl_int(RB_GPU_API* EnqueueNDRangeKernel)(cl_command_queue, cl_kernel, cl_uint, const size_t*, const size_t*, const size_t*,
                                            cl_uint, const cl_event*, cl_event*);
};

// ---------------------------------------------------------------- CUDA driver API (subset)
typedef int CUresult;
typedef struct CUctx_st* CUcontext;
typedef struct CUmod_st* CUmodule;
typedef struct CUfunc_st* CUfunction;
typedef struct CUstream_st* CUstream;
typedef unsigned long long CUdeviceptr;
enum : CUresult { CUDA_SUCCESS = 0 };

struct CudaDriver {
    CUresult(RB_GPU_API* CtxPushCurrent)(CUcontext);
    CUresult(RB_GPU_API* CtxPopCurrent)(CUcontext*);
    CUresult(RB_GPU_API* CtxSynchronize)();
    CUresult(RB_GPU_API* ModuleLoadData)(CUmodule*, const void*);
    CUresult(RB_GPU_API* ModuleUnload)(CUmodule);
    CUresult(RB_GPU_API* ModuleGetFunction)(CUfunction*, CUmodule, const char*);
    CUresult(RB_GPU_API* MemAlloc)(CUdeviceptr*, size_t);
    CUresult(RB_GPU_API* MemFree)(CUdeviceptr);
    CUresult(RB_GPU_API* MemcpyHtoD)(CUdeviceptr, const void*, size_t);
    CUresult(RB_GPU_API* LaunchKernel)(CUfunction, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, unsigned, CUstream,
                                      void**, void**);
};

// Loaded once (thread safe); null when the library or a function is missing.
const OpenCL* openCL();
const CudaDriver* cudaDriver();

}  // namespace rbgpu
