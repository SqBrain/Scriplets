// Fake After Effects host: drives EffectMain through setup, pre-render and render (CPU 8/32 bpc
// and GPU via OpenCL), with a mask path, a comp light and no camera. Uses the SDK stub headers.
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "Ribbon3D.h"

static std::vector<PF_ParamDef> gDefs;
static const int W = 960, H = 540;

// ---- param callbacks
static PF_Err AddParam(PF_ProgPtr, A_long, PF_ParamDef* d) { gDefs.push_back(*d); return PF_Err_NONE; }
static PF_Err CheckoutParam(PF_ProgPtr, A_long i, A_long, A_long, A_u_long, PF_ParamDef* p) {
    if (i < 1 || i > (A_long)gDefs.size()) return PF_Err_BAD_CALLBACK_PARAM;
    *p = gDefs[size_t(i - 1)];
    if (i == RB_PATH) p->u.path_d.path_id = 7;
    return PF_Err_NONE;
}
static PF_Err CheckinParam(PF_ProgPtr, PF_ParamDef*) { return PF_Err_NONE; }

// ---- mask: two cubic segments in layer pixels (full res comp 960x540)
struct Seg { double p[4][2]; };
static const Seg kSegs[2] = {{{{90, 350}, {200, 280}, {300, 150}, {480, 160}}}, {{{480, 160}, {660, 170}, {650, 410}, {870, 320}}}};
static void bez(const Seg& s, double t, double& x, double& y) {
    double u = 1 - t;
    x = u * u * u * s.p[0][0] + 3 * u * u * t * s.p[1][0] + 3 * u * t * t * s.p[2][0] + t * t * t * s.p[3][0];
    y = u * u * u * s.p[0][1] + 3 * u * u * t * s.p[1][1] + 3 * u * t * t * s.p[2][1] + t * t * t * s.p[3][1];
}
static double segLen(const Seg& s) {
    double L = 0, px, py, x, y;
    bez(s, 0, px, py);
    for (int i = 1; i <= 2000; i++) { bez(s, i / 2000.0, x, y); L += std::hypot(x - px, y - py); px = x; py = y; }
    return L;
}
static PF_Err CheckoutPath(PF_ProgPtr, PF_PathID id, A_long, A_long, A_u_long, PF_PathOutlinePtr* p) {
    *p = id == 7 ? (PF_PathOutlinePtr)0x1 : nullptr;
    return PF_Err_NONE;
}
static PF_Err CheckinPath(PF_ProgPtr, PF_PathID, PF_Boolean, PF_PathOutlinePtr) { return PF_Err_NONE; }
static PF_Err IsOpen(PF_ProgPtr, PF_PathOutlinePtr, PF_Boolean* o) { *o = TRUE; return PF_Err_NONE; }
static PF_Err NumSegs(PF_ProgPtr, PF_PathOutlinePtr, A_long* n) { *n = 2; return PF_Err_NONE; }
static PF_Err Prep(PF_ProgPtr, PF_PathOutlinePtr, A_long, A_long, PF_PathSegPrepPtr* pp) { *pp = (PF_PathSegPrepPtr)0x2; return PF_Err_NONE; }
static PF_Err GetLen(PF_ProgPtr, PF_PathOutlinePtr, A_long s, PF_PathSegPrepPtr*, PF_FpLong* l) { *l = segLen(kSegs[s]); return PF_Err_NONE; }
static PF_Err Eval(PF_ProgPtr, PF_PathOutlinePtr, PF_PathSegPrepPtr*, A_long s, PF_FpLong len, PF_FpLong* x, PF_FpLong* y) {
    // arc length -> t by bisection on a dense table
    const Seg& g = kSegs[s];
    double L = 0, px, py, cx, cy, t = 1;
    bez(g, 0, px, py);
    for (int i = 1; i <= 2000; i++) {
        bez(g, i / 2000.0, cx, cy);
        double d = std::hypot(cx - px, cy - py);
        if (L + d >= len) { t = (i - 1 + (d > 0 ? (len - L) / d : 0)) / 2000.0; break; }
        L += d; px = cx; py = cy;
    }
    bez(g, t, *x, *y);
    return PF_Err_NONE;
}
static PF_Err Cleanup(PF_ProgPtr, PF_PathOutlinePtr, A_long, PF_PathSegPrepPtr*) { return PF_Err_NONE; }

