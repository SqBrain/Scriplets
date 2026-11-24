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
    // Optional roll controls: at polyline sample `index` the ribbon's up vector should be `up`
    // (the ribbon twists smoothly between controls). Used by control nulls.
    struct RollControl { int index; V3 up; };
    std::vector<RollControl> rolls;
};

// A control null: world position, unit local axes (X = tangent direction, Z = ribbon up, so the
// ribbon's width lies along Y) and the handle scale (1 = smooth default, 0 = sharp corner).
struct ControlPoint {
    V3 pos;
    V3 axisX{1, 0, 0}, axisY{0, 1, 0}, axisZ{0, 0, 1};
    double handleScale = 1;
};
// Cubic spline through the control points: tangent along each point's X axis, handle length
// = handleScale * (distance to the neighbour) / 3. Adds a roll control per point.
Polyline controlPointsToPolyline(const std::vector<ControlPoint>& cps, bool closed, int samplesPerSegment = 32);

Polyline bezierToPolyline(const std::vector<Bezier>& segs, bool closed, const V3& up, int samplesPerSegment = 32);

struct PathTransform {
    double rotX = 0, rotY = 0, rotZ = 0;   // degrees, AE order
    V3 offset;
    double scale = 1;
    V3 pivot;                               // rotation / scale center
};
Polyline transformPolyline(const Polyline& in, const PathTransform& xf);

// "Front" orientation: the path as drawn in the comp is a view from above. It is laid onto the
// horizontal plane through `pivot`: drawing X stays X, drawing Y becomes depth (the top of the
// drawing is far, +Z), drawing Z becomes height. The ribbon lies flat with its front facing down
// (+Y, AE's Y axis points down). A rigid rotation, so lengths and the control nulls' rolls carry over.
Polyline topViewToFloor(const Polyline& in, const V3& pivot);

// Cross-section of a solid ribbon: closed loop (front face, right edge, back face, left edge) in the
// (width, front-normal) plane. The point count depends only on the segment settings.
struct ProfilePoint {
    double x, y;     // position: x along the width direction, y along the front normal
    double nx, ny;   // unit outward normal
    double t = 0;    // round / custom sections: position around the outline 0..1 (texture v)
};
// Cross sections: the flat ribbon (sheet, or a solid with Thickness and Bevel), a round tube, or a
// custom outline swept along the path.
enum SectionShape { kSectionFlat = 0, kSectionRound = 1, kSectionCustom = 2 };
// Unit-diameter circle, counter-clockwise from the width direction, seam duplicated at t = 1.
std::vector<ProfilePoint> roundProfile(int points);
// A closed outline (any position, scale and orientation; drawing y points down) -> a unit-width
// profile centered on its bounding box, drawing up = front: points at even arc length, outward
// normals, sharp corners kept sharp (split into two points with their own normals), seam
// duplicated at t = 1. Empty for fewer than 3 distinct points.
std::vector<ProfilePoint> outlineProfile(const std::vector<V3>& outline, int points);

// Wind: one end of the path stays pinned, the rest streams downwind and flutters. Procedural
// (a function of time, no simulation state), so every frame renders on its own.
struct WindSettings {
    double strength = 0;          // 0 = off, 1 = 100 %
    V3 direction{1, 0, 0};        // where the wind blows to, world space, unit
    bool pinStart = true;         // pinned end: path start (true) or end
    double flutterPx = 60;        // wave amplitude at the free end, at 100 %
    double flutterHz = 1.5;       // waves per second running from the pinned end to the free end
    double wavelength = 300;      // px along the ribbon
    double twistDeg = 40;         // ribbon twisting about its length, at the free end
    double gusts = 0.3;           // 0..1: random variation of the strength over time
    double stiffness = 0.3;       // 0..1: higher = the part near the pin moves less
    int seed = 0;
};
// Deterministic pseudo-random number in [0, 1) for (index, seed).
double hashUnit(long long index, int seed);
// Strength multiplier at time t: 1 +- gusts (smooth noise, same seed = same gusts).
double windGust(const WindSettings& w, double time);
// The world-space path bent by the wind at time t. Segment lengths are kept (the path is
// re-laid from the pinned end), so the ribbon bends instead of stretching.
Polyline applyWind(const Polyline& world, const WindSettings& w, double time);
// Extra twist (radians) from the wind at path fraction s (0..1), path length L.
double windTwist(const WindSettings& w, double s, double L, double time);

