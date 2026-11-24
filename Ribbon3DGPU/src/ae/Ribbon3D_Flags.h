// Plugin flags and version, written ONCE here. The code (GlobalSetup) uses these expressions and
// tools/PiPLValuesGen.cpp evaluates them with the real SDK headers at build time to generate the
// numbers for the PiPL resource, so PiPL and code can never disagree.
#pragma once

// Shown as MAJOR.MINOR.BUILD with two build digits: 0.1.14. Pre-release builds count up in BUILD;
// 1.0 is the first public release. BUILD lives in AE's 9-bit build field (up to 511): the
// bug-fix field has only 4 bits, too few for a build counter.
#define RB_MAJOR_VERSION 0
#define RB_MINOR_VERSION 1
#define RB_BUILD_VERSION 14
#define RB_STAGE PF_Stage_DEVELOP  // PF_Stage_RELEASE from 1.0 on

#define RB_VERSION PF_VERSION(RB_MAJOR_VERSION, RB_MINOR_VERSION, 0, RB_STAGE, RB_BUILD_VERSION)

// NON_PARAM_VARY: "Twist Speed" / "Wave Speed" animate with time alone.
#define RB_OUT_FLAGS (PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_NON_PARAM_VARY)

// I_MIX_GUID_DEPENDENCIES: the control nulls' transforms are mixed into the cache key in PreRender,
// so moving a null re-renders the ribbon.
#define RB_OUT_FLAGS2 (PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_FLOAT_COLOR_AWARE | \
                       PF_OutFlag2_I_MIX_GUID_DEPENDENCIES |                                  \
                       PF_OutFlag2_I_USE_3D_CAMERA | PF_OutFlag2_I_USE_3D_LIGHTS |           \
                       PF_OutFlag2_SUPPORTS_THREADED_RENDERING | PF_OutFlag2_SUPPORTS_GPU_RENDER_F32)
