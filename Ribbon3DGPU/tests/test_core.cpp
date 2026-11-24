// Ribbon3D GPU core tests: geometry, projection/clipping, rasterization, shading, compositing.
// Usage: ribbon_tests [preview_dir]
#include <chrono>
#include <cstdio>
#include <string>

#include "TestScene.h"
#include "../src/kernel/RibbonKernel.h"  // environment and glass helpers under test

using namespace rb;
using test::Image;

static double distToPolyline(const V3& p, const std::vector<V3>& pts) {
    double best = 1e300;
    for (size_t i = 1; i < pts.size(); i++) {
        V3 ab = pts[i] - pts[i - 1];
        double l2 = dot(ab, ab), t = l2 > 0 ? std::clamp(dot(p - pts[i - 1], ab) / l2, 0.0, 1.0) : 0;
        best = std::min(best, length(p - (pts[i - 1] + ab * t)));
    }
    return best;
}
static double polyLength(const std::vector<V3>& p) {
    double L = 0;
    for (size_t i = 1; i < p.size(); i++) L += length(p[i] - p[i - 1]);
    return L;
}

static void testGeometry() {
    std::printf("geometry\n");
    Polyline dense = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 400);
    Polyline path = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 48);
    RibbonSettings rs;
    rs.segments = 200;
    rs.twistTurns = 1;
    rs.twistAlongPath = false;
    rs.twistAngleDeg = 30;
    rs.width = 80;
    rs.widthStart = 0;
    RibbonMesh m = buildRibbon(path, rs);
    CHECK(m.verts.size() == 201 * 2, "verts %zu", m.verts.size());
    CHECK(m.tris.size() == 200 * 2 * 3, "tris %zu", m.tris.size());

    double worst = 0;
    for (const V3& c : m.center) worst = std::max(worst, distToPolyline(c, dense.pts));
    CHECK(worst < 0.05, "center line off the path by %.4f px", worst);

    // Width, taper and twist: angle between the width direction and the plane normal.
    for (int k = 1; k <= 200; k += 13) {
        double u = k / 200.0, w = length(V3(m.verts[k * 2 + 1].pos[0], m.verts[k * 2 + 1].pos[1], m.verts[k * 2 + 1].pos[2]) -
                                         V3(m.verts[k * 2].pos[0], m.verts[k * 2].pos[1], m.verts[k * 2].pos[2]));
        CHECK(std::fabs(w - 80 * u) < 1e-3, "width at %d: %.4f", k, w);
        double sinT = dot(m.widthDir[size_t(k)], V3(0, 0, 1));
        double want = std::sin((30 + 360 * u) * 3.14159265358979 / 180);
        CHECK(std::fabs(sinT - want) < 1e-3, "twist at %d: %.4f vs %.4f", k, sinT, want);
    }
    // Normals: unit, perpendicular to the ribbon; with no twist the front faces the camera (-Z).
    RibbonSettings flat = rs;
    flat.twistTurns = 0; flat.twistAngleDeg = 0; flat.widthStart = 1;
    RibbonMesh f = buildRibbon(path, flat);
    for (const RK_Vertex& v : f.verts) CHECK(v.nrm[2] < -0.999f, "front normal %.3f", v.nrm[2]);

    // Trim by arc length.
    RibbonSettings trim = flat;
    trim.start = 0.2; trim.end = 0.6;
    RibbonMesh t = buildRibbon(path, trim);
    double frac = polyLength(t.center) / polyLength(dense.pts);
    CHECK(std::fabs(frac - 0.4) < 0.002, "trim fraction %.5f", frac);

    // Empty when start == end.
    trim.start = trim.end = 0.5;
    CHECK(buildRibbon(path, trim).tris.empty(), "empty ribbon expected");
}

static void testFrame3D() {
    std::printf("rotation minimizing frame\n");
    Polyline h = test::helix(300, 1400, 2.5, V3(960, 540, 300));
    RibbonSettings rs;
    rs.twistTurns = 0;
    rs.segments = 400;
    RibbonMesh m = buildRibbon(h, rs);
    double minDot = 1;
    for (size_t k = 1; k < m.widthDir.size(); k++) minDot = std::min(minDot, dot(m.widthDir[k], m.widthDir[k - 1]));
    CHECK(minDot > 0.98, "width direction jumps between sections (min dot %.4f)", minDot);
    for (size_t k = 0; k < m.widthDir.size(); k++) {
        CHECK(std::fabs(dot(m.widthDir[k], m.tangent[k])) < 1e-6, "W not perpendicular to T at %zu", k);
    }
}

static void testWaveAndTransform() {
    std::printf("wave and path transform\n");
    Polyline path = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 48);
    RibbonSettings rs;
    rs.twistTurns = 0;
    rs.waveAmp = 50;
    rs.waveLength = 300;
    RibbonMesh m = buildRibbon(path, rs);
    double maxZ = 0;
    for (const V3& c : m.center) maxZ = std::max(maxZ, std::fabs(c.z));
    CHECK(maxZ > 48 && maxZ <= 50.0001, "wave depth %.3f", maxZ);

    PathTransform xf;
    xf.rotY = 90;
    xf.pivot = V3(960, 540, 0);
    Polyline r = transformPolyline(path, xf);
    CHECK(std::fabs(r.up.x - 1) < 1e-9, "up rotated to +X (%.3f %.3f %.3f)", r.up.x, r.up.y, r.up.z);
    CHECK(std::fabs(r.pts[0].x - 960) < 1e-9, "points rotated into the YZ plane");
}

static rb::Packet flatScene(int W, int H, int samples, V3 color, double widthPx) {
    Polyline path;
    path.pts = {V3(400, 540, 0), V3(1520, 540, 0)};
    RibbonSettings rs;
    rs.twistTurns = 0;
    rs.width = widthPx;
    rs.segments = 7;
    Material mat = test::flatMaterial(color, V3(0, 1, 0));
    mat.samples = samples;
    Packet pk;
    buildPacket(buildRibbon(path, rs), defaultCamera(W, H), {}, mat, W, H, pk);
    return pk;
}