struct RibbonSettings {
    double start = 0, end = 1, offset = 0;          // fractions of the path length
    double width = 60, widthStart = 1, widthEnd = 1; // px, multipliers at the ends
    double twistTurns = 1, twistAngleDeg = 0, twistSpeedDeg = 0;
    bool twistAlongPath = true;
    double waveAmp = 0, waveLength = 500, waveSpeed = 0;
    int segments = 300;                              // along the ribbon
    int across = 1;                                  // subdivisions across
    double time = 0;                                 // seconds
    // Thickness > 0 turns the sheet into a closed solid with a beveled cross-section.
    double thickness = 0;                            // px
    double bevel = 3;                                // px: corner radius (round) or cut size (chamfer)
    int bevelSegments = 5;                           // per corner, round bevel
    bool chamfer = false;                            // straight 45 degree cut instead of a round corner
    bool caps = true;                                // close both ends
    // Where start + offset .. end + offset leaves 0..1: true wraps the ribbon around (it continues
    // from the other end, also on open paths), false hides the part outside the path.
    bool cycle = true;
    // Cross section (kSectionFlat uses thickness / bevel; round and custom are sized by width, the
    // custom profile keeps its aspect). profile: unit-width loop from outlineProfile.
    int section = kSectionFlat;
    std::vector<ProfilePoint> profile;
    int profileDetail = 24;                          // points around a round section
    // This ribbon is the part uMapLo..uMapHi of a longer one (Individually trimmed masks): taper,
    // texture, gradient and rounded ends follow the whole ribbon of length lengthOverride (world px).
    double uMapLo = 0, uMapHi = 1, lengthOverride = 0;
    // Rounded ends: corner radius as a fraction of the half width (0 = straight cut, 1 = semicircle).
    double endRounding = 0;
    bool roundStart = true, roundEnd = true;
    WindSettings wind;                               // its twist part (applyWind bends the path)
};

// One continuous part of the ribbon: ribbon parameters u0..u1, path position s = a + (b - a) u - lap.
struct RibbonPiece {
    double u0, u1, lap;
};
// Pieces of the ribbon a..b (path fractions) on a closed or open path, see RibbonSettings::cycle.
std::vector<RibbonPiece> ribbonPieces(double a, double b, bool closed, bool cycle);


std::vector<ProfilePoint> ribbonProfile(double width, double thickness, double bevel, int bevelSegments, bool chamfer,
                                        int faceSegments);

struct RibbonMesh {
    std::vector<RK_Vertex> verts;
    std::vector<int> tris;  // 3 indices per triangle
    // Center line and frames per cross section (handy for tests and tools).
    std::vector<V3> center, tangent, widthDir;
    std::vector<double> widths;
    bool solid = false;  // closed mesh with thickness
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
    // Extra twist (radians) at s that turns the rotation minimizing frame onto the roll controls.
    double rollAt(double s) const;

private:
    std::vector<V3> X_, T_, R_;
    std::vector<double> S_;
    std::vector<double> rollS_, rollA_;  // control positions (fractions) and unwrapped angles
    double len_ = 0;
    bool closed_ = false;
};

RibbonMesh buildRibbon(const Polyline& path, const RibbonSettings& rs);
// Adds src's triangles to dst (several ribbons in one mesh, e.g. one per mask).
void appendMesh(RibbonMesh& dst, const RibbonMesh& src);

// Individually trimmed paths: the paths (lengths `lengths`, in order) are one long path; the ribbon
// a..b (fractions of the total, offset included) with Cycle wrapping around the whole sequence.
// One entry per part of the ribbon on one path: its path, its own start / end fractions on that
// path, and the part of the whole ribbon it is (uLo..uHi).
struct PathPart {
    int path;
    double start, end;
    double uLo, uHi;
};
std::vector<PathPart> individualParts(const std::vector<double>& lengths, double a, double b, bool cycle);

}  // namespace rb
