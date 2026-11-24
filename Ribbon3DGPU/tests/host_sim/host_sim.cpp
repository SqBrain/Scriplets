// Fake After Effects host: drives EffectMain through setup, pre-render and render (CPU 8/32 bpc
// and GPU via OpenCL), with a mask path, a comp light and no camera. Uses the SDK stub headers.
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include "core/RibbonGeometry.h"
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

// ---- AEGP: a small comp model - the effect layer, one point light, nulls created by the effect.
struct SimLayer {
    A_long id;
    bool light = false, is3D = false;
    double pos[3] = {0, 0, 0}, anchor[3] = {0, 0, 0}, scale[3] = {100, 100, 100};
    double rotX = 0, rotY = 0, rotZ = 0;  // degrees
    std::string name;
    struct Item { A_long w, h; A_Ratio par; } source = {0, 0, {1, 1}};  // w == 0: no source (text, shape)
};
static SimLayer::Item gCompItem = {960, 540, {1, 1}};
static bool IsLayerParam(int i) { return (i >= RB_NULL_1 && i < RB_NULL_1 + RB_MAX_NULLS) || i == RB_TEXTURE || i == RB_FIT_LAYER; }
static std::vector<SimLayer*> gLayers;  // comp order: index 0 = top
static SimLayer gSelf, gLight;
static std::map<int, double> gEffectNum;        // effect param streams (index -> value)
static std::map<int, A_long> gEffectLayerIds;   // layer param streams (index -> layer id)
static A_long gNextId = 100;
static int gUndoDepth = 0, gUndoGroups = 0;
static bool gAbsoluteTangents = false;          // PF_PathVertex convention used by this fake host

static void ResetComp() {
    for (SimLayer* l : gLayers)
        if (l != &gSelf && l != &gLight) delete l;
    gLayers.clear();
    gSelf = SimLayer(); gSelf.id = 1;
    gLight = SimLayer(); gLight.id = 2; gLight.light = true; gLight.is3D = true;
    gLight.pos[0] = 150; gLight.pos[1] = -150; gLight.pos[2] = -700;
    gLayers = {&gLight, &gSelf};
    gEffectNum.clear(); gEffectLayerIds.clear(); gNextId = 100;
}
static SimLayer* L(AEGP_LayerH h) { return reinterpret_cast<SimLayer*>(h); }

static A_Err GetEffectLayer(PF_ProgPtr, AEGP_LayerH* l) { *l = (AEGP_LayerH)&gSelf; return 0; }
static A_Err NewEffectForEffect(AEGP_PluginID, PF_ProgPtr, AEGP_EffectRefH* e) { *e = (AEGP_EffectRefH)0x77; return 0; }
static A_Err ToComp(PF_ProgPtr, A_long t, A_u_long s, A_Time* o) { o->value = t; o->scale = s; return 0; }
static A_Err GetCam(PF_ProgPtr, const A_Time*, AEGP_LayerH* c) { *c = nullptr; return 0; }
static A_Err NumLayers(AEGP_CompH, A_long* n) { *n = A_long(gLayers.size()); return 0; }
static A_Err LayerByIndex(AEGP_CompH, A_long i, AEGP_LayerH* l) { *l = (AEGP_LayerH)gLayers[size_t(i)]; return 0; }
static A_Err ParentComp(AEGP_LayerH, AEGP_CompH* c) { *c = (AEGP_CompH)0x20; return 0; }
static A_Err Flags(AEGP_LayerH, AEGP_LayerFlags* f) { *f = AEGP_LayerFlag_VIDEO_ACTIVE; return 0; }
static A_Err SetFlag(AEGP_LayerH l, AEGP_LayerFlags f, A_Boolean v) { if (f == AEGP_LayerFlag_LAYER_IS_3D) L(l)->is3D = v; return 0; }
static A_Err GetID(AEGP_LayerH l, AEGP_LayerIDVal* id) { *id = L(l)->id; return 0; }
static A_Err FromID(AEGP_CompH, AEGP_LayerIDVal id, AEGP_LayerH* l) {
    for (SimLayer* x : gLayers) if (x->id == id) { *l = (AEGP_LayerH)x; return 0; }
    return 1;
}
// world = position + R * (scale * (p - anchor)), R = Rx * Ry * Rz; written in AE's row-vector form.
static A_Err ToWorld(AEGP_LayerH h, const A_Time*, A_Matrix4* m) {
    SimLayer* l = L(h);
    const double d = 3.14159265358979 / 180;
    rb::M3 R = rb::rotationXYZ(l->rotX, l->rotY, l->rotZ);
    std::memset(m, 0, sizeof(*m));
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) m->mat[i][j] = R.m[j][i] * l->scale[i] / 100;  // row i = local axis i
    rb::V3 a(l->anchor[0], l->anchor[1], l->anchor[2]), t(0, 0, 0);
    for (int j = 0; j < 3; j++) t = t + rb::V3(m->mat[j][0], m->mat[j][1], m->mat[j][2]) * (j == 0 ? a.x : j == 1 ? a.y : a.z);
    m->mat[3][0] = l->pos[0] - t.x; m->mat[3][1] = l->pos[1] - t.y; m->mat[3][2] = l->pos[2] - t.z; m->mat[3][3] = 1;
    (void)d;
    return 0;
}
static A_Err ObjType(AEGP_LayerH l, AEGP_ObjectType* t) { *t = L(l)->light ? AEGP_ObjectType_LIGHT : AEGP_ObjectType_AV; return 0; }
static A_Err LightType(AEGP_LayerH, AEGP_LightType* t) { *t = AEGP_LightType_POINT; return 0; }

