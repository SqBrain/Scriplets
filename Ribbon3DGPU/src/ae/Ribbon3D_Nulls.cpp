#include "Ribbon3D_Nulls.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <vector>

#include "Ribbon3D_Suite.h"

namespace {

const double kPi = 3.14159265358979323846;

// Effects have no AEGP plug-in ID of their own; AE hands one out on request (as in the SDK's
// Supervisor sample). Needed for AEGP calls that modify the project or read streams.
AEGP_PluginID PluginID(PF_InData* in_data) {
    static std::once_flag once;
    static AEGP_PluginID id = 0;
    std::call_once(once, [&] {
        Suite<RB_UTILITY_SUITE> us(in_data, kAEGPUtilitySuite, RB_UTILITY_SUITE_VERSION);
        if (us) us->AEGP_RegisterWithAEGP(nullptr, "Ribbon3D GPU", &id);
    });
    return id;
}

// ---------------------------------------------------------------- mask vertices

double Dist2ToBezier(double px, double py, const double b[4][2]) {
    double best = 1e300;
    for (int i = 0; i <= 64; i++) {
        double t = i / 64.0, u = 1 - t;
        double x = u * u * u * b[0][0] + 3 * u * u * t * b[1][0] + 3 * u * t * t * b[2][0] + t * t * t * b[3][0];
        double y = u * u * u * b[0][1] + 3 * u * u * t * b[1][1] + 3 * u * t * t * b[2][1] + t * t * t * b[3][1];
        best = std::min(best, (x - px) * (x - px) + (y - py) * (y - py));
    }
    return best;
}

// Reads the mask's vertices with handle vectors relative to their vertex. The SDK does not say
// whether PF_PathVertex tangents are absolute or relative, so both readings are checked against
// points evaluated on the real curve and the matching one is used.
bool ReadMaskVertices(PF_InData* in_data, PF_PathID id, std::vector<RibbonMaskVertex>& out, bool& closed) {
    if (id == PF_PathID_NONE) return false;
    Suite<PF_PathQuerySuite1> pq(in_data, kPFPathQuerySuite, kPFPathQuerySuiteVersion1);
    Suite<PF_PathDataSuite1> pd(in_data, kPFPathDataSuite, kPFPathDataSuiteVersion1);
    if (!pq || !pd) return false;
    PF_PathOutlinePtr path = nullptr;
    if (pq->PF_CheckoutPath(in_data->effect_ref, id, in_data->current_time, in_data->time_step, in_data->time_scale, &path) ||
        !path)
        return false;
    PF_Boolean open = TRUE;
    A_long nseg = 0;
    pd->PF_PathIsOpen(in_data->effect_ref, path, &open);
    pd->PF_PathNumSegments(in_data->effect_ref, path, &nseg);
    closed = !open;
    const A_long nv = open ? nseg + 1 : nseg;
    std::vector<PF_PathVertex> raw(size_t(std::max<A_long>(nv, 0)));
    for (A_long i = 0; i < nv; i++) pd->PF_PathVertexInfo(in_data->effect_ref, path, i, &raw[size_t(i)]);

    double errRel = 0, errAbs = 0;
    for (A_long s = 0; s < nseg; s++) {
        const PF_PathVertex& a = raw[size_t(s)];
        const PF_PathVertex& b = raw[size_t((s + 1) % nv)];
        double rel[4][2] = {{a.x, a.y}, {a.x + a.tan_out_x, a.y + a.tan_out_y}, {b.x + b.tan_in_x, b.y + b.tan_in_y}, {b.x, b.y}};
        double abs_[4][2] = {{a.x, a.y}, {a.tan_out_x, a.tan_out_y}, {b.tan_in_x, b.tan_in_y}, {b.x, b.y}};
        PF_PathSegPrepPtr prep = nullptr;
        PF_FpLong len = 0;
        if (pd->PF_PathPrepareSegLength(in_data->effect_ref, path, s, 64, &prep)) continue;
        pd->PF_PathGetSegLength(in_data->effect_ref, path, s, &prep, &len);
        for (int q = 1; q < 4; q++) {
            PF_FpLong x = 0, y = 0;
            pd->PF_PathEvalSegLength(in_data->effect_ref, path, &prep, s, len * q / 4, &x, &y);
            errRel += Dist2ToBezier(x, y, rel);
            errAbs += Dist2ToBezier(x, y, abs_);
        }
        pd->PF_PathCleanupSegLength(in_data->effect_ref, path, s, &prep);
    }
    pq->PF_CheckinPath(in_data->effect_ref, id, FALSE, path);

    const bool relative = errRel <= errAbs;
    out.clear();
    for (const PF_PathVertex& v : raw) {
        RibbonMaskVertex m;
        m.x = v.x;
        m.y = v.y;
        m.inX = relative ? v.tan_in_x : v.tan_in_x - v.x;
        m.inY = relative ? v.tan_in_y : v.tan_in_y - v.y;
        m.outX = relative ? v.tan_out_x : v.tan_out_x - v.x;
        m.outY = relative ? v.tan_out_y : v.tan_out_y - v.y;
        out.push_back(m);
    }
    return out.size() >= 2;
}

// ---------------------------------------------------------------- streams

bool SetLayerStream(const RB_STREAM_SUITE* st, AEGP_PluginID pid, AEGP_LayerH layer, AEGP_LayerStream which, const A_Time& t,
                    double x, double y, double z, bool oneD) {
    AEGP_StreamRefH s = nullptr;
    if (st->AEGP_GetNewLayerStream(pid, layer, which, &s) || !s) return false;
    AEGP_StreamValue2 v;
    AEFX_CLR_STRUCT(v);
    bool ok = !st->AEGP_GetNewStreamValue(pid, s, AEGP_LTimeMode_CompTime, &t, TRUE, &v);
    if (ok) {
        if (oneD) {
            v.val.one_d = x;
        } else {
            v.val.three_d.x = x;
            v.val.three_d.y = y;
            v.val.three_d.z = z;
        }
        ok = !st->AEGP_SetStreamValue(pid, s, &v);
        st->AEGP_DisposeStreamValue(&v);
    }
    st->AEGP_DisposeStream(s);
    return ok;
}

bool GetLayerStream3(const RB_STREAM_SUITE* st, AEGP_PluginID pid, AEGP_LayerH layer, AEGP_LayerStream which, const A_Time& t,
                     double out[3]) {
    AEGP_StreamRefH s = nullptr;
    if (st->AEGP_GetNewLayerStream(pid, layer, which, &s) || !s) return false;
    AEGP_StreamValue2 v;
    AEFX_CLR_STRUCT(v);
    bool ok = !st->AEGP_GetNewStreamValue(pid, s, AEGP_LTimeMode_CompTime, &t, FALSE, &v);
    if (ok) {
        out[0] = v.val.three_d.x;
        out[1] = v.val.three_d.y;
        out[2] = v.val.three_d.z;
        st->AEGP_DisposeStreamValue(&v);
    }
    st->AEGP_DisposeStream(s);
    return ok;
}

}  // namespace

