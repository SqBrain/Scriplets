// Ribbon3D GPU - After Effects effect: entry point, parameters, SmartFX pre-render / render,
// scene gathering (mask path, comp camera, comp lights) and the CPU fallback.
//
// Apply to a comp-sized 2D layer (e.g. a solid) without transforms, draw a mask on it and pick
// the mask in "Path": the ribbon is rendered in comp space through the comp camera.
#include "Ribbon3D.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <new>
#include <vector>

#include "Ribbon3D_GPU.h"
#include "Ribbon3D_Nulls.h"
#include "Ribbon3D_Suite.h"
#include "core/RibbonCPU.h"
#include "core/RibbonGeometry.h"
#include "core/RibbonPacket.h"

// AEGP suite versions: Ribbon3D_Suite.h

namespace {

// ------------------------------------------------------------------ small helpers

// RAII parameter checkout at the current time.
class Param {
public:
    Param(PF_InData* in, int index) : in_(in) {
        AEFX_CLR_STRUCT(def_);
        err_ = PF_CHECKOUT_PARAM(in, index, in->current_time, in->time_step, in->time_scale, &def_);
    }
    ~Param() {
        if (!err_) PF_CHECKIN_PARAM(in_, &def_);
    }
    Param(const Param&) = delete;
    Param& operator=(const Param&) = delete;
    bool ok() const { return err_ == PF_Err_NONE; }
    double f() const { return ok() ? double(def_.u.fs_d.value) : 0.0; }             // float slider
    int i() const { return ok() ? int(def_.u.sd.value) : 0; }                        // int slider
    bool b() const { return ok() && def_.u.bd.value != 0; }                          // checkbox
    double angle() const { return ok() ? double(def_.u.ad.value) / 65536.0 : 0.0; }  // PF_Fixed degrees
    int popup() const { return ok() ? int(def_.u.pd.value) : 1; }                    // 1-based
    rb::V3 color() const {
        if (!ok()) return rb::V3(1, 1, 1);
        return rb::V3(def_.u.cd.value.red / 255.0, def_.u.cd.value.green / 255.0, def_.u.cd.value.blue / 255.0);
    }
    PF_PathID path() const { return ok() ? def_.u.path_d.path_id : PF_PathID_NONE; }

private:
    PF_InData* in_;
    PF_ParamDef def_;
    PF_Err err_ = PF_Err_NONE;
};

struct PreRenderData {
    PF_LRect outRect{};   // output buffer rect, downsampled layer pixels
    PF_LRect inRect{};    // input buffer rect
    bool hasTexture = false;
    bool hasEnv = false;
    bool hasNormal = false;
};

void DeletePreRenderData(void* p) { delete static_cast<PreRenderData*>(p); }

PF_LRect Intersect(const PF_LRect& a, const PF_LRect& b) {
    PF_LRect r;
    r.left = std::max(a.left, b.left);
    r.top = std::max(a.top, b.top);
    r.right = std::min(a.right, b.right);
    r.bottom = std::min(a.bottom, b.bottom);
    if (r.right < r.left) r.right = r.left;
    if (r.bottom < r.top) r.bottom = r.top;
    return r;
}

double Ratio(const PF_RationalScale& s) { return s.den ? double(s.num) / double(s.den) : 1.0; }

// ------------------------------------------------------------------ commands

PF_Err About(PF_InData* in_data, PF_OutData* out_data) {
    (void)in_data;
    std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "%s v%d.%d.%02d\r%s", RB_NAME, RB_MAJOR_VERSION,
                  RB_MINOR_VERSION, RB_BUILD_VERSION, RB_DESCRIPTION);
    return PF_Err_NONE;
}

PF_Err GlobalSetup(PF_InData* in_data, PF_OutData* out_data) {
    (void)in_data;
    out_data->my_version = RB_VERSION;
    out_data->out_flags = RB_OUT_FLAGS;
    out_data->out_flags2 = RB_OUT_FLAGS2;
    return PF_Err_NONE;
}

