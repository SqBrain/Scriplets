// Ribbon3D GPU - geometry: path -> arc length + rotation minimizing frame -> ribbon mesh.
// Portable C++17, no After Effects dependencies (unit-tested on its own).
#pragma once

#include <array>
#include <cmath>
#include <vector>

#include "../kernel/RibbonShared.h"

namespace rb {

struct V3 {
    double x = 0, y = 0, z = 0;
    V3() = default;
    V3(double x_, double y_, double z_) : x(x_), y(y_), z(z_) {}
    V3 operator+(const V3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    V3 operator-(const V3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    V3 operator*(double s) const { return {x * s, y * s, z * s}; }
    V3 operator-() const { return {-x, -y, -z}; }
};
inline double dot(const V3& a, const V3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 cross(const V3& a, const V3& b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline double length(const V3& a) { return std::sqrt(dot(a, a)); }
inline V3 normalize(const V3& a, const V3& fallback = V3(0, 0, 1)) {
    double l = length(a);
    return l > 1e-12 ? a * (1.0 / l) : fallback;
}
inline V3 lerp(const V3& a, const V3& b, double t) { return a + (b - a) * t; }
inline V3 orthoTo(const V3& v, const V3& unitT) { return v - unitT * dot(v, unitT); }

// 3x3 rotation, row-major, acting on column vectors.
struct M3 {
    double m[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    V3 operator*(const V3& v) const {
        return {m[0][0] * v.x + m[0][1] * v.y + m[0][2] * v.z, m[1][0] * v.x + m[1][1] * v.y + m[1][2] * v.z,
                m[2][0] * v.x + m[2][1] * v.y + m[2][2] * v.z};
    }
    M3 operator*(const M3& o) const {
        M3 r;
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++) r.m[i][j] = m[i][0] * o.m[0][j] + m[i][1] * o.m[1][j] + m[i][2] * o.m[2][j];
        return r;
    }
};
// After Effects orientation convention: R = Rx * Ry * Rz (degrees).
M3 rotationXYZ(double xDeg, double yDeg, double zDeg);

using Bezier = std::array<V3, 4>;

// Dense polyline in world space plus the reference "up" (normal of the plane the path was drawn on).
struct Polyline {
    std::vector<V3> pts;
    bool closed = false;
    V3 up{0, 0, 1};
};

Polyline bezierToPolyline(const std::vector<Bezier>& segs, bool closed, const V3& up, int samplesPerSegment = 32);

struct PathTransform {
    double rotX = 0, rotY = 0, rotZ = 0;   // degrees, AE order
    V3 offset;
    double scale = 1;
    V3 pivot;                               // rotation / scale center
};
Polyline transformPolyline(const Polyline& in, const PathTransform& xf);

struct RibbonSettings {
    double start = 0, end = 1, offset = 0;          // fractions of the path length
    double width = 60, widthStart = 1, widthEnd = 1; // px, multipliers at the ends
    double twistTurns = 1, twistAngleDeg = 0, twistSpeedDeg = 0;
    bool twistAlongPath = true;
    double waveAmp = 0, waveLength = 500, waveSpeed = 0;
    int segments = 300;                              // along the ribbon
    int across = 1;                                  // subdivisions across
    double time = 0;                                 // seconds
};

struct RibbonMesh {
    std::vector<RK_Vertex> verts;
    std::vector<int> tris;  // 3 indices per triangle
    // Center line and frames per cross section (handy for tests and tools).
    std::vector<V3> center, tangent, widthDir;
    std::vector<double> widths;
};

// Arc-length parametrised curve with a rotation minimizing frame.
class FramedCurve {
public:
    explicit FramedCurve(const Polyline& p);
    bool valid() const { return len_ > 1e-9; }
    bool closed() const { return closed_; }
    double length() const { return len_; }
    // s: fraction of the length (clamped for open, wrapped for closed paths).
    void at(double s, V3& pos, V3& tangent, V3& up) const;

private:
    std::vector<V3> X_, T_, R_;
    std::vector<double> S_;
    double len_ = 0;
    bool closed_ = false;
};

RibbonMesh buildRibbon(const Polyline& path, const RibbonSettings& rs);

}  // namespace rb
