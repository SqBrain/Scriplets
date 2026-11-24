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
#define RB_GRAD_FROM_COLORS 1
#define RB_GRAD_FROM_LAYER 2

// Parameter indices in the order the parameters appear in Effect Controls. The order may change
// between versions; saved projects identify parameters by their disk IDs (below), which never do.
enum {
    RB_INPUT = 0,
    RB_PATH,              // hidden in the multi-mask effect
    RB_TRIM_MASKS,        // multi-mask effect: 1: Simultaneously, 2: Individually (hidden in the other)
    RB_SEGMENTS,
    RB_START,
    RB_END,
    RB_OFFSET,
    RB_CYCLE,             // checkbox: the ribbon wraps around the ends of the path
    RB_WIDTH,
    RB_WIDTH_START,
    RB_WIDTH_END,
    RB_ORIENTATION,       // 1: Twist (ribbon follows the path frame, Twist applies), 2: Front
    RB_TWIST,
    RB_TWIST_ANGLE,
    RB_TWIST_SPEED,
    RB_TWIST_ALONG,
    RB_WAVE_AMP,
    RB_WAVE_LENGTH,
    RB_WAVE_SPEED,

    RB_SECTION_TOPIC,
    RB_SECTION,           // 1: Flat, 2: Round, 3: Custom Profile
    RB_PROFILE_MASK,      // mask with the cross-section outline
    RB_PROFILE_DETAIL,
    RB_SECTION_TOPIC_END,

    RB_SOLID_TOPIC,
    RB_THICKNESS,
    RB_BEVEL_SIZE,
    RB_BEVEL_STYLE,
    RB_BEVEL_SEGMENTS,
    RB_EDGE_COLOR_MODE,
    RB_EDGE_COLOR,
    RB_END_CAPS,
    RB_SOLID_TOPIC_END,

    RB_ENDS_TOPIC,
    RB_END_ROUNDING,
    RB_END_ROUND_WHICH,   // 1: Both, 2: Start, 3: End
    RB_ENDS_TOPIC_END,

    RB_XF_TOPIC,
    RB_XF_ROT_X,
    RB_XF_ROT_Y,
    RB_XF_ROT_Z,
    RB_XF_OFF_X,
    RB_XF_OFF_Y,
    RB_XF_OFF_Z,
    RB_XF_SCALE,
    RB_XF_TOPIC_END,

    RB_NULLS_TOPIC,       // hidden in the multi-mask effect
    RB_PATH_SOURCE,       // 1: mask, 2: control nulls
    RB_MAKE_NULLS,        // button
    RB_NULLS_CLOSED,
    RB_POINTS_TOPIC,      // collapsed group with the layer slots
    RB_NULL_1,
    RB_POINTS_TOPIC_END = RB_NULL_1 + RB_MAX_NULLS,
    RB_NULLS_TOPIC_END,

    RB_LOOK_TOPIC,        // "Material"
    RB_FRONT_COLOR,
    RB_BACK_COLOR,
    RB_TEXTURE,
    RB_TEX_REPEAT,
    RB_TEX_OFFSET,
    RB_FIT_LAYER,         // layer for the "Configure" button
    RB_FIT_CONFIGURE,     // button: texture = that layer, width = its aspect over the visible length
    RB_AMBIENT,
    RB_DIFFUSE,
    RB_SPECULAR,
    RB_GLOSS,
    RB_SATIN,
    RB_SHEEN,
    RB_TRANSLUCENCY,
    RB_METAL,
    RB_NRM_TOPIC,
    RB_NRM_LAYER,
    RB_NRM_STRENGTH,
    RB_NRM_MAPPING,       // 1: Tiles, 2: Like Texture
    RB_NRM_TILE,
    RB_NRM_FORMAT,        // 1: OpenGL (green up), 2: DirectX (green down)
    RB_NRM_TOPIC_END,
    RB_ROUGH_TOPIC,
    RB_ROUGH_LAYER,
    RB_ROUGH_AMOUNT,
    RB_ROUGH_INVERT,
    RB_ROUGH_TOPIC_END,
    RB_LOOK_TOPIC_END,