static void testRaster() {
    std::printf("rasterization\n");
    const int W = 1920, H = 1080;
    Image src(W, H);
    // Coverage: a 1120 x 101.5 px band at z = 0 seen by the default camera is 1:1 with the comp.
    Packet pk = flatScene(W, H, 4, V3(1, 0, 0), 101.5);
    Image out = test::render(pk, src);
    double area = 0, red = 0;
    for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) { area += out.px(x, y)[3]; red += out.px(x, y)[2]; }
    CHECK(std::fabs(area - 1120 * 101.5) < 1120 * 101.5 * 0.002, "covered area %.1f", area);
    CHECK(std::fabs(red - area) < 1e-3 * area, "flat front color everywhere");
    CHECK(out.px(960, 540)[3] == 1.0f && out.px(960, 600)[3] == 0.0f, "inside / outside");
    float edge = out.px(960, 540 + 50)[3];  // the edge at 590.75 cuts this row
    CHECK(edge > 0.6f && edge < 0.9f, "anti-aliased edge coverage %.3f", edge);

    // Downsampled buffer (half resolution) covers a quarter of the area.
    Camera half = defaultCamera(W, H);
    half.scaleX = half.scaleY = 0.5;
    Packet pk2;
    Polyline path;
    path.pts = {V3(400, 540, 0), V3(1520, 540, 0)};
    RibbonSettings rs;
    rs.twistTurns = 0; rs.width = 101.5; rs.segments = 7;
    buildPacket(buildRibbon(path, rs), half, {}, test::flatMaterial(V3(1, 0, 0), V3(0, 1, 0)), W / 2, H / 2, pk2);
    Image src2(W / 2, H / 2);
    Image out2 = test::render(pk2, src2);
    double area2 = 0;
    for (float* p = out2.bgra.data(); p < out2.bgra.data() + out2.bgra.size(); p += 4) area2 += p[3];
    CHECK(std::fabs(area2 * 4 - area) < area * 0.003, "half-res area %.1f vs %.1f", area2 * 4, area);
}

static void testDepthAndFacing() {
    std::printf("depth test and two-sided colors\n");
    const int W = 640, H = 360;
    Camera cam = defaultCamera(W, H);
    RibbonSettings rs;
    rs.twistTurns = 0; rs.width = 60; rs.segments = 4;
    // Horizontal ribbon at z = 0 (red), vertical ribbon in front at z = -200 (blue) via a translation.
    Polyline a, b;
    a.pts = {V3(100, 180, 0), V3(540, 180, 0)};
    b.pts = {V3(320, 40, -200), V3(320, 320, -200)};
    RibbonMesh ma = buildRibbon(a, rs), mb = buildRibbon(b, rs);
    int off = int(ma.verts.size());
    for (const RK_Vertex& v : mb.verts) ma.verts.push_back(v);
    for (int i : mb.tris) ma.tris.push_back(i + off);
    Material mat = test::flatMaterial(V3(1, 0, 0), V3(0, 1, 0));
    Packet pk;
    buildPacket(ma, cam, {}, mat, W, H, pk);
    // Front color is per triangle, so color the second ribbon through its normals: flip it to show the back (green).
    for (RK_Tri& t : pk.tris)
        if (t.v[0] >= off) t.front = 0;
    Image out = test::render(pk, Image(W, H));
    const float* c = out.px(320, 180);
    CHECK(c[1] > 0.99f && c[2] < 0.01f, "nearer ribbon wins at the crossing (g=%.3f r=%.3f)", c[1], c[2]);
    const float* r = out.px(150, 180);
    CHECK(r[2] > 0.99f, "far ribbon visible elsewhere");

    // Facing: a ribbon twisted by 180 degrees shows its back side.
    RibbonSettings tw = rs;
    tw.twistAngleDeg = 180;
    Packet pk2;
    buildPacket(buildRibbon(a, tw), cam, {}, mat, W, H, pk2);
    const float* g = test::render(pk2, Image(W, H)).px(320, 180);
    CHECK(g[1] > 0.99f && g[2] < 0.01f, "back side color when flipped");
}

static void testClipping() {
    std::printf("near-plane clipping\n");
    const int W = 960, H = 540;
    Camera cam = defaultCamera(W, H);  // camera at z = -666.7
    // A helix running through the camera position.
    Polyline h = test::helix(150, 4000, 3, V3(W / 2.0, H / 2.0, -666.7));
    // Rotate so the helix axis is along Z (through the camera).
    PathTransform xf;
    xf.rotY = 90;
    xf.pivot = V3(W / 2.0, H / 2.0, -666.7);
    RibbonSettings rs;
    rs.segments = 600;
    Packet pk;
    Material mat;
    mat.samples = 2;
    buildPacket(buildRibbon(transformPolyline(h, xf), rs), cam, test::studioLights(), mat, W, H, pk);
    bool finite = true;
    for (const RK_Tri& t : pk.tris)
        for (int k = 0; k < 3; k++) finite = finite && std::isfinite(t.px[k]) && std::isfinite(t.py[k]) && t.iw[k] > 0;
    CHECK(finite, "projected vertices finite and in front of the camera");
    CHECK(pk.verts.size() > buildRibbon(transformPolyline(h, xf), rs).verts.size(), "clipping created new vertices");
    Image out = test::render(pk, Image(W, H));
    double cov = 0;
    bool ok = true;
    for (float v : out.bgra) { ok = ok && std::isfinite(v); }
    for (int i = 3; i < int(out.bgra.size()); i += 4) cov += out.bgra[size_t(i)];
    CHECK(ok && cov > 1000, "renders a finite image (coverage %.0f)", cov);
}

static void testComposite() {
    std::printf("compositing\n");
    const int W = 1920, H = 1080;
    Image src(W, H, 0.25f, 0.25f, 0.25f, 0.5f);  // half transparent grey (premultiplied)
    Packet over = flatScene(W, H, 2, V3(1, 0, 0), 100);
    Packet behind = over, alone = over;
    behind.params.composite = RK_COMPOSITE_BEHIND;
    alone.params.composite = RK_COMPOSITE_ALONE;
    over.params.opacity = 0.5f;
    const float* o = test::render(over, src).px(960, 540);
    CHECK(std::fabs(o[2] - (0.5f + 0.25f * 0.5f)) < 1e-5 && std::fabs(o[3] - 0.75f) < 1e-5, "over: r=%.4f a=%.4f", o[2], o[3]);
    const float* b = test::render(behind, src).px(960, 540);
    CHECK(std::fabs(b[2] - (0.25f + 0.5f)) < 1e-5 && std::fabs(b[3] - 1.0f) < 1e-5, "behind: r=%.4f a=%.4f", b[2], b[3]);
    const float* a = test::render(alone, src).px(960, 540);
    CHECK(a[2] == 1.0f && a[1] == 0.0f && a[3] == 1.0f, "alone");
    const float* bg = test::render(alone, src).px(10, 10);
    CHECK(bg[3] == 0.0f, "alone: transparent outside the ribbon");
}