// ---- AEGP: one point light, no camera
static int kLight;
static A_Err GetEffectLayer(PF_ProgPtr, AEGP_LayerH* l) { *l = (AEGP_LayerH)0x10; return 0; }
static A_Err ToComp(PF_ProgPtr, A_long t, A_u_long s, A_Time* o) { o->value = t; o->scale = s; return 0; }
static A_Err GetCam(PF_ProgPtr, const A_Time*, AEGP_LayerH* c) { *c = nullptr; return 0; }
static A_Err NumLayers(AEGP_CompH, A_long* n) { *n = 2; return 0; }
static A_Err LayerByIndex(AEGP_CompH, A_long i, AEGP_LayerH* l) { *l = i == 0 ? (AEGP_LayerH)&kLight : (AEGP_LayerH)0x10; return 0; }
static A_Err ParentComp(AEGP_LayerH, AEGP_CompH* c) { *c = (AEGP_CompH)0x20; return 0; }
static A_Err Flags(AEGP_LayerH, AEGP_LayerFlags* f) { *f = AEGP_LayerFlag_VIDEO_ACTIVE; return 0; }
static A_Err ToWorld(AEGP_LayerH, const A_Time*, A_Matrix4* m) {
    std::memset(m, 0, sizeof(*m));
    m->mat[0][0] = m->mat[1][1] = m->mat[2][2] = m->mat[3][3] = 1;
    m->mat[3][0] = 150; m->mat[3][1] = -150; m->mat[3][2] = -700;  // row-vector translation
    return 0;
}
static A_Err ObjType(AEGP_LayerH l, AEGP_ObjectType* t) { *t = l == (AEGP_LayerH)&kLight ? AEGP_ObjectType_LIGHT : AEGP_ObjectType_AV; return 0; }
static A_Err LightType(AEGP_LayerH, AEGP_LightType* t) { *t = AEGP_LightType_POINT; return 0; }
static A_Err NewStream(AEGP_PluginID, AEGP_LayerH, AEGP_LayerStream w, AEGP_StreamRefH* s) { *s = (AEGP_StreamRefH)(intptr_t)(w + 1); return 0; }
static A_Err DisposeStream(AEGP_StreamRefH) { return 0; }
static A_Err StreamVal(AEGP_PluginID, AEGP_StreamRefH s, AEGP_LTimeMode, const A_Time*, A_Boolean, AEGP_StreamValue2* v) {
    int w = int((intptr_t)s) - 1;
    if (w == AEGP_LayerStream_INTENSITY) v->val.one_d = 120;
    else if (w == AEGP_LayerStream_COLOR) { v->val.color.redF = 1; v->val.color.greenF = 0.95; v->val.color.blueF = 0.9; }
    else v->val.one_d = 0;
    return 0;
}
static A_Err DisposeVal(AEGP_StreamValue2*) { return 0; }

// ---- GPU (OpenCL via pocl)
static cl_context gCtx; static cl_device_id gDev; static cl_command_queue gQ;
static PF_Err DevInfo(PF_ProgPtr, A_u_long, PF_GPUDeviceInfo* i) {
    std::memset(i, 0, sizeof(*i));
    i->device_framework = PF_GPU_Framework_OPENCL; i->devicePV = gDev; i->contextPV = gCtx; i->command_queuePV = gQ;
    return PF_Err_NONE;
}
static PF_Err WorldData(PF_ProgPtr, PF_EffectWorld* w, void** d) { *d = w->data; return PF_Err_NONE; }
static std::vector<std::vector<char>> gHandles;
static PF_Handle NewHandle(A_u_long size) { static std::vector<char*> ptrs; char* p = new char[size](); ptrs.push_back(p); return &ptrs.back(); }
static void DisposeHandle(PF_Handle) {}