PF_Err ParamsSetup(PF_InData* in_data, PF_OutData* out_data) {
    PF_Err err = PF_Err_NONE;
    PF_ParamDef def;

    AEFX_CLR_STRUCT(def);
    def.param_type = PF_Param_PATH;
    std::strncpy(def.PF_DEF_NAME, "Path", sizeof(def.PF_DEF_NAME) - 1);
    def.u.path_d.dephault = 1;  // first mask
    def.uu.id = RB_ID(RB_PATH);
    ERR(PF_ADD_PARAM(in_data, -1, &def));

    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER("Segments", 2, 4000, 10, 1500, 400, RB_ID(RB_SEGMENTS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Start", -100, 200, 0, 100, 0, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_START));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("End", -100, 200, 0, 100, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_END));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Offset", -10000, 10000, -100, 100, 0, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_OFFSET));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Width", 0, 10000, 0, 500, 60, PF_Precision_TENTHS, 0, 0, RB_ID(RB_WIDTH));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Width Start", 0, 1000, 0, 200, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_WIDTH_START));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Width End", 0, 1000, 0, 200, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_WIDTH_END));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Twist (turns)", -100, 100, -5, 5, 1, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_TWIST));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Twist Angle", 0, RB_ID(RB_TWIST_ANGLE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Twist Speed (deg/s)", -100000, 100000, -720, 720, 0, PF_Precision_TENTHS, 0, 0, RB_ID(RB_TWIST_SPEED));
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX("Twist Along Path", TRUE, 0, RB_ID(RB_TWIST_ALONG));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Wave Amplitude", -10000, 10000, 0, 300, 0, PF_Precision_TENTHS, 0, 0, RB_ID(RB_WAVE_AMP));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Wave Length", 1, 100000, 20, 2000, 500, PF_Precision_TENTHS, 0, 0, RB_ID(RB_WAVE_LENGTH));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Wave Speed (px/s)", -100000, 100000, -1000, 1000, 0, PF_Precision_TENTHS, 0, 0, RB_ID(RB_WAVE_SPEED));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Path Transform", RB_ID(RB_XF_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("X Rotation", 0, RB_ID(RB_XF_ROT_X));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Y Rotation", 0, RB_ID(RB_XF_ROT_Y));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Z Rotation", 0, RB_ID(RB_XF_ROT_Z));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Offset X", -100000, 100000, -1000, 1000, 0, PF_Precision_TENTHS, 0, 0, RB_ID(RB_XF_OFF_X));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Offset Y", -100000, 100000, -1000, 1000, 0, PF_Precision_TENTHS, 0, 0, RB_ID(RB_XF_OFF_Y));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Offset Z", -100000, 100000, -2000, 2000, 0, PF_Precision_TENTHS, 0, 0, RB_ID(RB_XF_OFF_Z));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Scale", 0, 10000, 0, 400, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_XF_SCALE));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_XF_TOPIC_END));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Material", RB_ID(RB_LOOK_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_COLOR("Front Color", 204, 18, 41, RB_ID(RB_FRONT_COLOR));
    AEFX_CLR_STRUCT(def);
    PF_ADD_COLOR("Back Color", 245, 204, 133, RB_ID(RB_BACK_COLOR));
    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER("Texture Layer", PF_LayerDefault_NONE, RB_ID(RB_TEXTURE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Texture Repeat", 0.01, 1000, 0.1, 20, 1, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_TEX_REPEAT));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Texture Offset", -100000, 100000, -1, 1, 0, PF_Precision_THOUSANDTHS, 0, 0, RB_ID(RB_TEX_OFFSET));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Ambient", 0, 10, 0, 1, 0.15, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_AMBIENT));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Diffuse", 0, 10, 0, 2, 0.9, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_DIFFUSE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Specular", 0, 10, 0, 2, 0.6, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_SPECULAR));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Glossiness", 1, 2000, 1, 200, 40, PF_Precision_TENTHS, 0, 0, RB_ID(RB_GLOSS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Satin", 0, 1, 0, 1, 0.5, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_SATIN));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Sheen", 0, 10, 0, 1, 0.25, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_SHEEN));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Translucency", 0, 10, 0, 1, 0.15, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_TRANSLUCENCY));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Metal", 0, 1, 0, 1, 0, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_METAL));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_LOOK_TOPIC_END));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Lighting", RB_ID(RB_LIGHT_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX("Use Comp Lights", TRUE, 0, RB_ID(RB_USE_COMP_LIGHTS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Light Azimuth", -35, RB_ID(RB_LIGHT_AZIMUTH));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Light Elevation", 40, RB_ID(RB_LIGHT_ELEVATION));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Light Intensity", 0, 10000, 0, 300, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_LIGHT_INTENSITY));
    AEFX_CLR_STRUCT(def);
    PF_ADD_COLOR("Light Color", 255, 255, 255, RB_ID(RB_LIGHT_COLOR));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_LIGHT_TOPIC_END));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Render", RB_ID(RB_RENDER_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Antialiasing", 4, 3, "Off (1x)|Low (4x)|High (9x)|Best (16x)", RB_ID(RB_QUALITY));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Composite", 3, 3, "Over Layer|Behind Layer|Ribbon Only", RB_ID(RB_COMPOSITE));  // default: Ribbon Only
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Opacity", 0, 100, 0, 100, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_OPACITY));
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX("Linear Working Space", FALSE, 0, RB_ID(RB_LINEAR));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_RENDER_TOPIC_END));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Thickness & Bevel", RB_ID(RB_SOLID_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Thickness", 0, 5000, 0, 100, 0, PF_Precision_TENTHS, 0, 0, RB_ID(RB_THICKNESS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Bevel Size", 0, 2500, 0, 50, 3, PF_Precision_TENTHS, 0, 0, RB_ID(RB_BEVEL_SIZE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Bevel Style", 2, 1, "Round|Chamfer", RB_ID(RB_BEVEL_STYLE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER("Bevel Smoothness", 1, 16, 1, 12, 5, RB_ID(RB_BEVEL_SEGMENTS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Edge Color", 3, 1, "Front Color|Back Color|Custom", RB_ID(RB_EDGE_COLOR_MODE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_COLOR("Custom Edge Color", 235, 225, 205, RB_ID(RB_EDGE_COLOR));
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX("End Caps", TRUE, 0, RB_ID(RB_END_CAPS));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_SOLID_TOPIC_END));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Control Nulls", RB_ID(RB_NULLS_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Path Source", 2, RB_SOURCE_MASK, "Mask|Control Nulls", RB_ID(RB_PATH_SOURCE));
    PF_ADD_BUTTON("Mask to Nulls", "Create Nulls from Mask", 0, PF_ParamFlag_SUPERVISE, RB_ID(RB_MAKE_NULLS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOXX("Closed Loop", FALSE, 0, RB_ID(RB_NULLS_CLOSED));
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Points", RB_ID(RB_POINTS_TOPIC));  // groups start collapsed in AE
    for (int i = 0; i < RB_MAX_NULLS; i++) {
        char name[32];
        std::snprintf(name, sizeof(name), "Point %d", i + 1);
        AEFX_CLR_STRUCT(def);
        PF_ADD_LAYER(name, PF_LayerDefault_NONE, RB_ID(RB_NULL_1 + i));
    }
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_POINTS_TOPIC_END));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_NULLS_TOPIC_END));

    // 0.1.04: outside the groups, so they are visible without twirling anything open.
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX("Cycle", "Wrap around path ends", TRUE, 0, RB_ID(RB_CYCLE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER("Fit Layer", PF_LayerDefault_NONE, RB_ID(RB_FIT_LAYER));
    AEFX_CLR_STRUCT(def);
    PF_ADD_BUTTON("Fit to Layer", "Configure", 0, PF_ParamFlag_SUPERVISE, RB_ID(RB_FIT_CONFIGURE));
    // 0.1.05: Front = the path is a view from above, the ribbon lies flat on that plane facing down.
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Orientation", 2, RB_ORIENT_TWIST, "Twist|Front", RB_ID(RB_ORIENTATION));

    // 0.1.06: image-based light from an HDRI layer, and glass.
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Environment", RB_ID(RB_ENV_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER("HDRI Layer", PF_LayerDefault_NONE, RB_ID(RB_ENV_LAYER));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Env Intensity", 0, 1000, 0, 300, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_ENV_INTENSITY));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Env Rotation", 0, RB_ID(RB_ENV_ROTATION));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Reflections", 0, 1000, 0, 300, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_ENV_REFLECTION));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_ENV_TOPIC_END));

    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Glass", RB_ID(RB_GLASS_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_CHECKBOX("Glass", "Enable", FALSE, 0, RB_ID(RB_GLASS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("IOR", 1, 3, 1, 2.5, 1.5, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_GLASS_IOR));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Transmission", 0, 100, 0, 100, 90, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_GLASS_TRANSMISSION));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Tint", 0, 100, 0, 100, 30, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_GLASS_TINT));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Refraction (px)", 0, 2000, 0, 200, 30, PF_Precision_TENTHS, 0, 0, RB_ID(RB_GLASS_REFRACTION));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_GLASS_TOPIC_END));

    // 0.1.07: wind - one end pinned, the rest streams downwind and flutters.
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Wind", RB_ID(RB_WIND_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Wind Strength", 0, 1000, 0, 200, 0, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_WIND_STRENGTH));
    AEFX_CLR_STRUCT(def);
    PF_ADD_ANGLE("Wind Direction", 90, RB_ID(RB_WIND_DIRECTION));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Pinned End", 2, 1, "Start|End", RB_ID(RB_WIND_PIN));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Flutter (px)", 0, 5000, 0, 300, 60, PF_Precision_TENTHS, 0, 0, RB_ID(RB_WIND_FLUTTER));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Flutter Speed (Hz)", 0, 50, 0, 6, 1.5, PF_Precision_HUNDREDTHS, 0, 0, RB_ID(RB_WIND_SPEED));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Flutter Wavelength", 10, 20000, 50, 2000, 300, PF_Precision_TENTHS, 0, 0, RB_ID(RB_WIND_WAVELENGTH));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Twist Flutter", 0, 720, 0, 180, 40, PF_Precision_TENTHS, 0, 0, RB_ID(RB_WIND_TWIST));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Gusts", 0, 100, 0, 100, 30, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_WIND_GUSTS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Stiffness", 0, 100, 0, 100, 30, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_WIND_STIFFNESS));
    AEFX_CLR_STRUCT(def);
    PF_ADD_SLIDER("Random Seed", 0, 100000, 0, 1000, 0, RB_ID(RB_WIND_SEED));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_WIND_TOPIC_END));

    // 0.1.08: normal map - relief on the ribbon's surface.
    AEFX_CLR_STRUCT(def);
    PF_ADD_TOPIC("Normal Map", RB_ID(RB_NRM_TOPIC));
    AEFX_CLR_STRUCT(def);
    PF_ADD_LAYER("Normal Map Layer", PF_LayerDefault_NONE, RB_ID(RB_NRM_LAYER));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Normal Strength", 0, 1000, 0, 300, 100, PF_Precision_TENTHS, PF_ValueDisplayFlag_PERCENT, 0, RB_ID(RB_NRM_STRENGTH));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Normal Mapping", 2, 1, "Tiles|Like Texture", RB_ID(RB_NRM_MAPPING));
    AEFX_CLR_STRUCT(def);
    PF_ADD_FLOAT_SLIDERX("Tile Size (px)", 1, 10000, 5, 500, 100, PF_Precision_TENTHS, 0, 0, RB_ID(RB_NRM_TILE));
    AEFX_CLR_STRUCT(def);
    PF_ADD_POPUP("Green Channel", 2, 1, "OpenGL (Y+)|DirectX (Y-)", RB_ID(RB_NRM_FORMAT));
    AEFX_CLR_STRUCT(def);
    PF_END_TOPIC(RB_ID(RB_NRM_TOPIC_END));

    out_data->num_params = RB_NUM_PARAMS;
    return err;
}