struct SimStream { SimLayer* layer; int which; int effectParam; };
static A_Err NewStream(AEGP_PluginID, AEGP_LayerH l, AEGP_LayerStream w, AEGP_StreamRefH* s) {
    *s = (AEGP_StreamRefH) new SimStream{L(l), int(w), -1};
    return 0;
}
static A_Err NewEffectStream(AEGP_PluginID, AEGP_EffectRefH, PF_ParamIndex i, AEGP_StreamRefH* s) {
    *s = (AEGP_StreamRefH) new SimStream{nullptr, 0, int(i)};
    return 0;
}
static A_Err DisposeStream(AEGP_StreamRefH s) { delete (SimStream*)s; return 0; }
static A_Err StreamVal(AEGP_PluginID, AEGP_StreamRefH sh, AEGP_LTimeMode, const A_Time*, A_Boolean, AEGP_StreamValue2* v) {
    SimStream* s = (SimStream*)sh;
    std::memset(&v->val, 0, sizeof(v->val));
    if (s->effectParam >= 0) {
        if (IsLayerParam(s->effectParam)) v->val.layer_id = gEffectLayerIds[s->effectParam];
        else v->val.one_d = gEffectNum[s->effectParam];
        return 0;
    }
    SimLayer* l = s->layer;
    switch (s->which) {
        case AEGP_LayerStream_INTENSITY: v->val.one_d = 120; break;
        case AEGP_LayerStream_COLOR: v->val.color.redF = 1; v->val.color.greenF = 0.95; v->val.color.blueF = 0.9; break;
        case AEGP_LayerStream_POSITION: v->val.three_d = {l->pos[0], l->pos[1], l->pos[2]}; break;
        case AEGP_LayerStream_ANCHORPOINT: v->val.three_d = {l->anchor[0], l->anchor[1], l->anchor[2]}; break;
        case AEGP_LayerStream_SCALE: v->val.three_d = {l->scale[0], l->scale[1], l->scale[2]}; break;
        case AEGP_LayerStream_ROTATION: v->val.one_d = l->rotZ; break;
        default: break;
    }
    return 0;
}
static A_Err SetStreamVal(AEGP_PluginID, AEGP_StreamRefH sh, AEGP_StreamValue2* v) {
    SimStream* s = (SimStream*)sh;
    if (!gUndoDepth) std::printf("  note: stream changed outside an undo group\n");
    if (s->effectParam >= 0) {
        if (IsLayerParam(s->effectParam)) gEffectLayerIds[s->effectParam] = v->val.layer_id;
        else gEffectNum[s->effectParam] = v->val.one_d;
        return 0;
    }
    SimLayer* l = s->layer;
    switch (s->which) {
        case AEGP_LayerStream_POSITION: l->pos[0] = v->val.three_d.x; l->pos[1] = v->val.three_d.y; l->pos[2] = v->val.three_d.z; break;
        case AEGP_LayerStream_SCALE: l->scale[0] = v->val.three_d.x; l->scale[1] = v->val.three_d.y; l->scale[2] = v->val.three_d.z; break;
        case AEGP_LayerStream_ROTATION: l->rotZ = v->val.one_d; break;
        default: return 1;
    }
    return 0;
}
static A_Err DisposeVal(AEGP_StreamValue2*) { return 0; }
static A_Err CreateNull(const A_UTF16Char* name, AEGP_CompH, const A_Time*, AEGP_LayerH* out) {
    if (!gUndoDepth) std::printf("  note: layer created outside an undo group\n");
    SimLayer* l = new SimLayer();
    l->id = gNextId++;
    l->anchor[0] = l->anchor[1] = 50;  // AE nulls are 100 x 100 with a centered anchor
    for (const A_UTF16Char* c = name; *c; c++) l->name += char(*c);
    gLayers.insert(gLayers.begin(), l);
    *out = (AEGP_LayerH)l;
    return 0;
}
static A_Err StartUndo(const A_char*) { gUndoDepth++; gUndoGroups++; return 0; }
static A_Err EndUndo() { gUndoDepth--; return 0; }
static A_Err Register(AEGP_GlobalRefcon, const A_char*, AEGP_PluginID* id) { *id = 42; return 0; }
static A_Err DisposeEffect(AEGP_EffectRefH) { return 0; }
static A_Err SourceItem(AEGP_LayerH l, AEGP_ItemH* i) { *i = L(l)->source.w ? (AEGP_ItemH)&L(l)->source : nullptr; return 0; }
static A_Err CompItem(AEGP_CompH, AEGP_ItemH* i) { *i = (AEGP_ItemH)&gCompItem; return 0; }
static A_Err ItemDims(AEGP_ItemH i, A_long* w, A_long* h) { *w = ((SimLayer::Item*)i)->w; *h = ((SimLayer::Item*)i)->h; return 0; }
static A_Err ItemPar(AEGP_ItemH i, A_Ratio* r) { *r = ((SimLayer::Item*)i)->par; return 0; }
static PF_Err VertexInfo(PF_ProgPtr, PF_PathOutlinePtr, A_long i, PF_PathVertex* v) {
    // Vertex i of the two-segment mask; handles relative or absolute depending on gAbsoluteTangents.
    const double* p = i == 0 ? kSegs[0].p[0] : (i == 1 ? kSegs[0].p[3] : kSegs[1].p[3]);
    const double* in = i == 0 ? kSegs[0].p[0] : (i == 1 ? kSegs[0].p[2] : kSegs[1].p[2]);
    const double* out = i == 0 ? kSegs[0].p[1] : (i == 1 ? kSegs[1].p[1] : kSegs[1].p[3]);
    v->x = p[0]; v->y = p[1];
    double o = gAbsoluteTangents ? 0 : 1;
    v->tan_in_x = in[0] - o * p[0]; v->tan_in_y = in[1] - o * p[1];
    v->tan_out_x = out[0] - o * p[0]; v->tan_out_y = out[1] - o * p[1];
    return PF_Err_NONE;
}

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
static PF_PathDataSuite1 sPD = {IsOpen, NumSegs, VertexInfo, Prep, GetLen, Eval, Cleanup};
static AEGP_PFInterfaceSuite1 sPFI = {GetEffectLayer, NewEffectForEffect, ToComp, GetCam};
static AEGP_LayerSuite8 sLS = {NumLayers, LayerByIndex, ParentComp, Flags, ToWorld, ObjType, SetFlag, GetID, FromID, SourceItem};
static AEGP_ItemSuite9 sIS = {ItemDims, ItemPar};
static AEGP_StreamSuite5 sSS = {NewStream, DisposeStream, StreamVal, DisposeVal, NewEffectStream, SetStreamVal};
static AEGP_CompSuite11 sCS = {CreateNull, CompItem};
static AEGP_UtilitySuite5 sUS = {StartUndo, EndUndo, Register};
static AEGP_EffectSuite4 sES = {DisposeEffect};
static AEGP_LightSuite2 sLT = {LightType};
static PF_GPUDeviceSuite1 sGPU = {nullptr, DevInfo, WorldData};
static PF_HandleSuite1 sHS = {NewHandle, nullptr, nullptr, DisposeHandle};
static int32 Acquire(const char* n, int32, const void** p) {
    std::string s(n);
    if (s == kPFPathQuerySuite) *p = &sPQ; else if (s == kPFPathDataSuite) *p = &sPD;
    else if (s == kAEGPPFInterfaceSuite) *p = &sPFI; else if (s == kAEGPLayerSuite) *p = &sLS;
    else if (s == kAEGPStreamSuite) *p = &sSS; else if (s == kAEGPLightSuite) *p = &sLT;
    else if (s == kPFGPUDeviceSuite) *p = &sGPU; else if (s == kPFHandleSuite) *p = &sHS;
    else if (s == kAEGPCompSuite) *p = &sCS; else if (s == kAEGPUtilitySuite) *p = &sUS; else if (s == kAEGPEffectSuite) *p = &sES;
    else if (s == kAEGPItemSuite) *p = &sIS;
    else return 1;
    return kSPNoError;
}
static int32 Release(const char*, int32) { return 0; }
static SPBasicSuite sBasic = {Acquire, Release};

