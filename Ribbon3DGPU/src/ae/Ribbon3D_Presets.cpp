#include "Ribbon3D_Presets.h"

#include <cstdio>
#include <string>
#include <vector>

namespace {

// One parameter value: sliders and float sliders take v, angles v in degrees, checkboxes and
// popups v as 0 / 1 and the 1-based item. Colors are separate.
struct Set {
    int param;
    double v;
};
struct Col {
    int param;
    unsigned char r, g, b;
};
struct Preset {
    const char* name;
    std::vector<Set> sets;
    std::vector<Col> cols;
};

// Popup values used below.
enum { kFlat = 1, kRound = 2 };
enum { kGradOff = 1, kGradAlong = 2, kGradRandom = 3 };
enum { k2Colors = 1, k3Colors = 2, k4Colors = 3 };
enum { kEndsBoth = 1, kEndsStart = 2, kEndsEnd = 3 };
enum { kBevelRound = 1, kBevelChamfer = 2 };
enum { kEdgeFront = 1, kEdgeBack = 2, kEdgeCustom = 3 };
enum { kAroundCenter = 1, kAroundStart = 2 };

const std::vector<Preset>& Presets() {
    static const std::vector<Preset> p = {
        {"Silk",
         {{RB_WIDTH, 70}, {RB_TWIST, 1.5}, {RB_WAVE_AMP, 60}, {RB_WAVE_LENGTH, 600}, {RB_SATIN, 0.75}, {RB_GLOSS, 30},
          {RB_SHEEN, 0.35}, {RB_TRANSLUCENCY, 0.3}, {RB_SPECULAR, 0.7}},
         {{RB_FRONT_COLOR, 178, 18, 48}, {RB_BACK_COLOR, 240, 198, 140}}},
        {"Gold Foil",
         {{RB_WIDTH, 55}, {RB_TWIST, 1}, {RB_THICKNESS, 2}, {RB_BEVEL_SIZE, 1}, {RB_METAL, 1}, {RB_SPECULAR, 1.3}, {RB_GLOSS, 160},
          {RB_SATIN, 0.5}, {RB_SHEEN, 0.4}, {RB_AMBIENT, 0.6}},
         {{RB_FRONT_COLOR, 255, 196, 84}, {RB_BACK_COLOR, 255, 196, 84}}},
        {"Chrome Tube",
         {{RB_SECTION, kRound}, {RB_WIDTH, 28}, {RB_END_ROUNDING, 100}, {RB_TWIST, 0}, {RB_METAL, 1}, {RB_SPECULAR, 1.5},
          {RB_GLOSS, 400}, {RB_SATIN, 0}, {RB_SHEEN, 0.6}, {RB_AMBIENT, 0.45}},
         {{RB_FRONT_COLOR, 232, 234, 240}, {RB_BACK_COLOR, 232, 234, 240}}},
        {"Clear Glass",
         {{RB_WIDTH, 70}, {RB_TWIST, 1}, {RB_THICKNESS, 10}, {RB_BEVEL_SIZE, 4}, {RB_END_ROUNDING, 100}, {RB_GLASS, 1},
          {RB_GLASS_IOR, 1.5}, {RB_GLASS_TRANSMISSION, 95}, {RB_GLASS_TINT, 25}, {RB_GLASS_REFRACTION, 30}, {RB_SPECULAR, 1.2},
          {RB_GLOSS, 300}, {RB_SATIN, 0}, {RB_SHEEN, 0.15}},
         {{RB_FRONT_COLOR, 200, 230, 255}, {RB_BACK_COLOR, 200, 230, 255}}},
        {"Frosted Glass",
         {{RB_WIDTH, 70}, {RB_TWIST, 1}, {RB_THICKNESS, 10}, {RB_BEVEL_SIZE, 4}, {RB_END_ROUNDING, 100}, {RB_GLASS, 1},
          {RB_GLASS_IOR, 1.5}, {RB_GLASS_TRANSMISSION, 85}, {RB_GLASS_TINT, 35}, {RB_GLASS_REFRACTION, 25}, {RB_GLASS_FROST, 8},
          {RB_SPECULAR, 0.8}, {RB_GLOSS, 60}, {RB_SATIN, 0}, {RB_SHEEN, 0.5}},
         {{RB_FRONT_COLOR, 225, 238, 255}, {RB_BACK_COLOR, 225, 238, 255}}},
        {"Neon Tube",
         {{RB_SECTION, kRound}, {RB_WIDTH, 14}, {RB_END_ROUNDING, 100}, {RB_TWIST, 0}, {RB_WAVE_AMP, 30}, {RB_WAVE_LENGTH, 700},
          {RB_AMBIENT, 1.2}, {RB_DIFFUSE, 0.2}, {RB_SPECULAR, 0.3}, {RB_GLOSS, 60}, {RB_SATIN, 0}, {RB_SHEEN, 0.6},
          {RB_TRANSLUCENCY, 0}},
         {{RB_FRONT_COLOR, 255, 40, 170}, {RB_BACK_COLOR, 255, 40, 170}}},
        {"Candy Stripes",
         {{RB_WIDTH, 48}, {RB_TWIST, 2}, {RB_THICKNESS, 8}, {RB_BEVEL_SIZE, 3.5}, {RB_GRAD_MODE, kGradAlong},
          {RB_GRAD_COUNT, k2Colors}, {RB_GRAD_REPEAT, 12}, {RB_SATIN, 0.1}, {RB_GLOSS, 120}, {RB_SPECULAR, 0.9}, {RB_SHEEN, 0.3}},
         {{RB_GRAD_COLOR_1, 255, 255, 255}, {RB_GRAD_COLOR_2, 226, 28, 52}}},
        {"Rainbow Silk",
         {{RB_WIDTH, 70}, {RB_TWIST, 1.25}, {RB_WAVE_AMP, 50}, {RB_WAVE_LENGTH, 600}, {RB_GRAD_MODE, kGradAlong},
          {RB_GRAD_COUNT, k4Colors}, {RB_SATIN, 0.7}, {RB_SHEEN, 0.35}, {RB_TRANSLUCENCY, 0.3}},
         {{RB_GRAD_COLOR_1, 255, 64, 96}, {RB_GRAD_COLOR_2, 255, 196, 64}, {RB_GRAD_COLOR_3, 64, 200, 170},
          {RB_GRAD_COLOR_4, 96, 96, 255}}},
        {"Ribbon Bundle",
         {{RB_WIDTH, 26}, {RB_TWIST, 1}, {RB_REP_COPIES, 10}, {RB_REP_POS_X, 30}, {RB_REP_POS_Y, 40}, {RB_REP_POS_Z, 300},
          {RB_REP_ROT, 8}, {RB_REP_TILT, 15}, {RB_REP_WIDTH, 40}, {RB_REP_OFFSET, 10}, {RB_GRAD_MODE, kGradRandom},
          {RB_GRAD_COUNT, k4Colors}, {RB_SATIN, 0.6}, {RB_SHEEN, 0.3}},
         {{RB_GRAD_COLOR_1, 255, 64, 96}, {RB_GRAD_COLOR_2, 255, 196, 64}, {RB_GRAD_COLOR_3, 64, 200, 170},
          {RB_GRAD_COLOR_4, 96, 96, 255}}},
        {"Starburst",
         {{RB_WIDTH, 22}, {RB_WIDTH_START, 10}, {RB_TWIST, 0.25}, {RB_END_ROUNDING, 100}, {RB_END_ROUND_WHICH, kEndsEnd},
          {RB_REP_COPIES, 16}, {RB_REP_PIVOT, kAroundStart}, {RB_REP_FAN, 360}, {RB_REP_LENGTH, 60}, {RB_REP_ROT, 4},
          {RB_GRAD_MODE, kGradAlong}, {RB_GRAD_COUNT, k3Colors}, {RB_AMBIENT, 0.3}, {RB_SHEEN, 0.4}},
         {{RB_GRAD_COLOR_1, 255, 240, 180}, {RB_GRAD_COLOR_2, 255, 120, 60}, {RB_GRAD_COLOR_3, 180, 40, 120}}},
        {"Fountain",
         {{RB_SECTION, kRound}, {RB_WIDTH, 10}, {RB_WIDTH_START, 30}, {RB_TWIST, 0}, {RB_END_ROUNDING, 100},
          {RB_END_ROUND_WHICH, kEndsEnd}, {RB_REP_COPIES, 14}, {RB_REP_PIVOT, kAroundStart}, {RB_REP_CONE, 40},
          {RB_REP_LENGTH, 50}, {RB_REP_TILT, 6}, {RB_GRAD_MODE, kGradAlong}, {RB_GRAD_COUNT, k3Colors}, {RB_SPECULAR, 0.9},
          {RB_GLOSS, 120}},
         {{RB_GRAD_COLOR_1, 120, 220, 255}, {RB_GRAD_COLOR_2, 60, 90, 255}, {RB_GRAD_COLOR_3, 170, 90, 255}}},
        {"Flag in the Wind",
         {{RB_WIDTH, 120}, {RB_TWIST, 0}, {RB_WIND_STRENGTH, 100}, {RB_WIND_FLUTTER, 50}, {RB_WIND_SPEED, 1.2},
          {RB_WIND_WAVELENGTH, 300}, {RB_WIND_TWIST, 30}, {RB_WIND_GUSTS, 30}, {RB_SATIN, 0.5}, {RB_TRANSLUCENCY, 0.4}},
         {{RB_FRONT_COLOR, 30, 90, 200}, {RB_BACK_COLOR, 240, 240, 240}}},
        {"Party Streamers",
         {{RB_WIDTH, 24}, {RB_TWIST, 2}, {RB_REP_COPIES, 8}, {RB_REP_POS_X, 120}, {RB_REP_POS_Y, 60}, {RB_REP_POS_Z, 200},
          {RB_REP_ROT, 20}, {RB_REP_TILT, 20}, {RB_REP_WIDTH, 30}, {RB_REP_OFFSET, 15}, {RB_WIND_STRENGTH, 70},
          {RB_WIND_FLUTTER, 35}, {RB_WIND_TWIST, 60}, {RB_GRAD_MODE, kGradRandom}, {RB_GRAD_COUNT, k4Colors}, {RB_SATIN, 0.5},
          {RB_SHEEN, 0.3}},
         {{RB_GRAD_COLOR_1, 255, 70, 90}, {RB_GRAD_COLOR_2, 255, 210, 60}, {RB_GRAD_COLOR_3, 60, 210, 140},
          {RB_GRAD_COLOR_4, 70, 140, 255}}},
        {"Leather Strap",
         {{RB_WIDTH, 70}, {RB_TWIST, 0.5}, {RB_THICKNESS, 14}, {RB_BEVEL_SIZE, 3}, {RB_BEVEL_STYLE, kBevelChamfer},
          {RB_EDGE_COLOR_MODE, kEdgeCustom}, {RB_END_ROUNDING, 100}, {RB_SPECULAR, 0.25}, {RB_GLOSS, 18}, {RB_SATIN, 0},
          {RB_SHEEN, 0.15}, {RB_TRANSLUCENCY, 0}},
         {{RB_FRONT_COLOR, 112, 64, 36}, {RB_BACK_COLOR, 92, 52, 30}, {RB_EDGE_COLOR, 70, 40, 24}}},
        {"Copper Wire",
         {{RB_WIDTH, 16}, {RB_TWIST, 8}, {RB_TWIST_ALONG, 1}, {RB_THICKNESS, 6}, {RB_BEVEL_SIZE, 2}, {RB_METAL, 1},
          {RB_SPECULAR, 1.3}, {RB_GLOSS, 150}, {RB_SATIN, 0.2}, {RB_SHEEN, 0.4}, {RB_AMBIENT, 0.55}},
         {{RB_FRONT_COLOR, 232, 132, 84}, {RB_BACK_COLOR, 232, 132, 84}}},
        {"Liquid Chrome",
         {{RB_WIDTH, 90}, {RB_TWIST, 0.5}, {RB_THICKNESS, 18}, {RB_BEVEL_SIZE, 9}, {RB_END_ROUNDING, 100}, {RB_WAVE_AMP, 120},
          {RB_WAVE_LENGTH, 450}, {RB_WAVE_SPEED, 300}, {RB_METAL, 1}, {RB_SPECULAR, 1.4}, {RB_GLOSS, 250}, {RB_SATIN, 0},
          {RB_SHEEN, 0.7}, {RB_AMBIENT, 0.45}},
         {{RB_FRONT_COLOR, 222, 226, 236}, {RB_BACK_COLOR, 222, 226, 236}}},
        {"Paper Strip",
         {{RB_WIDTH, 80}, {RB_TWIST, 0.5}, {RB_WAVE_AMP, 30}, {RB_SPECULAR, 0.08}, {RB_GLOSS, 10}, {RB_SATIN, 0}, {RB_SHEEN, 0.05},
          {RB_TRANSLUCENCY, 0.5}, {RB_DIFFUSE, 1.0}, {RB_AMBIENT, 0.25}},
         {{RB_FRONT_COLOR, 250, 248, 240}, {RB_BACK_COLOR, 235, 230, 214}}},
        {"Velvet",
         {{RB_WIDTH, 70}, {RB_TWIST, 1}, {RB_THICKNESS, 4}, {RB_BEVEL_SIZE, 2}, {RB_SPECULAR, 0.15}, {RB_GLOSS, 10}, {RB_SATIN, 0},
          {RB_SHEEN, 1.2}, {RB_DIFFUSE, 0.8}, {RB_TRANSLUCENCY, 0.1}},
         {{RB_FRONT_COLOR, 122, 10, 42}, {RB_BACK_COLOR, 92, 10, 62}}},
        {"Holographic Foil",
         {{RB_WIDTH, 60}, {RB_TWIST, 1.5}, {RB_METAL, 0.6}, {RB_GRAD_MODE, kGradAlong}, {RB_GRAD_COUNT, k4Colors},
          {RB_GRAD_REPEAT, 3}, {RB_SPECULAR, 1.1}, {RB_GLOSS, 160}, {RB_SHEEN, 0.6}, {RB_SATIN, 0.3}},
         {{RB_GRAD_COLOR_1, 255, 150, 220}, {RB_GRAD_COLOR_2, 150, 220, 255}, {RB_GRAD_COLOR_3, 180, 255, 200},
          {RB_GRAD_COLOR_4, 255, 240, 160}}},
        {"Comet Trail",
         {{RB_SECTION, kRound}, {RB_WIDTH, 40}, {RB_WIDTH_START, 0}, {RB_TWIST, 0}, {RB_END_ROUNDING, 100},
          {RB_END_ROUND_WHICH, kEndsEnd}, {RB_GRAD_MODE, kGradAlong}, {RB_GRAD_COUNT, k3Colors}, {RB_AMBIENT, 0.6},
          {RB_SHEEN, 0.5}, {RB_SPECULAR, 0.8}, {RB_GLOSS, 80}},
         {{RB_GRAD_COLOR_1, 20, 60, 160}, {RB_GRAD_COLOR_2, 120, 200, 255}, {RB_GRAD_COLOR_3, 255, 255, 255}}},
    };
    return p;
}

void Changed(PF_ParamDef* d) { d->uu.change_flags = PF_ChangeFlag_CHANGED_VALUE; }

// Every definition ParamsSetup registered, by parameter index: its type and default value.
PF_ParamDef gDefaults[RB_NUM_PARAMS];
bool gRecorded[RB_NUM_PARAMS];
int gAdded = 0;
PF_Err (*gHostAddParam)(PF_ProgPtr, PF_ParamIndex, PF_ParamDef*) = nullptr;

PF_Err RecordingAddParam(PF_ProgPtr effect_ref, PF_ParamIndex index, PF_ParamDef* def) {
    ++gAdded;  // parameters are added in index order, the input layer (0) is implicit
    if (gAdded < RB_NUM_PARAMS && def) {
        gDefaults[gAdded] = *def;
        gRecorded[gAdded] = true;
    }
    return gHostAddParam(effect_ref, index, def);
}

// A parameter's value from a preset number (by its type).
void SetValue(PF_ParamDef* d, int index, double v) {
    if (!gRecorded[index]) return;
    switch (gDefaults[index].param_type) {
        case PF_Param_FLOAT_SLIDER: d->u.fs_d.value = v; break;
        case PF_Param_SLIDER: d->u.sd.value = A_long(v); break;
        case PF_Param_ANGLE: d->u.ad.value = PF_Fixed(v * 65536.0); break;
        case PF_Param_CHECKBOX: d->u.bd.value = v != 0 ? 1 : 0; break;
        case PF_Param_POPUP: d->u.pd.value = A_long(v); break;
        default: return;
    }
    Changed(d);
}

// Back to the value the parameter was registered with.
void ResetValue(PF_ParamDef* d, int index) {
    if (!gRecorded[index]) return;
    const PF_ParamDef& def = gDefaults[index];
    switch (def.param_type) {
        case PF_Param_FLOAT_SLIDER: d->u.fs_d.value = def.u.fs_d.value; break;
        case PF_Param_SLIDER: d->u.sd.value = def.u.sd.value; break;
        case PF_Param_ANGLE: d->u.ad.value = def.u.ad.value; break;
        case PF_Param_CHECKBOX: d->u.bd.value = def.u.bd.value; break;
        case PF_Param_POPUP: d->u.pd.value = def.u.pd.value; break;
        case PF_Param_COLOR: d->u.cd.value = def.u.cd.value; break;
        default: return;  // layers, masks, buttons, groups
    }
    Changed(d);
}

bool In(int i, int a, int b) { return i >= a && i <= b; }

}  // namespace