static PF_PathQuerySuite1 sPQ = {CheckoutPath, CheckinPath};
static PF_PathDataSuite1 sPD = {IsOpen, NumSegs, Prep, GetLen, Eval, Cleanup};
static AEGP_PFInterfaceSuite1 sPFI = {GetEffectLayer, ToComp, GetCam};
static AEGP_LayerSuite8 sLS = {NumLayers, LayerByIndex, ParentComp, Flags, ToWorld, ObjType};
static AEGP_StreamSuite5 sSS = {NewStream, DisposeStream, StreamVal, DisposeVal};
static AEGP_LightSuite2 sLT = {LightType};
static PF_GPUDeviceSuite1 sGPU = {nullptr, DevInfo, WorldData};
static PF_HandleSuite1 sHS = {NewHandle, nullptr, nullptr, DisposeHandle};
static int32 Acquire(const char* n, int32, const void** p) {
    std::string s(n);
    if (s == kPFPathQuerySuite) *p = &sPQ; else if (s == kPFPathDataSuite) *p = &sPD;
    else if (s == kAEGPPFInterfaceSuite) *p = &sPFI; else if (s == kAEGPLayerSuite) *p = &sLS;
    else if (s == kAEGPStreamSuite) *p = &sSS; else if (s == kAEGPLightSuite) *p = &sLT;
    else if (s == kPFGPUDeviceSuite) *p = &sGPU; else if (s == kPFHandleSuite) *p = &sHS;
    else return 1;
    return kSPNoError;
}
static int32 Release(const char*, int32) { return 0; }
static SPBasicSuite sBasic = {Acquire, Release};

// ---- render callbacks
static PF_LRect Full() { PF_LRect r = {0, 0, W, H}; return r; }
static PF_Err CheckoutLayer(PF_ProgPtr, A_long index, A_long, const PF_RenderRequest* req, A_long, A_long, A_u_long, PF_CheckoutResult* r) {
    std::memset(r, 0, sizeof(*r));
    if (index == RB_INPUT) {
        r->max_result_rect = Full();
        r->result_rect = req->rect;
    }
    return PF_Err_NONE;  // texture: no layer selected
}
static PF_EffectWorld gSrc, gDst;
static PF_Err CheckoutPixels(PF_ProgPtr, A_long id, PF_EffectWorld** w) { *w = id == RB_CHECKOUT_INPUT ? &gSrc : nullptr; return PF_Err_NONE; }
static PF_Err CheckinPixels(PF_ProgPtr, A_long) { return PF_Err_NONE; }
static PF_Err CheckoutOutput(PF_ProgPtr, PF_EffectWorld** w) { *w = &gDst; return PF_Err_NONE; }

static PF_InData MakeInData() {
    PF_InData in;
    std::memset(&in, 0, sizeof(in));
    in.inter.add_param = AddParam; in.inter.checkout_param = CheckoutParam; in.inter.checkin_param = CheckinParam;
    in.quality = PF_Quality_HI; in.current_time = 12; in.time_step = 1; in.time_scale = 24;
    in.width = W; in.height = H; in.downsample_x = {1, 1}; in.downsample_y = {1, 1}; in.pica_basicP = &sBasic;
    return in;
}

static std::vector<float> RunCPU(int bpc) {
    PF_InData in = MakeInData();
    PF_OutData out; std::memset(&out, 0, sizeof(out));
    PF_PreRenderInput pin; std::memset(&pin, 0, sizeof(pin)); pin.output_request.rect = Full(); pin.bitdepth = short(bpc);
    PF_PreRenderOutput pout; std::memset(&pout, 0, sizeof(pout));
    PF_PreRenderCallbacks pcb = {CheckoutLayer};
    PF_PreRenderExtra pre = {&pin, &pout, &pcb};
    if (EffectMain(PF_Cmd_SMART_PRE_RENDER, &in, &out, nullptr, nullptr, &pre)) { std::printf("prerender failed\n"); return {}; }
    size_t px = bpc == 32 ? 16 : 4;
    std::vector<char> sbuf(W * H * px), dbuf(W * H * px);
    gSrc = {sbuf.data(), A_long(W * px), W, H}; gDst = {dbuf.data(), A_long(W * px), W, H};
    for (int i = 0; i < W * H; i++) {  // opaque dark grey input
        if (bpc == 32) { PF_PixelFloat* p = (PF_PixelFloat*)sbuf.data() + i; p->alpha = 1; p->red = p->green = p->blue = 0.2f; }
        else { PF_Pixel8* p = (PF_Pixel8*)sbuf.data() + i; p->alpha = 255; p->red = p->green = p->blue = 51; }
    }
    PF_SmartRenderInput sin; std::memset(&sin, 0, sizeof(sin)); sin.bitdepth = short(bpc); sin.pre_render_data = pout.pre_render_data;
    PF_SmartRenderCallbacks scb = {CheckoutPixels, CheckinPixels, CheckoutOutput};
    PF_SmartRenderExtra sre = {&sin, &scb};
    PF_Err e = EffectMain(PF_Cmd_SMART_RENDER, &in, &out, nullptr, nullptr, &sre);
    pout.delete_pre_render_data_func(pout.pre_render_data);
    if (e) { std::printf("render failed %d\n", e); return {}; }
    std::vector<float> res(W * H * 4);
    for (int i = 0; i < W * H; i++) {
        if (bpc == 32) { PF_PixelFloat* p = (PF_PixelFloat*)dbuf.data() + i; res[i * 4] = p->blue; res[i * 4 + 1] = p->green; res[i * 4 + 2] = p->red; res[i * 4 + 3] = p->alpha; }
        else { PF_Pixel8* p = (PF_Pixel8*)dbuf.data() + i; res[i * 4] = p->blue / 255.f; res[i * 4 + 1] = p->green / 255.f; res[i * 4 + 2] = p->red / 255.f; res[i * 4 + 3] = p->alpha / 255.f; }
    }
    return res;
}