bool FrameworkSupported(PF_GPU_Framework fw) { return RibbonGPU_Supports(fw); }

PF_Err PreRender(PF_InData* in_data, PF_OutData* out_data, PF_PreRenderExtra* extra) {
    (void)out_data;
    PF_Err err = PF_Err_NONE;
    PreRenderData* prd = new (std::nothrow) PreRenderData();
    if (!prd) return PF_Err_OUT_OF_MEMORY;

    // Input: request what AE asks for, the ribbon may cover the whole layer.
    PF_RenderRequest req = extra->input->output_request;
    PF_CheckoutResult probe;
    AEFX_CLR_STRUCT(probe);
    ERR(extra->cb->checkout_layer(in_data->effect_ref, RB_INPUT, RB_CHECKOUT_INPUT, &req, in_data->current_time,
                                  in_data->time_step, in_data->time_scale, &probe));
    if (!err) {
        prd->outRect = Intersect(req.rect, probe.max_result_rect);
        prd->inRect = Intersect(probe.result_rect, prd->outRect);
    }

    // Texture layer: probe its size, then request the whole frame.
    PF_RenderRequest treq = req;
    PF_CheckoutResult tprobe;
    AEFX_CLR_STRUCT(tprobe);
    ERR(extra->cb->checkout_layer(in_data->effect_ref, RB_TEXTURE, RB_CHECKOUT_TEX_PROBE, &treq, in_data->current_time,
                                  in_data->time_step, in_data->time_scale, &tprobe));
    if (!err && tprobe.max_result_rect.right > tprobe.max_result_rect.left &&
        tprobe.max_result_rect.bottom > tprobe.max_result_rect.top) {
        treq.rect = tprobe.max_result_rect;
        PF_CheckoutResult tres;
        AEFX_CLR_STRUCT(tres);
        ERR(extra->cb->checkout_layer(in_data->effect_ref, RB_TEXTURE, RB_CHECKOUT_TEX, &treq, in_data->current_time,
                                      in_data->time_step, in_data->time_scale, &tres));
        if (!err) prd->hasTexture = true;
    }

    // Environment (HDRI) layer: the whole frame, like the texture.
    PF_RenderRequest ereq = req;
    PF_CheckoutResult eprobe;
    AEFX_CLR_STRUCT(eprobe);
    ERR(extra->cb->checkout_layer(in_data->effect_ref, RB_ENV_LAYER, RB_CHECKOUT_ENV_PROBE, &ereq, in_data->current_time,
                                  in_data->time_step, in_data->time_scale, &eprobe));
    if (!err && eprobe.max_result_rect.right > eprobe.max_result_rect.left &&
        eprobe.max_result_rect.bottom > eprobe.max_result_rect.top) {
        ereq.rect = eprobe.max_result_rect;
        PF_CheckoutResult eres;
        AEFX_CLR_STRUCT(eres);
        ERR(extra->cb->checkout_layer(in_data->effect_ref, RB_ENV_LAYER, RB_CHECKOUT_ENV, &ereq, in_data->current_time,
                                      in_data->time_step, in_data->time_scale, &eres));
        if (!err) prd->hasEnv = true;
    }

    // Normal map layer: the whole frame, like the texture.
    PF_RenderRequest nreq = req;
    PF_CheckoutResult nprobe;
    AEFX_CLR_STRUCT(nprobe);
    ERR(extra->cb->checkout_layer(in_data->effect_ref, RB_NRM_LAYER, RB_CHECKOUT_NRM_PROBE, &nreq, in_data->current_time,
                                  in_data->time_step, in_data->time_scale, &nprobe));
    if (!err && nprobe.max_result_rect.right > nprobe.max_result_rect.left &&
        nprobe.max_result_rect.bottom > nprobe.max_result_rect.top) {
        nreq.rect = nprobe.max_result_rect;
        PF_CheckoutResult nres;
        AEFX_CLR_STRUCT(nres);
        ERR(extra->cb->checkout_layer(in_data->effect_ref, RB_NRM_LAYER, RB_CHECKOUT_NRM, &nreq, in_data->current_time,
                                      in_data->time_step, in_data->time_scale, &nres));
        if (!err) prd->hasNormal = true;
    }

    // Control nulls are other layers: tell AE's cache about their current transforms. With
    // PF_OutFlag2_I_MIX_GUID_DEPENDENCIES AE requires GuidMixInPtr on EVERY pre-render (else
    // "internal verification failure"), so it is called in every mode and even after an error.
    {
        const bool useNulls = Param(in_data, RB_PATH_SOURCE).popup() == RB_SOURCE_NULLS;
        PF_Err mixErr = RibbonMixInControlNulls(in_data, extra, useNulls);
        if (!err) err = mixErr;
    }

    if (err) {
        delete prd;
        return err;
    }
    extra->output->result_rect = prd->outRect;
    extra->output->max_result_rect = probe.max_result_rect;
    extra->output->solid = FALSE;
    extra->output->pre_render_data = prd;
    extra->output->delete_pre_render_data_func = DeletePreRenderData;
    // With an environment the GPU world has to be read back to the CPU; where that is not
    // implemented (Metal) the frame is rendered on the CPU.
    if (extra->input->what_gpu != PF_GPU_Framework_NONE && FrameworkSupported(extra->input->what_gpu) &&
        (!prd->hasEnv || RibbonGPU_CanReadBack(extra->input->what_gpu)))
        extra->output->flags |= PF_RenderOutputFlag_GPU_RENDER_POSSIBLE;
    return err;
}