// ---------------------------------------------------------------- vertex -> null

void RibbonVertexToNull(const std::vector<RibbonMaskVertex>& v, size_t i, bool closed, double& rotationDeg,
                        double& scalePercent) {
    const size_t n = v.size();
    const RibbonMaskVertex& c = v[i];
    const bool hasPrev = closed || i > 0, hasNext = closed || i + 1 < n;
    const RibbonMaskVertex& prev = v[(i + n - 1) % n];
    const RibbonMaskVertex& next = v[(i + 1) % n];
    const double lo = std::hypot(c.outX, c.outY), li = std::hypot(c.inX, c.inY);
    // Tangent: average of the out handle and the reversed in handle, else the neighbour chord.
    double dx = 0, dy = 0;
    if (lo > 1e-6) { dx += c.outX / lo; dy += c.outY / lo; }
    if (li > 1e-6) { dx -= c.inX / li; dy -= c.inY / li; }
    if (std::hypot(dx, dy) < 1e-6) {
        double ax = hasPrev ? prev.x : c.x, ay = hasPrev ? prev.y : c.y;
        double bx = hasNext ? next.x : c.x, by = hasNext ? next.y : c.y;
        dx = bx - ax;
        dy = by - ay;
    }
    rotationDeg = std::hypot(dx, dy) > 1e-9 ? std::atan2(dy, dx) * 180 / kPi : 0;
    // Handle length relative to the spline's default (a third of the neighbour distance).
    double sum = 0;
    int cnt = 0;
    if (hasNext) {
        double chord = std::hypot(next.x - c.x, next.y - c.y) / 3;
        if (chord > 1e-6) { sum += lo / chord; cnt++; }
    }
    if (hasPrev) {
        double chord = std::hypot(prev.x - c.x, prev.y - c.y) / 3;
        if (chord > 1e-6) { sum += li / chord; cnt++; }
    }
    scalePercent = cnt ? 100 * sum / cnt : 100;
}

