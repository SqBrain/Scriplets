// Ribbon3D GPU - GPU back ends (OpenCL, CUDA, Metal) behind one interface.
#pragma once

#include "Ribbon3D.h"
#include "core/RibbonPacket.h"

// True when this build can render with the framework (decides GPU_RENDER_POSSIBLE).
bool RibbonGPU_Supports(PF_GPU_Framework fw);

PF_Err RibbonGPU_DeviceSetup(PF_InData* in_data, PF_OutData* out_data, PF_GPUDeviceSetupExtra* extra);
PF_Err RibbonGPU_DeviceSetdown(PF_InData* in_data, PF_OutData* out_data, PF_GPUDeviceSetdownExtra* extra);

// Whether an environment layer can be used on this framework (its GPU world is read back to the
// CPU); PreRender leaves the frame to the CPU path where it cannot (Metal).
bool RibbonGPU_CanReadBack(PF_GPU_Framework fw);

// src / tex / env / nrm may be null. srcX / srcY: position of the source buffer inside the output buffer.
PF_Err RibbonGPU_Render(PF_InData* in_data, PF_SmartRenderExtra* extra, rb::Packet& pk, int srcX, int srcY,
                        PF_EffectWorld* src, PF_EffectWorld* dst, PF_EffectWorld* tex, PF_EffectWorld* env,
                        PF_EffectWorld* nrm);
