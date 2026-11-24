// CUDA entry point and launcher, compiled by nvcc into the plugin (Windows, NVIDIA).
#include <cuda_runtime.h>

#include "RibbonKernel.h"

__global__ void RibbonRenderKernel(const RK_Params* P, const RK_Tri* tris, const RK_Vertex* verts, const int* tileStart,
                                   const int* tileTris, const RK_Light* lights, const float* tex, const float* src, float* dst)
{
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    int y = blockIdx.y * blockDim.y + threadIdx.y;
    rk_render_pixel(x, y, P, tris, verts, tileStart, tileTris, lights, tex, src, dst);
}

// All pointers are device pointers. Returns a cudaError_t value (0 = success).
extern "C" int RibbonRender_CUDA(const void* P, const void* tris, const void* verts, const void* tileStart,
                                 const void* tileTris, const void* lights, const void* tex, const void* src, void* dst,
                                 int width, int height, void* stream)
{
    dim3 block(16, 16, 1);
    dim3 grid((width + 15) / 16, (height + 15) / 16, 1);
    RibbonRenderKernel<<<grid, block, 0, (cudaStream_t)stream>>>(
        (const RK_Params*)P, (const RK_Tri*)tris, (const RK_Vertex*)verts, (const int*)tileStart, (const int*)tileTris,
        (const RK_Light*)lights, (const float*)tex, (const float*)src, (float*)dst);
    return (int)cudaPeekAtLastError();
}
