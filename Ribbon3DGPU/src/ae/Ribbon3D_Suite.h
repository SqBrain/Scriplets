// RAII acquisition of host suites through SPBasic (no SDK Util sources needed). Never throws.
#pragma once

#include <cstdint>

#include "Ribbon3D.h"

// AEGP suite versions. Older versions stay available in AE for compatibility (declared in
// AE_GeneralPlugOld.h, included by AE_GeneralPlug.h).
#define RB_LAYER_SUITE AEGP_LayerSuite8
#define RB_LAYER_SUITE_VERSION kAEGPLayerSuiteVersion8
#define RB_STREAM_SUITE AEGP_StreamSuite5
#define RB_STREAM_SUITE_VERSION kAEGPStreamSuiteVersion5
#define RB_LIGHT_SUITE AEGP_LightSuite2
#define RB_LIGHT_SUITE_VERSION kAEGPLightSuiteVersion2
#define RB_COMP_SUITE AEGP_CompSuite11
#define RB_COMP_SUITE_VERSION kAEGPCompSuiteVersion11
#define RB_UTILITY_SUITE AEGP_UtilitySuite5
#define RB_UTILITY_SUITE_VERSION kAEGPUtilitySuiteVersion5
#define RB_EFFECT_SUITE AEGP_EffectSuite4
#define RB_EFFECT_SUITE_VERSION kAEGPEffectSuiteVersion4

template <typename T>
class Suite {
public:
    Suite(const PF_InData* in, const char* name, int32_t version) : basic_(in->pica_basicP), name_(name), version_(version) {
        const void* p = nullptr;
        if (basic_ && basic_->AcquireSuite(name, version, &p) == kSPNoError) suite_ = static_cast<const T*>(p);
    }
    ~Suite() {
        if (suite_) basic_->ReleaseSuite(name_, version_);
    }
    Suite(const Suite&) = delete;
    Suite& operator=(const Suite&) = delete;
    const T* operator->() const { return suite_; }
    const T* get() const { return suite_; }
    explicit operator bool() const { return suite_ != nullptr; }

private:
    SPBasicSuite* basic_;
    const char* name_;
    int32_t version_;
    const T* suite_ = nullptr;
};
