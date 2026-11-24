// RAII acquisition of host suites through SPBasic (no SDK Util sources needed). Never throws.
#pragma once

#include <cstdint>

#include "Ribbon3D.h"

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
