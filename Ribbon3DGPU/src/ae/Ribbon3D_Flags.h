// Plugin flags and version, written ONCE here. The code (GlobalSetup) uses these expressions and
// tools/PiPLValuesGen.cpp evaluates them with the real SDK headers at build time to generate the
// numbers for the PiPL resource, so PiPL and code can never disagree.
#pragma once

#define RB_MAJOR_VERSION 1
#define RB_MINOR_VERSION 0
#define RB_BUG_VERSION   0
#define RB_BUILD_VERSION 1

#define RB_VERSION PF_VERSION(RB_MAJOR_VERSION, RB_MINOR_VERSION, RB_BUG_VERSION, PF_Stage_RELEASE, RB_BUILD_VERSION)

// NON_PARAM_VARY: "Twist Speed" / "Wave Speed" animate with time alone.
#define RB_OUT_FLAGS (PF_OutFlag_DEEP_COLOR_AWARE | PF_OutFlag_NON_PARAM_VARY)

#define RB_OUT_FLAGS2 (PF_OutFlag2_SUPPORTS_SMART_RENDER | PF_OutFlag2_FLOAT_COLOR_AWARE | \
                       PF_OutFlag2_I_USE_3D_CAMERA | PF_OutFlag2_I_USE_3D_LIGHTS |           \
                       PF_OutFlag2_SUPPORTS_THREADED_RENDERING | PF_OutFlag2_SUPPORTS_GPU_RENDER_F32)