// ---- render callbacks
static PF_LRect Full() { PF_LRect r = {0, 0, W, H}; return r; }
// HDRI layer for "Environment": a sky (bright, blueish above, dark ground below, a sun).
static const int EW = 256, EH = 128;
static bool gEnvOn = false, gStripes = false;
static std::vector<float> gEnvPixels;
static void MakeEnv() {
    gEnvPixels.assign(size_t(EW) * EH * 4, 0.0f);
    for (int y = 0; y < EH; y++)
        for (int x = 0; x < EW; x++) {
            float* p = &gEnvPixels[(size_t(y) * EW + x) * 4];
            bool sky = y < EH / 2, sun = std::abs(x - 150) < 6 && std::abs(y - 30) < 6;
            float r = sun ? 8.0f : (sky ? 0.7f : 0.12f), g = sun ? 7.5f : (sky ? 0.8f : 0.1f), b = sun ? 7.0f : (sky ? 1.0f : 0.08f);
            p[0] = b; p[1] = g; p[2] = r; p[3] = 1;  // BGRA, opaque (32 bpc values above 1 allowed)
        }
}
static float InputGrey(int x) { return gStripes ? ((x / 12) % 2 ? 0.85f : 0.1f) : 0.2f; }
static PF_Err CheckoutLayer(PF_ProgPtr, A_long index, A_long, const PF_RenderRequest* req, A_long, A_long, A_u_long, PF_CheckoutResult* r) {
    std::memset(r, 0, sizeof(*r));
    if (index == RB_INPUT) {
        r->max_result_rect = Full();
        r->result_rect = req->rect;
    }
    if (index == RB_ENV_LAYER && gEnvOn) {
        r->max_result_rect = {0, 0, EW, EH};
        r->result_rect = req->rect;
    }
    return PF_Err_NONE;  // texture: no layer selected
}
static std::vector<unsigned char> gGuid;  // what the effect mixed into the cache key in the last PreRender
static int gGuidCalls = 0;                  // GuidMixInPtr calls in the current PreRender
static int gVerifyFailures = 0;             // pre-renders AE would reject ("internal verification failure")
static bool gMixFlag = false;               // effect declared PF_OutFlag2_I_MIX_GUID_DEPENDENCIES
static PF_Err GuidMixIn(PF_ProgPtr, A_u_long n, const void* buf) {
    const unsigned char* b = static_cast<const unsigned char*>(buf);
    gGuid.insert(gGuid.end(), b, b + n);
    gGuidCalls++;
    return PF_Err_NONE;
}
// SMART_PRE_RENDER the way AE checks it: with I_MIX_GUID_DEPENDENCIES the effect must call
// GuidMixInPtr every time, whatever its parameters are.
static PF_Err PreRenderChecked(PF_InData* in, PF_OutData* out, PF_PreRenderExtra* pre) {
    gGuidCalls = 0;
    PF_Err e = EffectMain(PF_Cmd_SMART_PRE_RENDER, in, out, nullptr, nullptr, pre);
    if (gMixFlag && gGuidCalls == 0) {
        std::printf("FAIL: I_MIX_GUID_DEPENDENCIES effect missing call to GuidMixInPtr during SMART_PRE_RENDER\n");
        gVerifyFailures++;
    }
    return e;
}
static PF_EffectWorld gSrc, gDst, gEnv;
static PF_Err CheckoutPixels(PF_ProgPtr, A_long id, PF_EffectWorld** w) {
    *w = id == RB_CHECKOUT_INPUT ? &gSrc : (id == RB_CHECKOUT_ENV ? &gEnv : nullptr);
    return PF_Err_NONE;
}
static PF_Err CheckinPixels(PF_ProgPtr, A_long) { return PF_Err_NONE; }
static PF_Err CheckoutOutput(PF_ProgPtr, PF_EffectWorld** w) { *w = &gDst; return PF_Err_NONE; }

