// Runs the embedded OpenCL kernel source (the exact string the plugin compiles at run time)
// on any available OpenCL device and compares it with the CPU path pixel by pixel.
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>

#include "RibbonKernelSource.h"
#include "TestScene.h"

using namespace rb;

#define CL_CHECK(x)                                                                       \
    do {                                                                                  \
        cl_int e_ = (x);                                                                  \
        if (e_ != CL_SUCCESS) { std::printf("OpenCL error %d at %s:%d\n", e_, __FILE__, __LINE__); return 2; } \
    } while (0)

int main(int argc, char** argv) {
    cl_platform_id platform;
    cl_uint np = 0;
    if (clGetPlatformIDs(1, &platform, &np) != CL_SUCCESS || np == 0) {
        std::printf("no OpenCL platform: skipped\n");
        return 0;
    }
    cl_device_id device;
    CL_CHECK(clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, nullptr));
    char name[256] = {0};
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(name), name, nullptr);
    std::printf("device: %s\n", name);
    cl_int err;
    cl_context ctx = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &err);
    CL_CHECK(err);
    cl_command_queue q = clCreateCommandQueue(ctx, device, 0, &err);
    CL_CHECK(err);

    std::vector<size_t> sizes;
    for (int i = 0; i < kRibbonKernelCLCount; i++) sizes.push_back(std::strlen(kRibbonKernelCL[i]));
    cl_program prog = clCreateProgramWithSource(ctx, cl_uint(kRibbonKernelCLCount), const_cast<const char**>(kRibbonKernelCL), sizes.data(), &err);
    CL_CHECK(err);
    // Same options as the plugin.
    err = clBuildProgram(prog, 1, &device, "-cl-single-precision-constant -cl-mad-enable", nullptr, nullptr);
    if (err != CL_SUCCESS) {
        size_t len = 0;
        clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &len);
        std::string log(len, '\0');
        clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, len, &log[0], nullptr);
        std::printf("build failed:\n%s\n", log.c_str());
        return 1;
    }
    cl_kernel k = clCreateKernel(prog, "RibbonRender", &err);
    CL_CHECK(err);

    // Scene with everything enabled: twist, wave, lights of all types, texture, supersampling.
    const int W = 960, H = 540, TW = 64, TH = 16;
    Polyline path = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 48);
    PathTransform xf;
    xf.scale = 0.5;
    Polyline p2 = transformPolyline(path, xf);
    RibbonSettings rs;
    rs.segments = 300; rs.twistTurns = 1.5; rs.width = 70; rs.waveAmp = 40; rs.waveLength = 300;
    Material mat;
    mat.samples = 3; mat.satin = 0.6; mat.translucency = 0.3; mat.metal = 0.2; mat.texRepeat = 3;
    std::vector<LightDesc> lights = test::studioLights();
    LightDesc spot;
    spot.type = RK_LIGHT_SPOT; spot.pos = V3(480, 0, -600); spot.dir = V3(0, 0.5, 1); spot.coneDeg = 60; spot.color = V3(1, 0.8, 0.6);
    LightDesc par;
    par.type = RK_LIGHT_PARALLEL; par.dir = V3(-1, 1, 1); par.intensity = 0.4;
    lights.push_back(spot);
    lights.push_back(par);
    Packet pk;
    buildPacket(buildRibbon(p2, rs), defaultCamera(W, H), lights, mat, W, H, pk);
    std::vector<float> tex(size_t(TW) * TH * 4);
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++) {
            float* t = &tex[(size_t(y) * TW + x) * 4];
            bool c = ((x / 8) + (y / 8)) % 2;
            float a = (x % 16 == 0) ? 0.0f : 1.0f;  // transparent stripes
            t[0] = (c ? 0.9f : 0.1f) * a; t[1] = 0.5f * a; t[2] = (c ? 0.2f : 0.8f) * a; t[3] = a;
        }
    setTexture(pk, TW, TH, TW);
    test::Image src(W, H, 0.1f, 0.2f, 0.3f, 1.0f);
    test::Image cpu = test::render(pk, src, tex.data());

    auto buf = [&](const void* data, size_t bytes, cl_mem_flags fl) {
        cl_int e;
        cl_mem m = clCreateBuffer(ctx, fl | (data ? CL_MEM_COPY_HOST_PTR : 0), bytes, const_cast<void*>(data), &e);
        return m;
    };
    cl_mem bP = buf(&pk.params, sizeof(RK_Params), CL_MEM_READ_ONLY);
    cl_mem bT = buf(pk.tris.data(), pk.tris.size() * sizeof(RK_Tri), CL_MEM_READ_ONLY);
    cl_mem bV = buf(pk.verts.data(), pk.verts.size() * sizeof(RK_Vertex), CL_MEM_READ_ONLY);
    cl_mem bS = buf(pk.tileStart.data(), pk.tileStart.size() * sizeof(int), CL_MEM_READ_ONLY);
    cl_mem bI = buf(pk.tileTris.data(), pk.tileTris.size() * sizeof(int), CL_MEM_READ_ONLY);
    cl_mem bL = buf(pk.lights.data(), pk.lights.size() * sizeof(RK_Light), CL_MEM_READ_ONLY);
    cl_mem bX = buf(tex.data(), tex.size() * sizeof(float), CL_MEM_READ_ONLY);
    cl_mem bSrc = buf(src.bgra.data(), src.bgra.size() * sizeof(float), CL_MEM_READ_ONLY);
    cl_mem bDst = buf(nullptr, src.bgra.size() * sizeof(float), CL_MEM_WRITE_ONLY);
    cl_mem args[] = {bP, bT, bV, bS, bI, bL, bX, bSrc, bDst};
    for (cl_uint i = 0; i < 9; i++) CL_CHECK(clSetKernelArg(k, i, sizeof(cl_mem), &args[i]));
    size_t local[2] = {16, 16}, global[2] = {size_t((W + 15) / 16 * 16), size_t((H + 15) / 16 * 16)};
    auto t0 = std::chrono::steady_clock::now();
    CL_CHECK(clEnqueueNDRangeKernel(q, k, 2, nullptr, global, local, 0, nullptr, nullptr));
    CL_CHECK(clFinish(q));
    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    test::Image gpu(W, H);
    CL_CHECK(clEnqueueReadBuffer(q, bDst, CL_TRUE, 0, gpu.bgra.size() * sizeof(float), gpu.bgra.data(), 0, nullptr, nullptr));

    double maxDiff = 0, sumDiff = 0;
    int bad = 0;
    for (size_t i = 0; i < cpu.bgra.size(); i++) {
        double d = std::fabs(double(cpu.bgra[i]) - gpu.bgra[i]);
        maxDiff = std::max(maxDiff, d);
        sumDiff += d;
        if (d > 1e-3) bad++;
    }
    std::printf("OpenCL kernel %.1f ms; CPU vs OpenCL: max |diff| %.2e, mean %.2e, values > 1e-3: %d of %zu\n", ms, maxDiff,
                sumDiff / cpu.bgra.size(), bad, cpu.bgra.size());
    if (argc > 1) {
        const float bg[3] = {0, 0, 0};
        test::writePNG(std::string(argv[1]) + "/opencl.png", gpu, bg);
        test::writePNG(std::string(argv[1]) + "/opencl_cpu.png", cpu, bg);
    }
    // Rare edge-sample flips (float rounding exactly on a shared edge) are allowed, nothing else.
    bool ok = bad < int(cpu.bgra.size() / 20000) + 8;
    std::printf(ok ? "parity OK\n" : "parity FAILED\n");
    for (cl_mem m : args) clReleaseMemObject(m);
    clReleaseKernel(k);
    clReleaseProgram(prog);
    clReleaseCommandQueue(q);
    clReleaseContext(ctx);
    return ok ? 0 : 1;
}