static void testTexture() {
    std::printf("texture mapping\n");
    const int W = 1920, H = 1080, TW = 8, TH = 2;
    // 8x2 texture: column i has red = i/7, top row green = 0, bottom row green = 1.
    std::vector<float> tex(size_t(TW) * TH * 4);
    for (int y = 0; y < TH; y++)
        for (int x = 0; x < TW; x++) {
            float* p = &tex[(size_t(y) * TW + x) * 4];
            p[0] = 0; p[1] = float(y); p[2] = x / 7.0f; p[3] = 1;
        }
    Packet pk = flatScene(W, H, 1, V3(1, 1, 1), 100);
    setTexture(pk, TW, TH, TW);
    Image out = test::render(pk, Image(W, H), tex.data());
    // Ribbon spans x 400..1520 (u 0..1), v 0 at y=590 (the "-W" edge) ... check centers of texels.
    for (int i = 0; i < TW; i++) {
        int x = int(400 + (i + 0.5) / TW * 1120);
        const float* p = out.px(x, 565);  // v = 0.25 -> texel row 0
        CHECK(std::fabs(p[2] - i / 7.0f) < 0.02f, "texel %d red %.3f", i, p[2]);
    }
    float g0 = out.px(960, 565)[1], g1 = out.px(960, 515)[1];
    CHECK((g0 < 0.05f && g1 > 0.95f) || (g0 > 0.95f && g1 < 0.05f), "v runs across the ribbon (%.2f %.2f)", g0, g1);
}

static void testSolidGeometry() {
    std::printf("thickness and bevel geometry\n");
    // Round bevel: bounds, radius, unit outward normals.
    std::vector<ProfilePoint> pr = ribbonProfile(60, 10, 3, 5, false, 2);
    double mx = 0, my = 0;
    for (const ProfilePoint& q : pr) {
        mx = std::max(mx, std::fabs(q.x));
        my = std::max(my, std::fabs(q.y));
        CHECK(std::fabs(std::hypot(q.nx, q.ny) - 1) < 1e-12, "unit normal");
        CHECK(q.x * q.nx + q.y * q.ny > 0, "outward normal at (%.2f %.2f)", q.x, q.y);
        if (std::fabs(q.x) > 27 + 1e-9 && std::fabs(q.y) > 2 + 1e-9) {  // on a corner arc
            double d = std::hypot(std::fabs(q.x) - 27, std::fabs(q.y) - 2);
            CHECK(std::fabs(d - 3) < 1e-9, "bevel radius %.4f", d);
        }
    }
    CHECK(std::fabs(mx - 30) < 1e-9 && std::fabs(my - 5) < 1e-9, "profile bounds %.3f x %.3f", mx, my);
    CHECK(pr.size() == ribbonProfile(1, 1, 0, 5, false, 2).size(), "point count independent of size");
    // Chamfer: the corner facet is a 45 degree cut.
    for (const ProfilePoint& q : ribbonProfile(60, 10, 3, 5, true, 1))
        if (std::fabs(q.nx) > 1e-9 && std::fabs(q.ny) > 1e-9)
            CHECK(std::fabs(std::fabs(q.nx) - std::sqrt(0.5)) < 1e-9, "chamfer normal %.3f %.3f", q.nx, q.ny);
    // A bevel larger than half the thickness is clamped: fully rounded edge, edge points only at y = 0.
    for (const ProfilePoint& q : ribbonProfile(60, 10, 100, 8, false, 1))
        if (std::fabs(std::fabs(q.x) - 30) < 1e-9) CHECK(std::fabs(q.y) < 1e-9, "fully round edge");

    // Swept mesh: outward normals around the center line, caps along the tangent.
    Polyline path = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 48);
    RibbonSettings rs;
    rs.segments = 120; rs.width = 80; rs.thickness = 16; rs.bevel = 4; rs.twistTurns = 1.5; rs.widthEnd = 0.3;
    RibbonMesh m = buildRibbon(path, rs);
    CHECK(m.solid, "solid mesh");
    const int R = int(ribbonProfile(1, 1, 0, rs.bevelSegments, false, 1).size());
    int inward = 0;
    for (int k = 0; k <= rs.segments; k++)
        for (int i = 0; i < R; i++) {
            const RK_Vertex& v = m.verts[size_t(k * R + i)];
            V3 d = V3(v.pos[0], v.pos[1], v.pos[2]) - m.center[size_t(k)];
            if (dot(d, V3(v.nrm[0], v.nrm[1], v.nrm[2])) < -1e-3) inward++;
            double half = std::max(m.widths[size_t(k)], 16.0) / 2;
            CHECK(length(d) <= std::hypot(half, 8.0) + 1e-3, "vertex inside the section bounds");
        }
    CHECK(inward == 0, "%d inward normals", inward);
    CHECK(m.verts.size() == size_t((rs.segments + 1) * R + 2 * (R + 1)), "caps present (%zu verts)", m.verts.size());
    const RK_Vertex& cap = m.verts[size_t((rs.segments + 1) * R)];
    CHECK(dot(V3(cap.nrm[0], cap.nrm[1], cap.nrm[2]), m.tangent[0]) < -0.999, "start cap faces backwards");
}