A_long gTimeOffset = 0;  // frames added to the current time (wind animation)
static PF_InData MakeInData() {
    PF_InData in;
    std::memset(&in, 0, sizeof(in));
    in.inter.add_param = AddParam; in.inter.checkout_param = CheckoutParam; in.inter.checkin_param = CheckinParam;
    in.quality = PF_Quality_HI; in.current_time = 12 + gTimeOffset; in.time_step = 1; in.time_scale = 24;
    in.width = W; in.height = H; in.downsample_x = {1, 1}; in.downsample_y = {1, 1}; in.pica_basicP = &sBasic;
    return in;
}

static std::vector<float> RunCPU(int bpc) {
    PF_InData in = MakeInData();
    PF_OutData out; std::memset(&out, 0, sizeof(out));
    PF_PreRenderInput pin; std::memset(&pin, 0, sizeof(pin)); pin.output_request.rect = Full(); pin.bitdepth = short(bpc);
    PF_PreRenderOutput pout; std::memset(&pout, 0, sizeof(pout));
    PF_PreRenderCallbacks pcb = {CheckoutLayer, GuidMixIn};
    PF_PreRenderExtra pre = {&pin, &pout, &pcb};
    if (PreRenderChecked(&in, &out, &pre)) { std::printf("prerender failed\n"); return {}; }
    size_t px = bpc == 32 ? 16 : 4;
    std::vector<char> sbuf(W * H * px), dbuf(W * H * px);
    gSrc = {sbuf.data(), A_long(W * px), W, H}; gDst = {dbuf.data(), A_long(W * px), W, H};
    for (int i = 0; i < W * H; i++) {  // opaque input: dark grey, or vertical stripes
        float v = InputGrey(i % W);
        if (bpc == 32) { PF_PixelFloat* p = (PF_PixelFloat*)sbuf.data() + i; p->alpha = 1; p->red = p->green = p->blue = v; }
        else { PF_Pixel8* p = (PF_Pixel8*)sbuf.data() + i; p->alpha = 255; p->red = p->green = p->blue = (unsigned char)(v * 255 + 0.5f); }
    }
    std::vector<char> ebuf;
    if (gEnvOn) {  // the CPU path gets the HDRI at the render bit depth, like from AE
        size_t epx = bpc == 32 ? 16 : 4;
        ebuf.resize(size_t(EW) * EH * epx);
        for (int i = 0; i < EW * EH; i++) {
            const float* e = &gEnvPixels[size_t(i) * 4];
            if (bpc == 32) { PF_PixelFloat* p = (PF_PixelFloat*)ebuf.data() + i; p->blue = e[0]; p->green = e[1]; p->red = e[2]; p->alpha = e[3]; }
            else { PF_Pixel8* p = (PF_Pixel8*)ebuf.data() + i; p->blue = (unsigned char)(std::min(e[0], 1.0f) * 255); p->green = (unsigned char)(std::min(e[1], 1.0f) * 255); p->red = (unsigned char)(std::min(e[2], 1.0f) * 255); p->alpha = 255; }
        }
        gEnv = {ebuf.data(), A_long(EW * epx), EW, EH};
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
    PF_PreRenderCallbacks pcb = {CheckoutLayer, GuidMixIn};
    PF_PreRenderExtra pre = {&pin, &pout, &pcb};
    PreRenderChecked(&in, &out, &pre);
    if (!(pout.flags & PF_RenderOutputFlag_GPU_RENDER_POSSIBLE)) { std::printf("GPU render not offered\n"); return {}; }
    std::vector<float> host(W * H * 4);
    for (int i = 0; i < W * H; i++) { host[i * 4] = host[i * 4 + 1] = host[i * 4 + 2] = InputGrey(i % W); host[i * 4 + 3] = 1; }
    cl_mem envMem = nullptr;
    if (gEnvOn) {  // GPU worlds live on the device: the effect has to read the HDRI back
        cl_int ee;
        envMem = clCreateBuffer(gCtx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, gEnvPixels.size() * 4, gEnvPixels.data(), &ee);
        gEnv = {envMem, EW * 16, EW, EH};
    }
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
    if (envMem) clReleaseMemObject(envMem);
    PF_GPUDeviceSetdownInput din = {gout.gpu_data, PF_GPU_Framework_OPENCL, 0};
    PF_GPUDeviceSetdownExtra dex = {&din};
    EffectMain(PF_Cmd_GPU_DEVICE_SETDOWN, &in, &out, nullptr, nullptr, &dex);
    if (err) { std::printf("GPU render failed %d\n", err); return {}; }
    return res;
}

// Presses a button like AE does and writes the changed plain params back.
static PF_Err PressButton(int index, PF_OutData& out) {
    std::vector<PF_ParamDef> cur(gDefs);
    PF_ParamDef input;
    std::memset(&input, 0, sizeof(input));
    std::vector<PF_ParamDef*> ptrs(static_cast<size_t>(RB_NUM_PARAMS), nullptr);
    ptrs[0] = &input;
    for (size_t i = 1; i < ptrs.size(); i++) ptrs[i] = &cur[i - 1];
    cur[RB_PATH - 1].u.path_d.path_id = 7;
    for (PF_ParamDef& d : cur) d.uu.change_flags = 0;
    PF_UserChangedParamExtra ex = {index};
    PF_InData in = MakeInData();
    std::memset(&out, 0, sizeof(out));
    PF_Err err = EffectMain(PF_Cmd_USER_CHANGED_PARAM, &in, &out, ptrs.data(), nullptr, &ex);
    for (size_t i = 0; i < cur.size(); i++)
        if (cur[i].uu.change_flags & PF_ChangeFlag_CHANGED_VALUE) gDefs[i].u = cur[i].u;
    return err;
}
static PF_Err PressMakeNulls(PF_OutData& out) { return PressButton(RB_MAKE_NULLS, out); }

static double DiffFraction(const std::vector<float>& a, const std::vector<float>& b) {
    size_t n = 0;
    for (size_t i = 0; i < a.size(); i += 4) n += std::fabs(a[i + 2] - b[i + 2]) > 0.02f;
    return double(n) / double(a.size() / 4);
}

static int TestControlNulls(const std::vector<float>& maskImg) {
    int fails = 0;
    double firstRot[2] = {0, 0}, firstScale[2] = {0, 0};
    for (int conv = 0; conv < 2; conv++) {
        ResetComp();
        gAbsoluteTangents = conv == 1;
        gUndoGroups = 0;
        PF_OutData out;
        gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
        if (PressMakeNulls(out)) { std::printf("FAIL: button returned an error\n"); return 1; }
        size_t made = gLayers.size() - 2;
        std::printf("control nulls (%s tangents): %zu nulls, top \"%s\", undo groups %d, source %d\n",
                    conv ? "absolute" : "relative", made, gLayers[0]->name.c_str(), gUndoGroups, int(gDefs[RB_PATH_SOURCE - 1].u.pd.value));
        if (made != 3 || gLayers[0]->name != "Ribbon Pt 01" || gUndoGroups != 1 || gUndoDepth != 0) { std::printf("FAIL: nulls\n"); fails++; }
        if (gDefs[RB_PATH_SOURCE - 1].u.pd.value != RB_SOURCE_NULLS) { std::printf("FAIL: path source not switched\n"); fails++; }
        for (int i = 0; i < 3; i++) {
            SimLayer* l = gLayers[size_t(i)];
            const double* v = i == 0 ? kSegs[0].p[0] : (i == 1 ? kSegs[0].p[3] : kSegs[1].p[3]);
            if (!l->is3D || std::fabs(l->pos[0] - v[0]) > 1e-9 || std::fabs(l->pos[1] - v[1]) > 1e-9 || gEffectLayerIds[RB_NULL_1 + i] != l->id) {
                std::printf("FAIL: null %d position / slot\n", i); fails++;
            }
        }
        if (gEffectLayerIds[RB_NULL_1 + 3] != 0) { std::printf("FAIL: unused slot set\n"); fails++; }
        firstRot[conv] = gLayers[0]->rotZ;
        firstScale[conv] = gLayers[0]->scale[0];
    }
    std::printf("  Pt 01: rotation %.2f deg, X scale %.1f %% (both conventions: %.2f / %.1f)\n", firstRot[0], firstScale[0],
                firstRot[1], firstScale[1]);
    double wantRot = std::atan2(280.0 - 350.0, 200.0 - 90.0) * 180 / 3.14159265358979;
    if (std::fabs(firstRot[0] - wantRot) > 1e-6 || std::fabs(firstRot[0] - firstRot[1]) > 1e-9 || std::fabs(firstScale[0] - firstScale[1]) > 1e-9) {
        std::printf("FAIL: tangent convention detection\n"); fails++;
    }

    // The ribbon now follows the nulls and looks like the mask ribbon.
    gGuid.clear();
    std::vector<float> nullsImg = RunCPU(32);
    std::vector<unsigned char> key0 = gGuid;
    gGuid.clear();
    RunCPU(32);
    bool sameKey = gGuid == key0;
    gLayers[2]->pos[0] += 1;  // nudge the last null by one pixel
    gGuid.clear();
    RunCPU(32);
    bool movedKey = !gGuid.empty() && gGuid != key0;
    gLayers[2]->pos[0] -= 1;
    std::printf("  cache key: %zu bytes mixed in, stable when nothing moves: %s, changes when a null moves: %s\n", key0.size(),
                sameKey ? "yes" : "no", movedKey ? "yes" : "no");
    if (key0.empty() || !sameKey || !movedKey) { std::printf("FAIL: control nulls not in the cache key\n"); fails++; }
    double d = DiffFraction(maskImg, nullsImg);
    std::printf("  nulls vs mask: %.2f %% of pixels differ\n", 100 * d);
    if (d > 0.01) { std::printf("FAIL: nulls do not reproduce the mask\n"); fails++; }
    std::vector<float> gpu = RunGPU();
    double maxG = 0;
    for (size_t i = 0; i < gpu.size(); i++) maxG = std::max(maxG, double(std::fabs(gpu[i] - nullsImg[i])));
    std::printf("  CPU vs GPU with nulls: max diff %.2e\n", maxG);
    if (maxG > 1e-3) { std::printf("FAIL: GPU differs (nulls)\n"); fails++; }

    // Rotating the middle null around its X axis rolls the ribbon; X scale changes the handles.
    gLayers[1]->rotX = 90;
    double dr = DiffFraction(nullsImg, RunCPU(32));
    gLayers[1]->rotX = 0;
    gLayers[1]->scale[0] = 0;
    double ds = DiffFraction(nullsImg, RunCPU(32));
    gLayers[1]->scale[0] = 100;
    gLayers[1]->pos[2] = -300;
    double dp = DiffFraction(nullsImg, RunCPU(32));
    std::printf("  changed pixels: roll %.2f %%, scale 0 %.2f %%, move in Z %.2f %%\n", 100 * dr, 100 * ds, 100 * dp);
    if (dr < 0.005 || ds < 0.003 || dp < 0.005) { std::printf("FAIL: null transforms have no effect\n"); fails++; }
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    return fails;
}

// "Configure": the chosen layer becomes the texture and Width keeps its aspect on the visible ribbon.
static int TestConfigure() {
    int fails = 0;
    ResetComp();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    const double L = segLen(kSegs[0]) + segLen(kSegs[1]);  // the mask, untransformed
    SimLayer* footage = new SimLayer(); footage->id = 300; footage->source = {1920, 1080, {1, 1}};
    SimLayer* anamorphic = new SimLayer(); anamorphic->id = 301; anamorphic->source = {960, 1080, {2, 1}};
    SimLayer* text = new SimLayer(); text->id = 302;  // no source: comp-sized
    gLayers.insert(gLayers.begin(), {footage, anamorphic, text});
    struct Case { const char* what; A_long fit, tex; double start, end, repeat, want; A_long texAfter; };
    const Case cases[] = {
        {"footage 1920x1080", 300, 0, 0, 100, 1, L * 1080 / 1920, 300},
        {"Start 20 / End 70, repeat 2", 300, 0, 20, 70, 2, L * 0.5 / 2 * 1080 / 1920, 300},
        {"pixel aspect 2:1", 301, 0, 0, 100, 1, L * 1080 / 1920, 301},
        {"text layer (comp size)", 302, 0, 0, 100, 1, L * 540 / 960, 302},
        {"empty Fit Layer uses Texture Layer", 0, 300, 0, 100, 1, L * 1080 / 1920, 300},
    };
    for (const Case& c : cases) {
        gEffectLayerIds[RB_FIT_LAYER] = c.fit;
        gEffectLayerIds[RB_TEXTURE] = c.tex;
        gDefs[RB_START - 1].u.fs_d.value = c.start;
        gDefs[RB_END - 1].u.fs_d.value = c.end;
        gDefs[RB_TEX_REPEAT - 1].u.fs_d.value = c.repeat;
        gDefs[RB_WIDTH - 1].u.fs_d.value = 60;
        gUndoDepth = 0;
        PF_OutData out;
        PF_Err e = PressButton(RB_FIT_CONFIGURE, out);
        double w = gDefs[RB_WIDTH - 1].u.fs_d.value;
        std::printf("configure, %s: width %.2f (expected %.2f), texture layer %d\n", c.what, w, c.want, int(gEffectLayerIds[RB_TEXTURE]));
        if (e || (out.out_flags & PF_OutFlag_DISPLAY_ERROR_MESSAGE) || std::fabs(w - c.want) > 0.05 ||
            gEffectLayerIds[RB_TEXTURE] != c.texAfter || gUndoDepth != 0) {
            std::printf("FAIL: configure (%s)\n", c.what); fails++;
        }
    }
    // Nothing chosen: a message, Width untouched.
    gEffectLayerIds[RB_FIT_LAYER] = 0;
    gEffectLayerIds[RB_TEXTURE] = 0;
    gDefs[RB_WIDTH - 1].u.fs_d.value = 60;
    PF_OutData out;
    PressButton(RB_FIT_CONFIGURE, out);
    std::printf("configure without a layer: \"%s\"\n", out.return_msg);
    if (!(out.out_flags & PF_OutFlag_DISPLAY_ERROR_MESSAGE) || gDefs[RB_WIDTH - 1].u.fs_d.value != 60) { std::printf("FAIL: no-layer case\n"); fails++; }

    gDefs[RB_START - 1].u.fs_d.value = 0; gDefs[RB_END - 1].u.fs_d.value = 100; gDefs[RB_TEX_REPEAT - 1].u.fs_d.value = 1;
    gDefs[RB_WIDTH - 1].u.fs_d.value = 60;
    ResetComp();
    return fails;
}

// Offset past the end of the open mask: Cycle on brings the cut part back at the start.
static int TestCycle() {
    int fails = 0;
    ResetComp();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_START - 1].u.fs_d.value = 0;
    gDefs[RB_END - 1].u.fs_d.value = 40;
    gDefs[RB_OFFSET - 1].u.fs_d.value = 80;
    auto cover = [](const std::vector<float>& img, int x0, int x1) {
        size_t n = 0;
        for (int y = 0; y < H; y++)
            for (int x = x0; x < x1; x++) n += std::fabs(img[(size_t(y) * W + x) * 4 + 2] - 0.2f) > 0.02f;
        return n;
    };
    gDefs[RB_CYCLE - 1].u.bd.value = TRUE;
    std::vector<float> on = RunCPU(32);
    gDefs[RB_CYCLE - 1].u.bd.value = FALSE;
    std::vector<float> off = RunCPU(32);
    // The mask starts at x = 90 and ends at x = 870: the wrapped part shows up on the left.
    size_t leftOn = cover(on, 0, 300), leftOff = cover(off, 0, 300), rightOn = cover(on, 660, W), rightOff = cover(off, 660, W);
    std::printf("cycle: start of the path %zu px with Cycle, %zu without; end of the path %zu / %zu\n", leftOn, leftOff, rightOn, rightOff);
    if (leftOn < 2000 || leftOff > 0 || rightOn < 2000 || std::labs(long(rightOn) - long(rightOff)) > long(rightOn / 50)) {
        std::printf("FAIL: cycle\n"); fails++;
    }
    gDefs[RB_CYCLE - 1].u.bd.value = TRUE;
    gDefs[RB_END - 1].u.fs_d.value = 100;
    gDefs[RB_OFFSET - 1].u.fs_d.value = 0;
    return fails;
}