bool RibbonIsLookParam(int i) {
    // Left as they are although they sit among the look: where the path wraps and how it lies
    // (Cycle, Orientation), how the user's own maps are placed, and where the wind blows from.
    switch (i) {
        case RB_CYCLE:
        case RB_ORIENTATION:
        case RB_TEX_REPEAT:
        case RB_TEX_OFFSET:
        case RB_WIND_DIRECTION:
        case RB_WIND_PIN:
        case RB_ROUGH_AMOUNT:
        case RB_ROUGH_INVERT:
            return false;
        default: break;
    }
    if (In(i, RB_NRM_TOPIC, RB_NRM_TOPIC_END)) return false;
    return In(i, RB_CYCLE, RB_WAVE_SPEED) || In(i, RB_SECTION_TOPIC, RB_ENDS_TOPIC_END) || In(i, RB_REP_TOPIC, RB_REP_TOPIC_END) ||
           In(i, RB_LOOK_TOPIC, RB_GLASS_TOPIC_END) || In(i, RB_WIND_TOPIC, RB_WIND_TOPIC_END);
}

const char* RibbonPresetMenu() {
    static const std::string menu = [] {
        std::string m = "None";
        for (const Preset& p : Presets()) m += std::string("|") + p.name;
        return m;
    }();
    return menu.c_str();
}

