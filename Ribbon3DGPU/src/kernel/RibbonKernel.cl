// OpenCL entry point. RibbonShared.h and RibbonKernel.h are prepended when the source is
// embedded (cmake/EmbedKernel.cmake), the program is compiled at run time by the plugin.
__kernel void RibbonRender(__global const RK_Params* P, __global const RK_Tri* tris, __global const RK_Vertex* verts,
                           __global const int* tileStart, __global const int* tileTris, __global const RK_Light* lights,
                           __global const float* env, __global const float* nm, __global const float* rm,
                           __global const float* tex, __global const float* src, __global float* dst)
{
    rk_render_pixel((int)get_global_id(0), (int)get_global_id(1), P, tris, verts, tileStart, tileTris, lights, env, nm, rm, tex, src,
                    dst);
}
