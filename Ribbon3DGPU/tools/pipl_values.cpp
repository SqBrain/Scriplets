// Compiled (not run) by tools/build_win_mingw.sh with the target compiler and the real SDK headers:
// the PiPL numbers are read back from the object file's ".rbpipl" section, so a cross build needs
// no way to execute Windows programs.
#include "AEConfig.h"
#include "AE_Effect.h"
#include "AE_EffectVers.h"
#include "Ribbon3D_Flags.h"

extern "C" __attribute__((used, section(".rbpipl"))) const unsigned int rb_pipl_values[5] = {
    (unsigned int)(RB_VERSION), (unsigned int)(RB_OUT_FLAGS), (unsigned int)(RB_OUT_FLAGS2),
    (unsigned int)(PF_PLUG_IN_VERSION), (unsigned int)(PF_PLUG_IN_SUBVERS)};