static std::vector<float> RunGPU() {
    PF_InData in = MakeInData();
    PF_OutData out; std::memset(&out, 0, sizeof(out));
    PF_GPUDeviceSetupInput gin = {PF_GPU_Framework_OPENCL, 0};
    PF_GPUDeviceSetupOutput gout = {nullptr};
    PF_GPUDeviceSetupExtra gex = {&gin, &gout};
    if (EffectMain(PF_Cmd_GPU_DEVICE_SETUP, &in, &out, nullptr, nullptr, &gex) || !gout.gpu_data ||
        !(out.out_flags2 & PF_OutFlag2_SUPPORTS_GPU_RENDER_F32)) { std::printf("GPU setup failed\n"); return {}; }
    PF_PreRenderInput pin; std::memset(&pin, 0, sizeof(pin)); pin.output_request.rect = Full(); pin.bitdepth = 32; pin.what_gpu = PF_GPU_Framework_OPENCL;
    PF_PreRenderOutput pout; std::memset(&pout, 0, sizeof(pout));
    PF_PreRenderCallbacks pcb = {CheckoutLayer};
    PF_PreRenderExtra pre = {&pin, &pout, &pcb};
    EffectMain(PF_Cmd_SMART_PRE_RENDER, &in, &out, nullptr, nullptr, &pre);
    if (!(pout.flags & PF_RenderOutputFlag_GPU_RENDER_POSSIBLE)) { std::printf("GPU render not offered\n"); return {}; }
    std::vector<float> host(W * H * 4);
    for (int i = 0; i < W * H; i++) { host[i * 4] = host[i * 4 + 1] = host[i * 4 + 2] = 0.2f; host[i * 4 + 3] = 1; }
    cl_int e;
    cl_mem s = clCreateBuffer(gCtx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, host.size() * 4, host.data(), &e);
    cl_mem d = clCreateBuffer(gCtx, CL_MEM_READ_WRITE, host.size() * 4, nullptr, &e);
    gSrc = {s, W * 16, W, H}; gDst = {d, W * 16, W, H};
    PF_SmartRenderInput sin; std::memset(&sin, 0, sizeof(sin)); sin.bitdepth = 32; sin.pre_render_data = pout.pre_render_data;
    sin.gpu_data = gout.gpu_data; sin.what_gpu = PF_GPU_Framework_OPENCL;
    PF_SmartRenderCallbacks scb = {CheckoutPixels, CheckinPixels, CheckoutOutput};
    PF_SmartRenderExtra sre = {&sin, &scb};
    PF_Err err = EffectMain(PF_Cmd_SMART_RENDER_GPU, &in, &out, nullptr, nullptr, &sre);
    clFinish(gQ);
    std::vector<float> res(host.size());
    clEnqueueReadBuffer(gQ, d, CL_TRUE, 0, res.size() * 4, res.data(), 0, nullptr, nullptr);
    pout.delete_pre_render_data_func(pout.pre_render_data);
    PF_GPUDeviceSetdownInput din = {gout.gpu_data, PF_GPU_Framework_OPENCL, 0};
    PF_GPUDeviceSetdownExtra dex = {&din};
    EffectMain(PF_Cmd_GPU_DEVICE_SETDOWN, &in, &out, nullptr, nullptr, &dex);
    if (err) { std::printf("GPU render failed %d\n", err); return {}; }
    return res;
}