// ------------------------------------------------------------------ scene gathering

bool ReadMaskPolyline(PF_InData* in_data, PF_PathID id, rb::Polyline& out) {
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
    out.pts.clear();
    out.closed = !open;
    out.up = rb::V3(0, 0, 1);  // the layer plane (comp-facing)
    for (A_long s = 0; s < nseg; s++) {
        PF_PathSegPrepPtr prep = nullptr;
        PF_FpLong len = 0;
        if (pd->PF_PathPrepareSegLength(in_data->effect_ref, path, s, 64, &prep)) continue;
        pd->PF_PathGetSegLength(in_data->effect_ref, path, s, &prep, &len);
        // Arc-length samples (mask coordinates are full-resolution layer pixels).
        int k = int(std::clamp(std::ceil(len / 3.0), 8.0, 512.0));
        for (int q = (out.pts.empty() ? 0 : 1); q <= k; q++) {
            PF_FpLong x = 0, y = 0;
            pd->PF_PathEvalSegLength(in_data->effect_ref, path, &prep, s, len * q / k, &x, &y);
            out.pts.push_back(rb::V3(x, y, 0));
        }
        pd->PF_PathCleanupSegLength(in_data->effect_ref, path, s, &prep);
    }
    pq->PF_CheckinPath(in_data->effect_ref, id, FALSE, path);
    return out.pts.size() >= 2;
}

void MatrixToDoubles(const A_Matrix4& m, double out[4][4]) {
    for (int r = 0; r < 4; r++)
        for (int c = 0; c < 4; c++) out[r][c] = double(m.mat[r][c]);
}

// Local axis `row` of a layer-to-world matrix (row-vector or column-vector layout).
rb::V3 MatrixAxis(const double m[4][4], int row) {
    bool rowVectors = std::fabs(m[0][3]) + std::fabs(m[1][3]) + std::fabs(m[2][3]) < 1e-9;
    return rowVectors ? rb::V3(m[row][0], m[row][1], m[row][2]) : rb::V3(m[0][row], m[1][row], m[2][row]);
}

bool StreamValue(const RB_STREAM_SUITE* st, AEGP_LayerH layer, AEGP_LayerStream which, const A_Time& t,
                 AEGP_StreamVal2& out) {
    AEGP_StreamRefH stream = nullptr;
    if (st->AEGP_GetNewLayerStream(0, layer, which, &stream) != A_Err_NONE || !stream) return false;
    AEGP_StreamValue2 v;
    AEFX_CLR_STRUCT(v);
    bool ok = st->AEGP_GetNewStreamValue(0, stream, AEGP_LTimeMode_CompTime, &t, FALSE, &v) == A_Err_NONE;
    if (ok) {
        out = v.val;
        st->AEGP_DisposeStreamValue(&v);
    }
    st->AEGP_DisposeStream(stream);
    return ok;
}

rb::Camera CompCamera(PF_InData* in_data, const A_Time& compTime, double compW, double compH) {
    rb::Camera cam = rb::defaultCamera(compW, compH);
    Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    Suite<RB_LAYER_SUITE> ls(in_data, kAEGPLayerSuite, RB_LAYER_SUITE_VERSION);
    Suite<RB_STREAM_SUITE> ss(in_data, kAEGPStreamSuite, RB_STREAM_SUITE_VERSION);
    if (!pfi || !ls || !ss) return cam;
    AEGP_LayerH camLayer = nullptr;
    if (pfi->AEGP_GetEffectCamera(in_data->effect_ref, &compTime, &camLayer) || !camLayer) return cam;
    A_Matrix4 m;
    if (ls->AEGP_GetLayerToWorldXform(camLayer, &compTime, &m)) return cam;
    double md[4][4];
    MatrixToDoubles(m, md);
    AEGP_StreamVal2 zoom;
    AEFX_CLR_STRUCT(zoom);
    double z = StreamValue(ss.get(), camLayer, AEGP_LayerStream_ZOOM, compTime, zoom) ? double(zoom.one_d) : cam.zoom;
    return rb::cameraFromMatrix(md, z, compW, compH);
}

