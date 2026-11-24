// Metal entry point. RibbonShared.h and RibbonKernel.h are prepended when the source is
// embedded (cmake/EmbedKernel.cmake), the library is compiled at run time by the plugin.
kernel void RibbonRender(device const RK_Params* P [[buffer(0)]],
                         device const RK_Tri* tris [[buffer(1)]],
                         device const RK_Vertex* verts [[buffer(2)]],
                         device const int* tileStart [[buffer(3)]],
                         device const int* tileTris [[buffer(4)]],
                         device const RK_Light* lights [[buffer(5)]],
                         device const float* env [[buffer(6)]],
                         device const float* tex [[buffer(7)]],
                         device const float* src [[buffer(8)]],
                         device float* dst [[buffer(9)]],
                         uint2 gid [[thread_position_in_grid]])
{
    rk_render_pixel((int)gid.x, (int)gid.y, P, tris, verts, tileStart, tileTris, lights, env, tex, src, dst);
}