// Orientation = Front: the mask is the floor seen from above, the ribbon faces down. Edge-on for the
// default camera; tilted with Path Transform X Rotation one way the front (down) side shows, the
// other way the back side. CPU and GPU agree.
static int TestFront() {
    int fails = 0;
    ResetComp();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_ORIENTATION - 1].u.pd.value = RB_ORIENT_FRONT;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;  // ribbon only
    PF_Pixel oldFront = gDefs[RB_FRONT_COLOR - 1].u.cd.value, oldBack = gDefs[RB_BACK_COLOR - 1].u.cd.value;
    gDefs[RB_FRONT_COLOR - 1].u.cd.value = {255, 255, 0, 0};  // A R G B: red
    gDefs[RB_BACK_COLOR - 1].u.cd.value = {255, 0, 0, 255};   // blue
    auto count = [](const std::vector<float>& img, size_t& red, size_t& blue, size_t& any) {
        red = blue = any = 0;
        for (size_t i = 0; i < img.size(); i += 4) {
            if (img[i + 3] < 0.5f) continue;
            any++;
            red += img[i + 2] > img[i] + 0.1f;
            blue += img[i] > img[i + 2] + 0.1f;
        }
    };
    size_t r, b, a;
    count(RunCPU(32), r, b, a);
    std::printf("front, default camera: %zu px visible (edge-on)\n", a);
    if (a > 4000) { std::printf("FAIL: front ribbon not edge-on for the front camera\n"); fails++; }
    size_t seen[2][2] = {{0, 0}, {0, 0}};
    for (int k = 0; k < 2; k++) {
        gDefs[RB_XF_ROT_X - 1].u.ad.value = (k ? -60 : 60) * 65536;
        std::vector<float> c = RunCPU(32), g = RunGPU();
        double maxG = 0;
        for (size_t i = 0; i < c.size() && i < g.size(); i++) maxG = std::max(maxG, double(std::fabs(c[i] - g[i])));
        count(c, r, b, a);
        seen[k][0] = r; seen[k][1] = b;
        std::printf("front, X rotation %+d: %zu px, front color %zu, back color %zu, CPU vs GPU %.2e\n", k ? -60 : 60, a, r, b, maxG);
        if (a < 5000 || g.empty() || maxG > 1e-3) { std::printf("FAIL: front tilted render\n"); fails++; }
    }
    // One tilt shows the underside (front color), the other the top (back color).
    bool oneEach = (seen[0][0] > 10 * seen[0][1] && seen[1][1] > 10 * seen[1][0]) || (seen[1][0] > 10 * seen[1][1] && seen[0][1] > 10 * seen[0][0]);
    if (!oneEach) { std::printf("FAIL: front / back sides\n"); fails++; }

    // Configure gives the same width in both orientations (Front is a rigid rotation).
    SimLayer* footage = new SimLayer(); footage->id = 300; footage->source = {1920, 1080, {1, 1}};
    gLayers.insert(gLayers.begin(), footage);
    gEffectLayerIds[RB_FIT_LAYER] = 300;
    gDefs[RB_XF_ROT_X - 1].u.ad.value = 0;
    double w[2];
    for (int k = 0; k < 2; k++) {
        gDefs[RB_ORIENTATION - 1].u.pd.value = k ? RB_ORIENT_FRONT : RB_ORIENT_TWIST;
        PF_OutData out;
        PressButton(RB_FIT_CONFIGURE, out);
        w[k] = gDefs[RB_WIDTH - 1].u.fs_d.value;
    }
    std::printf("front, configure width: twist %.3f, front %.3f\n", w[0], w[1]);
    if (std::fabs(w[0] - w[1]) > 1e-6) { std::printf("FAIL: configure differs in Front\n"); fails++; }

    gDefs[RB_WIDTH - 1].u.fs_d.value = 60;
    gDefs[RB_ORIENTATION - 1].u.pd.value = RB_ORIENT_TWIST;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    gDefs[RB_FRONT_COLOR - 1].u.cd.value = oldFront;
    gDefs[RB_BACK_COLOR - 1].u.cd.value = oldBack;
    ResetComp();
    return fails;
}