// ---------------------------------------------------------------- button

PF_Err RibbonMakeNullsFromMask(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[]) {
    std::vector<RibbonMaskVertex> verts;
    bool closed = false;
    if (!ReadMaskVertices(in_data, params[RB_PATH]->u.path_d.path_id, verts, closed)) {
        std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                      "Ribbon 3D GPU: draw a mask on this layer and pick it in \"Path\" first.");
        out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
        return PF_Err_NONE;
    }
    const size_t n = std::min(verts.size(), size_t(RB_MAX_NULLS));

    Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    Suite<RB_LAYER_SUITE> ls(in_data, kAEGPLayerSuite, RB_LAYER_SUITE_VERSION);
    Suite<RB_STREAM_SUITE> ss(in_data, kAEGPStreamSuite, RB_STREAM_SUITE_VERSION);
    Suite<RB_COMP_SUITE> cs(in_data, kAEGPCompSuite, RB_COMP_SUITE_VERSION);
    Suite<RB_UTILITY_SUITE> us(in_data, kAEGPUtilitySuite, RB_UTILITY_SUITE_VERSION);
    Suite<RB_EFFECT_SUITE> es(in_data, kAEGPEffectSuite, RB_EFFECT_SUITE_VERSION);
    if (!pfi || !ls || !ss || !cs || !us || !es) return PF_Err_INTERNAL_STRUCT_DAMAGED;
    const AEGP_PluginID pid = PluginID(in_data);

    AEGP_LayerH self = nullptr;
    AEGP_CompH comp = nullptr;
    A_Time compTime = {0, 1};
    if (pfi->AEGP_GetEffectLayer(in_data->effect_ref, &self) || !self) return PF_Err_INTERNAL_STRUCT_DAMAGED;
    if (ls->AEGP_GetLayerParentComp(self, &comp) || !comp) return PF_Err_INTERNAL_STRUCT_DAMAGED;
    pfi->AEGP_ConvertEffectToCompTime(in_data->effect_ref, in_data->current_time, in_data->time_scale, &compTime);

    us->AEGP_StartUndoGroup("Ribbon 3D: Create Nulls from Mask");
    std::vector<AEGP_LayerIDVal> ids;
    // Created in reverse so that "Pt 01" ends up on top of the timeline.
    std::vector<AEGP_LayerIDVal> rev;
    for (size_t k = n; k-- > 0;) {
        char name[64];
        std::snprintf(name, sizeof(name), "Ribbon Pt %02d", int(k + 1));
        A_UTF16Char name16[64];
        size_t c = 0;
        for (; name[c] && c + 1 < 64; c++) name16[c] = A_UTF16Char(name[c]);
        name16[c] = 0;
        AEGP_LayerH nl = nullptr;
        if (cs->AEGP_CreateNullInComp(name16, comp, nullptr, &nl) || !nl) break;
        ls->AEGP_SetLayerFlag(nl, AEGP_LayerFlag_LAYER_IS_3D, TRUE);
        double rot = 0, scale = 100;
        RibbonVertexToNull(verts, k, closed, rot, scale);
        // The effect layer is comp-sized and untransformed: mask coordinates are comp coordinates.
        SetLayerStream(ss.get(), pid, nl, AEGP_LayerStream_POSITION, compTime, verts[k].x, verts[k].y, 0, false);
        SetLayerStream(ss.get(), pid, nl, AEGP_LayerStream_ROTATION, compTime, rot, 0, 0, true);
        SetLayerStream(ss.get(), pid, nl, AEGP_LayerStream_SCALE, compTime, scale, 100, 100, false);
        AEGP_LayerIDVal lid = 0;
        ls->AEGP_GetLayerID(nl, &lid);
        rev.push_back(lid);
    }
    ids.assign(rev.rbegin(), rev.rend());

    // Fill the layer slots (layer parameters can only be set through their streams).
    AEGP_EffectRefH effect = nullptr;
    if (!pfi->AEGP_GetNewEffectForEffect(pid, in_data->effect_ref, &effect) && effect) {
        for (int i = 0; i < RB_MAX_NULLS; i++) {
            AEGP_StreamRefH s = nullptr;
            if (ss->AEGP_GetNewEffectStreamByIndex(pid, effect, RB_NULL_1 + i, &s) || !s) continue;
            AEGP_StreamValue2 v;
            AEFX_CLR_STRUCT(v);
            if (!ss->AEGP_GetNewStreamValue(pid, s, AEGP_LTimeMode_CompTime, &compTime, TRUE, &v)) {
                v.val.layer_id = size_t(i) < ids.size() ? ids[size_t(i)] : 0;
                ss->AEGP_SetStreamValue(pid, s, &v);
                ss->AEGP_DisposeStreamValue(&v);
            }
            ss->AEGP_DisposeStream(s);
        }
        es->AEGP_DisposeEffect(effect);
    }
    us->AEGP_EndUndoGroup();

    // Plain parameters: through params[] as AE expects during USER_CHANGED_PARAM.
    params[RB_PATH_SOURCE]->u.pd.value = RB_SOURCE_NULLS;
    params[RB_PATH_SOURCE]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
    params[RB_NULLS_CLOSED]->u.bd.value = closed ? 1 : 0;
    params[RB_NULLS_CLOSED]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
    out_data->out_flags |= PF_OutFlag_FORCE_RERENDER | PF_OutFlag_REFRESH_UI;

    if (verts.size() > n) {
        std::snprintf(out_data->return_msg, sizeof(out_data->return_msg),
                      "Ribbon 3D GPU: the mask has %d points, nulls were created for the first %d.", int(verts.size()),
                      RB_MAX_NULLS);
        out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
    }
    return PF_Err_NONE;
}