static void testSolidRaster() {
    std::printf("thickness and bevel rendering\n");
    const int W = 1920, H = 1080;
    Polyline path;
    path.pts = {V3(400, 540, 0), V3(1520, 540, 0)};
    Material mat = test::flatMaterial(V3(1, 0, 0), V3(0, 1, 0));
    mat.edge = V3(0, 0, 1);
    auto render = [&](double thickness, double twistDeg) {
        RibbonSettings rs;
        rs.twistTurns = 0; rs.twistAngleDeg = twistDeg; rs.width = 100; rs.segments = 8;
        rs.thickness = thickness; rs.bevel = 2;
        Packet pk;
        buildPacket(buildRibbon(path, rs), defaultCamera(W, H), {}, mat, W, H, pk);
        return test::render(pk, Image(W, H));
    };
    auto area = [](const Image& im) { double a = 0; for (size_t i = 3; i < im.bgra.size(); i += 4) a += im.bgra[i]; return a; };
    // Edge-on: a sheet almost vanishes, a 20 px thick ribbon shows a 20 px band in the edge color.
    double sheet = area(render(0, 90)), thick = area(render(20, 90));
    Image edgeOn = render(20, 90);
    CHECK(sheet < 2000, "edge-on sheet area %.0f", sheet);
    CHECK(thick > 1120 * 20 * 0.95 && thick < 1120 * 20 * 1.15, "edge-on thick area %.0f", thick);
    const float* e = edgeOn.px(960, 540);
    CHECK(e[0] > 0.99f && e[2] < 0.01f, "edge color (b=%.3f r=%.3f)", e[0], e[2]);
    // Face-on: front and back faces keep their colors.
    Image faceOn = render(20, 0), backOn = render(20, 180);
    const float* f = faceOn.px(960, 540);
    CHECK(f[2] > 0.99f && f[0] < 0.01f, "front face color");
    const float* bk = backOn.px(960, 540);
    CHECK(bk[1] > 0.99f && bk[2] < 0.01f, "back face color");
}

// Normal map: a flat map or strength 0 change nothing; the tilt follows the tangent frame.
static void testNormalMap() {
    std::printf("normal map\n");
    const int NW = 64, NH = 32;
    auto makeMap = [&](float r, float g, float b) {
        std::vector<float> m(size_t(NW) * NH * 4);
        for (size_t i = 0; i < m.size(); i += 4) { m[i] = b; m[i + 1] = g; m[i + 2] = r; m[i + 3] = 1; }
        return m;
    };
    // Kernel helper: front normal -Z, tangent +X; width direction = T x N = +Y (down the image = +v).
    RK_Params P{};
    P.nrmWidth = NW; P.nrmHeight = NH; P.nrmPitch = NW; P.nrmStrength = 1; P.nrmRepeatU = 1; P.nrmRepeatV = 1;
    std::vector<float> tiltX = makeMap(1.0f, 0.5f, 0.5f), tiltUp = makeMap(0.5f, 1.0f, 0.5f);
    rk3 N = rk_v(0, 0, -1), T = rk_v(1, 0, 0);
    rk3 a = rk_normal_map(&P, tiltX.data(), N, T, 0.3f, 0.5f);
    // (1, 0.5, 0.5) is a normal lying along +x (z = 0, kept just above the surface).
    CHECK(a.x > 0.99f && a.z < 0 && std::fabs(a.y) < 1e-6f, "red tilts the normal along the ribbon (%.2f %.2f %.2f)", a.x, a.y, a.z);
    std::vector<float> half = makeMap(0.75f, 0.5f, 0.5f + 0.5f * 0.8660254f);  // 30 degrees toward +x
    rk3 h = rk_normal_map(&P, half.data(), N, T, 0.3f, 0.5f);
    CHECK(std::fabs(h.x - 0.5f) < 0.01f && std::fabs(h.z + 0.866f) < 0.01f, "30 degree tilt (%.3f %.3f)", h.x, h.z);
    rk3 gl = rk_normal_map(&P, tiltUp.data(), N, T, 0.3f, 0.5f);
    P.nrmDirectX = 1;
    rk3 dx = rk_normal_map(&P, tiltUp.data(), N, T, 0.3f, 0.5f);
    CHECK(gl.y < -0.85f && dx.y > 0.85f, "green: OpenGL tilts toward -v (image up), DirectX toward +v (%.2f / %.2f)", gl.y, dx.y);
    P.nrmDirectX = 0;

    // Rendering: flat map and strength 0 give the image without a map; a tilted map changes it.
    Polyline path = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 48);
    RibbonSettings rs;
    rs.segments = 200; rs.width = 120; rs.twistTurns = 0.5;
    const int W = 480, H = 270;
    for (auto& q : path.pts) { q.x /= 4; q.y /= 4; }
    rs.width = 30;
    Material mat;
    mat.samples = 2;
    auto renderWith = [&](const std::vector<float>* map, double strength) {
        Material m = mat;
        m.normalStrength = strength;
        Packet pk;
        buildPacket(buildRibbon(path, rs), defaultCamera(W, H), test::studioLights(), m, W, H, pk, 400, rs.width);
        if (map) setNormalMap(pk, NW, NH, NW);
        return test::render(pk, test::Image(W, H), nullptr, map ? map->data() : nullptr);
    };
    test::Image plain = renderWith(nullptr, 1);
    std::vector<float> flatMap = makeMap(0.5f, 0.5f, 1.0f);
    test::Image flat = renderWith(&flatMap, 1), zero = renderWith(&tiltX, 0), tilted = renderWith(&tiltX, 1);
    auto maxDiff = [](const test::Image& x, const test::Image& y) {
        double d = 0;
        for (size_t i = 0; i < x.bgra.size(); i++) d = std::max(d, double(std::fabs(x.bgra[i] - y.bgra[i])));
        return d;
    };
    CHECK(maxDiff(plain, flat) < 1e-4 && maxDiff(plain, zero) < 1e-4, "flat map / strength 0: unchanged (%.1e, %.1e)", maxDiff(plain, flat),
          maxDiff(plain, zero));
    CHECK(maxDiff(plain, tilted) > 0.05, "tilted map changes the shading (%.3f)", maxDiff(plain, tilted));
}

