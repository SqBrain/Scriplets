#include "AEConfig.h"
#include "AE_EffectVers.h"

#ifndef AE_OS_WIN
    #include <AE_General.r>
#endif

/* RB_PIPL_* come from the generated header (tools/PiPLValuesGen.cpp). */
#include "Ribbon3D_PiPLValues.h"

resource 'PiPL' (16000) {
    {
        Kind {
            AEEffect
        },
        Name {
#ifdef RB_MULTI_MASK
            "Ribbon 3D GPU Multi-Mask"
#else
            "Ribbon 3D GPU"
#endif
        },
        Category {
            "Generate"
        },
#ifdef AE_OS_WIN
    #if defined(AE_PROC_ARM64)
        CodeWinARM64 {"EffectMain"},
    #else
        CodeWin64X86 {"EffectMain"},
    #endif
#else
        CodeMacIntel64 {"EffectMain"},
        CodeMacARM64 {"EffectMain"},
#endif
        AE_PiPL_Version {
            2,
            0
        },
        AE_Effect_Spec_Version {
            PF_PLUG_IN_VERSION,
            PF_PLUG_IN_SUBVERS
        },
        AE_Effect_Version {
            RB_PIPL_VERSION
        },
        AE_Effect_Info_Flags {
            0
        },
        AE_Effect_Global_OutFlags {
            RB_PIPL_OUTFLAGS
        },
        AE_Effect_Global_OutFlags_2 {
            RB_PIPL_OUTFLAGS2
        },
        AE_Effect_Match_Name {
#ifdef RB_MULTI_MASK
            "SQB Ribbon3D GPU MultiMask"
#else
            "SQB Ribbon3D GPU"
#endif
        },
        AE_Reserved_Info {
            0
        },
        AE_Effect_Support_URL {
            "https://github.com/SqBrain/Scriplets"
        }
    }
};
