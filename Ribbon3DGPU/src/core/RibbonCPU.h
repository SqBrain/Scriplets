// Ribbon3D GPU - CPU fallback: runs the same per-pixel kernel (compiled as C++) on worker threads.
#pragma once

#include "RibbonPacket.h"

namespace rb {

// src / dst / tex: BGRA float, premultiplied, pitches taken from the packet params.
// tex may be null when the packet has no texture. threads <= 0: hardware concurrency.
void renderCPU(const Packet& p, const float* src, float* dst, const float* tex, int threads = 0);

}  // namespace rb