// Wind: pinned end fixed, lengths kept, free end downwind, flutter moves with time.
static void testWind() {
    std::printf("wind\n");
    Polyline line;  // hangs straight down from (500, 100), 600 px
    for (int i = 0; i <= 200; i++) line.pts.push_back(V3(500, 100 + 3.0 * i, 0));
    WindSettings w;
    w.strength = 1;
    w.direction = V3(1, 0, 0);
    Polyline a = applyWind(line, w, 0.4), b = applyWind(line, w, 0.9);
    double maxLenErr = 0;
    for (size_t i = 1; i < a.pts.size(); i++) maxLenErr = std::max(maxLenErr, std::fabs(length(a.pts[i] - a.pts[i - 1]) - 3.0));
    CHECK(maxLenErr < 1e-9, "segment lengths kept (%.2e)", maxLenErr);
    CHECK(length(a.pts[0] - line.pts[0]) < 1e-12, "pinned start stays");
    CHECK(a.pts.back().x > 500 + 150, "free end blown downwind (x %.1f)", a.pts.back().x);
    CHECK(length(a.pts.back() - b.pts.back()) > 5, "flutter moves over time (%.1f px)", length(a.pts.back() - b.pts.back()));
    Polyline a2 = applyWind(line, w, 0.4);
    CHECK(length(a2.pts.back() - a.pts.back()) < 1e-12, "same time, same shape");
    double nearPin = length(a.pts[10] - line.pts[10]), farEnd = length(a.pts.back() - line.pts.back());
    CHECK(nearPin < farEnd * 0.05, "the part near the pin barely moves (%.2f vs %.1f px)", nearPin, farEnd);
    w.pinStart = false;
    Polyline e = applyWind(line, w, 0.4);
    CHECK(length(e.pts.back() - line.pts.back()) < 1e-12 && length(e.pts[0] - line.pts[0]) > 100, "pinned end can be the end");
    w.pinStart = true;
    w.strength = 0;
    Polyline z = applyWind(line, w, 0.4);
    bool same = true;
    for (size_t i = 0; i < z.pts.size(); i++) same = same && length(z.pts[i] - line.pts[i]) == 0;
    CHECK(same, "strength 0: no change");
    w.strength = 1;
    CHECK(windTwist(w, 0, 600, 0.3) == 0 && std::fabs(windTwist(w, 1, 600, 0.3)) <= 40 * 3.14159265 / 180 * 1.3 + 1e-9, "twist: none at the pin, bounded at the end");
    double gmin = 1e9, gmax = -1e9;
    for (int i = 0; i < 400; i++) { double g = windGust(w, i * 0.05); gmin = std::min(gmin, g); gmax = std::max(gmax, g); }
    CHECK(gmin >= 0.7 - 1e-9 && gmax <= 1.3 + 1e-9 && gmax - gmin > 0.15, "gusts vary within +-30 %% (%.2f..%.2f)", gmin, gmax);
    // The mesh twists with the wind: width directions differ between two times at the free end.
    RibbonSettings rs;
    rs.segments = 100; rs.twistTurns = 0; rs.width = 40; rs.wind = w;
    rs.time = 0.2;
    RibbonMesh m1 = buildRibbon(applyWind(line, w, rs.time), rs);
    rs.time = 0.5;
    RibbonMesh m2 = buildRibbon(applyWind(line, w, rs.time), rs);
    CHECK(std::fabs(dot(m1.widthDir.front(), m2.widthDir.front())) > 0.999 && std::fabs(dot(m1.widthDir.back(), m2.widthDir.back())) < 0.99,
          "pinned end keeps its orientation, the free end turns");
}

// Environment: atan2 approximation, harmonics for a uniform and a sky environment, orientation of
// the map, mip levels; glass over a background.
static void testEnvironment() {
    std::printf("environment\n");
    double worst = 0;
    for (int i = 0; i < 20000; i++) {
        double a = -3.14159 + 6.28318 * i / 20000, r = 0.1 + (i % 7);
        float y = float(r * std::sin(a)), x = float(r * std::cos(a));
        worst = std::max(worst, std::fabs(double(rk_atan2(y, x)) - std::atan2(double(y), double(x))));
    }
    CHECK(worst < 2e-5, "atan2 approximation error %.2e", worst);

    const int EW = 512, EH = 256;
    auto makeEnv = [&](auto color) {
        std::vector<float> img(size_t(EW) * EH * 4);
        for (int y = 0; y < EH; y++)
            for (int x = 0; x < EW; x++) {
                V3 c = color(x, y);
                float* p = &img[(size_t(y) * EW + x) * 4];
                p[0] = float(c.z); p[1] = float(c.y); p[2] = float(c.x); p[3] = 1;  // BGRA
            }
        return img;
    };
    Packet pk;
    Material mat;
    mat.gamma = 1;
    buildPacket(RibbonMesh(), defaultCamera(640, 360), {}, mat, 64, 64, pk);

    // Uniform 0.5: diffuse light 0.5 from every side, reflections 0.5 at every blur level.
    std::vector<float> uni = makeEnv([](int, int) { return V3(0.5, 0.5, 0.5); });
    setEnvironmentImage(pk, uni.data(), EW, EH, EW);
    const RK_Params* P = &pk.params;
    CHECK(P->hasEnv && P->envWidth == 512 && P->envHeight == 256 && P->envLevels == 7, "mip chain %d x %d, %d levels",
          P->envWidth, P->envHeight, P->envLevels);
    bool flat = true;
    const rk3 dirs[6] = {rk_v(1, 0, 0), rk_v(-1, 0, 0), rk_v(0, 1, 0), rk_v(0, -1, 0), rk_v(0, 0, 1), rk_v(0.6f, -0.48f, 0.64f)};
    for (const rk3& d : dirs) {
        rk3 e = rk_env_irr(P, d);
        flat = flat && std::fabs(e.x - 0.5f) < 0.005f;
        for (float lod = 0; lod <= 6; lod += 1.5f) flat = flat && std::fabs(rk_env(P, pk.env.data(), d, lod).y - 0.5f) < 1e-4f;
    }
    CHECK(flat, "uniform environment: irradiance / pi and reflections 0.5 everywhere");

    // Sky: upper half 1, lower half 0. Facing up gets ~1, down ~0, sideways ~0.5.
    std::vector<float> sky = makeEnv([&](int, int y) { return y < EH / 2 ? V3(1, 1, 1) : V3(0, 0, 0); });
    setEnvironmentImage(pk, sky.data(), EW, EH, EW);
    float up = rk_env_irr(P, rk_v(0, -1, 0)).x, down = rk_env_irr(P, rk_v(0, 1, 0)).x, side = rk_env_irr(P, rk_v(1, 0, 0)).x;
    CHECK(std::fabs(up - 1) < 0.1f && down < 0.1f && std::fabs(side - 0.5f) < 0.03f, "sky irradiance up %.3f down %.3f side %.3f", up, down, side);
    CHECK(rk_env(P, pk.env.data(), rk_v(0, -1, 0), 0).x > 0.99f && rk_env(P, pk.env.data(), rk_v(0, 1, 0), 0).x < 0.01f,
          "top of the map is up (-Y)");

    // A bright spot in the image center lies straight ahead (+Z); Env Rotation turns it.
    std::vector<float> spot = makeEnv([&](int x, int y) {
        return (std::abs(x - EW / 2) < 8 && std::abs(y - EH / 2) < 8) ? V3(10, 10, 10) : V3(0, 0, 0);
    });
    setEnvironmentImage(pk, spot.data(), EW, EH, EW);
    CHECK(rk_env(P, pk.env.data(), rk_v(0, 0, 1), 0).x > 5 && rk_env(P, pk.env.data(), rk_v(0, 0, -1), 0).x < 0.01f,
          "image center = +Z (%.2f)", rk_env(P, pk.env.data(), rk_v(0, 0, 1), 0).x);
    float sharp = rk_env(P, pk.env.data(), rk_v(0.25f, 0, 0.968f), 0).x, blurred = rk_env(P, pk.env.data(), rk_v(0.25f, 0, 0.968f), 4).x;
    CHECK(sharp < 0.01f && blurred > 0.01f, "higher levels blur the spot (%.3f -> %.3f)", sharp, blurred);
    mat.envRotationDeg = 90;
    buildPacket(RibbonMesh(), defaultCamera(640, 360), {}, mat, 64, 64, pk);
    setEnvironmentImage(pk, spot.data(), EW, EH, EW);
    float px = rk_env(P, pk.env.data(), rk_v(1, 0, 0), 0).x, nx = rk_env(P, pk.env.data(), rk_v(-1, 0, 0), 0).x;
    CHECK((px > 5) != (nx > 5) && rk_env(P, pk.env.data(), rk_v(0, 0, 1), 0).x < 0.01f, "rotation 90 moves the spot to the side (%.2f / %.2f)", px, nx);

    // Glass over a transparent background: reflection alpha + the body that does not transmit.
    rk_hit h;
    h.r = h.g = h.b = 0.1f; h.a = 1; h.sr = h.sg = h.sb = 0.2f; h.sa = 0.1f; h.fres = 0.1f;
    h.tr = h.tg = h.tb = 0.8f; h.topaque = 0.2f; h.dx = h.dy = 0;
    rk4 none; none.r = none.g = none.b = none.a = 0;
    rk4 o = rk_glass_over(h, none);
    CHECK(std::fabs(o.a - (0.1f + 0.9f * 0.2f)) < 1e-6f && std::fabs(o.r - (0.2f + 0.9f * 0.1f)) < 1e-6f, "glass over nothing");
    rk4 white; white.r = white.g = white.b = white.a = 1;
    o = rk_glass_over(h, white);
    CHECK(std::fabs(o.a - 1) < 1e-6f && std::fabs(o.r - (0.2f + 0.9f * (0.8f + 0.1f))) < 1e-6f, "glass over an opaque background");
}