    RB_GRAD_TOPIC,
    RB_GRAD_MODE,         // RB_GRAD_*
    RB_GRAD_SOURCE,       // 1: Colors, 2: Layer
    RB_GRAD_LAYER,        // a layer with a gradient (e.g. a shape layer with Gradient Fill)
    RB_GRAD_COUNT,        // 1..3: 2..4 colors
    RB_GRAD_COLOR_1,
    RB_GRAD_COLOR_2,
    RB_GRAD_COLOR_3,
    RB_GRAD_COLOR_4,
    RB_GRAD_REPEAT,
    RB_GRAD_OFFSET,
    RB_GRAD_SEED,
    RB_GRAD_TOPIC_END,

    RB_GLASS_TOPIC,
    RB_GLASS,             // checkbox
    RB_GLASS_IOR,
    RB_GLASS_TRANSMISSION,
    RB_GLASS_TINT,
    RB_GLASS_REFRACTION,
    RB_GLASS_FROST,
    RB_FROST_QUALITY,     // 1: Low (8), 2: Medium (16), 3: High (32)
    RB_GLASS_TOPIC_END,

    RB_ENV_TOPIC,
    RB_ENV_LAYER,         // HDRI layer (equirectangular)
    RB_ENV_INTENSITY,
    RB_ENV_ROTATION,
    RB_ENV_REFLECTION,
    RB_ENV_TOPIC_END,

    RB_LIGHT_TOPIC,
    RB_USE_COMP_LIGHTS,
    RB_LIGHT_AZIMUTH,
    RB_LIGHT_ELEVATION,
    RB_LIGHT_INTENSITY,
    RB_LIGHT_COLOR,
    RB_LIGHT_TOPIC_END,

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

    RB_RENDER_TOPIC,
    RB_QUALITY,
    RB_COMPOSITE,
    RB_OPACITY,
    RB_LINEAR,
    RB_RENDER_TOPIC_END,

    RB_NUM_PARAMS
};