// ---------------------------------------------------------------- layer parameters

AEGP_LayerIDVal RibbonGetLayerParam(PF_InData* in_data, int paramIndex, const A_Time& compTime) {
    Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    Suite<RB_STREAM_SUITE> ss(in_data, kAEGPStreamSuite, RB_STREAM_SUITE_VERSION);
    Suite<RB_EFFECT_SUITE> es(in_data, kAEGPEffectSuite, RB_EFFECT_SUITE_VERSION);
    if (!pfi || !ss || !es) return 0;
    const AEGP_PluginID pid = PluginID(in_data);
    AEGP_EffectRefH effect = nullptr;
    if (pfi->AEGP_GetNewEffectForEffect(pid, in_data->effect_ref, &effect) || !effect) return 0;
    AEGP_LayerIDVal id = 0;
    AEGP_StreamRefH s = nullptr;
    if (!ss->AEGP_GetNewEffectStreamByIndex(pid, effect, paramIndex, &s) && s) {
        AEGP_StreamValue2 v;
        AEFX_CLR_STRUCT(v);
        if (!ss->AEGP_GetNewStreamValue(pid, s, AEGP_LTimeMode_CompTime, &compTime, FALSE, &v)) {
            id = v.val.layer_id;
            ss->AEGP_DisposeStreamValue(&v);
        }
        ss->AEGP_DisposeStream(s);
    }
    es->AEGP_DisposeEffect(effect);
    return id;
}

