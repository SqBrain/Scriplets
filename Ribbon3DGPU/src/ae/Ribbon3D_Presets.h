// Ribbon3D GPU - built-in presets: the "Preset" popup and the "Apply Preset" button.
#pragma once

#include "Ribbon3D.h"

// The popup's items: "None|Silk|Gold Foil|...". The string lives for the whole session.
const char* RibbonPresetMenu();
int RibbonPresetCount();  // presets, without "None"

// PF_Cmd_USER_CHANGED_PARAM for RB_PRESET_APPLY: the look of the ribbon (shape, material,
// gradient, glass, wind, repeater) goes back to its defaults, then the chosen preset is set.
// The path and its trim / animation, Path Transform, control nulls, layer slots, lighting,
// environment and render settings stay as they are.
// Picking a preset in the popup applies it at once (quiet: "None" does nothing); the button
// applies it again, which also drops any changes made after it.
PF_Err RibbonApplyPreset(PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[], bool quiet);

// ParamsSetup adds its parameters through this copy of in_data, which remembers every definition
// (its type and default) on the way to the host: that is what Apply Preset resets to.
PF_InData RibbonPresetRecorder(const PF_InData* in_data);

// Whether Apply Preset resets this parameter (exposed for tests).
bool RibbonIsLookParam(int index);