std::vector<rb::LightDesc> CompLights(PF_InData* in_data, const A_Time& compTime) {
    std::vector<rb::LightDesc> lights;
    Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
    Suite<RB_LAYER_SUITE> ls(in_data, kAEGPLayerSuite, RB_LAYER_SUITE_VERSION);
    Suite<RB_STREAM_SUITE> ss(in_data, kAEGPStreamSuite, RB_STREAM_SUITE_VERSION);
    Suite<RB_LIGHT_SUITE> lts(in_data, kAEGPLightSuite, RB_LIGHT_SUITE_VERSION);
    if (!pfi || !ls || !ss || !lts) return lights;
    AEGP_LayerH self = nullptr;
    AEGP_CompH comp = nullptr;
    A_long n = 0;
    if (pfi->AEGP_GetEffectLayer(in_data->effect_ref, &self) || !self) return lights;
    if (ls->AEGP_GetLayerParentComp(self, &comp) || !comp) return lights;
    if (ls->AEGP_GetCompNumLayers(comp, &n)) return lights;
    for (A_long i = 0; i < n && lights.size() < RK_MAX_LIGHTS; i++) {
        AEGP_LayerH L = nullptr;
        AEGP_ObjectType type = AEGP_ObjectType_NONE;
        AEGP_LayerFlags flags = 0;
        if (ls->AEGP_GetCompLayerByIndex(comp, i, &L) || !L) continue;
        if (ls->AEGP_GetLayerObjectType(L, &type) || type != AEGP_ObjectType_LIGHT) continue;
        if (ls->AEGP_GetLayerFlags(L, &flags) || !(flags & AEGP_LayerFlag_VIDEO_ACTIVE)) continue;
        AEGP_LightType lt;
        if (lts->AEGP_GetLightType(L, &lt)) continue;
        rb::LightDesc d;
        if (lt == AEGP_LightType_PARALLEL) d.type = RK_LIGHT_PARALLEL;
        else if (lt == AEGP_LightType_SPOT) d.type = RK_LIGHT_SPOT;
        else if (lt == AEGP_LightType_POINT) d.type = RK_LIGHT_POINT;
        else if (lt == AEGP_LightType_AMBIENT) d.type = RK_LIGHT_AMBIENT;
        else continue;  // newer light kinds (environment) are not supported
        A_Matrix4 m;
        if (ls->AEGP_GetLayerToWorldXform(L, &compTime, &m)) continue;
        double md[4][4];
        MatrixToDoubles(m, md);
        d.pos = MatrixAxis(md, 3);
        d.dir = rb::normalize(MatrixAxis(md, 2), rb::V3(0, 0, 1));  // lights aim along their local +Z
        AEGP_StreamVal2 v;
        AEFX_CLR_STRUCT(v);
        if (StreamValue(ss.get(), L, AEGP_LayerStream_INTENSITY, compTime, v)) d.intensity = v.one_d / 100.0;
        if (StreamValue(ss.get(), L, AEGP_LayerStream_COLOR, compTime, v)) d.color = rb::V3(v.color.redF, v.color.greenF, v.color.blueF);
        if (StreamValue(ss.get(), L, AEGP_LayerStream_CONE_ANGLE, compTime, v)) d.coneDeg = v.one_d;
        if (StreamValue(ss.get(), L, AEGP_LayerStream_CONE_FEATHER, compTime, v)) d.featherPct = v.one_d;
        lights.push_back(d);
    }
    return lights;
}

// What decides the ribbon's path: the source and the Path Transform group.
struct PathInputs {
    int source = RB_SOURCE_MASK;
    bool nullsClosed = false;
    PF_PathID mask = PF_PathID_NONE;
    bool front = false;  // Orientation = Front: the path is a view from above
    rb::PathTransform xf;
};

// Center of the path's bounding box in the drawing plane (z = 0): the pivot for Front and Path Transform.
rb::V3 PathPivot(const rb::Polyline& path) {
    double minx = 1e300, miny = 1e300, maxx = -1e300, maxy = -1e300;
    for (const rb::V3& q : path.pts) {
        minx = std::min(minx, q.x); maxx = std::max(maxx, q.x);
        miny = std::min(miny, q.y); maxy = std::max(maxy, q.y);
    }
    return rb::V3((minx + maxx) / 2, (miny + maxy) / 2, 0);
}

// The ribbon's path in world (comp) space at compTime: the mask or the control nulls, laid onto the
// floor for Orientation = Front, then the Path Transform group around the path's bounding box center.
bool RibbonWorldPath(PF_InData* in_data, const A_Time& compTime, PathInputs in, rb::Polyline& out) {
    rb::Polyline path;
    const bool ok = in.source == RB_SOURCE_NULLS ? RibbonReadControlNulls(in_data, compTime, in.nullsClosed, path)
                                                 : ReadMaskPolyline(in_data, in.mask, path);
    if (!ok) return false;
    if (in.front) path = rb::topViewToFloor(path, PathPivot(path));
    in.xf.pivot = PathPivot(path);
    out = rb::transformPolyline(path, in.xf);
    return true;
}

