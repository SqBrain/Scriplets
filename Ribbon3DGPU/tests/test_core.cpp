// Ribbon3D GPU core tests: geometry, projection/clipping, rasterization, shading, compositing.
// Usage: ribbon_tests [preview_dir]
#include <chrono>
#include <cstdio>
#include <string>

#include "TestScene.h"

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
    if (argc > 1) previews(argv[1]);
    std::printf(test::failures() ? "\n%d check(s) FAILED\n" : "\nall checks passed\n", test::failures());
    return test::failures() ? 1 : 0;
}
