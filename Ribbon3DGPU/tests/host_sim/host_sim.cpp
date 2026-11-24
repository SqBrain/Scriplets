// Fake After Effects host: drives EffectMain through setup, pre-render and render (CPU 8/32 bpc
// and GPU via OpenCL), with a mask path, a comp light and no camera. Uses the SDK stub headers.
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include <algorithm>
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
// Two masks on the layer: id 7 (the one in "Path") and id 8, the same shape moved down by gMask2Dy.
static int gNumMasks = 2;
static double gMask2Dy = 150;
static PF_Err NumPaths(PF_ProgPtr, A_long* n) { *n = gNumMasks; return PF_Err_NONE; }
static PF_Err PathInfo(PF_ProgPtr, A_long i, PF_PathID* id) { *id = PF_PathID(7 + i); return PF_Err_NONE; }
// Mask id 9: a closed 40 x 20 rectangle at (700, 60), used as a cross-section profile.
static const double kRect[4][2] = {{700, 60}, {740, 60}, {740, 80}, {700, 80}};
static bool IsRect(PF_PathOutlinePtr p) { return p == (PF_PathOutlinePtr)0x3; }
static PF_Err CheckoutPath(PF_ProgPtr, PF_PathID id, A_long, A_long, A_u_long, PF_PathOutlinePtr* p) {
    *p = id == 7 ? (PF_PathOutlinePtr)0x1 : (id == 8 ? (PF_PathOutlinePtr)0x2 : (id == 9 ? (PF_PathOutlinePtr)0x3 : nullptr));
    return PF_Err_NONE;
}
static double MaskDy(PF_PathOutlinePtr p) { return p == (PF_PathOutlinePtr)0x2 ? gMask2Dy : 0.0; }
static PF_Err CheckinPath(PF_ProgPtr, PF_PathID, PF_Boolean, PF_PathOutlinePtr) { return PF_Err_NONE; }
static PF_Err IsOpen(PF_ProgPtr, PF_PathOutlinePtr p, PF_Boolean* o) { *o = IsRect(p) ? FALSE : TRUE; return PF_Err_NONE; }
static PF_Err NumSegs(PF_ProgPtr, PF_PathOutlinePtr p, A_long* n) { *n = IsRect(p) ? 4 : 2; return PF_Err_NONE; }
static PF_Err Prep(PF_ProgPtr, PF_PathOutlinePtr, A_long, A_long, PF_PathSegPrepPtr* pp) { *pp = (PF_PathSegPrepPtr)0x2; return PF_Err_NONE; }
static PF_Err GetLen(PF_ProgPtr, PF_PathOutlinePtr p, A_long s, PF_PathSegPrepPtr*, PF_FpLong* l) {
    if (IsRect(p)) { const double* a = kRect[s]; const double* b = kRect[(s + 1) % 4]; *l = std::hypot(b[0] - a[0], b[1] - a[1]); }
    else *l = segLen(kSegs[s]);
    return PF_Err_NONE;
}
static PF_Err Eval(PF_ProgPtr, PF_PathOutlinePtr path, PF_PathSegPrepPtr*, A_long s, PF_FpLong len, PF_FpLong* x, PF_FpLong* y) {
    if (IsRect(path)) {
        const double* a = kRect[s]; const double* b = kRect[(s + 1) % 4];
        const double L = std::hypot(b[0] - a[0], b[1] - a[1]), f = L > 0 ? len / L : 0;
        *x = a[0] + (b[0] - a[0]) * f; *y = a[1] + (b[1] - a[1]) * f;
        return PF_Err_NONE;
    }
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
    *y += MaskDy(path);
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
static PF_Err VertexInfo(PF_ProgPtr, PF_PathOutlinePtr path, A_long i, PF_PathVertex* v) {
    if (IsRect(path)) {
        v->x = kRect[i % 4][0]; v->y = kRect[i % 4][1];
        v->tan_in_x = v->tan_in_y = v->tan_out_x = v->tan_out_y = gAbsoluteTangents ? 0 : 0;
        if (gAbsoluteTangents) { v->tan_in_x = v->tan_out_x = v->x; v->tan_in_y = v->tan_out_y = v->y; }
        return PF_Err_NONE;
    }
    // Vertex i of the two-segment mask; handles relative or absolute depending on gAbsoluteTangents.
    const double* p = i == 0 ? kSegs[0].p[0] : (i == 1 ? kSegs[0].p[3] : kSegs[1].p[3]);
    const double* in = i == 0 ? kSegs[0].p[0] : (i == 1 ? kSegs[0].p[2] : kSegs[1].p[2]);
    const double* out = i == 0 ? kSegs[0].p[1] : (i == 1 ? kSegs[1].p[1] : kSegs[1].p[3]);
    const double dy = MaskDy(path);
    v->x = p[0]; v->y = p[1] + dy;
    double o = gAbsoluteTangents ? 0 : 1;
    v->tan_in_x = in[0] - o * p[0]; v->tan_in_y = (in[1] + dy) - o * (p[1] + dy);
    v->tan_out_x = out[0] - o * p[0]; v->tan_out_y = (out[1] + dy) - o * (p[1] + dy);
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

static PF_PathQuerySuite1 sPQ = {NumPaths, PathInfo, CheckoutPath, CheckinPath};
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
// Normal map layer: round bumps (OpenGL convention), 64 x 64.
static const int NMW = 64, NMH = 64;
static bool gNrmOn = false;
static std::vector<float> gNrmPixels;
static void MakeNormalMap() {
    gNrmPixels.assign(size_t(NMW) * NMH * 4, 0.0f);
    for (int y = 0; y < NMH; y++)
        for (int x = 0; x < NMW; x++) {
            double dx = (x + 0.5) / NMW - 0.5, dy = (y + 0.5) / NMH - 0.5, r = std::hypot(dx, dy);
            double nx = r < 0.4 ? dx * 2 : 0, ny = r < 0.4 ? -dy * 2 : 0, nz = std::sqrt(std::max(0.05, 1 - nx * nx - ny * ny));
            float* p = &gNrmPixels[(size_t(y) * NMW + x) * 4];
            p[0] = float(nz * 0.5 + 0.5); p[1] = float(ny * 0.5 + 0.5); p[2] = float(nx * 0.5 + 0.5); p[3] = 1;
        }
}
// Roughness layer: vertical bands, smooth (0.05) and rough (0.8).
static bool gRoughOn = false;
static std::vector<float> gRoughPixels;
static void MakeRoughness() {
    gRoughPixels.assign(size_t(NMW) * NMH * 4, 0.0f);
    for (int y = 0; y < NMH; y++)
        for (int x = 0; x < NMW; x++) {
            float g = (x / 16) % 2 ? 0.8f : 0.05f;
            float* p = &gRoughPixels[(size_t(y) * NMW + x) * 4];
            p[0] = p[1] = p[2] = g; p[3] = 1;
        }
}
// Gradient layer: 120 x 9, transparent margins, red -> blue between x 20 and 99, fading out over
// the last 20 visible pixels (like a shape layer's gradient-filled rectangle).
static const int GW = 120, GH = 9;
static bool gGradOn = false;
static std::vector<float> gGradPixels;
static void MakeGradientLayer() {
    gGradPixels.assign(size_t(GW) * GH * 4, 0.0f);
    for (int y = 0; y < GH; y++)
        for (int x = 20; x < 100; x++) {
            float f = (x - 20) / 79.0f, a = x >= 80 ? 1 - (x - 79) / 21.0f : 1;
            float* p = &gGradPixels[(size_t(y) * GW + x) * 4];
            p[0] = f * a; p[1] = 0; p[2] = (1 - f) * a; p[3] = a;  // BGRA premultiplied
        }
}
static float InputGrey(int x) { return gStripes ? ((x / 12) % 2 ? 0.85f : 0.1f) : 0.2f; }
static PF_Err CheckoutLayer(PF_ProgPtr, A_long index, A_long, const PF_RenderRequest* req, A_long, A_long, A_u_long, PF_CheckoutResult* r) {
    std::memset(r, 0, sizeof(*r));
    if (index == RB_INPUT) {
        r->max_result_rect = Full();
        r->result_rect = req->rect;
    }
    if (index == RB_GRAD_LAYER && gGradOn) {
        r->max_result_rect = {0, 0, GW, GH};
        r->result_rect = req->rect;
    }
    if (index == RB_ROUGH_LAYER && gRoughOn) {
        r->max_result_rect = {0, 0, NMW, NMH};
        r->result_rect = req->rect;
    }
    if (index == RB_NRM_LAYER && gNrmOn) {
        r->max_result_rect = {0, 0, NMW, NMH};
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
static PF_EffectWorld gSrc, gDst, gEnv, gNrm, gRough, gGrad;
static PF_Err CheckoutPixels(PF_ProgPtr, A_long id, PF_EffectWorld** w) {
    *w = id == RB_CHECKOUT_INPUT ? &gSrc
       : id == RB_CHECKOUT_ENV   ? &gEnv
       : id == RB_CHECKOUT_NRM   ? &gNrm
       : id == RB_CHECKOUT_ROUGH ? &gRough
       : id == RB_CHECKOUT_GRAD  ? &gGrad
                                 : nullptr;
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
    std::vector<char> nbuf;
    if (gNrmOn) {  // normal map at the render bit depth
        size_t npx = bpc == 32 ? 16 : 4;
        nbuf.resize(size_t(NMW) * NMH * npx);
        for (int i = 0; i < NMW * NMH; i++) {
            const float* e = &gNrmPixels[size_t(i) * 4];
            if (bpc == 32) { PF_PixelFloat* p = (PF_PixelFloat*)nbuf.data() + i; p->blue = e[0]; p->green = e[1]; p->red = e[2]; p->alpha = e[3]; }
            else { PF_Pixel8* p = (PF_Pixel8*)nbuf.data() + i; p->blue = (unsigned char)(e[0] * 255 + 0.5f); p->green = (unsigned char)(e[1] * 255 + 0.5f); p->red = (unsigned char)(e[2] * 255 + 0.5f); p->alpha = 255; }
        }
        gNrm = {nbuf.data(), A_long(NMW * npx), NMW, NMH};
    }
    std::vector<char> rbuf;
    if (gRoughOn) {
        size_t rpx = bpc == 32 ? 16 : 4;
        rbuf.resize(size_t(NMW) * NMH * rpx);
        for (int i = 0; i < NMW * NMH; i++) {
            const float* e = &gRoughPixels[size_t(i) * 4];
            if (bpc == 32) { PF_PixelFloat* p = (PF_PixelFloat*)rbuf.data() + i; p->blue = e[0]; p->green = e[1]; p->red = e[2]; p->alpha = e[3]; }
            else { PF_Pixel8* p = (PF_Pixel8*)rbuf.data() + i; p->blue = p->green = p->red = (unsigned char)(e[0] * 255 + 0.5f); p->alpha = 255; }
        }
        gRough = {rbuf.data(), A_long(NMW * rpx), NMW, NMH};
    }
    std::vector<char> gbuf;
    if (gGradOn) {
        size_t gpx = bpc == 32 ? 16 : 4;
        gbuf.resize(size_t(GW) * GH * gpx);
        for (int i = 0; i < GW * GH; i++) {
            const float* e = &gGradPixels[size_t(i) * 4];
            if (bpc == 32) { PF_PixelFloat* p = (PF_PixelFloat*)gbuf.data() + i; p->blue = e[0]; p->green = e[1]; p->red = e[2]; p->alpha = e[3]; }
            else { PF_Pixel8* p = (PF_Pixel8*)gbuf.data() + i; p->blue = (unsigned char)(e[0] * 255 + 0.5f); p->green = (unsigned char)(e[1] * 255 + 0.5f); p->red = (unsigned char)(e[2] * 255 + 0.5f); p->alpha = (unsigned char)(e[3] * 255 + 0.5f); }
        }
        gGrad = {gbuf.data(), A_long(GW * gpx), GW, GH};
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
    cl_mem nrmMem = nullptr;
    if (gNrmOn) {
        cl_int ee;
        nrmMem = clCreateBuffer(gCtx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, gNrmPixels.size() * 4, gNrmPixels.data(), &ee);
        gNrm = {nrmMem, NMW * 16, NMW, NMH};
    }
    cl_mem gradMem = nullptr;
    if (gGradOn) {
        cl_int ee;
        gradMem = clCreateBuffer(gCtx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, gGradPixels.size() * 4, gGradPixels.data(), &ee);
        gGrad = {gradMem, GW * 16, GW, GH};
    }
    cl_mem roughMem = nullptr;
    if (gRoughOn) {
        cl_int ee;
        roughMem = clCreateBuffer(gCtx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, gRoughPixels.size() * 4, gRoughPixels.data(), &ee);
        gRough = {roughMem, NMW * 16, NMW, NMH};
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
    if (nrmMem) clReleaseMemObject(nrmMem);
    if (roughMem) clReleaseMemObject(roughMem);
    if (gradMem) clReleaseMemObject(gradMem);
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
    std::vector<float> refracted = compare("glass, refraction 60 px", 0.01, straight);
    // Frosted: the stripes seen through the glass lose their contrast.
    gDefs[RB_GLASS_FROST - 1].u.fs_d.value = 20;
    std::vector<float> frosted = compare("glass, frost 20 px", 0.01, refracted);
    // Ribbon coverage: the same glass alone (Ribbon Only) has alpha > 0 only on the ribbon.
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    std::vector<float> cover = RunCPU(32);
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    // Sharpness of the stripes seen through the glass: mean change between neighbouring pixels,
    // well inside the ribbon (both neighbours fully covered).
    auto stripeContrast = [&](const std::vector<float>& img) {
        double sum = 0;
        size_t n = 0;
        for (int y = 0; y < H; y++)
            for (int x = 0; x + 1 < W; x++) {
                size_t i = (size_t(y) * W + x) * 4;
                if (cover[i + 3] < 0.2f || cover[i + 7] < 0.2f) continue;
                sum += std::fabs(img[i + 6] - img[i + 2]);
                n++;
            }
        return n ? sum / n : 0.0;
    };
    double cClear = stripeContrast(refracted), cFrost = stripeContrast(frosted);
    std::printf("glass, frost: stripe sharpness through the glass %.4f -> %.4f\n", cClear, cFrost);
    if (!(cFrost < cClear * 0.6)) { std::printf("FAIL: frost does not blur\n"); fails++; }
    gDefs[RB_GLASS_FROST - 1].u.fs_d.value = 0;
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

// Normal map layer: changes the shading, CPU and GPU agree, tiles follow Tile Size.
static int TestNormalMap() {
    int fails = 0;
    ResetComp();
    MakeNormalMap();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    std::vector<float> plain = RunCPU(32);
    gNrmOn = true;
    std::vector<float> c = RunCPU(32), g = RunGPU(), c8 = RunCPU(8);
    double maxG = 0, max8 = 0;
    for (size_t i = 0; i < c.size() && i < g.size(); i++) maxG = std::max(maxG, double(std::fabs(c[i] - g[i])));
    for (size_t i = 0; i < c.size(); i++) max8 = std::max(max8, double(std::fabs(std::min(c[i], 1.0f) - c8[i])));
    gDefs[RB_NRM_TILE - 1].u.fs_d.value = 40;
    std::vector<float> small = RunCPU(32);
    double changed = DiffFraction(plain, c), retiled = DiffFraction(c, small);
    std::printf("normal map: %.1f %% of pixels change, tile 40 px vs 100 px %.1f %%, CPU vs GPU %.2e, 8 vs 32 bpc %.3f\n", 100 * changed,
                100 * retiled, maxG, max8);
    if (changed < 0.02 || retiled < 0.01 || g.empty() || maxG > 2e-3 || max8 > 0.05) { std::printf("FAIL: normal map\n"); fails++; }
    gDefs[RB_NRM_TILE - 1].u.fs_d.value = 100;
    gNrmOn = false;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    return fails;
}

// Roughness layer through the effect (CPU / GPU), rounded ends take area off the tips.
static int TestRoughEnds() {
    int fails = 0;
    ResetComp();
    MakeRoughness();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    std::vector<float> plain = RunCPU(32);
    gRoughOn = true;
    std::vector<float> c = RunCPU(32), g = RunGPU();
    gRoughOn = false;
    double maxG = 0;
    for (size_t i = 0; i < c.size() && i < g.size(); i++) maxG = std::max(maxG, double(std::fabs(c[i] - g[i])));
    double changed = DiffFraction(plain, c);
    std::printf("roughness map: %.1f %% of pixels change, CPU vs GPU %.2e\n", 100 * changed, maxG);
    if (changed < 0.01 || g.empty() || maxG > 2e-3) { std::printf("FAIL: roughness map\n"); fails++; }
    gDefs[RB_WIDTH - 1].u.fs_d.value = 80;
    std::vector<float> square = RunCPU(32);
    gDefs[RB_END_ROUNDING - 1].u.fs_d.value = 100;
    std::vector<float> round = RunCPU(32), roundG = RunGPU();
    double maxR = 0, aSq = 0, aRd = 0;
    for (size_t i = 0; i < round.size() && i < roundG.size(); i++) maxR = std::max(maxR, double(std::fabs(round[i] - roundG[i])));
    for (size_t i = 3; i < round.size(); i += 4) { aSq += square[i]; aRd += round[i]; }
    // A semicircle instead of a w x w/2 rectangle saves (1 - pi/4) * w^2 / 2 per end: ~687 px for w = 80.
    std::printf("rounded ends: area %.0f -> %.0f px (%.0f less), CPU vs GPU %.2e\n", aSq, aRd, aSq - aRd, maxR);
    if (aSq - aRd < 1000 || aSq - aRd > 1800 || roundG.empty() || maxR > 2e-3) { std::printf("FAIL: rounded ends\n"); fails++; }
    gDefs[RB_END_ROUNDING - 1].u.fs_d.value = 0;
    gDefs[RB_WIDTH - 1].u.fs_d.value = 60;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    return fails;
}

// Cross sections in the single effect: round tube, custom profile from mask 9; CPU = GPU.
static int TestSections() {
    int fails = 0;
    ResetComp();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    auto area = [](const std::vector<float>& img) { double s = 0; for (size_t i = 3; i < img.size(); i += 4) s += img[i]; return s; };
    std::vector<float> flat = RunCPU(32);
    gDefs[RB_SECTION - 1].u.pd.value = 2;
    std::vector<float> tube = RunCPU(32), tubeG = RunGPU();
    gDefs[RB_SECTION - 1].u.pd.value = 3;
    gDefs[RB_PROFILE_MASK - 1].u.path_d.path_id = 9;
    std::vector<float> bar = RunCPU(32), barG = RunGPU();
    auto maxDiff = [](const std::vector<float>& a, const std::vector<float>& b) {
        double d = b.empty() ? 1 : 0;
        for (size_t i = 0; i < a.size() && i < b.size(); i++) d = std::max(d, double(std::fabs(a[i] - b[i])));
        return d;
    };
    // The twisted flat ribbon narrows where it turns edge-on; a tube of the same width does not.
    std::printf("sections: area flat %.0f, round %.0f, custom 2:1 bar %.0f; CPU vs GPU round %.2e, custom %.2e\n", area(flat), area(tube),
                area(bar), maxDiff(tube, tubeG), maxDiff(bar, barG));
    if (area(tube) < area(flat) * 1.1 || area(bar) < area(flat) * 0.5 || DiffFraction(tube, bar) < 0.01 || maxDiff(tube, tubeG) > 2e-3 ||
        maxDiff(bar, barG) > 2e-3) {
        std::printf("FAIL: cross sections\n"); fails++;
    }
    if (!(gDefs[RB_TRIM_MASKS - 1].ui_flags & PF_PUI_INVISIBLE)) { std::printf("FAIL: Trim Masks visible in the single-mask effect\n"); fails++; }
    gDefs[RB_SECTION - 1].u.pd.value = 1;
    gDefs[RB_PROFILE_MASK - 1].u.path_d.path_id = 0;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    return fails;
}

static void MeanColor(const std::vector<float>& img, int x0, int x1, int y0, int y1, double rgb[3]);

// Repeater: copies with steps and random position / rotation / width / offset; the first copy is
// the original; deterministic per seed; per-copy gradient colors.
static int TestRepeater() {
    int fails = 0;
    ResetComp();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    auto area = [](const std::vector<float>& img, int y0, int y1) {
        double s = 0;
        for (int y = y0; y < y1; y++)
            for (int x = 0; x < W; x++) s += img[(size_t(y) * W + x) * 4 + 3];
        return s;
    };
    auto maxDiff = [](const std::vector<float>& a, const std::vector<float>& b) {
        double d = b.size() == a.size() ? 0 : 1;
        for (size_t i = 0; i < a.size() && i < b.size(); i++) d = std::max(d, double(std::fabs(a[i] - b[i])));
        return d;
    };
    std::vector<float> one = RunCPU(32);
    gDefs[RB_REP_POS_X - 1].u.fs_d.value = 80;  // random settings do nothing with a single copy
    gDefs[RB_REP_WIDTH - 1].u.fs_d.value = 50;
    std::vector<float> still = RunCPU(32);
    gDefs[RB_REP_POS_X - 1].u.fs_d.value = 0;
    gDefs[RB_REP_WIDTH - 1].u.fs_d.value = 0;
    gDefs[RB_REP_COPIES - 1].u.sd.value = 4;
    gDefs[RB_REP_STEP_Y - 1].u.fs_d.value = 70;
    std::vector<float> four = RunCPU(32), fourG = RunGPU();
    std::printf("repeater: 1 copy %.0f px (unchanged %s), 4 copies (step Y 70) %.0f px, CPU vs GPU %.2e\n", area(one, 0, H),
                maxDiff(one, still) == 0 ? "yes" : "no", area(four, 0, H), maxDiff(four, fourG));
    if (maxDiff(one, still) != 0 || area(four, 0, H) < 1.8 * area(one, 0, H) || maxDiff(four, fourG) > 2e-3) { std::printf("FAIL: repeater copies\n"); fails++; }
    // Random position / rotation / width / offset: same seed = same picture, another seed differs.
    gDefs[RB_REP_POS_X - 1].u.fs_d.value = 60;
    gDefs[RB_REP_POS_Z - 1].u.fs_d.value = 200;
    gDefs[RB_REP_ROT - 1].u.fs_d.value = 20;
    gDefs[RB_REP_TILT - 1].u.fs_d.value = 15;
    gDefs[RB_REP_WIDTH - 1].u.fs_d.value = 60;
    gDefs[RB_REP_OFFSET - 1].u.fs_d.value = 20;
    gDefs[RB_END - 1].u.fs_d.value = 70;
    std::vector<float> r1 = RunCPU(32), r1b = RunCPU(32), r1g = RunGPU();
    gDefs[RB_REP_SEED - 1].u.sd.value = 7;
    std::vector<float> r2 = RunCPU(32);
    gDefs[RB_REP_SEED - 1].u.sd.value = 0;
    // Where copies cross at the same depth, CPU and GPU rounding may pick different ribbons for a
    // sample: count such pixels instead of taking the maximum.
    size_t off = 0;
    for (size_t i = 0; i + 3 < r1.size() && i + 3 < r1g.size(); i += 4) {
        double d = 0;
        for (int k = 0; k < 4; k++) d = std::max(d, double(std::fabs(r1[i + k] - r1g[i + k])));
        if (d > 2e-3) {
            if (off < 5) std::printf("  pixel %zu,%zu differs by %.3f\n", (i / 4) % W, (i / 4) / W, d);
            off++;
        }
    }
    std::printf("repeater random: same seed max diff %.1e, other seed %.1f %% of pixels differ, vs no randomness %.1f %%, CPU vs GPU: %zu px differ\n",
                maxDiff(r1, r1b), 100 * DiffFraction(r1, r2), 100 * DiffFraction(four, r1), off);
    if (maxDiff(r1, r1b) != 0 || DiffFraction(r1, r2) < 0.01 || DiffFraction(four, r1) < 0.01 || r1g.empty() || off > 40) {
        std::printf("FAIL: repeater randomness\n"); fails++;
    }
    // Per-copy colors: two copies far apart, flat shading, Random per Path.
    for (int i : {RB_REP_POS_X, RB_REP_POS_Z, RB_REP_ROT, RB_REP_TILT, RB_REP_WIDTH, RB_REP_OFFSET}) gDefs[size_t(i - 1)].u.fs_d.value = 0;
    gDefs[RB_END - 1].u.fs_d.value = 100;
    gDefs[RB_REP_COPIES - 1].u.sd.value = 2;
    gDefs[RB_REP_STEP_Y - 1].u.fs_d.value = 250;
    gDefs[RB_AMBIENT - 1].u.fs_d.value = 1;
    gDefs[RB_DIFFUSE - 1].u.fs_d.value = gDefs[RB_SPECULAR - 1].u.fs_d.value = 0;
    gDefs[RB_SHEEN - 1].u.fs_d.value = gDefs[RB_TRANSLUCENCY - 1].u.fs_d.value = 0;
    gDefs[RB_GRAD_MODE - 1].u.pd.value = RB_GRAD_RANDOM;
    gDefs[RB_GRAD_COUNT - 1].u.pd.value = 1;
    gDefs[RB_GRAD_COLOR_1 - 1].u.cd.value = {255, 255, 0, 0};
    gDefs[RB_GRAD_COLOR_2 - 1].u.cd.value = {255, 0, 0, 255};
    double best = 0;
    for (int seed = 0; seed < 4; seed++) {
        gDefs[RB_GRAD_SEED - 1].u.sd.value = seed;
        std::vector<float> c = RunCPU(32);
        double a[3], b[3];
        MeanColor(c, 0, W, 120, 300, a);  // the original only
        MeanColor(c, 0, W, 430, H, b);    // the copy only
        best = std::max(best, std::fabs(a[0] - b[0]));
    }
    std::printf("repeater + Random per Path: copies differ in red by up to %.2f\n", best);
    if (best < 0.15) { std::printf("FAIL: per-copy colors\n"); fails++; }
    ResetComp();
    gDefs[RB_REP_COPIES - 1].u.sd.value = 1;
    gDefs[RB_REP_STEP_Y - 1].u.fs_d.value = 0;
    gDefs[RB_GRAD_MODE - 1].u.pd.value = RB_GRAD_OFF;
    gDefs[RB_GRAD_SEED - 1].u.sd.value = 0;
    gDefs[RB_AMBIENT - 1].u.fs_d.value = 0.15; gDefs[RB_DIFFUSE - 1].u.fs_d.value = 0.9; gDefs[RB_SPECULAR - 1].u.fs_d.value = 0.6;
    gDefs[RB_SHEEN - 1].u.fs_d.value = 0.25; gDefs[RB_TRANSLUCENCY - 1].u.fs_d.value = 0.15;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    return fails;
}

// Mean color of the ribbon pixels (alpha > 0.5) in columns x0..x1 (Ribbon Only renders).
static void MeanColor(const std::vector<float>& img, int x0, int x1, int y0, int y1, double rgb[3]) {
    double n = 0;
    rgb[0] = rgb[1] = rgb[2] = 0;
    for (int y = y0; y < y1; y++)
        for (int x = x0; x < x1; x++) {
            const float* p = &img[(size_t(y) * W + x) * 4];
            if (p[3] < 0.5f) continue;
            rgb[0] += p[2] / p[3]; rgb[1] += p[1] / p[3]; rgb[2] += p[0] / p[3];
            n++;
        }
    if (n > 0) { rgb[0] /= n; rgb[1] /= n; rgb[2] /= n; }
}

// Saved projects identify parameters by disk ID: unique, and unchanged by the new layout.
static int TestDiskIds() {
    int fails = 0;
    std::vector<A_long> ids;
    for (const PF_ParamDef& d : gDefs) ids.push_back(d.uu.id);
    std::vector<A_long> sorted = ids;
    std::sort(sorted.begin(), sorted.end());
    bool unique = std::adjacent_find(sorted.begin(), sorted.end()) == sorted.end();
    // IDs of 0.1.11 (index in the old order + 1) and the new parameters after them.
    struct Known { int param; A_long id; const char* name; } known[] = {
        {RB_PATH, 2, "Path"}, {RB_WIDTH, 7, "Width"}, {RB_TEXTURE, 29, "Texture Layer"}, {RB_GLASS_FROST, RB_DISK_ID(RB_GLASS_FROST), "Frost (px)"},
        {RB_NRM_LAYER, RB_DISK_ID(RB_NRM_LAYER), "Normal Map Layer"}, {RB_GRAD_LAYER, RB_DISK_ID(RB_GRAD_LAYER), "Gradient Layer"}};
    bool ok = unique;
    for (const Known& k : known) {
        const PF_ParamDef& d = gDefs[size_t(k.param - 1)];
        if (d.uu.id != k.id || std::strcmp(d.PF_DEF_NAME, k.name) != 0) {
            std::printf("  param %d: id %d name \"%s\", expected id %d \"%s\"\n", k.param, int(d.uu.id), d.PF_DEF_NAME, int(k.id), k.name);
            ok = false;
        }
    }
    const bool newAfterOld = RB_DISK_ID(RB_GRAD_SOURCE) > RB_DISK_ID(RB_TRIM_MASKS) && RB_DISK_ID(RB_GRAD_LAYER) > RB_DISK_ID(RB_TRIM_MASKS);
    // Frost now sits in the Glass group, the surface maps in Material.
    const bool layout = RB_GLASS_TOPIC < RB_GLASS_FROST && RB_GLASS_FROST < RB_GLASS_TOPIC_END && RB_LOOK_TOPIC < RB_NRM_TOPIC &&
                        RB_ROUGH_TOPIC_END < RB_LOOK_TOPIC_END;
    std::printf("disk IDs: %zu unique: %s, old IDs kept: %s, layout: %s\n", ids.size(), unique ? "yes" : "no", ok ? "yes" : "no",
                layout ? "ok" : "wrong");
    if (!ok || !newAfterOld || !layout) { std::printf("FAIL: disk IDs\n"); fails++; }
    return fails;
}

// Gradient along the path: red-ish at the start of the mask, blue-ish at its end; CPU = GPU.
static int TestGradient() {
    int fails = 0;
    ResetComp();
    gDefs[RB_PATH_SOURCE - 1].u.pd.value = RB_SOURCE_MASK;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    gDefs[RB_TWIST - 1].u.fs_d.value = 0;
    gDefs[RB_GRAD_MODE - 1].u.pd.value = RB_GRAD_ALONG;
    gDefs[RB_GRAD_COUNT - 1].u.pd.value = 1;  // 2 colors
    gDefs[RB_GRAD_COLOR_1 - 1].u.cd.value = {255, 255, 0, 0};
    gDefs[RB_GRAD_COLOR_2 - 1].u.cd.value = {255, 0, 0, 255};
    std::vector<float> c = RunCPU(32), g = RunGPU();
    double maxG = 0;
    for (size_t i = 0; i < c.size() && i < g.size(); i++) maxG = std::max(maxG, double(std::fabs(c[i] - g[i])));
    double a[3], b[3];
    MeanColor(c, 80, 160, 0, H, a);   // mask start (x 90)
    MeanColor(c, 780, 880, 0, H, b);  // mask end (x 870)
    std::printf("gradient along path: start rgb %.2f %.2f %.2f, end rgb %.2f %.2f %.2f, CPU vs GPU %.2e\n", a[0], a[1], a[2], b[0], b[1], b[2], maxG);
    if (!(a[0] > a[2] * 2 && b[2] > b[0] * 2) || g.empty() || maxG > 2e-3) { std::printf("FAIL: gradient along path\n"); fails++; }
    // Random per Path works for Repeater copies too: the seed is visible in the single-mask effect.
    if (gDefs[RB_GRAD_SEED - 1].ui_flags & PF_PUI_INVISIBLE) { std::printf("FAIL: gradient seed hidden\n"); fails++; }
    // From a layer (AE's own gradient editor): its colors, and its opacity fades the ribbon's end.
    MakeGradientLayer();
    gGradOn = true;
    gDefs[RB_GRAD_SOURCE - 1].u.pd.value = RB_GRAD_FROM_LAYER;
    gDefs[RB_GRAD_COLOR_1 - 1].u.cd.value = {255, 0, 255, 0};  // colors are ignored now (green)
    gDefs[RB_GRAD_COLOR_2 - 1].u.cd.value = {255, 0, 255, 0};
    std::vector<float> l = RunCPU(32), lg = RunGPU();
    gGradOn = false;
    gDefs[RB_GRAD_SOURCE - 1].u.pd.value = RB_GRAD_FROM_COLORS;
    double ml = 0;
    for (size_t i = 0; i < l.size() && i < lg.size(); i++) ml = std::max(ml, double(std::fabs(l[i] - lg[i])));
    double s0[3], s1[3];
    MeanColor(l, 80, 160, 0, H, s0);
    MeanColor(l, 600, 700, 0, H, s1);
    double aStart = 0, aEnd = 0, nS = 0, nE = 0;
    for (int y = 0; y < H; y++) {
        for (int x = 90; x < 150; x++) { float a = l[(size_t(y) * W + x) * 4 + 3]; if (a > 0.01f) { aStart += a; nS++; } }
        for (int x = 820; x < 870; x++) { float a = l[(size_t(y) * W + x) * 4 + 3]; if (a > 0.01f) { aEnd += a; nE++; } }
    }
    aStart /= std::max(nS, 1.0); aEnd /= std::max(nE, 1.0);
    std::printf("gradient layer: start rgb %.2f %.2f %.2f, middle rgb %.2f %.2f %.2f, opacity start %.2f end %.2f, CPU vs GPU %.2e\n", s0[0],
                s0[1], s0[2], s1[0], s1[1], s1[2], aStart, aEnd, ml);
    // Red start and blue middle from the layer, not the (green) Color 1 / 2; lighting adds some white.
    if (!(s0[0] > s0[2] * 2 && s0[1] < s0[0] * 0.5 && s1[2] > s1[0] && s1[1] < s1[2] * 0.5 && aStart > 0.95 && aEnd < 0.5) || lg.empty() ||
        ml > 2e-3) {
        std::printf("FAIL: gradient layer\n"); fails++;
    }
    gDefs[RB_GRAD_MODE - 1].u.pd.value = RB_GRAD_OFF;
    gDefs[RB_TWIST - 1].u.fs_d.value = 1;
    gDefs[RB_COMPOSITE - 1].u.pd.value = 1;
    return fails;
}

// Multi-mask effect: a ribbon along each of the two masks, trimmed simultaneously, gradient per path.
static int TestMultiMask() {
    int fails = 0;
    ResetComp();
    gDefs[RB_COMPOSITE - 1].u.pd.value = 3;
    gDefs[RB_TWIST - 1].u.fs_d.value = 0;
    auto area = [](const std::vector<float>& img, int y0, int y1) {
        double s = 0;
        for (int y = y0; y < y1; y++)
            for (int x = 0; x < W; x++) s += img[(size_t(y) * W + x) * 4 + 3];
        return s;
    };
    gNumMasks = 1;
    std::vector<float> one = RunCPU(32);
    gNumMasks = 2;
    std::vector<float> two = RunCPU(32), twoG = RunGPU();
    double maxG = 0;
    for (size_t i = 0; i < two.size() && i < twoG.size(); i++) maxG = std::max(maxG, double(std::fabs(two[i] - twoG[i])));
    const double a1 = area(one, 0, H), a2 = area(two, 0, H);
    std::printf("multi-mask: one mask %.0f px, two masks %.0f px, CPU vs GPU %.2e\n", a1, a2, maxG);
    if (a2 < a1 * 1.7 || a2 > a1 * 2.3 || twoG.empty() || maxG > 2e-3) { std::printf("FAIL: two masks\n"); fails++; }
    // Simultaneously: End 50 % halves both ribbons.
    gDefs[RB_END - 1].u.fs_d.value = 50;
    std::vector<float> half = RunCPU(32);
    gDefs[RB_END - 1].u.fs_d.value = 100;
    // Lower half of the frame holds mostly mask 2 (moved down 150 px), upper half mask 1.
    const double r1 = area(half, 0, 270) / area(two, 0, 270), r2 = area(half, 270, H) / area(two, 270, H);
    std::printf("multi-mask, End 50 %%: upper part %.2f, lower part %.2f of the full ribbons\n", r1, r2);
    if (r1 > 0.8 || r2 > 0.8 || r1 < 0.2 || r2 < 0.2) { std::printf("FAIL: simultaneous trim\n"); fails++; }
    // Individually: the two masks are one path (mask 1, then mask 2); End 50 % draws all of mask 1
    // and nothing of mask 2. Offset 50 % then: all of mask 2, nothing of mask 1.
    gDefs[RB_TRIM_MASKS - 1].u.pd.value = 2;
    gDefs[RB_END - 1].u.fs_d.value = 50;
    std::vector<float> ind = RunCPU(32), indG = RunGPU();
    gDefs[RB_OFFSET - 1].u.fs_d.value = 50;
    std::vector<float> indOff = RunCPU(32);
    gDefs[RB_OFFSET - 1].u.fs_d.value = 0;
    gDefs[RB_END - 1].u.fs_d.value = 100;
    gDefs[RB_TRIM_MASKS - 1].u.pd.value = 1;
    double gi = 0;
    for (size_t i = 0; i < ind.size() && i < indG.size(); i++) gi = std::max(gi, double(std::fabs(ind[i] - indG[i])));
    // Ribbon 1 covers y 120..410, ribbon 2 (150 px lower, 60 px wide) 280..560: compare rows only one covers.
    const double i1 = area(ind, 120, 270) / area(two, 120, 270), i2 = area(ind, 415, H) / area(two, 415, H);
    const double o1 = area(indOff, 120, 270) / area(two, 120, 270), o2 = area(indOff, 415, H) / area(two, 415, H);
    std::printf("multi-mask Individually, End 50 %%: mask 1 %.2f, mask 2 %.2f; + Offset 50 %%: %.2f / %.2f; CPU vs GPU %.2e\n", i1, i2, o1,
                o2, gi);
    if (i1 < 0.95 || i2 > 0.05 || o1 > 0.05 || o2 < 0.95 || indG.empty() || gi > 2e-3) { std::printf("FAIL: individually\n"); fails++; }
    // A custom profile from mask 9: that mask is the cross section, not a third ribbon.
    gNumMasks = 3;
    gDefs[RB_SECTION - 1].u.pd.value = 3;
    gDefs[RB_PROFILE_MASK - 1].u.path_d.path_id = 9;
    std::vector<float> prof = RunCPU(32);
    double nearRect = 0;
    for (int y = 40; y < 100; y++)
        for (int x = 680; x < 760; x++) nearRect += prof[(size_t(y) * W + x) * 4 + 3];
    std::printf("multi-mask with a profile mask: coverage around the profile mask %.0f px\n", nearRect);
    if (nearRect > 5 || area(prof, 0, H) < area(two, 0, H) * 0.3) { std::printf("FAIL: profile mask drawn as a ribbon\n"); fails++; }
    gNumMasks = 2;
    gDefs[RB_SECTION - 1].u.pd.value = 1;
    gDefs[RB_PROFILE_MASK - 1].u.path_d.path_id = 0;
    // Unused parameters are hidden.
    const int hidden[] = {RB_PATH, RB_NULLS_TOPIC, RB_PATH_SOURCE, RB_MAKE_NULLS, RB_NULLS_CLOSED, RB_NULL_1, RB_NULL_1 + RB_MAX_NULLS - 1};
    bool allHidden = true;
    for (int i : hidden) allHidden = allHidden && (gDefs[size_t(i - 1)].ui_flags & PF_PUI_INVISIBLE);
    if (!allHidden || (gDefs[RB_GRAD_SEED - 1].ui_flags & PF_PUI_INVISIBLE)) { std::printf("FAIL: hidden parameters\n"); fails++; }
    // Cache key: moving the second mask changes it.
    gGuid.clear(); RunCPU(32);
    std::vector<unsigned char> k0 = gGuid;
    gMask2Dy = 160;
    gGuid.clear(); RunCPU(32);
    bool moved = gGuid != k0;
    gMask2Dy = 150;
    std::printf("multi-mask cache key: %zu bytes, changes when the second mask moves: %s\n", k0.size(), moved ? "yes" : "no");
    if (!moved) { std::printf("FAIL: masks not in the cache key\n"); fails++; }
    // Random per path: each ribbon one color from the gradient (red .. blue), different seeds differ.
    // Flat shading (ambient light only), so a pixel's color is the gradient color itself.
    const double keep[5] = {gDefs[RB_AMBIENT - 1].u.fs_d.value, gDefs[RB_DIFFUSE - 1].u.fs_d.value, gDefs[RB_SPECULAR - 1].u.fs_d.value,
                            gDefs[RB_SHEEN - 1].u.fs_d.value, gDefs[RB_TRANSLUCENCY - 1].u.fs_d.value};
    gDefs[RB_AMBIENT - 1].u.fs_d.value = 1;
    gDefs[RB_DIFFUSE - 1].u.fs_d.value = gDefs[RB_SPECULAR - 1].u.fs_d.value = 0;
    gDefs[RB_SHEEN - 1].u.fs_d.value = gDefs[RB_TRANSLUCENCY - 1].u.fs_d.value = 0;
    gDefs[RB_GRAD_MODE - 1].u.pd.value = RB_GRAD_RANDOM;
    gDefs[RB_GRAD_COUNT - 1].u.pd.value = 1;
    gDefs[RB_GRAD_COLOR_1 - 1].u.cd.value = {255, 255, 0, 0};
    gDefs[RB_GRAD_COLOR_2 - 1].u.cd.value = {255, 0, 0, 255};
    double best = 0;
    for (int seed = 0; seed < 4; seed++) {
        gDefs[RB_GRAD_SEED - 1].u.sd.value = seed;
        std::vector<float> c = RunCPU(32), g = RunGPU();
        double m1[3], m2[3], e1[3], e2[3], gg = 0;
        for (size_t i = 0; i < c.size() && i < g.size(); i++) gg = std::max(gg, double(std::fabs(c[i] - g[i])));
        MeanColor(c, 80, 160, 300, 400, m1);   // start of mask 1 (y 350)
        MeanColor(c, 780, 880, 250, 350, e1);  // end of mask 1 (y 320)
        MeanColor(c, 80, 160, 450, 540, m2);   // start of mask 2 (y 500)
        MeanColor(c, 780, 880, 420, 520, e2);  // end of mask 2 (y 470)
        const double along1 = std::fabs(m1[0] - e1[0]), along2 = std::fabs(m2[0] - e2[0]), between = std::fabs(m1[0] - m2[0]);
        std::printf("  seed %d: path 1 r %.2f, path 2 r %.2f (along each path %.3f / %.3f), CPU vs GPU %.2e\n", seed, m1[0], m2[0], along1, along2, gg);
        if (along1 > 0.02 || along2 > 0.02 || gg > 2e-3) { std::printf("FAIL: per-path color not constant\n"); fails++; }
        best = std::max(best, between);
    }
    if (best < 0.15) { std::printf("FAIL: per-path colors do not differ\n"); fails++; }
    gDefs[RB_AMBIENT - 1].u.fs_d.value = keep[0]; gDefs[RB_DIFFUSE - 1].u.fs_d.value = keep[1];
    gDefs[RB_SPECULAR - 1].u.fs_d.value = keep[2]; gDefs[RB_SHEEN - 1].u.fs_d.value = keep[3];
    gDefs[RB_TRANSLUCENCY - 1].u.fs_d.value = keep[4];
    gDefs[RB_GRAD_MODE - 1].u.pd.value = RB_GRAD_OFF;
    gDefs[RB_TWIST - 1].u.fs_d.value = 1;
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

#ifdef RB_MULTI_MASK
    // The multi-mask effect (same sources, built with RB_MULTI_MASK).
    std::printf("effect: %s\n", RB_NAME);
    fails += TestMultiMask();
    std::printf(fails ? "host simulation (multi-mask) FAILED\n" : "host simulation (multi-mask) OK\n");
    return fails;
#endif

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
    fails += TestNormalMap();
    fails += TestRoughEnds();
    fails += TestDiskIds();
    fails += TestGradient();
    fails += TestSections();
    fails += TestRepeater();

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