// Builds the render packet for an output buffer of outW x outH pixels.
PF_Err BuildPacket(PF_InData* in_data, const PreRenderData& prd, int outW, int outH, rb::Packet& pk) {
    const double t = double(in_data->current_time) / double(in_data->time_scale ? in_data->time_scale : 1);

    rb::RibbonSettings rs;
    rb::Material mat;
    A_Time compTime = {0, 1};
    {
        Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
        if (pfi) pfi->AEGP_ConvertEffectToCompTime(in_data->effect_ref, in_data->current_time, in_data->time_scale, &compTime);
    }
    PathInputs pin;
    pin.source = Param(in_data, RB_PATH_SOURCE).popup();
    pin.nullsClosed = Param(in_data, RB_NULLS_CLOSED).b();
    pin.mask = Param(in_data, RB_PATH).path();
    pin.front = Param(in_data, RB_ORIENTATION).popup() == RB_ORIENT_FRONT;
    rb::PathTransform& xf = pin.xf;
    rs.segments = std::max(2, Param(in_data, RB_SEGMENTS).i());
    rs.start = Param(in_data, RB_START).f() / 100;
    rs.end = Param(in_data, RB_END).f() / 100;
    rs.offset = Param(in_data, RB_OFFSET).f() / 100;
    rs.width = Param(in_data, RB_WIDTH).f();
    rs.widthStart = Param(in_data, RB_WIDTH_START).f() / 100;
    rs.widthEnd = Param(in_data, RB_WIDTH_END).f() / 100;
    rs.twistTurns = Param(in_data, RB_TWIST).f();
    rs.twistAngleDeg = Param(in_data, RB_TWIST_ANGLE).angle();
    rs.twistSpeedDeg = Param(in_data, RB_TWIST_SPEED).f();
    rs.twistAlongPath = Param(in_data, RB_TWIST_ALONG).b();
    rs.waveAmp = Param(in_data, RB_WAVE_AMP).f();
    rs.waveLength = Param(in_data, RB_WAVE_LENGTH).f();
    rs.waveSpeed = Param(in_data, RB_WAVE_SPEED).f();
    rs.time = t;
    rs.thickness = Param(in_data, RB_THICKNESS).f();
    rs.bevel = Param(in_data, RB_BEVEL_SIZE).f();
    rs.chamfer = Param(in_data, RB_BEVEL_STYLE).popup() == 2;
    rs.bevelSegments = std::clamp(Param(in_data, RB_BEVEL_SEGMENTS).i(), 1, 16);
    if (in_data->quality == PF_Quality_LO) rs.bevelSegments = std::min(rs.bevelSegments, 2);  // draft preview
    rs.caps = Param(in_data, RB_END_CAPS).b();
    rs.cycle = Param(in_data, RB_CYCLE).b();
    if (pin.front) rs.twistTurns = rs.twistAngleDeg = rs.twistSpeedDeg = 0;  // Front: flat, no twist
    {
        rb::WindSettings& w = rs.wind;
        w.strength = std::max(0.0, Param(in_data, RB_WIND_STRENGTH).f() / 100);
        // Angle as on AE's dial: 0 = up, 90 = right (comp space). Front: the comp is a view from
        // above, so "up" in the comp means away from the camera on the floor.
        const double a = Param(in_data, RB_WIND_DIRECTION).angle() * 3.14159265358979 / 180;
        w.direction = pin.front ? rb::V3(std::sin(a), 0, std::cos(a)) : rb::V3(std::sin(a), -std::cos(a), 0);
        w.pinStart = Param(in_data, RB_WIND_PIN).popup() != 2;
        w.flutterPx = Param(in_data, RB_WIND_FLUTTER).f();
        w.flutterHz = Param(in_data, RB_WIND_SPEED).f();
        w.wavelength = Param(in_data, RB_WIND_WAVELENGTH).f();
        w.twistDeg = Param(in_data, RB_WIND_TWIST).f();
        w.gusts = Param(in_data, RB_WIND_GUSTS).f() / 100;
        w.stiffness = Param(in_data, RB_WIND_STIFFNESS).f() / 100;
        w.seed = Param(in_data, RB_WIND_SEED).i();
    }

    xf.rotX = Param(in_data, RB_XF_ROT_X).angle();
    xf.rotY = Param(in_data, RB_XF_ROT_Y).angle();
    xf.rotZ = Param(in_data, RB_XF_ROT_Z).angle();
    xf.offset = rb::V3(Param(in_data, RB_XF_OFF_X).f(), Param(in_data, RB_XF_OFF_Y).f(), Param(in_data, RB_XF_OFF_Z).f());
    xf.scale = Param(in_data, RB_XF_SCALE).f() / 100;

    mat.front = Param(in_data, RB_FRONT_COLOR).color();
    mat.back = Param(in_data, RB_BACK_COLOR).color();
    {
        int edgeMode = Param(in_data, RB_EDGE_COLOR_MODE).popup();
        mat.edge = edgeMode == 2 ? mat.back : (edgeMode == 3 ? Param(in_data, RB_EDGE_COLOR).color() : mat.front);
    }
    mat.texRepeat = Param(in_data, RB_TEX_REPEAT).f();
    mat.texOffset = Param(in_data, RB_TEX_OFFSET).f();
    mat.ambient = Param(in_data, RB_AMBIENT).f();
    mat.diffuse = Param(in_data, RB_DIFFUSE).f();
    mat.specular = Param(in_data, RB_SPECULAR).f();
    mat.gloss = Param(in_data, RB_GLOSS).f();
    mat.satin = Param(in_data, RB_SATIN).f();
    mat.sheen = Param(in_data, RB_SHEEN).f();
    mat.translucency = Param(in_data, RB_TRANSLUCENCY).f();
    mat.metal = Param(in_data, RB_METAL).f();
    mat.samples = std::clamp(Param(in_data, RB_QUALITY).popup(), 1, 4);
    if (in_data->quality == PF_Quality_LO) mat.samples = 1;  // draft preview
    int comp = Param(in_data, RB_COMPOSITE).popup();
    mat.composite = comp == 2 ? RK_COMPOSITE_BEHIND : (comp == 3 ? RK_COMPOSITE_ALONE : RK_COMPOSITE_OVER);
    mat.opacity = Param(in_data, RB_OPACITY).f() / 100;
    mat.gamma = Param(in_data, RB_LINEAR).b() ? 1.0 : 2.2;
    mat.envIntensity = Param(in_data, RB_ENV_INTENSITY).f() / 100;
    mat.envReflection = Param(in_data, RB_ENV_REFLECTION).f() / 100;
    mat.envRotationDeg = Param(in_data, RB_ENV_ROTATION).angle();
    mat.glass = Param(in_data, RB_GLASS).b();
    mat.ior = Param(in_data, RB_GLASS_IOR).f();
    mat.transmission = Param(in_data, RB_GLASS_TRANSMISSION).f() / 100;
    mat.tint = Param(in_data, RB_GLASS_TINT).f() / 100;
    mat.refractPx = Param(in_data, RB_GLASS_REFRACTION).f();
    mat.normalStrength = Param(in_data, RB_NRM_STRENGTH).f() / 100;
    mat.normalTiles = Param(in_data, RB_NRM_MAPPING).popup() != 2;
    mat.normalTilePx = Param(in_data, RB_NRM_TILE).f();
    mat.normalDirectX = Param(in_data, RB_NRM_FORMAT).popup() == 2;

    // The effect layer is comp-sized: layer pixels == comp pixels.
    const double compW = in_data->width, compH = in_data->height;
    rb::Camera cam = CompCamera(in_data, compTime, compW, compH);
    cam.scaleX = Ratio(in_data->downsample_x);
    cam.scaleY = Ratio(in_data->downsample_y);
    cam.originX = prd.outRect.left;
    cam.originY = prd.outRect.top;

    std::vector<rb::LightDesc> lights;
    if (Param(in_data, RB_USE_COMP_LIGHTS).b()) lights = CompLights(in_data, compTime);
    if (lights.empty() && !prd.hasEnv) {
        // Built-in key light: shines from azimuth / elevation (0 / 0 = from the camera side).
        // An HDRI environment replaces it (both at once would light the ribbon twice).
        double az = Param(in_data, RB_LIGHT_AZIMUTH).angle() * 3.14159265358979 / 180;
        double el = Param(in_data, RB_LIGHT_ELEVATION).angle() * 3.14159265358979 / 180;
        rb::LightDesc key;
        key.type = RK_LIGHT_PARALLEL;
        rb::V3 toLight(std::cos(el) * std::sin(az), -std::sin(el), -std::cos(el) * std::cos(az));
        key.dir = toLight * -1.0;
        key.intensity = Param(in_data, RB_LIGHT_INTENSITY).f() / 100;
        key.color = Param(in_data, RB_LIGHT_COLOR).color();
        lights.push_back(key);
    }

    rb::RibbonMesh mesh;
    rb::Polyline world;
    double ribbonLength = 0;
    if (RibbonWorldPath(in_data, compTime, pin, world)) {
        world = rb::applyWind(world, rs.wind, rs.time);
        mesh = rb::buildRibbon(world, rs);
        ribbonLength = rb::FramedCurve(world).length() * std::fabs(rs.end - rs.start);  // for normal map tiles
    }
    rb::buildPacket(mesh, cam, lights, mat, outW, outH, pk, ribbonLength, rs.width);
    return PF_Err_NONE;
}

// ------------------------------------------------------------------ "Configure" button

PF_Err ShowMessage(PF_OutData* out_data, const char* msg) {
    std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "Ribbon 3D GPU: %s", msg);
    out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
    return PF_Err_NONE;
}