int main(int argc, char** argv) {
    PF_InData in = MakeInData();
    PF_OutData out; std::memset(&out, 0, sizeof(out));
    if (EffectMain(PF_Cmd_GLOBAL_SETUP, &in, &out, nullptr, nullptr, nullptr)) return 1;
    if (EffectMain(PF_Cmd_PARAMS_SETUP, &in, &out, nullptr, nullptr, nullptr)) return 1;
    std::printf("params: %zu added, num_params %d (expected %d incl. input)\n", gDefs.size(), out.num_params, RB_NUM_PARAMS);
    int fails = 0;
    if (int(gDefs.size()) + 1 != out.num_params) { std::printf("FAIL: param count mismatch\n"); fails++; }

    cl_platform_id plat; clGetPlatformIDs(1, &plat, nullptr);
    clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, 1, &gDev, nullptr);
    gCtx = clCreateContext(nullptr, 1, &gDev, nullptr, nullptr, nullptr);
    gQ = clCreateCommandQueue(gCtx, gDev, 0, nullptr);

    std::vector<float> c32 = RunCPU(32), c8 = RunCPU(8), g = RunGPU();
    if (c32.empty() || c8.empty() || g.empty()) return 1;
    double cover = 0, maxG = 0, max8 = 0;
    for (size_t i = 0; i < c32.size(); i++) {
        maxG = std::max(maxG, double(std::fabs(c32[i] - g[i])));
        max8 = std::max(max8, double(std::fabs(std::min(1.0f, std::max(0.0f, c32[i])) - c8[i])));
        if (i % 4 == 2 && std::fabs(c32[i] - 0.2f) > 1e-3) cover++;
    }
    std::printf("ribbon pixels changed: %.0f; CPU32 vs GPU(OpenCL) max diff %.2e; CPU32 vs CPU8 max diff %.4f\n", cover, maxG, max8);
    if (cover < 5000) { std::printf("FAIL: ribbon not rendered\n"); fails++; }
    if (maxG > 1e-3) { std::printf("FAIL: GPU differs from CPU\n"); fails++; }
    if (max8 > 2.5 / 255) { std::printf("FAIL: 8-bit path differs\n"); fails++; }
    // Thickness & Bevel through the effect parameters: CPU and GPU must agree, the solid covers more.
    gDefs[RB_THICKNESS - 1].u.fs_d.value = 18;
    gDefs[RB_BEVEL_SIZE - 1].u.fs_d.value = 5;
    std::vector<float> t32 = RunCPU(32), tg = RunGPU();
    if (t32.empty() || tg.empty()) return 1;
    double coverT = 0, maxT = 0;
    for (size_t i = 0; i < t32.size(); i++) {
        maxT = std::max(maxT, double(std::fabs(t32[i] - tg[i])));
        if (i % 4 == 2 && std::fabs(t32[i] - 0.2f) > 1e-3) coverT++;
    }
    std::printf("thick ribbon: pixels changed %.0f (thin %.0f); CPU32 vs GPU(OpenCL) max diff %.2e\n", coverT, cover, maxT);
    if (coverT <= cover) { std::printf("FAIL: thickness had no effect\n"); fails++; }
    if (maxT > 1e-3) { std::printf("FAIL: GPU differs from CPU (thick)\n"); fails++; }
    g = tg;

    if (argc > 1) {
        FILE* f = std::fopen(argv[1], "wb");
        std::fprintf(f, "P6 %d %d 255\n", W, H);
        for (int i = 0; i < W * H; i++) for (int c = 2; c >= 0; c--) std::fputc(int(std::min(1.0f, std::max(0.0f, g[i * 4 + c])) * 255 + 0.5f), f);
        std::fclose(f);
    }
    std::printf(fails ? "host simulation FAILED\n" : "host simulation OK\n");
    return fails;
}