// Orientation = Front: the drawing is a view from above, the ribbon lies flat on the floor facing down.
static void testFront() {
    std::printf("front\n");
    Polyline drawn = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 48);
    double minx = 1e300, maxx = -1e300, miny = 1e300, maxy = -1e300;
    for (const V3& q : drawn.pts) { minx = std::min(minx, q.x); maxx = std::max(maxx, q.x); miny = std::min(miny, q.y); maxy = std::max(maxy, q.y); }
    const V3 pivot((minx + maxx) / 2, (miny + maxy) / 2, 0);
    Polyline floor = topViewToFloor(drawn, pivot);
    bool mapped = true;
    for (size_t i = 0; i < drawn.pts.size(); i++) {
        const V3 &a = drawn.pts[i], &b = floor.pts[i];
        mapped = mapped && std::fabs(b.x - a.x) < 1e-9 && std::fabs(b.y - pivot.y) < 1e-9 && std::fabs(b.z - (pivot.y - a.y)) < 1e-9;
    }
    CHECK(mapped, "x kept, height = path center, depth = center - y (top of the drawing is far)");
    CHECK(std::fabs(FramedCurve(floor).length() - FramedCurve(drawn).length()) < 1e-9, "length preserved");

    RibbonSettings rs;
    rs.segments = 120;
    rs.twistTurns = 0;
    rs.width = 40;
    RibbonMesh f = buildRibbon(floor, rs), d = buildRibbon(drawn, rs);
    bool down = true, flat = true;
    for (const RK_Vertex& v : f.verts) {
        down = down && v.nrm[1] > 0.999f;
        flat = flat && std::fabs(v.pos[1] - pivot.y) < 1e-3;
    }
    CHECK(down, "front faces down (+Y)");
    CHECK(flat, "ribbon lies in the horizontal plane");
    // Texture handedness seen from the front side: the same as in the default orientation.
    auto hand = [](const RibbonMesh& m, int k) {
        auto P = [&](int i) { return V3(m.verts[size_t(i)].pos[0], m.verts[size_t(i)].pos[1], m.verts[size_t(i)].pos[2]); };
        V3 du = P((k + 1) * 2) - P(k * 2), dv = P(k * 2 + 1) - P(k * 2);
        const RK_Vertex& v = m.verts[size_t(k * 2)];
        return dot(cross(du, dv), V3(v.nrm[0], v.nrm[1], v.nrm[2]));
    };
    CHECK(hand(f, 30) * hand(d, 30) > 0, "texture not mirrored on the front side");

    // Control nulls drawn in the comp: their rolls turn with the path (default nulls: flat, facing down).
    std::vector<ControlPoint> cps(3);
    cps[0].pos = V3(200, 500, 0); cps[1].pos = V3(600, 300, 0); cps[2].pos = V3(1000, 500, 0);
    for (ControlPoint& c : cps) { c.axisX = V3(1, 0, 0); c.axisY = V3(0, 1, 0); c.axisZ = V3(0, 0, 1); c.handleScale = 1; }
    Polyline np = topViewToFloor(controlPointsToPolyline(cps, false, 48), V3(600, 400, 0));
    RibbonMesh nm = buildRibbon(np, rs);
    bool nDown = true;
    for (const RK_Vertex& v : nm.verts) nDown = nDown && v.nrm[1] > 0.99f;
    CHECK(nDown, "control-null ribbon faces down too");
}

