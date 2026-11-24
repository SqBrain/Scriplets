// CUDA entry point compiled to PTX by clang without the CUDA toolkit (tools/build_ptx.sh).
// The PTX is embedded in the plugin (RibbonKernelPTX.h) and loaded through the CUDA driver API,
// so neither building nor running the plugin needs the CUDA toolkit.
#define RK_PTX_NO_LIBDEVICE 1
#define __global__ __attribute__((global))
#define __device__ __attribute__((device))

#include "RibbonKernel.h"

extern "C" __global__ void RibbonRender(const RK_Params* P, const RK_Tri* tris, const RK_Vertex* verts, const int* tileStart,
                                        const int* tileTris, const RK_Light* lights, const float* tex, const float* src, float* dst)
{
    int x = int(__nvvm_read_ptx_sreg_ctaid_x() * __nvvm_read_ptx_sreg_ntid_x() + __nvvm_read_ptx_sreg_tid_x());
    int y = int(__nvvm_read_ptx_sreg_ctaid_y() * __nvvm_read_ptx_sreg_ntid_y() + __nvvm_read_ptx_sreg_tid_y());
    rk_render_pixel(x, y, P, tris, verts, tileStart, tileTris, lights, tex, src, dst);
}
