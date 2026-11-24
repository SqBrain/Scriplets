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

// Two effects from one source: "Ribbon 3D GPU" (one mask or control nulls) and, built with
// RB_MULTI_MASK, "Ribbon 3D GPU Multi-Mask" (a ribbon along every mask of the layer, trimmed
// simultaneously). Both have the same parameter list; the multi-mask effect hides what it does not use.
#ifdef RB_MULTI_MASK
#define RB_NAME        "Ribbon 3D GPU Multi-Mask"
#define RB_MATCH_NAME  "SQB Ribbon3D GPU MultiMask"
#define RB_DESCRIPTION "Realistic 3D ribbons along every mask of the layer, rendered on the GPU.\rCopyright 2026 Scriplets."
#else
#define RB_NAME        "Ribbon 3D GPU"
#define RB_MATCH_NAME  "SQB Ribbon3D GPU"
#define RB_DESCRIPTION "Realistic 3D ribbon along a mask path, rendered on the GPU.\rCopyright 2026 Scriplets."
#endif
#define RB_CATEGORY    "Generate"

#define RB_MAX_NULLS 32
#define RB_SOURCE_MASK 1
#define RB_SOURCE_NULLS 2
#define RB_ORIENT_TWIST 1
#define RB_ORIENT_FRONT 2
#define RB_GRAD_OFF 1
#define RB_GRAD_ALONG 2
#define RB_GRAD_RANDOM 3     // multi-mask effect only

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

    // 0.1.01 - appended at the end so projects saved with 0.1.00 keep their parameter indices.
    RB_SOLID_TOPIC,
    RB_THICKNESS,
    RB_BEVEL_SIZE,
    RB_BEVEL_STYLE,
    RB_BEVEL_SEGMENTS,
    RB_EDGE_COLOR_MODE,
    RB_EDGE_COLOR,
    RB_END_CAPS,
    RB_SOLID_TOPIC_END,

    // 0.1.02 - control nulls (appended for the same reason).
    RB_NULLS_TOPIC,
    RB_PATH_SOURCE,       // 1: mask, 2: control nulls
    RB_MAKE_NULLS,        // button
    RB_NULLS_CLOSED,
    RB_POINTS_TOPIC,      // collapsed group with the layer slots
    RB_NULL_1,
    RB_POINTS_TOPIC_END = RB_NULL_1 + RB_MAX_NULLS,
    RB_NULLS_TOPIC_END,

    // 0.1.04 (appended for the same reason).
    RB_CYCLE,             // checkbox: the ribbon wraps around the ends of the path
    RB_FIT_LAYER,         // layer for the "Configure" button
    RB_FIT_CONFIGURE,     // button: texture = that layer, width = its aspect over the visible length

    // 0.1.05 (appended for the same reason).
    RB_ORIENTATION,       // 1: Twist (ribbon follows the path frame, Twist applies), 2: Front

    // 0.1.06 (appended for the same reason).
    RB_ENV_TOPIC,
    RB_ENV_LAYER,         // HDRI layer (equirectangular)
    RB_ENV_INTENSITY,
    RB_ENV_ROTATION,
    RB_ENV_REFLECTION,
    RB_ENV_TOPIC_END,
    RB_GLASS_TOPIC,
    RB_GLASS,             // checkbox
    RB_GLASS_IOR,
    RB_GLASS_TRANSMISSION,
    RB_GLASS_TINT,
    RB_GLASS_REFRACTION,
    RB_GLASS_TOPIC_END,

    // 0.1.07 (appended for the same reason).
    RB_WIND_TOPIC,
    RB_WIND_STRENGTH,     // 0 = off
    RB_WIND_DIRECTION,
    RB_WIND_PIN,          // 1: Start, 2: End
    RB_WIND_FLUTTER,
    RB_WIND_SPEED,
    RB_WIND_WAVELENGTH,
    RB_WIND_TWIST,
    RB_WIND_GUSTS,
    RB_WIND_STIFFNESS,
    RB_WIND_SEED,
    RB_WIND_TOPIC_END,

    // 0.1.08 (appended for the same reason).
    RB_NRM_TOPIC,
    RB_NRM_LAYER,
    RB_NRM_STRENGTH,
    RB_NRM_MAPPING,       // 1: Tiles, 2: Like Texture
    RB_NRM_TILE,
    RB_NRM_FORMAT,        // 1: OpenGL (green up), 2: DirectX (green down)
    RB_NRM_TOPIC_END,

    // 0.1.09 (appended for the same reason).
    RB_ROUGH_TOPIC,
    RB_ROUGH_LAYER,
    RB_ROUGH_AMOUNT,
    RB_ROUGH_INVERT,
    RB_ROUGH_TOPIC_END,
    RB_FROST_TOPIC,
    RB_GLASS_FROST,
    RB_FROST_QUALITY,     // 1: Low (8), 2: Medium (16), 3: High (32)
    RB_FROST_TOPIC_END,
    RB_ENDS_TOPIC,
    RB_END_ROUNDING,
    RB_END_ROUND_WHICH,   // 1: Both, 2: Start, 3: End
    RB_ENDS_TOPIC_END,

    // 0.1.10 (appended for the same reason).
    RB_GRAD_TOPIC,
    RB_GRAD_MODE,         // RB_GRAD_*
    RB_GRAD_COUNT,        // 1..3: 2..4 colors
    RB_GRAD_COLOR_1,
    RB_GRAD_COLOR_2,
    RB_GRAD_COLOR_3,
    RB_GRAD_COLOR_4,
    RB_GRAD_REPEAT,
    RB_GRAD_OFFSET,
    RB_GRAD_SEED,
    RB_GRAD_TOPIC_END,

    RB_NUM_PARAMS
};

// Disk IDs: stable identifiers for saved projects (1-based, never reuse).
enum { RB_DISK_ID_BASE = 1 };
#define RB_ID(param) (RB_DISK_ID_BASE + (param))

// Checkout IDs for SmartFX layer requests.
enum { RB_CHECKOUT_INPUT = 1, RB_CHECKOUT_TEX_PROBE = 2, RB_CHECKOUT_TEX = 3, RB_CHECKOUT_ENV_PROBE = 4, RB_CHECKOUT_ENV = 5,
       RB_CHECKOUT_NRM_PROBE = 6, RB_CHECKOUT_NRM = 7, RB_CHECKOUT_ROUGH_PROBE = 8, RB_CHECKOUT_ROUGH = 9 };

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                                       PF_LayerDef* output, void* extra);