// Offset past the ends of the path: Cycle wraps the ribbon (also on open paths), else it is hidden.
static void testCycle() {
    std::printf("cycle\n");
    auto near = [](double x, double y) { return std::fabs(x - y) < 1e-9; };
    std::vector<RibbonPiece> P = ribbonPieces(0.8, 1.2, false, true);
    CHECK(P.size() == 2 && near(P[0].u0, 0) && near(P[0].u1, 0.5) && P[0].lap == 0 && near(P[1].u0, 0.5) &&
              near(P[1].u1, 1) && P[1].lap == 1,
          "open, cycle: two pieces around the end");
    P = ribbonPieces(0.8, 1.2, false, false);
    CHECK(P.size() == 1 && near(P[0].u1, 0.5) && P[0].lap == 0, "open, no cycle: the part past the end is hidden");
    P = ribbonPieces(0.8, 1.2, true, true);
    CHECK(P.size() == 1 && near(P[0].u0, 0) && near(P[0].u1, 1) && P[0].lap == 0, "closed, cycle: one piece across the seam");
    P = ribbonPieces(0.8, 1.2, true, false);
    CHECK(P.size() == 1 && near(P[0].u1, 0.5), "closed, no cycle: cut at the seam");
    P = ribbonPieces(-0.3, 0.2, false, true);
    CHECK(P.size() == 2 && near(P[0].u1, 0.6) && P[0].lap == -1 && P[1].lap == 0, "before the start wraps to the end");
    P = ribbonPieces(1.2, 0.8, false, true);
    CHECK(P.size() == 2 && near(P[0].u1, 0.5) && P[0].lap == 1 && P[1].lap == 0, "reversed ribbon");
    P = ribbonPieces(5.8, 6.2, false, true);
    CHECK(P.size() == 2 && P[0].lap == 5 && P[1].lap == 6, "large offsets");
    P = ribbonPieces(0.2, 0.6, false, true);
    CHECK(P.size() == 1 && near(P[0].u0, 0) && near(P[0].u1, 1) && P[0].lap == 0, "inside the path: unchanged");

    // Straight open path, 1000 px along X; a 400 px ribbon at offset 80 %: 800..1000 and 0..200.
    Polyline line;
    line.pts = {V3(0, 0, 0), V3(1000, 0, 0)};
    RibbonSettings rs;
    rs.segments = 100;
    rs.twistTurns = 0;
    rs.width = 20;
    rs.start = 0;
    rs.end = 0.4;
    rs.offset = 0.8;
    RibbonMesh m = buildRibbon(line, rs);
    CHECK(m.center.size() == 102 && m.verts.size() == 102 * 2 && m.tris.size() == 100 * 6, "two pieces of 50 segments (%zu)",
          m.center.size());
    bool inRange = true;
    double len = 0;
    for (size_t k = 0; k < m.center.size(); k++) {
        double x = m.center[k].x;
        inRange = inRange && ((x >= 800 - 1e-6 && x <= 1000 + 1e-6) || (x >= -1e-6 && x <= 200 + 1e-6));
        if (k != 0 && k != 51) len += length(m.center[k] - m.center[k - 1]);
    }
    CHECK(inRange, "ribbon only on 800..1000 and 0..200");
    CHECK(std::fabs(len - 400) < 1e-6, "visible length %.4f", len);
    CHECK(std::fabs(m.center[50].x - 1000) < 1e-6 && std::fabs(m.center[51].x) < 1e-6, "cut exactly at the path ends");
    // Texture u runs on over the cut: the texture travels with the ribbon.
    CHECK(std::fabs(m.verts[50 * 2].uv[0] - 0.5f) < 1e-6f && std::fabs(m.verts[51 * 2].uv[0] - 0.5f) < 1e-6f &&
              std::fabs(m.verts[101 * 2].uv[0] - 1.0f) < 1e-6f,
          "u continuous across the pieces");

    rs.cycle = false;
    RibbonMesh h = buildRibbon(line, rs);
    double minX = 1e300;
    for (const V3& c : h.center) minX = std::min(minX, c.x);
    CHECK(h.center.size() == 51 && minX > 800 - 1e-6, "no cycle: only 800..1000 (%zu sections, min x %.2f)", h.center.size(), minX);
    rs.offset = 1.2;
    CHECK(buildRibbon(line, rs).tris.empty(), "no cycle: fully past the end is empty");
    rs.cycle = true;
    RibbonMesh w = buildRibbon(line, rs);
    double maxX = -1e300;
    minX = 1e300;
    for (const V3& c : w.center) { minX = std::min(minX, c.x); maxX = std::max(maxX, c.x); }
    CHECK(std::fabs(minX - 200) < 1e-6 && std::fabs(maxX - 600) < 1e-6, "cycle: offset 120 %% = 20 %% (%.1f..%.1f)", minX, maxX);

    // Solid: every piece is closed with caps.
    rs.offset = 0.8;
    rs.thickness = 6;
    rs.bevel = 2;
    RibbonMesh s = buildRibbon(line, rs);
    const size_t R = ribbonProfile(1, 1, 0, rs.bevelSegments, rs.chamfer, rs.across).size();
    CHECK(s.verts.size() == 2 * (51 * R + 2 * (R + 1)), "solid pieces with caps (%zu verts)", s.verts.size());
}

