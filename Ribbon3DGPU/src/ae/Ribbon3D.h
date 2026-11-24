// Ribbon3D GPU - After Effects effect plugin.
#pragma once

#include "AEConfig.h"
#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_EffectCBSuites.h"
#include "AE_EffectSuites.h"
#include "AE_EffectGPUSuites.h"
#include "AE_GeneralPlug.h"   // also brings the older AEGP suite versions (AE_GeneralPlugOld.h)
#include "AE_Macros.h"
#include "Param_Utils.h"

#include "Ribbon3D_Flags.h"

#ifndef PF_DEF_NAME  // SDKs before the name field was hidden behind this macro
#define PF_DEF_NAME name
#endif

#define RB_NAME        "Ribbon 3D GPU"
#define RB_MATCH_NAME  "SQB Ribbon3D GPU"
#define RB_CATEGORY    "Generate"
#define RB_DESCRIPTION "Realistic 3D ribbon along a mask path, rendered on the GPU.\rCopyright 2026 Scriplets."

// Parameter indices. Never reorder: AE stores projects by these and by the disk IDs below.
enum {
    RB_INPUT = 0,
    RB_PATH,
    RB_SEGMENTS,
    RB_START,
    RB_END,
    RB_OFFSET,
    RB_WIDTH,
    RB_WIDTH_START,
    RB_WIDTH_END,
    RB_TWIST,
    RB_TWIST_ANGLE,
    RB_TWIST_SPEED,
    RB_TWIST_ALONG,
    RB_WAVE_AMP,
    RB_WAVE_LENGTH,
    RB_WAVE_SPEED,

    RB_XF_TOPIC,
    RB_XF_ROT_X,
    RB_XF_ROT_Y,
    RB_XF_ROT_Z,
    RB_XF_OFF_X,
    RB_XF_OFF_Y,
    RB_XF_OFF_Z,
    RB_XF_SCALE,
    RB_XF_TOPIC_END,

    RB_LOOK_TOPIC,
    RB_FRONT_COLOR,
    RB_BACK_COLOR,
    RB_TEXTURE,
    RB_TEX_REPEAT,
    RB_TEX_OFFSET,
    RB_AMBIENT,
    RB_DIFFUSE,
    RB_SPECULAR,
    RB_GLOSS,
    RB_SATIN,
    RB_SHEEN,
    RB_TRANSLUCENCY,
    RB_METAL,
    RB_LOOK_TOPIC_END,

    RB_LIGHT_TOPIC,
    RB_USE_COMP_LIGHTS,
    RB_LIGHT_AZIMUTH,
    RB_LIGHT_ELEVATION,
    RB_LIGHT_INTENSITY,
    RB_LIGHT_COLOR,
    RB_LIGHT_TOPIC_END,

    RB_RENDER_TOPIC,
    RB_QUALITY,
    RB_COMPOSITE,
    RB_OPACITY,
    RB_LINEAR,
    RB_RENDER_TOPIC_END,

    RB_NUM_PARAMS
};

// Disk IDs: stable identifiers for saved projects (1-based, never reuse).
enum { RB_DISK_ID_BASE = 1 };
#define RB_ID(param) (RB_DISK_ID_BASE + (param))

// Checkout IDs for SmartFX layer requests.
enum { RB_CHECKOUT_INPUT = 1, RB_CHECKOUT_TEX_PROBE = 2, RB_CHECKOUT_TEX = 3 };

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                                       PF_LayerDef* output, void* extra);