// Environment (HDRI layer) and glass, CPU against GPU (OpenCL, with the HDRI read back from the device).
static int TestEnvGlass() {
    int fails = 0;
    ResetComp();
    MakeEnv();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;  // over the layer
    gDefs[RB_THICKNESS - 1].u.fs_d.value = 0;
    auto compare = [&](const char* what, double minChanged, const std::vector<float>& ref) {
        std::vector<float> c = RunCPU(32), g = RunGPU();
        double maxG = 0;
        for (size_t i = 0; i < c.size() && i < g.size(); i++) maxG = std::max(maxG, double(std::fabs(c[i] - g[i])));
        double changed = ref.empty() ? 1.0 : DiffFraction(ref, c);
        std::printf("%s: CPU vs GPU %.2e, %.2f %% of pixels differ from the reference\n", what, maxG, 100 * changed);
        if (g.empty() || maxG > 2e-3 || changed < minChanged) { std::printf("FAIL: %s\n", what); fails++; }
        return c;
    };
    std::vector<float> plain = RunCPU(32);
    gEnvOn = true;
    std::vector<float> lit = compare("environment (32 bpc HDRI)", 0.01, plain);
    std::vector<float> lit8 = RunCPU(8);
    double d8 = 0;
    for (size_t i = 0; i < lit.size(); i++) d8 = std::max(d8, double(std::fabs(std::min(lit[i], 1.0f) - lit8[i])));
    std::printf("environment, 8 bpc: max diff to 32 bpc %.3f (the 8-bit HDRI is clipped at 1)\n", d8);
    if (lit8.empty()) { std::printf("FAIL: 8 bpc environment\n"); fails++; }

    gDefs[RB_ENV_ROTATION - 1].u.ad.value = 120 * 65536;
    compare("environment rotated 120 degrees", 0.005, lit);
    gDefs[RB_ENV_ROTATION - 1].u.ad.value = 0;

    // Glass over striped input: refraction shifts the stripes seen through the ribbon.
    gStripes = true;
    gDefs[RB_GLASS - 1].u.bd.value = TRUE;
    gDefs[RB_GLASS_REFRACTION - 1].u.fs_d.value = 0;
    std::vector<float> straight = compare("glass, no refraction", 0.0, {});
    gDefs[RB_GLASS_REFRACTION - 1].u.fs_d.value = 60;
    compare("glass, refraction 60 px", 0.01, straight);
    // Ribbon only: glass is see-through (alpha between 0 and 1 inside the ribbon).
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    std::vector<float> alone = RunCPU(32);
    size_t partial = 0, opaque = 0;
    for (size_t i = 3; i < alone.size(); i += 4) { partial += alone[i] > 0.02f && alone[i] < 0.9f; opaque += alone[i] >= 0.999f; }
    std::printf("glass, ribbon only: %zu see-through pixels, %zu opaque\n", partial, opaque);
    if (partial < 5000 || opaque > partial / 4) { std::printf("FAIL: glass transparency\n"); fails++; }
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    // Thick glass: the far side of the solid is seen through the near one.
    gDefs[RB_THICKNESS - 1].u.fs_d.value = 14;
    gDefs[RB_BEVEL_SIZE - 1].u.fs_d.value = 4;
    compare("thick glass", 0.005, straight);

    gDefs[RB_THICKNESS - 1].u.fs_d.value = 0;
    gDefs[RB_GLASS - 1].u.bd.value = FALSE;
    gDefs[RB_GLASS_REFRACTION - 1].u.fs_d.value = 30;
    gEnvOn = false;
    gStripes = false;
    ResetComp();
    return fails;
}