// Disk IDs: what saved projects store. They follow the order in which the parameters were first
// added (0.1.00 ... 0.1.11, then new ones at the end) and never change, whatever the display order.
enum {
    D_RB_INPUT = 0,
    D_RB_PATH,
    D_RB_SEGMENTS,
    D_RB_START,
    D_RB_END,
    D_RB_OFFSET,
    D_RB_WIDTH,
    D_RB_WIDTH_START,
    D_RB_WIDTH_END,
    D_RB_TWIST,
    D_RB_TWIST_ANGLE,
    D_RB_TWIST_SPEED,
    D_RB_TWIST_ALONG,
    D_RB_WAVE_AMP,
    D_RB_WAVE_LENGTH,
    D_RB_WAVE_SPEED,
    D_RB_XF_TOPIC,
    D_RB_XF_ROT_X,
    D_RB_XF_ROT_Y,
    D_RB_XF_ROT_Z,
    D_RB_XF_OFF_X,
    D_RB_XF_OFF_Y,
    D_RB_XF_OFF_Z,
    D_RB_XF_SCALE,
    D_RB_XF_TOPIC_END,
    D_RB_LOOK_TOPIC,
    D_RB_FRONT_COLOR,
    D_RB_BACK_COLOR,
    D_RB_TEXTURE,
    D_RB_TEX_REPEAT,
    D_RB_TEX_OFFSET,
    D_RB_AMBIENT,
    D_RB_DIFFUSE,
    D_RB_SPECULAR,
    D_RB_GLOSS,
    D_RB_SATIN,
    D_RB_SHEEN,
    D_RB_TRANSLUCENCY,
    D_RB_METAL,
    D_RB_LOOK_TOPIC_END,
    D_RB_LIGHT_TOPIC,
    D_RB_USE_COMP_LIGHTS,
    D_RB_LIGHT_AZIMUTH,
    D_RB_LIGHT_ELEVATION,
    D_RB_LIGHT_INTENSITY,
    D_RB_LIGHT_COLOR,
    D_RB_LIGHT_TOPIC_END,
    D_RB_RENDER_TOPIC,
    D_RB_QUALITY,
    D_RB_COMPOSITE,
    D_RB_OPACITY,
    D_RB_LINEAR,
    D_RB_RENDER_TOPIC_END,
    D_RB_SOLID_TOPIC,
    D_RB_THICKNESS,
    D_RB_BEVEL_SIZE,
    D_RB_BEVEL_STYLE,
    D_RB_BEVEL_SEGMENTS,
    D_RB_EDGE_COLOR_MODE,
    D_RB_EDGE_COLOR,
    D_RB_END_CAPS,
    D_RB_SOLID_TOPIC_END,
    D_RB_NULLS_TOPIC,
    D_RB_PATH_SOURCE,
    D_RB_MAKE_NULLS,
    D_RB_NULLS_CLOSED,
    D_RB_POINTS_TOPIC,
    D_RB_NULL_1,
    D_RB_POINTS_TOPIC_END = D_RB_NULL_1 + RB_MAX_NULLS,
    D_RB_NULLS_TOPIC_END,
    D_RB_CYCLE,
    D_RB_FIT_LAYER,
    D_RB_FIT_CONFIGURE,
    D_RB_ORIENTATION,
    D_RB_ENV_TOPIC,
    D_RB_ENV_LAYER,
    D_RB_ENV_INTENSITY,
    D_RB_ENV_ROTATION,
    D_RB_ENV_REFLECTION,
    D_RB_ENV_TOPIC_END,
    D_RB_GLASS_TOPIC,
    D_RB_GLASS,
    D_RB_GLASS_IOR,
    D_RB_GLASS_TRANSMISSION,
    D_RB_GLASS_TINT,
    D_RB_GLASS_REFRACTION,
    D_RB_GLASS_TOPIC_END,
    D_RB_WIND_TOPIC,
    D_RB_WIND_STRENGTH,
    D_RB_WIND_DIRECTION,
    D_RB_WIND_PIN,
    D_RB_WIND_FLUTTER,
    D_RB_WIND_SPEED,
    D_RB_WIND_WAVELENGTH,
    D_RB_WIND_TWIST,
    D_RB_WIND_GUSTS,
    D_RB_WIND_STIFFNESS,
    D_RB_WIND_SEED,
    D_RB_WIND_TOPIC_END,
    D_RB_NRM_TOPIC,
    D_RB_NRM_LAYER,
    D_RB_NRM_STRENGTH,
    D_RB_NRM_MAPPING,
    D_RB_NRM_TILE,
    D_RB_NRM_FORMAT,
    D_RB_NRM_TOPIC_END,
    D_RB_ROUGH_TOPIC,
    D_RB_ROUGH_LAYER,
    D_RB_ROUGH_AMOUNT,
    D_RB_ROUGH_INVERT,
    D_RB_ROUGH_TOPIC_END,
    D_RB_FROST_TOPIC,
    D_RB_GLASS_FROST,
    D_RB_FROST_QUALITY,
    D_RB_FROST_TOPIC_END,
    D_RB_ENDS_TOPIC,
    D_RB_END_ROUNDING,
    D_RB_END_ROUND_WHICH,
    D_RB_ENDS_TOPIC_END,
    D_RB_GRAD_TOPIC,
    D_RB_GRAD_MODE,
    D_RB_GRAD_COUNT,
    D_RB_GRAD_COLOR_1,
    D_RB_GRAD_COLOR_2,
    D_RB_GRAD_COLOR_3,
    D_RB_GRAD_COLOR_4,
    D_RB_GRAD_REPEAT,
    D_RB_GRAD_OFFSET,
    D_RB_GRAD_SEED,
    D_RB_GRAD_TOPIC_END,
    D_RB_SECTION_TOPIC,
    D_RB_SECTION,
    D_RB_PROFILE_MASK,
    D_RB_PROFILE_DETAIL,
    D_RB_SECTION_TOPIC_END,
    D_RB_TRIM_MASKS,
    // 0.1.12
    D_RB_GRAD_SOURCE,
    D_RB_GRAD_LAYER,
    D_RB_DISK_END
};

enum { RB_DISK_ID_BASE = 1 };
// Disk ID of a parameter, by its RB_ name (the D_ entry of the same name).
#define RB_DISK_ID(param) (RB_DISK_ID_BASE + D_##param)

// Checkout IDs for SmartFX layer requests.
enum { RB_CHECKOUT_INPUT = 1, RB_CHECKOUT_TEX_PROBE = 2, RB_CHECKOUT_TEX = 3, RB_CHECKOUT_ENV_PROBE = 4, RB_CHECKOUT_ENV = 5,
       RB_CHECKOUT_NRM_PROBE = 6, RB_CHECKOUT_NRM = 7, RB_CHECKOUT_ROUGH_PROBE = 8, RB_CHECKOUT_ROUGH = 9,
       RB_CHECKOUT_GRAD_PROBE = 10, RB_CHECKOUT_GRAD = 11 };

extern "C" DllExport PF_Err EffectMain(PF_Cmd cmd, PF_InData* in_data, PF_OutData* out_data, PF_ParamDef* params[],
                                       PF_LayerDef* output, void* extra);
