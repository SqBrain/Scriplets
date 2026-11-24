// Shared helpers for the Ribbon3D GPU tests: scenes, PNG writer, tiny check macros.
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/core/RibbonCPU.h"
#include "../src/core/RibbonGeometry.h"
#include "../src/core/RibbonPacket.h"

namespace test {

inline int& failures() { static int f = 0; return f; }
#define CHECK(cond, ...)                                                                   \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            std::printf("  FAIL %s:%d: %s | ", __FILE__, __LINE__, #cond);                \
            std::printf(__VA_ARGS__);                                                      \
            std::printf("\n");                                                             \
            test::failures()++;                                                            \
        }                                                                                  \
    } while (0)

struct Image {
    int w = 0, h = 0;
    std::vector<float> bgra;  // premultiplied
    Image() = default;
    Image(int w_, int h_, float b = 0, float g = 0, float r = 0, float a = 0) : w(w_), h(h_), bgra(size_t(w_) * h_ * 4) {
        for (size_t i = 0; i < size_t(w) * h; i++) { bgra[i * 4] = b; bgra[i * 4 + 1] = g; bgra[i * 4 + 2] = r; bgra[i * 4 + 3] = a; }
    }
    const float* px(int x, int y) const { return &bgra[(size_t(y) * w + x) * 4]; }
};

// S-curve drawn on a comp-facing plane (z = 0), comp 1920x1080.
inline std::vector<rb::Bezier> sCurve() {
    using rb::V3;
    return {{V3(180, 700, 0), V3(400, 560, 0), V3(600, 300, 0), V3(960, 320, 0)},
            {V3(960, 320, 0), V3(1320, 340, 0), V3(1300, 820, 0), V3(1740, 640, 0)}};
}

// Spiral through 3D space (for RMF / perspective / clipping tests).
inline rb::Polyline helix(double radius, double pitch, double turns, rb::V3 center, int n = 600) {
    rb::Polyline p;
    for (int i = 0; i <= n; i++) {
        double a = turns * 2 * 3.14159265358979 * i / n;
        p.pts.push_back(center + rb::V3((i - n / 2.0) * pitch / n, radius * std::sin(a), radius * std::cos(a)));
    }
    p.up = rb::V3(0, 0, -1);
    return p;
}

inline rb::Material flatMaterial(rb::V3 front, rb::V3 back) {
    rb::Material m;
    m.front = front; m.back = back;
    m.ambient = 1; m.diffuse = 0; m.specular = 0; m.sheen = 0; m.translucency = 0;
    m.gamma = 1; m.samples = 4;
    return m;
}

inline std::vector<rb::LightDesc> studioLights() {
    std::vector<rb::LightDesc> L(2);
    L[0].type = RK_LIGHT_POINT; L[0].pos = rb::V3(300, -300, -1400); L[0].intensity = 1.1;
    L[1].type = RK_LIGHT_AMBIENT; L[1].intensity = 0.12;
    return L;
}

inline Image render(const rb::Packet& pk, const Image& src, const float* tex = nullptr, const float* nrm = nullptr) {
    Image out(src.w, src.h);
    rb::renderCPU(pk, src.bgra.data(), out.bgra.data(), tex, 0, nrm);
    return out;
}

// ---- PNG (uncompressed deflate blocks: no zlib dependency)
inline uint32_t crc32(const uint8_t* d, size_t n, uint32_t c = 0xffffffffu) {
    static uint32_t t[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) { uint32_t k = i; for (int j = 0; j < 8; j++) k = (k & 1) ? 0xedb88320u ^ (k >> 1) : k >> 1; t[i] = k; }
        init = true;
    }
    for (size_t i = 0; i < n; i++) c = t[(c ^ d[i]) & 0xff] ^ (c >> 8);
    return c;
}
inline void writePNG(const std::string& file, const Image& img, const float bg[3]) {
    std::vector<uint8_t> raw;
    for (int y = 0; y < img.h; y++) {
        raw.push_back(0);
        for (int x = 0; x < img.w; x++) {
            const float* p = img.px(x, y);
            float a = p[3];
            for (int c = 2; c >= 0; c--) {
                float v = p[c] + bg[2 - c] * (1 - a);  // composite over a background (premultiplied)
                v = v < 0 ? 0 : (v > 1 ? 1 : v);
                raw.push_back(uint8_t(std::lround(v * 255)));
            }
        }
    }
    std::vector<uint8_t> z = {0x78, 0x01};
    uint32_t a1 = 1, b1 = 0;
    for (uint8_t v : raw) { a1 = (a1 + v) % 65521; b1 = (b1 + a1) % 65521; }
    for (size_t pos = 0; pos < raw.size() || pos == 0;) {
        size_t n = std::min<size_t>(65535, raw.size() - pos);
        z.push_back(pos + n >= raw.size() ? 1 : 0);
        z.push_back(uint8_t(n)); z.push_back(uint8_t(n >> 8)); z.push_back(uint8_t(~n)); z.push_back(uint8_t((~n) >> 8));
        z.insert(z.end(), raw.begin() + long(pos), raw.begin() + long(pos + n));
        pos += n;
        if (n == 0) break;
    }
    uint32_t ad = (b1 << 16) | a1;
    for (int i = 3; i >= 0; i--) z.push_back(uint8_t(ad >> (i * 8)));
    FILE* f = std::fopen(file.c_str(), "wb");
    if (!f) return;
    auto chunk = [&](const char* type, const std::vector<uint8_t>& data) {
        uint8_t len[4] = {uint8_t(data.size() >> 24), uint8_t(data.size() >> 16), uint8_t(data.size() >> 8), uint8_t(data.size())};
        std::fwrite(len, 1, 4, f);
        std::vector<uint8_t> td(type, type + 4);
        td.insert(td.end(), data.begin(), data.end());
        std::fwrite(td.data(), 1, td.size(), f);
        uint32_t c = crc32(td.data(), td.size()) ^ 0xffffffffu;
        uint8_t cb[4] = {uint8_t(c >> 24), uint8_t(c >> 16), uint8_t(c >> 8), uint8_t(c)};
        std::fwrite(cb, 1, 4, f);
    };
    const uint8_t sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    std::fwrite(sig, 1, 8, f);
    std::vector<uint8_t> ihdr = {uint8_t(img.w >> 24), uint8_t(img.w >> 16), uint8_t(img.w >> 8), uint8_t(img.w),
                                 uint8_t(img.h >> 24), uint8_t(img.h >> 16), uint8_t(img.h >> 8), uint8_t(img.h), 8, 2, 0, 0, 0};
    chunk("IHDR", ihdr);
    chunk("IDAT", z);
    chunk("IEND", {});
    std::fclose(f);
}

}  // namespace test