// Wind: the ribbon moves over time, the pinned start stays where it is; CPU and GPU agree.
static int TestWind() {
    int fails = 0;
    ResetComp();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    std::vector<float> still = RunCPU(32);
    gDefs[RB_WIND_STRENGTH - 1].u.fs_d.value = 100;
    std::vector<float> f1 = RunCPU(32), g1 = RunGPU();
    gTimeOffset = 7;  // another frame: 7 frames later
    std::vector<float> f2 = RunCPU(32);
    gTimeOffset = 0;
    double maxG = 0;
    for (size_t i = 0; i < f1.size() && i < g1.size(); i++) maxG = std::max(maxG, double(std::fabs(f1[i] - g1[i])));
    // Pinned start: the mask begins at (90, 350); the area around it looks the same in every frame.
    auto region = [](const std::vector<float>& a, const std::vector<float>& b, int x0, int y0, int x1, int y1) {
        size_t n = 0;
        for (int y = y0; y < y1; y++)
            for (int x = x0; x < x1; x++) n += std::fabs(a[(size_t(y) * W + x) * 4 + 3] - b[(size_t(y) * W + x) * 4 + 3]) > 0.1f;
        return n;
    };
    size_t pinDiff = region(f1, f2, 60, 320, 120, 380), moved = region(f1, f2, 0, 0, W, H), windVsStill = region(still, f1, 0, 0, W, H);
    std::printf("wind: %zu px change between frames, %zu px near the pinned start, %zu px differ from no wind, CPU vs GPU %.2e\n",
                moved, pinDiff, windVsStill, maxG);
    if (moved < 2000 || pinDiff > 30 || windVsStill < 2000 || g1.empty() || maxG > 2e-3) { std::printf("FAIL: wind\n"); fails++; }
    gDefs[RB_WIND_STRENGTH - 1].u.fs_d.value = 0;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    return fails;
}