bool RibbonSetLayerParam(PF_InData* in_data, int paramIndex, const A_Time& compTime, AEGP_LayerIDVal id) {
    Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    Suite<RB_STREAM_SUITE> ss(in_data, kAEGPStreamSuite, RB_STREAM_SUITE_VERSION);
    Suite<RB_EFFECT_SUITE> es(in_data, kAEGPEffectSuite, RB_EFFECT_SUITE_VERSION);
    if (!pfi || !ss || !es) return false;
    const AEGP_PluginID pid = PluginID(in_data);
    AEGP_EffectRefH effect = nullptr;
    if (pfi->AEGP_GetNewEffectForEffect(pid, in_data->effect_ref, &effect) || !effect) return false;
    bool ok = false;
    AEGP_StreamRefH s = nullptr;
    if (!ss->AEGP_GetNewEffectStreamByIndex(pid, effect, paramIndex, &s) && s) {
        AEGP_StreamValue2 v;
        AEFX_CLR_STRUCT(v);
        if (!ss->AEGP_GetNewStreamValue(pid, s, AEGP_LTimeMode_CompTime, &compTime, TRUE, &v)) {
            v.val.layer_id = id;
            ok = !ss->AEGP_SetStreamValue(pid, s, &v);
            ss->AEGP_DisposeStreamValue(&v);
        }
        ss->AEGP_DisposeStream(s);
    }
    es->AEGP_DisposeEffect(effect);
    return ok;
}

bool RibbonLayerSourceSize(PF_InData* in_data, AEGP_LayerIDVal id, double& width, double& height) {
    Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    Suite<RB_LAYER_SUITE> ls(in_data, kAEGPLayerSuite, RB_LAYER_SUITE_VERSION);
    Suite<RB_COMP_SUITE> cs(in_data, kAEGPCompSuite, RB_COMP_SUITE_VERSION);
    Suite<RB_ITEM_SUITE> is(in_data, kAEGPItemSuite, RB_ITEM_SUITE_VERSION);
    if (!pfi || !ls || !cs || !is || !id) return false;
    AEGP_LayerH self = nullptr, layer = nullptr;
    AEGP_CompH comp = nullptr;
    if (pfi->AEGP_GetEffectLayer(in_data->effect_ref, &self) || !self) return false;
    if (ls->AEGP_GetLayerParentComp(self, &comp) || !comp) return false;
    if (ls->AEGP_GetLayerFromLayerID(comp, id, &layer) || !layer) return false;
    AEGP_ItemH item = nullptr;
    if (ls->AEGP_GetLayerSourceItem(layer, &item) || !item) {
        item = nullptr;  // text and shape layers render comp-sized
        if (cs->AEGP_GetItemFromComp(comp, &item) || !item) return false;
    }
    A_long w = 0, h = 0;
    if (is->AEGP_GetItemDimensions(item, &w, &h) || w <= 0 || h <= 0) return false;
    A_Ratio par = {1, 1};
    if (is->AEGP_GetItemPixelAspectRatio(item, &par) || par.num <= 0 || par.den == 0) par = {1, 1};
    width = double(w) * double(par.num) / double(par.den);
    height = double(h);
    return true;
}

// ---------------------------------------------------------------- render

