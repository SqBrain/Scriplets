// Ribbon3D GPU - Metal back end (implemented in Ribbon3D_Metal.mm, macOS only).
#pragma once

#include "core/RibbonPacket.h"

// device: id<MTLDevice>. On success *pipeline receives a retained id<MTLComputePipelineState>.
bool RibbonMetal_Setup(void* device, void** pipeline);
void RibbonMetal_Release(void* pipeline);
// queue: id<MTLCommandQueue>; src / dst / tex: id<MTLBuffer> (src and tex must be valid buffers).
bool RibbonMetal_Render(void* pipeline, void* device, void* queue, const rb::Packet& pk, void* src, void* dst, void* tex, void* nrm, void* rough);