static void testControlNulls() {
    std::printf("control nulls\n");
    auto cp = [](V3 pos, double zRotDeg, double xRotDeg, double scale) {
        M3 R = rotationXYZ(0, 0, zRotDeg) * rotationXYZ(xRotDeg, 0, 0);  // AE: Z rotation of a null, then its own X roll
        ControlPoint c;
        c.pos = pos;
        c.axisX = R * V3(1, 0, 0); c.axisY = R * V3(0, 1, 0); c.axisZ = R * V3(0, 0, 1);
        c.handleScale = scale;
        return c;
    };
    // Four nulls, the third rolled by 90 degrees around its X axis, the last one turned.
    std::vector<ControlPoint> cps = {cp(V3(300, 600, 0), -20, 0, 1), cp(V3(800, 400, 0), 0, 0, 1),
                                     cp(V3(1200, 500, 100), 15, 90, 1.5), cp(V3(1600, 700, 0), 40, 0, 0.5)};
    Polyline pl = controlPointsToPolyline(cps, false, 64);
    // The curve passes through every null with its tangent along the null's X axis.
    for (size_t i = 0; i < cps.size(); i++) {
        CHECK(length(pl.pts[i * 64] - cps[i].pos) < 1e-9, "passes through null %zu", i);
        size_t j = std::min(pl.pts.size() - 1, i * 64 + 1), k = j - 1;
        V3 t = normalize(pl.pts[j] - pl.pts[k]);
        CHECK(dot(t, cps[i].axisX) > 0.995, "tangent along null %zu X axis (%.4f)", i, dot(t, cps[i].axisX));
    }
    // At every null the ribbon's width lies along the null's Y axis and its front faces -Z.
    RibbonSettings rs;
    rs.twistTurns = 0; rs.segments = 600; rs.width = 50;
    RibbonMesh m = buildRibbon(pl, rs);
    FramedCurve fc(pl);
    for (size_t i = 0; i < cps.size(); i++) {
        double best = 1e300; size_t at = 0;
        for (size_t k = 0; k < m.center.size(); k++) {
            double d = length(m.center[k] - cps[i].pos);
            if (d < best) { best = d; at = k; }
        }
        double wy = dot(m.widthDir[at], cps[i].axisY);
        CHECK(wy > 0.98, "null %zu: width along its Y axis (%.4f)", i, wy);
    }
    // Rolling a null twists the ribbon around it, but not far away from it.
    std::vector<ControlPoint> flat = cps;
    flat[2] = cp(V3(1200, 500, 100), 15, 0, 1.5);
    RibbonMesh m2 = buildRibbon(controlPointsToPolyline(flat, false, 64), rs);
    CHECK(dot(m.widthDir.front(), m2.widthDir.front()) > 0.999, "first section unaffected by the third null's roll");
    // Handle scale 0 gives a sharp corner at that null.
    std::vector<ControlPoint> corner = {cp(V3(300, 600, 0), 0, 0, 0), cp(V3(800, 400, 0), 0, 0, 0), cp(V3(1300, 600, 0), 0, 0, 0)};
    Polyline pc = controlPointsToPolyline(corner, false, 64);
    V3 tin = normalize(pc.pts[64] - pc.pts[63]), tout = normalize(pc.pts[65] - pc.pts[64]);
    CHECK(dot(tin, tout) < 0.9, "sharp corner with zero handles (%.3f)", dot(tin, tout));
    // The path transform rotates the roll targets together with the points.
    PathTransform xf;
    xf.rotY = 90;
    xf.pivot = V3(960, 540, 0);
    Polyline r = transformPolyline(pl, xf);
    CHECK(std::fabs(dot(r.rolls[2].up, rotationXYZ(0, 90, 0) * cps[2].axisZ) - 1) < 1e-9, "roll targets follow the path transform");
}

static void previews(const std::string& dir) {
    std::printf("previews -> %s\n", dir.c_str());
    const int W = 1920, H = 1080;
    const float bg[3] = {0.06f, 0.065f, 0.08f};
    auto timeIt = [](auto&& fn) {
        auto t0 = std::chrono::steady_clock::now();
        fn();
        return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    };
    {
        Polyline path = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 64);
        RibbonSettings rs;
        rs.segments = 600; rs.twistTurns = 1.5; rs.twistAlongPath = false; rs.width = 140;
        rs.widthStart = 0.15; rs.waveAmp = 90; rs.waveLength = 700;
        Material mat;
        mat.samples = 4; mat.satin = 0.65; mat.gloss = 35; mat.sheen = 0.3; mat.translucency = 0.25;
        Packet pk;
        double tGeo = timeIt([&] { buildPacket(buildRibbon(path, rs), defaultCamera(W, H), test::studioLights(), mat, W, H, pk); });
        Image out;
        double tRaster = timeIt([&] { out = test::render(pk, Image(W, H)); });
        test::writePNG(dir + "/gpu_satin.png", out, bg);
        std::printf("  satin: %zu tris, geometry %.1f ms, CPU raster %.1f ms (1920x1080, 16 spp)\n", pk.tris.size(), tGeo, tRaster);
    }
    {
        Polyline path = bezierToPolyline(test::sCurve(), false, V3(0, 0, 1), 64);
        RibbonSettings rs;
        rs.segments = 500; rs.twistTurns = 1.25; rs.twistAlongPath = false; rs.width = 150;
        rs.waveAmp = 90; rs.waveLength = 700; rs.thickness = 26; rs.bevel = 9; rs.bevelSegments = 6;
        Material mat;
        mat.samples = 4; mat.satin = 0.4; mat.gloss = 45; mat.sheen = 0.2;
        mat.edge = V3(0.95, 0.85, 0.6);
        Packet pk;
        buildPacket(buildRibbon(path, rs), defaultCamera(W, H), test::studioLights(), mat, W, H, pk);
        Image out;
        double ms = timeIt([&] { out = test::render(pk, Image(W, H)); });
        test::writePNG(dir + "/gpu_thick.png", out, bg);
        std::printf("  thick: %zu tris, CPU raster %.1f ms\n", pk.tris.size(), ms);
    }
    {
        Polyline h = test::helix(260, 1300, 2.2, V3(960, 540, 200));
        RibbonSettings rs;
        rs.segments = 800; rs.twistTurns = 3; rs.width = 90;
        Material mat;
        mat.samples = 4; mat.front = V3(0.1, 0.35, 0.95); mat.back = V3(0.95, 0.95, 0.97);
        mat.metal = 0.3; mat.gloss = 60; mat.satin = 0.2;
        Packet pk;
        buildPacket(buildRibbon(h, rs), defaultCamera(W, H), test::studioLights(), mat, W, H, pk);
        test::writePNG(dir + "/gpu_helix.png", test::render(pk, Image(W, H)), bg);
    }
}

int main(int argc, char** argv) {
    testGeometry();
    testFrame3D();
    testWaveAndTransform();
    testRaster();
    testDepthAndFacing();
    testClipping();
    testComposite();
    testTexture();
    testSolidGeometry();
    testSolidRaster();
    testControlNulls();
    testCycle();
    testFront();
    testEnvironment();
    testWind();
    testNormalMap();
    if (argc > 1) previews(argv[1]);
    std::printf(test::failures() ? "\n%d check(s) FAILED\n" : "\nall checks passed\n", test::failures());
    return test::failures() ? 1 : 0;
}