int main(int argc, char** argv) {
    ResetComp();
    PF_InData in = MakeInData();
    PF_OutData out; std::memset(&out, 0, sizeof(out));
    if (EffectMain(PF_Cmd_GLOBAL_SETUP, &in, &out, nullptr, nullptr, nullptr)) return 1;
    if (EffectMain(PF_Cmd_PARAMS_SETUP, &in, &out, nullptr, nullptr, nullptr)) return 1;
    std::printf("params: %zu added, num_params %d (expected %d incl. input)\n", gDefs.size(), out.num_params, RB_NUM_PARAMS);
    int fails = 0;
    if (int(gDefs.size()) + 1 != out.num_params) { std::printf("FAIL: param count mismatch\n"); fails++; }
    if (gDefs[RB_COMPOSITE - 1].u.pd.value != 3) { std::printf("FAIL: Composite does not default to Ribbon Only\n"); fails++; }
    gMixFlag = (out.out_flags2 & PF_OutFlag2_I_MIX_GUID_DEPENDENCIES) != 0;
    if (!gMixFlag) { std::printf("FAIL: I_MIX_GUID_DEPENDENCIES not set\n"); fails++; }

    cl_platform_id plat; clGetPlatformIDs(1, &plat, nullptr);
    clGetDeviceIDs(plat, CL_DEVICE_TYPE_ALL, 1, &gDev, nullptr);
    gCtx = clCreateContext(nullptr, 1, &gDev, nullptr, nullptr, nullptr);
    gQ = clCreateCommandQueue(gCtx, gDev, 0, nullptr);

    // Default (Ribbon Only): transparent around the ribbon, on CPU and GPU alike.
    {
        std::vector<float> a = RunCPU(32), ag = RunGPU();
        size_t clear = 0, opaque = 0;
        double maxA = 0;
        for (size_t i = 0; i < a.size(); i += 4) {
            clear += a[i + 3] == 0.0f;
            opaque += a[i + 3] > 0.99f;
            if (!ag.empty()) maxA = std::max(maxA, double(std::fabs(a[i + 3] - ag[i + 3])));
        }
        std::printf("ribbon only: %zu transparent px, %zu opaque px, CPU vs GPU alpha max diff %.2e\n", clear, opaque, maxA);
        if (clear < size_t(W * H / 2) || opaque < 5000 || ag.empty() || maxA > 1e-3) { std::printf("FAIL: ribbon only\n"); fails++; }
    }
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;  // the checks below compare with the grey input: Over Layer
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

    gDefs[RB_THICKNESS - 1].u.fs_d.value = 0;
    fails += TestControlNulls(c32);
    fails += TestConfigure();
    fails += TestCycle();
    fails += TestFront();
    fails += TestEnvGlass();
    fails += TestWind();

    if (argc > 1) {
        FILE* f = std::fopen(argv[1], "wb");
        std::fprintf(f, "P6 %d %d 255\n", W, H);
        for (int i = 0; i < W * H; i++) for (int c = 2; c >= 0; c--) std::fputc(int(std::min(1.0f, std::max(0.0f, g[i * 4 + c])) * 255 + 0.5f), f);
        std::fclose(f);
    }
    if (gVerifyFailures) std::printf("FAIL: %d pre-renders AE would reject\n", gVerifyFailures);
    fails += gVerifyFailures;
    std::printf(fails ? "host simulation FAILED\n" : "host simulation OK\n");
    return fails;
}
