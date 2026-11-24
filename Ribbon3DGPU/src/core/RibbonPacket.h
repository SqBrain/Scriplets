// Ribbon3D GPU - "vertex stage" on the CPU: camera projection, near-plane clipping,
// tile binning and material/light setup. The result (Packet) is uploaded as-is to the GPU
// or rendered by the CPU fallback with the very same kernel code.
#pragma once

#include <vector>

#include "../kernel/RibbonShared.h"
#include "RibbonGeometry.h"

namespace rb {

// After Effects camera: local axes x right, y down, z forward (into the scene).
struct Camera {
    V3 pos;
    V3 axisX{1, 0, 0}, axisY{0, 1, 0}, axisZ{0, 0, 1};  // unit, world space
    double zoom = 1000;                                 // AE "Zoom", pixels
    double cx = 0, cy = 0;                              // principal point = comp center, comp pixels
    // comp pixels -> output buffer pixels: buf = comp * scale - origin (downsampling, buffer offset)
    double scaleX = 1, scaleY = 1, originX = 0, originY = 0;
    double nearZ = 1;
};

// AE's default view when the comp has no camera (50 mm on a 36 mm film).
Camera defaultCamera(double compW, double compH);

// Camera from an AE layer-to-world matrix (A_Matrix4). Accepts both row-vector
// (translation in the last row, AE's convention) and column-vector layouts.
Camera cameraFromMatrix(const double m[4][4], double zoom, double compW, double compH);

struct LightDesc {
    int type = RK_LIGHT_PARALLEL;   // RK_LIGHT_*
    V3 pos, dir{0, 0, 1};           // dir: where the light shines to
    V3 color{1, 1, 1};              // gamma-encoded 0..1 (as in the AE UI)
    double intensity = 1;           // 1 = 100 %
    double coneDeg = 90, featherPct = 50;
};

struct Material {
    V3 front{0.80, 0.07, 0.16}, back{0.96, 0.80, 0.52};   // gamma-encoded
    V3 edge{0.80, 0.07, 0.16};                            // edges of a solid ribbon, gamma-encoded
    double ambient = 0.15, diffuse = 0.9, specular = 0.6, gloss = 40;
    double satin = 0.5, sheen = 0.25, translucency = 0.15, metal = 0;
    double texRepeat = 1, texOffset = 0;
    double opacity = 1;
    double gamma = 2.2;             // 1 for a linear working space
    int composite = RK_COMPOSITE_OVER;
    int samples = 3;                // supersampling per axis (1..4)
};

struct Packet {
    RK_Params params{};
    std::vector<RK_Tri> tris;
    std::vector<RK_Vertex> verts;
    std::vector<int> tileStart;     // tilesX * tilesY + 1 offsets into tileTris
    std::vector<int> tileTris;
    std::vector<RK_Light> lights;
};

// Fills everything except the buffer pitches and texture fields (see setBuffers / setTexture).
void buildPacket(const RibbonMesh& mesh, const Camera& cam, const std::vector<LightDesc>& lights,
                 const Material& mat, int bufWidth, int bufHeight, Packet& out);

inline void setBuffers(Packet& p, int srcPitchPixels, int dstPitchPixels) {
    p.params.srcPitch = srcPitchPixels;
    p.params.dstPitch = dstPitchPixels;
}
// Source buffer placed at (x, y) inside the output buffer, w x h pixels (0 x 0: no source).
inline void setSource(Packet& p, int x, int y, int w, int h, int pitchPixels) {
    p.params.srcX = x;
    p.params.srcY = y;
    p.params.srcW = w;
    p.params.srcH = h;
    p.params.srcPitch = pitchPixels;
}
inline void setTexture(Packet& p, int width, int height, int pitchPixels) {
    p.params.hasTexture = (width > 0 && height > 0) ? 1 : 0;
    p.params.texWidth = width;
    p.params.texHeight = height;
    p.params.texPitch = pitchPixels;
}

}  // namespace rb
