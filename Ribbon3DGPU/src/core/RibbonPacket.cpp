#include "RibbonPacket.h"

#include <algorithm>
#include <cmath>

#include "../kernel/RibbonKernel.h"  // rk_sh_basis: the host projects with the kernel's own basis

namespace rb {

Camera defaultCamera(double compW, double compH) {
    Camera c;
    c.zoom = compW * 50.0 / 36.0;
    c.pos = V3(compW / 2, compH / 2, -c.zoom);
    c.cx = compW / 2;
    c.cy = compH / 2;
    return c;
}

Camera cameraFromMatrix(const double m[4][4], double zoom, double compW, double compH) {
    Camera c = defaultCamera(compW, compH);
    bool rowVectors = std::fabs(m[0][3]) + std::fabs(m[1][3]) + std::fabs(m[2][3]) < 1e-9;
    auto at = [&](int r, int col) { return rowVectors ? m[r][col] : m[col][r]; };
    // Row-vector layout: rows 0..2 = local axes in world (scaled), row 3 = translation.
    c.axisX = normalize(V3(at(0, 0), at(0, 1), at(0, 2)), V3(1, 0, 0));
    c.axisY = normalize(V3(at(1, 0), at(1, 1), at(1, 2)), V3(0, 1, 0));
    c.axisZ = normalize(V3(at(2, 0), at(2, 1), at(2, 2)), V3(0, 0, 1));
    c.pos = V3(at(3, 0), at(3, 1), at(3, 2));
    c.zoom = zoom > 0 ? zoom : c.zoom;
    return c;
}

static float lin(double c, double gamma) { return float(gamma == 1.0 ? c : std::pow(std::max(c, 0.0), gamma)); }

namespace {

struct Builder {
    const Camera& cam;
    Packet& out;
    int W, H;

    V3 toCam(const RK_Vertex& v) const {
        V3 d = V3(v.pos[0], v.pos[1], v.pos[2]) - cam.pos;
        return V3(dot(d, cam.axisX), dot(d, cam.axisY), dot(d, cam.axisZ));
    }

    static RK_Vertex mix(const RK_Vertex& a, const RK_Vertex& b, float t) {
        RK_Vertex r;
        for (int i = 0; i < 3; i++) {
            r.pos[i] = a.pos[i] + (b.pos[i] - a.pos[i]) * t;
            r.nrm[i] = a.nrm[i] + (b.nrm[i] - a.nrm[i]) * t;
            r.tan[i] = a.tan[i] + (b.tan[i] - a.tan[i]) * t;
        }
        r.uv[0] = a.uv[0] + (b.uv[0] - a.uv[0]) * t;
        r.uv[1] = a.uv[1] + (b.uv[1] - a.uv[1]) * t;
        r.side = a.side + (b.side - a.side) * t;
        return r;
    }

    void emit(int i0, int i1, int i2) {
        const int ids[3] = {i0, i1, i2};
        RK_Tri t{};
        double nx = 0, ny = 0, nz = 0, gx = 0, gy = 0, gz = 0;
        for (int k = 0; k < 3; k++) {
            const RK_Vertex& v = out.verts[size_t(ids[k])];
            V3 c = toCam(v);
            double sx = (cam.cx + c.x * cam.zoom / c.z) * cam.scaleX - cam.originX;
            double sy = (cam.cy + c.y * cam.zoom / c.z) * cam.scaleY - cam.originY;
            t.px[k] = float(sx);
            t.py[k] = float(sy);
            t.iw[k] = float(1.0 / c.z);
            t.v[k] = ids[k];
            nx += v.nrm[0]; ny += v.nrm[1]; nz += v.nrm[2];
            gx += v.pos[0]; gy += v.pos[1]; gz += v.pos[2];
        }
        double area = double(t.px[1] - t.px[0]) * (t.py[2] - t.py[0]) - double(t.py[1] - t.py[0]) * (t.px[2] - t.px[0]);
        if (std::fabs(area) < 1e-8) return;
        t.ia = float(1.0 / area);
        t.minx = std::min({t.px[0], t.px[1], t.px[2]});
        t.maxx = std::max({t.px[0], t.px[1], t.px[2]});
        t.miny = std::min({t.py[0], t.py[1], t.py[2]});
        t.maxy = std::max({t.py[0], t.py[1], t.py[2]});
        if (t.maxx < 0 || t.maxy < 0 || t.minx > W || t.miny > H) return;
        V3 centroid(gx / 3, gy / 3, gz / 3);
        t.front = dot(V3(nx, ny, nz), cam.pos - centroid) > 0 ? 1.0f : 0.0f;
        out.tris.push_back(t);
    }