bool RibbonReadControlPoints(PF_InData* in_data, const A_Time& compTime, std::vector<rb::ControlPoint>& cps) {
    cps.clear();
    Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    Suite<RB_LAYER_SUITE> ls(in_data, kAEGPLayerSuite, RB_LAYER_SUITE_VERSION);
    Suite<RB_STREAM_SUITE> ss(in_data, kAEGPStreamSuite, RB_STREAM_SUITE_VERSION);
    Suite<RB_EFFECT_SUITE> es(in_data, kAEGPEffectSuite, RB_EFFECT_SUITE_VERSION);
    if (!pfi || !ls || !ss || !es) return false;
    const AEGP_PluginID pid = PluginID(in_data);
    AEGP_LayerH self = nullptr;
    AEGP_CompH comp = nullptr;
    AEGP_EffectRefH effect = nullptr;
    if (pfi->AEGP_GetEffectLayer(in_data->effect_ref, &self) || !self) return false;
    if (ls->AEGP_GetLayerParentComp(self, &comp) || !comp) return false;
    if (pfi->AEGP_GetNewEffectForEffect(pid, in_data->effect_ref, &effect) || !effect) return false;

    for (int i = 0; i < RB_MAX_NULLS; i++) {
        AEGP_StreamRefH s = nullptr;
        if (ss->AEGP_GetNewEffectStreamByIndex(pid, effect, RB_NULL_1 + i, &s) || !s) continue;
        AEGP_StreamValue2 v;
        AEFX_CLR_STRUCT(v);
        AEGP_LayerIDVal lid = 0;
        if (!ss->AEGP_GetNewStreamValue(pid, s, AEGP_LTimeMode_CompTime, &compTime, FALSE, &v)) {
            lid = v.val.layer_id;
            ss->AEGP_DisposeStreamValue(&v);
        }
        ss->AEGP_DisposeStream(s);
        AEGP_LayerH L = nullptr;
        if (!lid || ls->AEGP_GetLayerFromLayerID(comp, lid, &L) || !L) continue;

        A_Matrix4 m;
        if (ls->AEGP_GetLayerToWorldXform(L, &compTime, &m)) continue;
        // Row-vector layout (translation in the last row); transposed if the column form is seen.
        const bool rows = std::fabs(m.mat[0][3]) + std::fabs(m.mat[1][3]) + std::fabs(m.mat[2][3]) < 1e-9;
        auto at = [&](int r, int c) { return rows ? m.mat[r][c] : m.mat[c][r]; };
        rb::V3 ax(at(0, 0), at(0, 1), at(0, 2)), ay(at(1, 0), at(1, 1), at(1, 2)), az(at(2, 0), at(2, 1), at(2, 2));
        double anchor[3] = {0, 0, 0}, scale[3] = {100, 100, 100};
        GetLayerStream3(ss.get(), pid, L, AEGP_LayerStream_ANCHORPOINT, compTime, anchor);
        GetLayerStream3(ss.get(), pid, L, AEGP_LayerStream_SCALE, compTime, scale);

        rb::ControlPoint cp;
        cp.pos = ax * anchor[0] + ay * anchor[1] + az * anchor[2] + rb::V3(at(3, 0), at(3, 1), at(3, 2));
        // Directions survive a zero scale on one axis (X scale 0 = sharp corner).
        rb::V3 X = rb::length(ax) > 1e-9 ? rb::normalize(ax) : rb::normalize(rb::cross(ay, az), rb::V3(1, 0, 0));
        rb::V3 Z = rb::length(az) > 1e-9 ? az : rb::cross(ax, ay);
        Z = rb::normalize(rb::orthoTo(Z, X), rb::V3(0, 0, 1));
        cp.axisX = X;
        cp.axisZ = Z;
        cp.axisY = rb::cross(Z, X);
        cp.handleScale = scale[0] / 100;
        cps.push_back(cp);
    }
    es->AEGP_DisposeEffect(effect);
    return true;
}

bool RibbonReadControlNulls(PF_InData* in_data, const A_Time& compTime, bool closed, rb::Polyline& out) {
    std::vector<rb::ControlPoint> cps;
    if (!RibbonReadControlPoints(in_data, compTime, cps) || cps.size() < 2) return false;
    out = rb::controlPointsToPolyline(cps, closed, 48);
    return true;
}

void RibbonControlNullsKey(PF_InData* in_data, bool useNulls, std::vector<double>& key) {
    std::vector<rb::ControlPoint> cps;
    if (useNulls) {
        A_Time compTime = {0, 1};
        {
            Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
            if (pfi) pfi->AEGP_ConvertEffectToCompTime(in_data->effect_ref, in_data->current_time, in_data->time_scale, &compTime);
        }
        RibbonReadControlPoints(in_data, compTime, cps);
    }
    // Everything the render uses from the nulls: position, tangent and up axes, handle scale.
    // Never empty: the mode marker comes first (mask mode adds just that).
    key.push_back(useNulls ? 1.0 : 0.0);
    key.push_back(double(cps.size()));
    for (const rb::ControlPoint& c : cps) {
        const rb::V3* v[3] = {&c.pos, &c.axisX, &c.axisZ};
        for (const rb::V3* p : v) {
            key.push_back(p->x);
            key.push_back(p->y);
            key.push_back(p->z);
        }
        key.push_back(c.handleScale);
    }
}