// PF_Cmd_USER_CHANGED_PARAM for RB_FIT_CONFIGURE: the layer in "Fit Layer" (else the one in
// "Texture Layer") becomes the texture, and Width is set so that the texture keeps its aspect on
// the visible ribbon: width = path length * |End - Start| / Texture Repeat * height / width.
PF_Err ConfigureFromLayer(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[]) {
    A_Time compTime = {0, 1};
    {
        Suite<AEGP_PFInterfaceSuite1> pfi(in_data, kAEGPPFInterfaceSuite, kAEGPPFInterfaceSuiteVersion1);
        if (pfi) pfi->AEGP_ConvertEffectToCompTime(in_data->effect_ref, in_data->current_time, in_data->time_scale, &compTime);
    }
    const AEGP_LayerIDVal fit = RibbonGetLayerParam(in_data, RB_FIT_LAYER, compTime);
    const AEGP_LayerIDVal tex = RibbonGetLayerParam(in_data, RB_TEXTURE, compTime);
    const AEGP_LayerIDVal id = fit ? fit : tex;
    if (!id) return ShowMessage(out_data, "pick a layer in \"Fit Layer\" first, then press Configure.");
    double lw = 0, lh = 0;
    if (!RibbonLayerSourceSize(in_data, id, lw, lh)) return ShowMessage(out_data, "could not read the size of the chosen layer.");

    // Plain parameter values come from params[] during USER_CHANGED_PARAM.
    PathInputs pin;
    pin.source = int(params[RB_PATH_SOURCE]->u.pd.value);
    pin.nullsClosed = params[RB_NULLS_CLOSED]->u.bd.value != 0;
    pin.mask = params[RB_PATH]->u.path_d.path_id;
    pin.front = params[RB_ORIENTATION]->u.pd.value == RB_ORIENT_FRONT;
    pin.xf.rotX = params[RB_XF_ROT_X]->u.ad.value / 65536.0;
    pin.xf.rotY = params[RB_XF_ROT_Y]->u.ad.value / 65536.0;
    pin.xf.rotZ = params[RB_XF_ROT_Z]->u.ad.value / 65536.0;
    pin.xf.offset = rb::V3(params[RB_XF_OFF_X]->u.fs_d.value, params[RB_XF_OFF_Y]->u.fs_d.value, params[RB_XF_OFF_Z]->u.fs_d.value);
    pin.xf.scale = params[RB_XF_SCALE]->u.fs_d.value / 100;
    rb::Polyline world;
    if (!RibbonWorldPath(in_data, compTime, pin, world))
        return ShowMessage(out_data, "no path yet: pick a mask in \"Path\" or set up the control nulls.");
    const rb::FramedCurve curve(world);
    const double span = std::fabs(params[RB_END]->u.fs_d.value - params[RB_START]->u.fs_d.value) / 100;
    if (!curve.valid()) return ShowMessage(out_data, "the path has no length.");
    if (span < 1e-6) return ShowMessage(out_data, "Start and End are equal, the ribbon has no length.");
    const double repeat = std::max(0.01, double(params[RB_TEX_REPEAT]->u.fs_d.value));
    const double want = curve.length() * span / repeat * lh / lw;
    const double width = std::clamp(want, 0.0, 10000.0);  // Width's valid range

    if (fit && fit != tex) {
        Suite<RB_UTILITY_SUITE> us(in_data, kAEGPUtilitySuite, RB_UTILITY_SUITE_VERSION);
        if (us) us->AEGP_StartUndoGroup("Ribbon 3D: Fit to Layer");
        RibbonSetLayerParam(in_data, RB_TEXTURE, compTime, fit);
        if (us) us->AEGP_EndUndoGroup();
    }
    params[RB_WIDTH]->u.fs_d.value = width;
    params[RB_WIDTH]->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE;
    out_data->out_flags |= PF_OutFlag_FORCE_RERENDER | PF_OutFlag_REFRESH_UI;
    if (want > width + 1e-6) {
        char msg[200];
        std::snprintf(msg, sizeof(msg), "the layer needs a width of %.0f px, Width was set to its maximum of 10000.", want);
        return ShowMessage(out_data, msg);
    }
    return PF_Err_NONE;
}

// ------------------------------------------------------------------ CPU path

template <typename Pix, int MaxV>
void WorldToBGRA(const PF_EffectWorld* w, std::vector<float>& out) {
    out.assign(size_t(w->width) * w->height * 4, 0.0f);
    const float k = 1.0f / float(MaxV);
    for (A_long y = 0; y < w->height; y++) {
        const Pix* row = reinterpret_cast<const Pix*>(reinterpret_cast<const char*>(w->data) + y * w->rowbytes);
        float* o = &out[size_t(y) * w->width * 4];
        for (A_long x = 0; x < w->width; x++, o += 4) {
            o[0] = float(row[x].blue) * k; o[1] = float(row[x].green) * k; o[2] = float(row[x].red) * k; o[3] = float(row[x].alpha) * k;
        }
    }
}

template <typename Pix, typename Chan, int MaxV>
void BGRAToWorld(const std::vector<float>& in, PF_EffectWorld* w) {
    const bool clampIt = MaxV != 1;
    for (A_long y = 0; y < w->height; y++) {
        Pix* row = reinterpret_cast<Pix*>(reinterpret_cast<char*>(w->data) + y * w->rowbytes);
        const float* s = &in[size_t(y) * w->width * 4];
        for (A_long x = 0; x < w->width; x++, s += 4) {
            auto cv = [&](float v) {
                if (clampIt) v = std::clamp(v, 0.0f, 1.0f) * float(MaxV) + 0.5f;
                return Chan(v);
            };
            row[x].blue = cv(s[0]); row[x].green = cv(s[1]); row[x].red = cv(s[2]); row[x].alpha = cv(s[3]);
        }
    }
}

void ToBGRA(const PF_EffectWorld* w, int bpc, std::vector<float>& out) {
    if (bpc == 32) WorldToBGRA<PF_PixelFloat, 1>(w, out);
    else if (bpc == 16) WorldToBGRA<PF_Pixel16, PF_MAX_CHAN16>(w, out);
    else WorldToBGRA<PF_Pixel8, PF_MAX_CHAN8>(w, out);
}

void FromBGRA(const std::vector<float>& in, int bpc, PF_EffectWorld* w) {
    if (bpc == 32) BGRAToWorld<PF_PixelFloat, PF_FpShort, 1>(in, w);
    else if (bpc == 16) BGRAToWorld<PF_Pixel16, A_u_short, PF_MAX_CHAN16>(in, w);
    else BGRAToWorld<PF_Pixel8, A_u_char, PF_MAX_CHAN8>(in, w);
}

