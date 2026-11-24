// Ribbon3D GPU - CPU fallback: runs the same per-pixel kernel (compiled as C++) on worker threads.
#pragma once

#include "RibbonPacket.h"

namespace rb {

// src / dst / tex / nrm / rough: BGRA float, premultiplied, pitches taken from the packet params.
// tex / nrm may be null when the packet has no texture / normal map. threads <= 0: hardware concurrency.
void renderCPU(const Packet& p, const float* src, float* dst, const float* tex, int threads = 0, const float* nrm = nullptr,
               const float* rough = nullptr);

}  // namespace rb