int RibbonPresetCount() { return int(Presets().size()); }

PF_InData RibbonPresetRecorder(const PF_InData* in_data) {
    PF_InData rec = *in_data;
    gAdded = 0;
    for (bool& r : gRecorded) r = false;
    gHostAddParam = in_data->inter.add_param;
    rec.inter.add_param = RecordingAddParam;
    return rec;
}

PF_Err RibbonApplyPreset(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], bool quiet) {
    (void)in_data;
    const int sel = int(params[RB_PRESET]->u.pd.value) - 2;  // item 1 is "None"
    if (sel < 0 || sel >= RibbonPresetCount()) {
        if (!quiet) {
            std::snprintf(out_data->return_msg, sizeof(out_data->return_msg), "%s: choose a preset in \"Preset\" first.", RB_NAME);
            out_data->out_flags |= PF_OutFlag_DISPLAY_ERROR_MESSAGE;
        }
        return PF_Err_NONE;
    }
    for (int i = 1; i < RB_NUM_PARAMS; i++)
        if (RibbonIsLookParam(i)) ResetValue(params[i], i);
    const Preset& p = Presets()[size_t(sel)];
    for (const Set& s : p.sets) SetValue(params[s.param], s.param, s.v);
    for (const Col& c : p.cols) {
        PF_ParamDef* d = params[c.param];
        d->u.cd.value.red = c.r;
        d->u.cd.value.green = c.g;
        d->u.cd.value.blue = c.b;
        d->u.cd.value.alpha = 255;
        Changed(d);
    }
    out_data->out_flags |= PF_OutFlag_FORCE_RERENDER | PF_OutFlag_REFRESH_UI;
    return PF_Err_NONE;
}