    // Sutherland-Hodgman against z >= near in camera space, then fan triangulation.
    void addTriangle(int i0, int i1, int i2) {
        int ids[3] = {i0, i1, i2};
        double z[3];
        int inside = 0;
        for (int k = 0; k < 3; k++) {
            z[k] = toCam(out.verts[size_t(ids[k])]).z;
            if (z[k] >= cam.nearZ) inside++;
        }
        if (inside == 3) { emit(i0, i1, i2); return; }
        if (inside == 0) return;
        int poly[4], n = 0;
        for (int k = 0; k < 3; k++) {
            int a = ids[k], b = ids[(k + 1) % 3];
            double za = z[k], zb = z[(k + 1) % 3];
            bool ina = za >= cam.nearZ, inb = zb >= cam.nearZ;
            if (ina) poly[n++] = a;
            if (ina != inb) {
                float t = float((cam.nearZ - za) / (zb - za));
                RK_Vertex v = mix(out.verts[size_t(a)], out.verts[size_t(b)], t);
                out.verts.push_back(v);
                poly[n++] = int(out.verts.size()) - 1;
            }
        }
        for (int k = 1; k + 1 < n; k++) emit(poly[0], poly[k], poly[k + 1]);
    }
};

}  // namespace

void buildPacket(const RibbonMesh& mesh, const Camera& cam, const std::vector<LightDesc>& lights,
                 const Material& mat, int W, int H, Packet& out, double ribbonLength, double ribbonWidth) {
    out.verts = mesh.verts;
    out.tris.clear();
    out.tileTris.clear();
    out.lights.clear();

    Builder b{cam, out, W, H};
    for (size_t i = 0; i + 2 < mesh.tris.size(); i += 3) b.addTriangle(mesh.tris[i], mesh.tris[i + 1], mesh.tris[i + 2]);

    // Tile binning by bounding box.
    const int tilesX = std::max(1, (W + RK_TILE - 1) / RK_TILE), tilesY = std::max(1, (H + RK_TILE - 1) / RK_TILE);
    out.tileStart.assign(size_t(tilesX) * tilesY + 1, 0);
    auto range = [&](const RK_Tri& t, int& x0, int& y0, int& x1, int& y1) {
        // Clamp before converting: clipped triangles can project very far outside the buffer.
        auto cx = [&](float v) { return int(std::floor(std::clamp(double(v), 0.0, double(W - 1)))); };
        auto cy = [&](float v) { return int(std::floor(std::clamp(double(v), 0.0, double(H - 1)))); };
        x0 = cx(t.minx) / RK_TILE;
        y0 = cy(t.miny) / RK_TILE;
        x1 = std::min(tilesX - 1, cx(t.maxx) / RK_TILE);
        y1 = std::min(tilesY - 1, cy(t.maxy) / RK_TILE);
    };
    for (const RK_Tri& t : out.tris) {
        int x0, y0, x1, y1;
        range(t, x0, y0, x1, y1);
        for (int ty = y0; ty <= y1; ty++)
            for (int tx = x0; tx <= x1; tx++) out.tileStart[size_t(ty) * tilesX + tx + 1]++;
    }
    for (size_t i = 1; i < out.tileStart.size(); i++) out.tileStart[i] += out.tileStart[i - 1];
    out.tileTris.assign(size_t(out.tileStart.back()), 0);
    std::vector<int> fill(out.tileStart.begin(), out.tileStart.end() - 1);
    for (size_t i = 0; i < out.tris.size(); i++) {
        int x0, y0, x1, y1;
        range(out.tris[i], x0, y0, x1, y1);
        for (int ty = y0; ty <= y1; ty++)
            for (int tx = x0; tx <= x1; tx++) out.tileTris[size_t(fill[size_t(ty) * tilesX + tx]++)] = int(i);
    }
    if (out.tileTris.empty()) out.tileTris.push_back(0);  // never empty: some APIs reject zero-size buffers
    if (out.tris.empty()) out.tris.push_back(RK_Tri{});
    if (out.verts.empty()) out.verts.push_back(RK_Vertex{});

    for (const LightDesc& L : lights) {
        if (out.lights.size() >= RK_MAX_LIGHTS) break;
        RK_Light l{};
        l.type = L.type;
        l.pos[0] = float(L.pos.x); l.pos[1] = float(L.pos.y); l.pos[2] = float(L.pos.z);
        V3 d = normalize(L.dir, V3(0, 0, 1));
        l.dir[0] = float(d.x); l.dir[1] = float(d.y); l.dir[2] = float(d.z);
        l.color[0] = lin(L.color.x, mat.gamma) * float(L.intensity);
        l.color[1] = lin(L.color.y, mat.gamma) * float(L.intensity);
        l.color[2] = lin(L.color.z, mat.gamma) * float(L.intensity);
        double outer = std::clamp(L.coneDeg, 0.1, 179.9) * 0.5 * 3.14159265358979 / 180;
        double inner = outer * (1 - std::clamp(L.featherPct, 0.0, 100.0) / 100);
        l.cosOuter = float(std::cos(outer));
        l.cosInner = float(std::cos(inner));
        out.lights.push_back(l);
    }
    if (out.lights.empty()) out.lights.push_back(RK_Light{});

    RK_Params& P = out.params;
    P = RK_Params{};
    P.width = W;
    P.height = H;
    P.srcPitch = P.dstPitch = W;
    P.srcX = P.srcY = 0;
    P.srcW = W;
    P.srcH = H;
    P.tilesX = tilesX;
    P.tilesY = tilesY;
    P.samples = std::clamp(mat.samples, 1, 4);
    P.numLights = int(std::min<size_t>(lights.size(), RK_MAX_LIGHTS));
    P.composite = mat.composite;
    P.camPos[0] = float(cam.pos.x); P.camPos[1] = float(cam.pos.y); P.camPos[2] = float(cam.pos.z);
    P.frontColor[0] = lin(mat.front.x, mat.gamma); P.frontColor[1] = lin(mat.front.y, mat.gamma); P.frontColor[2] = lin(mat.front.z, mat.gamma);
    P.backColor[0] = lin(mat.back.x, mat.gamma); P.backColor[1] = lin(mat.back.y, mat.gamma); P.backColor[2] = lin(mat.back.z, mat.gamma);
    P.ambient = float(mat.ambient);
    P.diffuse = float(mat.diffuse);
    P.specular = float(mat.specular);
    P.gloss = float(std::max(1.0, mat.gloss));
    P.satin = float(std::clamp(mat.satin, 0.0, 1.0));
    P.sheen = float(mat.sheen);
    P.translucency = float(mat.translucency);
    P.metal = float(std::clamp(mat.metal, 0.0, 1.0));
    P.texRepeat = float(mat.texRepeat);
    P.texOffset = float(mat.texOffset);
    P.opacity = float(std::clamp(mat.opacity, 0.0, 1.0));
    P.gamma = float(mat.gamma);
    P.solid = mesh.solid ? 1 : 0;
    P.edgeColor[0] = lin(mat.edge.x, mat.gamma); P.edgeColor[1] = lin(mat.edge.y, mat.gamma); P.edgeColor[2] = lin(mat.edge.z, mat.gamma);

    const double rot = mat.envRotationDeg * 3.14159265358979 / 180;
    P.envRotCos = float(std::cos(rot));
    P.envRotSin = float(std::sin(rot));
    P.envIntensity = float(std::max(0.0, mat.envIntensity));
    P.envReflection = float(std::max(0.0, mat.envReflection));
    P.glass = mat.glass ? 1 : 0;
    P.glassIor = float(std::clamp(mat.ior, 1.0, 4.0));
    P.glassF0 = float(std::pow((P.glassIor - 1.0) / (P.glassIor + 1.0), 2.0));
    P.glassTransmission = float(std::clamp(mat.transmission, 0.0, 1.0));
    P.glassTint = float(std::clamp(mat.tint, 0.0, 1.0));
    P.refractPx = float(mat.refractPx * cam.scaleX);  // comp pixels -> output pixels (downsampling)
    P.glassFrost = float(std::max(0.0, mat.frostPx) * cam.scaleX);
    P.frostTaps = std::clamp(mat.frostTaps, 4, 64);
    P.camRight[0] = float(cam.axisX.x); P.camRight[1] = float(cam.axisX.y); P.camRight[2] = float(cam.axisX.z);
    P.camDown[0] = float(cam.axisY.x); P.camDown[1] = float(cam.axisY.y); P.camDown[2] = float(cam.axisY.z);
    if (out.env.empty()) out.env.assign(4, 0.0f);

    P.nrmStrength = float(std::max(0.0, mat.normalStrength));
    P.roughAmount = float(std::clamp(mat.roughAmount, 0.0, 1.0));
    P.roughInvert = mat.roughInvert ? 1 : 0;
    P.nrmDirectX = mat.normalDirectX ? 1 : 0;
    if (mat.normalTiles) {
        // Square tiles of normalTilePx: as many along the visible ribbon as its length allows.
        const double tile = std::max(1.0, mat.normalTilePx);
        P.nrmRepeatU = float(std::max(ribbonLength, 1.0) / tile);
        P.nrmRepeatV = float(std::max(ribbonWidth, 1.0) / tile);
        P.nrmOffsetU = 0;
        P.nrmWrapV = 1;
    } else {
        P.nrmRepeatU = float(mat.texRepeat);
        P.nrmRepeatV = 1;
        P.nrmOffsetU = float(mat.texOffset);
        P.nrmWrapV = 0;
    }
}

void setEnvironmentImage(Packet& p, const float* bgra, int w, int h, int pitch) {
    RK_Params& P = p.params;
    P.hasEnv = 0;
    p.env.assign(4, 0.0f);
    if (!bgra || w <= 0 || h <= 0) return;
    const double gamma = P.gamma;
    // Level 0: area-averaged resample to W0 x H0 (2:1, the equirectangular layout).
    const int W0 = std::min(1024, std::max(2, w)), H0 = std::max(1, W0 / 2);
    std::vector<float> lvl(size_t(W0) * H0 * 3, 0.0f);
    std::vector<double> acc(size_t(W0) * 3);
    for (int y = 0; y < H0; y++) {
        const int sy0 = int(std::floor(double(y) * h / H0)), sy1 = std::max(sy0 + 1, int(std::floor(double(y + 1) * h / H0)));
        std::fill(acc.begin(), acc.end(), 0.0);
        std::vector<int> cnt(size_t(W0), 0);
        for (int sy = sy0; sy < std::min(sy1, h); sy++) {
            const float* row = bgra + size_t(sy) * size_t(pitch) * 4;
            for (int sx = 0; sx < w; sx++) {
                const float* px = row + size_t(sx) * 4;
                const int x = std::min(W0 - 1, int(double(sx) * W0 / w));
                const double a = px[3];
                double r = 0, g = 0, b = 0;
                if (a > 1e-6) {
                    r = px[2] / a; g = px[1] / a; b = px[0] / a;
                    if (gamma != 1.0) {
                        r = std::pow(std::max(r, 0.0), gamma); g = std::pow(std::max(g, 0.0), gamma); b = std::pow(std::max(b, 0.0), gamma);
                    }
                    r *= a; g *= a; b *= a;  // transparent parts of the map count as black
                }
                acc[size_t(x) * 3] += r; acc[size_t(x) * 3 + 1] += g; acc[size_t(x) * 3 + 2] += b;
                cnt[size_t(x)]++;
            }
        }
        for (int x = 0; x < W0; x++) {
            const double k = cnt[size_t(x)] ? 1.0 / cnt[size_t(x)] : 0.0;
            for (int c = 0; c < 3; c++) lvl[(size_t(y) * W0 + x) * 3 + c] = float(acc[size_t(x) * 3 + c] * k);
        }
    }
    // Mip chain down to 8 pixels wide: every level a 2 x 2 average of the previous one.
    std::vector<float> chain = lvl;
    int levels = 1, lw = W0, lh = H0;
    std::vector<float> prev = lvl;
    while (lw > 8 && levels < 10) {
        const int nw = std::max(1, lw / 2), nh = std::max(1, lh / 2);
        std::vector<float> next(size_t(nw) * nh * 3, 0.0f);
        for (int y = 0; y < nh; y++)
            for (int x = 0; x < nw; x++)
                for (int c = 0; c < 3; c++) {
                    double s = 0;
                    for (int dy = 0; dy < 2; dy++)
                        for (int dx = 0; dx < 2; dx++) {
                            const int sx = std::min(lw - 1, x * 2 + dx), sy = std::min(lh - 1, y * 2 + dy);
                            s += prev[(size_t(sy) * lw + sx) * 3 + c];
                        }
                    next[(size_t(y) * nw + x) * 3 + c] = float(s / 4);
                }
        chain.insert(chain.end(), next.begin(), next.end());
        prev.swap(next);
        lw = nw;
        lh = nh;
        levels++;
    }
    // Diffuse light: project the sharpest level onto L2 spherical harmonics (solid-angle weighted),
    // convolve with the cosine lobe and divide by pi (A_l / pi = 1, 2/3, 1/4).
    double L[27] = {0};
    for (int y = 0; y < H0; y++) {
        const double th = (y + 0.5) / H0 * 3.14159265358979;
        const double dw = (2 * 3.14159265358979 / W0) * (3.14159265358979 / H0) * std::sin(th);
        for (int x = 0; x < W0; x++) {
            const double ph = ((x + 0.5) / W0 - 0.5) * 2 * 3.14159265358979;
            rk3 q = rk_v(float(std::sin(th) * std::sin(ph)), float(std::cos(th)), float(std::sin(th) * std::cos(ph)));
            rk_sh9 b = rk_sh_basis(q);
            const float* c = &lvl[(size_t(y) * W0 + x) * 3];
            for (int k = 0; k < 9; k++)
                for (int ch = 0; ch < 3; ch++) L[k * 3 + ch] += c[ch] * b.c[k] * dw;
        }
    }
    static const double kBand[9] = {1.0, 2.0 / 3, 2.0 / 3, 2.0 / 3, 0.25, 0.25, 0.25, 0.25, 0.25};
    for (int k = 0; k < 9; k++)
        for (int ch = 0; ch < 3; ch++) P.envSH[k * 3 + ch] = float(L[k * 3 + ch] * kBand[k]);
    p.env.swap(chain);
    P.hasEnv = 1;
    P.envWidth = W0;
    P.envHeight = H0;
    P.envLevels = levels;
}

}  // namespace rb
