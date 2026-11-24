// Ribbon3D GPU - Metal back end. Compiled as Objective-C++ without ARC (manual retain/release).
#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

#include <algorithm>

#include "Ribbon3D_Metal.h"
#include "RibbonKernelSource.h"

bool RibbonMetal_Setup(void* devicePV, void** pipeline) {
    @autoreleasepool {
        id<MTLDevice> device = (id<MTLDevice>)devicePV;
        if (!device || !pipeline) return false;
        NSMutableString* source = [NSMutableString string];
        for (int i = 0; i < kRibbonKernelMetalCount; i++) [source appendString:[NSString stringWithUTF8String:kRibbonKernelMetal[i]]];
        MTLCompileOptions* options = [[[MTLCompileOptions alloc] init] autorelease];
        options.fastMathEnabled = YES;
        NSError* error = nil;
        id<MTLLibrary> library = [device newLibraryWithSource:source options:options error:&error];
        if (!library) {
            NSLog(@"Ribbon3D GPU: Metal kernel compile failed: %@", error);
            return false;
        }
        id<MTLFunction> function = [library newFunctionWithName:@"RibbonRender"];
        id<MTLComputePipelineState> state = function ? [device newComputePipelineStateWithFunction:function error:&error] : nil;
        [function release];
        [library release];
        if (!state) {
            NSLog(@"Ribbon3D GPU: Metal pipeline failed: %@", error);
            return false;
        }
        *pipeline = (void*)state;  // +1, released in RibbonMetal_Release
        return true;
    }
}

void RibbonMetal_Release(void* pipeline) {
    if (pipeline) [(id<MTLComputePipelineState>)pipeline release];
}

bool RibbonMetal_Render(void* pipelinePV, void* devicePV, void* queuePV, const rb::Packet& pk, void* src, void* dst, void* tex) {
    @autoreleasepool {
        id<MTLComputePipelineState> pipeline = (id<MTLComputePipelineState>)pipelinePV;
        id<MTLDevice> device = (id<MTLDevice>)devicePV;
        id<MTLCommandQueue> queue = (id<MTLCommandQueue>)queuePV;
        if (!pipeline || !device || !queue || !dst) return false;

        const void* data[7] = {&pk.params, pk.tris.data(), pk.verts.data(), pk.tileStart.data(), pk.tileTris.data(), pk.lights.data(),
                               pk.env.data()};
        const size_t bytes[7] = {sizeof(RK_Params), pk.tris.size() * sizeof(RK_Tri), pk.verts.size() * sizeof(RK_Vertex),
                                 pk.tileStart.size() * sizeof(int), pk.tileTris.size() * sizeof(int), pk.lights.size() * sizeof(RK_Light),
                                 pk.env.size() * sizeof(float)};
        id<MTLBuffer> buffers[7];
        for (int i = 0; i < 7; i++) buffers[i] = [device newBufferWithBytes:data[i] length:bytes[i] options:MTLResourceStorageModeShared];

        id<MTLCommandBuffer> cmd = [queue commandBuffer];
        id<MTLComputeCommandEncoder> enc = [cmd computeCommandEncoder];
        [enc setComputePipelineState:pipeline];
        for (int i = 0; i < 7; i++) [enc setBuffer:buffers[i] offset:0 atIndex:NSUInteger(i)];
        [enc setBuffer:(id<MTLBuffer>)tex offset:0 atIndex:7];
        [enc setBuffer:(id<MTLBuffer>)src offset:0 atIndex:8];
        [enc setBuffer:(id<MTLBuffer>)dst offset:0 atIndex:9];
        NSUInteger tw = 16, th = std::max<NSUInteger>(1, std::min<NSUInteger>(16, pipeline.maxTotalThreadsPerThreadgroup / tw));
        MTLSize perGroup = MTLSizeMake(tw, th, 1);
        MTLSize groups = MTLSizeMake((NSUInteger(pk.params.width) + tw - 1) / tw, (NSUInteger(pk.params.height) + th - 1) / th, 1);
        [enc dispatchThreadgroups:groups threadsPerThreadgroup:perGroup];
        [enc endEncoding];
        [cmd commit];  // the command buffer retains the buffers until it completes
        for (int i = 0; i < 7; i++) [buffers[i] release];
        return true;
    }
}