PF_Err RenderCPU(int bpc, rb::Packet& pk, const PreRenderData& prd, PF_EffectWorld* src, PF_EffectWorld* dst, PF_EffectWorld* tex,
                 PF_EffectWorld* env, PF_EffectWorld* nrm) {
    std::vector<float> s, t, nm, d(size_t(dst->width) * dst->height * 4, 0.0f);
    if (src) {
        ToBGRA(src, bpc, s);
        rb::setSource(pk, prd.inRect.left - prd.outRect.left, prd.inRect.top - prd.outRect.top, src->width, src->height, src->width);
    } else {
        s.assign(4, 0.0f);
        rb::setSource(pk, 0, 0, 0, 0, 1);
    }
    pk.params.dstPitch = dst->width;
    if (tex) {
        ToBGRA(tex, bpc, t);
        rb::setTexture(pk, tex->width, tex->height, tex->width);
    }
    if (env) {
        std::vector<float> e;
        ToBGRA(env, bpc, e);
        rb::setEnvironmentImage(pk, e.data(), env->width, env->height, env->width);
    }
    if (nrm) {
        ToBGRA(nrm, bpc, nm);
        rb::setNormalMap(pk, nrm->width, nrm->height, nrm->width);
    }
    rb::renderCPU(pk, s.data(), d.data(), t.empty() ? nullptr : t.data(), 0, nm.empty() ? nullptr : nm.data());
    FromBGRA(d, bpc, dst);
    return PF_Err_NONE;
}

PF_Err SmartRender(PF_InData* in_data, PF_OutData* out_data, PF_SmartRenderExtra* extra, bool gpu) {
    (void)out_data;
    PF_Err err = PF_Err_NONE, err2 = PF_Err_NONE;
    const PreRenderData* prd = static_cast<const PreRenderData*>(extra->input->pre_render_data);
    if (!prd) return PF_Err_BAD_CALLBACK_PARAM;
    PF_EffectWorld *src = nullptr, *dst = nullptr, *tex = nullptr, *env = nullptr, *nrm = nullptr;
    ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, RB_CHECKOUT_INPUT, &src));
    ERR(extra->cb->checkout_output(in_data->effect_ref, &dst));
    if (!err && prd->hasTexture) ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, RB_CHECKOUT_TEX, &tex));
    if (!err && prd->hasEnv) ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, RB_CHECKOUT_ENV, &env));
    if (!err && prd->hasNormal) ERR(extra->cb->checkout_layer_pixels(in_data->effect_ref, RB_CHECKOUT_NRM, &nrm));

    if (!err && dst) {
        rb::Packet pk;
        ERR(BuildPacket(in_data, *prd, dst->width, dst->height, pk));
        if (!err) {
            if (gpu) err = RibbonGPU_Render(in_data, extra, pk, prd->inRect.left - prd->outRect.left,
                                            prd->inRect.top - prd->outRect.top, src, dst, tex, env, nrm);
            else err = RenderCPU(int(extra->input->bitdepth), pk, *prd, src, dst, tex, env, nrm);
        }
    }
    ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, RB_CHECKOUT_INPUT));
    if (prd->hasTexture) ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, RB_CHECKOUT_TEX));
    if (prd->hasEnv) ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, RB_CHECKOUT_ENV));
    if (prd->hasNormal) ERR2(extra->cb->checkin_layer_pixels(in_data->effect_ref, RB_CHECKOUT_NRM));
    return err;
}

}  // namespace

// ------------------------------------------------------------------ registration and entry point

#ifdef PF_REGISTER_EFFECT_EXT2
extern "C" DllExport PF_Err PluginDataEntryFunction2(PF_PluginDataPtr inPtr, PF_PluginDataCB2 inPluginDataCallBackPtr,
                                                     SPBasicSuite* inSPBasicSuitePtr, const char* inHostName,
                                                     const char* inHostVersion) {
    (void)inSPBasicSuitePtr;
    (void)inHostName;
    (void)inHostVersion;
    PF_Err result = PF_Err_INVALID_CALLBACK;  // the SDK macro assigns to "result"
    PF_REGISTER_EFFECT_EXT2(inPtr, inPluginDataCallBackPtr, RB_NAME, RB_MATCH_NAME, RB_CATEGORY, AE_RESERVED_INFO,
                            "EffectMain", "https://github.com/SqBrain/Scriplets");
    return result;
}
#else  // SDKs before AE 2022
extern "C" DllExport PF_Err PluginDataEntryFunction(PF_PluginDataPtr inPtr, PF_PluginDataCB inPluginDataCallBackPtr,
                                                    SPBasicSuite* inSPBasicSuitePtr, const char* inHostName,
                                                    const char* inHostVersion) {
    (void)inSPBasicSuitePtr;
    (void)inHostName;
    (void)inHostVersion;
    PF_Err result = PF_Err_INVALID_CALLBACK;
    PF_REGISTER_EFFECT(inPtr, inPluginDataCallBackPtr, RB_NAME, RB_MATCH_NAME, RB_CATEGORY, AE_RESERVED_INFO);
    return result;
}
#endif

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                                       PF_LayerDef* output, void* extra) {
    (void)output;
    PF_Err err = PF_Err_NONE;
    try {
        switch (cmd) {
            case PF_Cmd_ABOUT: err = About(in_data, out_data); break;
            case PF_Cmd_GLOBAL_SETUP: err = GlobalSetup(in_data, out_data); break;
            case PF_Cmd_PARAMS_SETUP: err = ParamsSetup(in_data, out_data); break;
            case PF_Cmd_GPU_DEVICE_SETUP:
                err = RibbonGPU_DeviceSetup(in_data, out_data, static_cast<PF_GPUDeviceSetupExtra*>(extra));
                break;
            case PF_Cmd_GPU_DEVICE_SETDOWN:
                err = RibbonGPU_DeviceSetdown(in_data, out_data, static_cast<PF_GPUDeviceSetdownExtra*>(extra));
                break;
            case PF_Cmd_SMART_PRE_RENDER: err = PreRender(in_data, out_data, static_cast<PF_PreRenderExtra*>(extra)); break;
            case PF_Cmd_SMART_RENDER: err = SmartRender(in_data, out_data, static_cast<PF_SmartRenderExtra*>(extra), false); break;
            case PF_Cmd_SMART_RENDER_GPU: err = SmartRender(in_data, out_data, static_cast<PF_SmartRenderExtra*>(extra), true); break;
            case PF_Cmd_USER_CHANGED_PARAM: {
                const PF_UserChangedParamExtra* ex = static_cast<const PF_UserChangedParamExtra*>(extra);
                if (ex && ex->param_index == RB_MAKE_NULLS) err = RibbonMakeNullsFromMask(in_data, out_data, params);
                if (ex && ex->param_index == RB_FIT_CONFIGURE) err = ConfigureFromLayer(in_data, out_data, params);
                break;
            }
            default: break;
        }
    } catch (const std::bad_alloc&) {
        err = PF_Err_OUT_OF_MEMORY;
    } catch (...) {
        err = PF_Err_INTERNAL_STRUCT_DAMAGED;
    }
    return err;
}
