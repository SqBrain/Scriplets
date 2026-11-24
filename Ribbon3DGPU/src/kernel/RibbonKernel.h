// Ribbon3D GPU - per-pixel render kernel.
//
// Single source compiled as C++ (CPU fallback), OpenCL C, CUDA and Metal Shading Language.
// Rules that keep it portable: C99 subset, no function overloading, no references, no
// compound literals, float literals always with an "f" suffix, no out-parameters
// (Metal would need "thread" pointers) - small structs are returned by value instead.
//
// Per pixel: SxS samples; for every sample the triangles binned to the pixel's tile are
// tested (edge functions), the nearest one wins (1/w depth), the hit is shaded with a
// perspective-correct interpolation of the vertex attributes. Shading is cached per
// triangle inside a pixel, so it usually runs once or twice per pixel.
#ifndef RIBBON_KERNEL_H
#define RIBBON_KERNEL_H

#if defined(__OPENCL_VERSION__)
    #define RK_FN
    #define RK_IN(T) __global const T*
    #define RK_OUT(T) __global T*
    #define RK_SQRT sqrt
    #define RK_POW pow
    #define RK_FLOOR floor
    #define RK_FABS fabs
#elif defined(__CUDACC__) || defined(__CUDA__)
    #define RK_FN __device__ static inline
    #define RK_IN(T) const T*
    #define RK_OUT(T) T*
    #if defined(RK_PTX_NO_LIBDEVICE)
        /* PTX built by clang without the CUDA toolkit: NVVM builtins instead of libdevice.
           RK_POW is only ever called with a base >= 0 and an exponent > 0. */
        #define RK_SQRT __nvvm_sqrt_rn_f
        #define RK_POW(x, y) __nvvm_ex2_approx_f((y) * __nvvm_lg2_approx_f(x))
        #define RK_FLOOR __nvvm_floor_f
        #define RK_FABS __nvvm_fabs_f
    #else
        #define RK_SQRT sqrtf
        #define RK_POW powf
        #define RK_FLOOR floorf
        #define RK_FABS fabsf
    #endif
#elif defined(__METAL_VERSION__)
    #include <metal_stdlib>
    using namespace metal;
    #define RK_FN static inline
    #define RK_IN(T) device const T*
    #define RK_OUT(T) device T*
    #define RK_SQRT sqrt
    #define RK_POW pow
    #define RK_FLOOR floor
    #define RK_FABS fabs
#else
    #include <cmath>
    #define RK_FN static inline
    #define RK_IN(T) const T*
    #define RK_OUT(T) T*
    #define RK_SQRT std::sqrt
    #define RK_POW std::pow
    #define RK_FLOOR std::floor
    #define RK_FABS std::fabs
#endif

#include "RibbonShared.h"

typedef struct { float x, y, z; } rk3;
typedef struct { float r, g, b, a; } rk4;

RK_FN rk3 rk_v(float x, float y, float z) { rk3 r; r.x = x; r.y = y; r.z = z; return r; }
RK_FN rk3 rk_add(rk3 a, rk3 b) { return rk_v(a.x + b.x, a.y + b.y, a.z + b.z); }
RK_FN rk3 rk_sub(rk3 a, rk3 b) { return rk_v(a.x - b.x, a.y - b.y, a.z - b.z); }
RK_FN rk3 rk_scale(rk3 a, float s) { return rk_v(a.x * s, a.y * s, a.z * s); }
RK_FN rk3 rk_mulv(rk3 a, rk3 b) { return rk_v(a.x * b.x, a.y * b.y, a.z * b.z); }
RK_FN float rk_dot(rk3 a, rk3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
RK_FN float rk_maxf(float a, float b) { return a > b ? a : b; }
RK_FN float rk_minf(float a, float b) { return a < b ? a : b; }
RK_FN float rk_clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
RK_FN float rk_mixf(float a, float b, float t) { return a + (b - a) * t; }
RK_FN rk3 rk_mix3(rk3 a, rk3 b, float t) { return rk_add(a, rk_scale(rk_sub(b, a), t)); }
RK_FN rk3 rk_norm(rk3 a, rk3 fb) {
    float l = RK_SQRT(rk_dot(a, a));
    return l > 1e-12f ? rk_scale(a, 1.0f / l) : fb;
}
RK_FN float rk_smooth(float e0, float e1, float x) {
    float t = rk_clampf((x - e0) / rk_maxf(e1 - e0, 1e-6f), 0.0f, 1.0f);
    return t * t * (3.0f - 2.0f * t);
}
RK_FN float rk_lin(float c, float gamma) { return gamma == 1.0f ? c : RK_POW(rk_maxf(c, 0.0f), gamma); }
RK_FN float rk_enc(float c, float gamma) { return gamma == 1.0f ? c : RK_POW(rk_maxf(c, 0.0f), 1.0f / gamma); }

RK_FN rk3 rk_vtx3(RK_IN(float) a) { return rk_v(a[0], a[1], a[2]); }

// Bilinear texture fetch (BGRA float, premultiplied); u wraps, v clamps. Returns linear straight color + alpha.
RK_FN rk4 rk_texture(RK_IN(RK_Params) P, RK_IN(float) tex, float u, float v) {
    float fu = u * P->texRepeat + P->texOffset;
    fu = fu - RK_FLOOR(fu);
    float fx = fu * (float)P->texWidth - 0.5f, fy = rk_clampf(v, 0.0f, 1.0f) * (float)P->texHeight - 0.5f;
    float x0f = RK_FLOOR(fx), y0f = RK_FLOOR(fy), tx = fx - x0f, ty = fy - y0f;
    int x0 = (int)x0f, y0 = (int)y0f, W = P->texWidth, H = P->texHeight;
    int x1 = x0 + 1, y1 = y0 + 1;
    x0 = ((x0 % W) + W) % W; x1 = ((x1 % W) + W) % W;
    y0 = y0 < 0 ? 0 : (y0 >= H ? H - 1 : y0);
    y1 = y1 < 0 ? 0 : (y1 >= H ? H - 1 : y1);
    int i00 = (y0 * P->texPitch + x0) * 4, i10 = (y0 * P->texPitch + x1) * 4;
    int i01 = (y1 * P->texPitch + x0) * 4, i11 = (y1 * P->texPitch + x1) * 4;
    float w00 = (1.0f - tx) * (1.0f - ty), w10 = tx * (1.0f - ty), w01 = (1.0f - tx) * ty, w11 = tx * ty;
    rk4 c;
    c.b = tex[i00] * w00 + tex[i10] * w10 + tex[i01] * w01 + tex[i11] * w11;
    c.g = tex[i00 + 1] * w00 + tex[i10 + 1] * w10 + tex[i01 + 1] * w01 + tex[i11 + 1] * w11;
    c.r = tex[i00 + 2] * w00 + tex[i10 + 2] * w10 + tex[i01 + 2] * w01 + tex[i11 + 2] * w11;
    c.a = tex[i00 + 3] * w00 + tex[i10 + 3] * w10 + tex[i01 + 3] * w01 + tex[i11 + 3] * w11;
    if (c.a > 1e-6f) {
        float ia = 1.0f / c.a;
        c.r = rk_lin(c.r * ia, P->gamma); c.g = rk_lin(c.g * ia, P->gamma); c.b = rk_lin(c.b * ia, P->gamma);
    } else {
        c.r = 0.0f; c.g = 0.0f; c.b = 0.0f; c.a = 0.0f;
    }
    return c;
}

// ---------------------------------------------------------------- environment (HDRI)

#define RK_PI 3.14159265f

// atan2 for every quadrant, error about 1e-5 rad (Abramowitz & Stegun 4.4.49). Own polynomial:
// the PTX is built without libdevice, where atan2 / acos do not exist.
RK_FN float rk_atan2(float y, float x) {
    float ax = RK_FABS(x), ay = RK_FABS(y);
    float mx = rk_maxf(ax, ay), mn = rk_minf(ax, ay);
    if (mx <= 0.0f) return 0.0f;
    float a = mn / mx, s = a * a;
    float r = a * (0.9998660f + s * (-0.3302995f + s * (0.1801410f + s * (-0.0851330f + s * 0.0208351f))));
    if (ay > ax) r = 1.57079637f - r;
    if (x < 0.0f) r = 3.14159274f - r;
    if (y < 0.0f) r = -r;
    return r;
}

// World direction -> environment frame (x, up, z) after the rotation around the vertical axis.
RK_FN rk3 rk_env_frame(RK_IN(RK_Params) P, rk3 d) {
    return rk_v(d.x * P->envRotCos - d.z * P->envRotSin, -d.y, d.x * P->envRotSin + d.z * P->envRotCos);
}

// L2 spherical harmonics basis at q (environment frame). The host projects the map with the same function.
typedef struct { float c[9]; } rk_sh9;
RK_FN rk_sh9 rk_sh_basis(rk3 q) {
    rk_sh9 b;
    b.c[0] = 0.282095f;
    b.c[1] = 0.488603f * q.y;
    b.c[2] = 0.488603f * q.z;
    b.c[3] = 0.488603f * q.x;
    b.c[4] = 1.092548f * q.x * q.y;
    b.c[5] = 1.092548f * q.y * q.z;
    b.c[6] = 0.315392f * (3.0f * q.z * q.z - 1.0f);
    b.c[7] = 1.092548f * q.x * q.z;
    b.c[8] = 0.546274f * (q.x * q.x - q.y * q.y);
    return b;
}

// Diffuse light arriving at a surface with normal n: irradiance / pi.
RK_FN rk3 rk_env_irr(RK_IN(RK_Params) P, rk3 n) {
    rk_sh9 b = rk_sh_basis(rk_env_frame(P, n));
    rk3 e = rk_v(0.0f, 0.0f, 0.0f);
    for (int k = 0; k < 9; k++) e = rk_add(e, rk_scale(rk_v(P->envSH[k * 3], P->envSH[k * 3 + 1], P->envSH[k * 3 + 2]), b.c[k]));
    return rk_v(rk_maxf(e.x, 0.0f), rk_maxf(e.y, 0.0f), rk_maxf(e.z, 0.0f));
}

// Bilinear fetch from mip level l (RGB floats); u wraps around, v clamps at the poles.
RK_FN rk3 rk_env_level(RK_IN(RK_Params) P, RK_IN(float) env, int l, float u, float v) {
    int W = P->envWidth, H = P->envHeight, off = 0;
    for (int k = 0; k < l; k++) {
        off += W * H * 3;
        W = W > 1 ? W / 2 : 1;
        H = H > 1 ? H / 2 : 1;
    }
    float fx = (u - RK_FLOOR(u)) * (float)W - 0.5f, fy = rk_clampf(v, 0.0f, 1.0f) * (float)H - 0.5f;
    float x0f = RK_FLOOR(fx), y0f = RK_FLOOR(fy), tx = fx - x0f, ty = fy - y0f;
    int x0 = (int)x0f, y0 = (int)y0f;
    int x1 = x0 + 1, y1 = y0 + 1;
    x0 = ((x0 % W) + W) % W; x1 = ((x1 % W) + W) % W;
    y0 = y0 < 0 ? 0 : (y0 >= H ? H - 1 : y0);
    y1 = y1 < 0 ? 0 : (y1 >= H ? H - 1 : y1);
    int i00 = off + (y0 * W + x0) * 3, i10 = off + (y0 * W + x1) * 3;
    int i01 = off + (y1 * W + x0) * 3, i11 = off + (y1 * W + x1) * 3;
    float w00 = (1.0f - tx) * (1.0f - ty), w10 = tx * (1.0f - ty), w01 = (1.0f - tx) * ty, w11 = tx * ty;
    return rk_v(env[i00] * w00 + env[i10] * w10 + env[i01] * w01 + env[i11] * w11,
                env[i00 + 1] * w00 + env[i10 + 1] * w10 + env[i01 + 1] * w01 + env[i11 + 1] * w11,
                env[i00 + 2] * w00 + env[i10 + 2] * w10 + env[i01 + 2] * w01 + env[i11 + 2] * w11);
}

// Environment radiance seen along direction d; lod 0 = sharp, every level blurs twice as much.
RK_FN rk3 rk_env(RK_IN(RK_Params) P, RK_IN(float) env, rk3 d, float lod) {
    rk3 q = rk_env_frame(P, d);
    float u = 0.5f + rk_atan2(q.x, q.z) * (0.5f / RK_PI);
    float v = rk_atan2(RK_SQRT(q.x * q.x + q.z * q.z), q.y) * (1.0f / RK_PI);
    lod = rk_clampf(lod, 0.0f, (float)(P->envLevels - 1));
    int l0 = (int)RK_FLOOR(lod);
    float f = lod - (float)l0;
    rk3 c = rk_env_level(P, env, l0, u, v);
    if (f > 0.0f && l0 + 1 < P->envLevels) c = rk_mix3(c, rk_env_level(P, env, l0 + 1, u, v), f);
    return c;
}

// ---------------------------------------------------------------- shading

// Result of shading one hit, before it is combined with what lies behind it.
typedef struct {
    float r, g, b;      // opaque: lit color (linear). Glass: light scattered by the body, already x (1 - T)
    float a;            // coverage (texture alpha)
    float sr, sg, sb;   // glass: reflected light (environment, highlights, sheen)
    float sa;           // glass: alpha of the reflected light
    float fres;         // glass: Fresnel reflectance
    float tr, tg, tb;   // glass: transmitted fraction per channel (T x tint)
    float topaque;      // glass: 1 - T
    float dx, dy;       // glass: screen offset of the refracted background, output pixels
} rk_hit;

// Shades one hit. w0..w2: screen-space barycentrics, iw: interpolated 1/depth.
RK_FN rk_hit rk_shade(RK_IN(RK_Params) P, RK_IN(RK_Tri) t, RK_IN(RK_Vertex) verts, RK_IN(RK_Light) lights,
                      RK_IN(float) tex, RK_IN(float) env, float w0, float w1, float w2, float iw) {
    RK_IN(RK_Vertex) a = verts + t->v[0];
    RK_IN(RK_Vertex) b = verts + t->v[1];
    RK_IN(RK_Vertex) c = verts + t->v[2];
    float q0 = w0 * t->iw[0] / iw, q1 = w1 * t->iw[1] / iw, q2 = w2 * t->iw[2] / iw;

    rk3 pos = rk_add(rk_add(rk_scale(rk_vtx3(a->pos), q0), rk_scale(rk_vtx3(b->pos), q1)), rk_scale(rk_vtx3(c->pos), q2));
    rk3 n = rk_add(rk_add(rk_scale(rk_vtx3(a->nrm), q0), rk_scale(rk_vtx3(b->nrm), q1)), rk_scale(rk_vtx3(c->nrm), q2));
    rk3 tg = rk_add(rk_add(rk_scale(rk_vtx3(a->tan), q0), rk_scale(rk_vtx3(b->tan), q1)), rk_scale(rk_vtx3(c->tan), q2));
    float u = a->uv[0] * q0 + b->uv[0] * q1 + c->uv[0] * q2;
    float v = a->uv[1] * q0 + b->uv[1] * q1 + c->uv[1] * q2;

    rk3 V = rk_norm(rk_sub(rk_v(P->camPos[0], P->camPos[1], P->camPos[2]), pos), rk_v(0.0f, 0.0f, -1.0f));
    int front = t->front > 0.5f;
    n = rk_norm(n, rk_v(0.0f, 0.0f, -1.0f));
    // Thin sheet: two-sided, the normal follows the visible side. Solid: normals are outward; a
    // glass solid also shows its far side through the near one: there the light leaves the glass.
    int exiting = 0;
    if (!P->solid && !front) n = rk_scale(n, -1.0f);
    if (P->solid && P->glass && rk_dot(n, V) < 0.0f) { n = rk_scale(n, -1.0f); exiting = 1; }
    // An interpolated normal may turn slightly away at grazing angles: bend it back to the viewer.
    float nv = rk_dot(n, V);
    if (nv < 0.02f) n = rk_norm(rk_add(n, rk_scale(V, 0.02f - nv)), V);
    nv = rk_clampf(rk_dot(n, V), 0.0f, 1.0f);
    tg = rk_norm(tg, rk_v(1.0f, 0.0f, 0.0f));

    rk3 base;
    float alpha = 1.0f;
    if (P->solid) {
        // Faces get the front / back color (or the texture over it), edges the edge color; the
        // bevel blends between them by how much its normal faces front or back.
        float side = a->side * q0 + b->side * q1 + c->side * q2;
        rk3 face = side >= 0.0f ? rk_v(P->frontColor[0], P->frontColor[1], P->frontColor[2])
                                : rk_v(P->backColor[0], P->backColor[1], P->backColor[2]);
        if (P->hasTexture) {
            rk4 tc = rk_texture(P, tex, u, v);
            face = rk_mix3(face, rk_v(tc.r, tc.g, tc.b), tc.a);
        }
        base = rk_mix3(rk_v(P->edgeColor[0], P->edgeColor[1], P->edgeColor[2]), face, rk_smooth(0.15f, 0.85f, RK_FABS(side)));
    } else if (P->hasTexture) {
        rk4 tc = rk_texture(P, tex, u, v);
        base = rk_v(tc.r, tc.g, tc.b);
        alpha = tc.a;
    } else {
        base = front ? rk_v(P->frontColor[0], P->frontColor[1], P->frontColor[2])
                     : rk_v(P->backColor[0], P->backColor[1], P->backColor[2]);
    }

    rk3 white = rk_v(1.0f, 1.0f, 1.0f);
    rk3 specCol = rk_mix3(white, base, P->metal);
    rk3 body = rk_scale(base, P->ambient);    // light scattered by the material
    rk3 hl = rk_v(0.0f, 0.0f, 0.0f);          // highlights
    for (int i = 0; i < P->numLights; i++) {
        RK_IN(RK_Light) L = lights + i;
        rk3 lc = rk_vtx3(L->color);
        if (L->type == RK_LIGHT_AMBIENT) { body = rk_add(body, rk_mulv(base, lc)); continue; }
        rk3 ld;
        float att = 1.0f;
        if (L->type == RK_LIGHT_PARALLEL) {
            ld = rk_scale(rk_vtx3(L->dir), -1.0f);
        } else {
            ld = rk_norm(rk_sub(rk_vtx3(L->pos), pos), V);
            if (L->type == RK_LIGHT_SPOT) att = rk_smooth(L->cosOuter, L->cosInner, -rk_dot(ld, rk_vtx3(L->dir)));
        }
        float ndl = rk_dot(n, ld);
        float diff = rk_maxf(ndl, 0.0f) * P->diffuse * (1.0f - P->metal);
        float trans = rk_maxf(-ndl, 0.0f) * P->translucency;
        rk3 H = rk_norm(rk_add(ld, V), n);
        float blinn = RK_POW(rk_maxf(rk_dot(n, H), 0.0f), P->gloss);
        float th = rk_dot(tg, H);
        float aniso = RK_POW(RK_SQRT(rk_maxf(1.0f - th * th, 0.0f)), P->gloss);
        float spec = rk_mixf(blinn, aniso, P->satin) * P->specular * rk_clampf(ndl * 4.0f, 0.0f, 1.0f);
        body = rk_add(body, rk_scale(rk_mulv(lc, base), (diff + trans) * att));
        hl = rk_add(hl, rk_scale(rk_mulv(lc, specCol), spec * att));
    }
    float f5 = RK_POW(1.0f - nv, 5.0f);
    hl = rk_add(hl, rk_scale(rk_mix3(white, base, 0.5f), f5 * P->sheen));

    // Environment: diffuse light from all around (and through the ribbon), mirrored reflection.
    rk3 R = rk_sub(rk_scale(n, 2.0f * nv), V);
    rk3 envRefl = rk_scale(white, P->ambient);  // without an environment: a faint uniform surround
    if (P->hasEnv) {
        rk3 irr = rk_scale(rk_env_irr(P, n), P->diffuse * (1.0f - P->metal));
        rk3 irrBack = rk_scale(rk_env_irr(P, rk_scale(n, -1.0f)), P->translucency);
        body = rk_add(body, rk_scale(rk_mulv(base, rk_add(irr, irrBack)), P->envIntensity));
        float rough = RK_SQRT(2.0f / (P->gloss + 2.0f));
        float lod = rough * (float)(P->envLevels - 1) * 1.5f + P->satin;
        envRefl = rk_scale(rk_env(P, env, R, lod), P->envIntensity * P->envReflection);
    }

    rk_hit h;
    h.a = alpha;
    h.sr = h.sg = h.sb = 0.0f; h.sa = 0.0f; h.fres = 0.0f;
    h.tr = h.tg = h.tb = 0.0f; h.topaque = 1.0f; h.dx = 0.0f; h.dy = 0.0f;
    if (!P->glass) {
        // Fresnel-weighted reflection: dielectric (4 % head-on) or metal (tinted by the color).
        float fd = 0.04f + 0.96f * f5;
        rk3 fm = rk_add(base, rk_scale(rk_sub(white, base), f5));
        rk3 fr = rk_scale(rk_mix3(rk_v(fd, fd, fd), fm, P->metal), P->hasEnv ? P->specular : 0.0f);
        rk3 col = rk_add(rk_add(body, hl), rk_mulv(envRefl, fr));
        h.r = col.x; h.g = col.y; h.b = col.z;
        return h;
    }

    // Glass: Fresnel reflection on top; of the rest, T passes (tinted), 1 - T scatters in the body.
    float F = P->glassF0 + (1.0f - P->glassF0) * f5;
    float T = P->glassTransmission;
    rk3 tint = rk_mix3(white, base, P->glassTint);
    rk3 refl = rk_add(rk_scale(envRefl, F), hl);
    h.sr = refl.x; h.sg = refl.y; h.sb = refl.z;
    h.sa = rk_minf(1.0f, F + (hl.x + hl.y + hl.z) * (1.0f / 3.0f));
    h.fres = F;
    h.r = body.x * (1.0f - T); h.g = body.y * (1.0f - T); h.b = body.z * (1.0f - T);
    h.tr = tint.x * T; h.tg = tint.y * T; h.tb = tint.z * T;
    h.topaque = 1.0f - T;
    // Refraction: Snell's law, the bend of the ray moves the background on screen.
    float eta = exiting ? P->glassIor : 1.0f / P->glassIor;
    rk3 I = rk_scale(V, -1.0f);
    float k = 1.0f - eta * eta * (1.0f - nv * nv);
    rk3 Td = k < 0.0f ? rk_sub(I, rk_scale(n, -2.0f * nv)) : rk_add(rk_scale(I, eta), rk_scale(n, eta * nv - RK_SQRT(k)));
    rk3 bend = rk_sub(Td, I);
    h.dx = P->refractPx * rk_dot(bend, rk_vtx3(P->camRight));
    h.dy = P->refractPx * rk_dot(bend, rk_vtx3(P->camDown));
    return h;
}

// The layer behind the ribbon at output position (x, y), premultiplied linear; bilinear,
// clamped to the layer. Only "Over Layer" has it: otherwise the ribbon is alone or in the back.
RK_FN rk4 rk_background(RK_IN(RK_Params) P, RK_IN(float) src, float x, float y) {
    rk4 o;
    o.r = o.g = o.b = o.a = 0.0f;
    if (P->composite != RK_COMPOSITE_OVER || P->srcW <= 0 || P->srcH <= 0) return o;
    float fx = rk_clampf(x - (float)P->srcX - 0.5f, 0.0f, (float)(P->srcW - 1));
    float fy = rk_clampf(y - (float)P->srcY - 0.5f, 0.0f, (float)(P->srcH - 1));
    float x0f = RK_FLOOR(fx), y0f = RK_FLOOR(fy), tx = fx - x0f, ty = fy - y0f;
    int x0 = (int)x0f, y0 = (int)y0f;
    int x1 = x0 + 1 < P->srcW ? x0 + 1 : x0, y1 = y0 + 1 < P->srcH ? y0 + 1 : y0;
    int i00 = (y0 * P->srcPitch + x0) * 4, i10 = (y0 * P->srcPitch + x1) * 4;
    int i01 = (y1 * P->srcPitch + x0) * 4, i11 = (y1 * P->srcPitch + x1) * 4;
    float w00 = (1.0f - tx) * (1.0f - ty), w10 = tx * (1.0f - ty), w01 = (1.0f - tx) * ty, w11 = tx * ty;
    float B = src[i00] * w00 + src[i10] * w10 + src[i01] * w01 + src[i11] * w11;
    float G = src[i00 + 1] * w00 + src[i10 + 1] * w10 + src[i01 + 1] * w01 + src[i11 + 1] * w11;
    float Rr = src[i00 + 2] * w00 + src[i10 + 2] * w10 + src[i01 + 2] * w01 + src[i11 + 2] * w11;
    float A = src[i00 + 3] * w00 + src[i10 + 3] * w10 + src[i01 + 3] * w01 + src[i11 + 3] * w11;
    if (A > 1e-6f) {
        float ia = 1.0f / A;
        o.r = rk_lin(Rr * ia, P->gamma) * A; o.g = rk_lin(G * ia, P->gamma) * A; o.b = rk_lin(B * ia, P->gamma) * A;
        o.a = A;
    }
    return o;
}

// A glass hit over what lies behind it (both premultiplied linear).
RK_FN rk4 rk_glass_over(rk_hit h, rk4 bg) {
    float k = 1.0f - h.fres;
    rk4 o;
    o.r = h.sr + k * (h.tr * bg.r + h.r);
    o.g = h.sg + k * (h.tg * bg.g + h.g);
    o.b = h.sb + k * (h.tb * bg.b + h.b);
    o.a = rk_minf(1.0f, h.sa + k * (h.topaque + (1.0f - h.topaque) * bg.a));
    // Texture alpha: where the ribbon is cut away the background shows unchanged.
    o.r = bg.r + (o.r - bg.r) * h.a; o.g = bg.g + (o.g - bg.g) * h.a;
    o.b = bg.b + (o.b - bg.b) * h.a; o.a = bg.a + (o.a - bg.a) * h.a;
    return o;
}

// Shading cache inside a pixel: up to three triangles (the nearest and, for glass, the one behind).
#define RK_CACHED(ID, W0, W1, W2, IW, OUT)                                                     \
    if ((ID) == id0) OUT = h0;                                                                 \
    else if ((ID) == id1) OUT = h1;                                                            \
    else if ((ID) == id2) OUT = h2;                                                            \
    else {                                                                                     \
        OUT = rk_shade(P, tris + (ID), verts, lights, tex, env, W0, W1, W2, IW);               \
        id2 = id1; h2 = h1; id1 = id0; h1 = h0; id0 = (ID); h0 = OUT;                          \
    }

RK_FN void rk_render_pixel(int x, int y, RK_IN(RK_Params) P, RK_IN(RK_Tri) tris, RK_IN(RK_Vertex) verts,
                           RK_IN(int) tileStart, RK_IN(int) tileTris, RK_IN(RK_Light) lights, RK_IN(float) env,
                           RK_IN(float) tex, RK_IN(float) src, RK_OUT(float) dst) {
    if (x >= P->width || y >= P->height) return;
    int tile = (y / RK_TILE) * P->tilesX + (x / RK_TILE);
    int first = tileStart[tile], last = tileStart[tile + 1];
    int S = P->samples;
    float invS = 1.0f / (float)S;
    float cr = 0.0f, cg = 0.0f, cb = 0.0f, ca = 0.0f;

    if (first < last) {
        int id0 = -1, id1 = -1, id2 = -1;
        rk_hit h0, h1, h2;
        h0.r = h0.g = h0.b = h0.a = 0.0f;
        h0.sr = h0.sg = h0.sb = h0.sa = h0.fres = 0.0f;
        h0.tr = h0.tg = h0.tb = 0.0f; h0.topaque = 1.0f; h0.dx = h0.dy = 0.0f;
        h1 = h0; h2 = h0;
        for (int sy = 0; sy < S; sy++) {
            for (int sx = 0; sx < S; sx++) {
                float px = (float)x + ((float)sx + 0.5f) * invS;
                float py = (float)y + ((float)sy + 0.5f) * invS;
                // Nearest hit and, for glass, the next surface clearly behind it (not the
                // neighbouring triangle of the same surface).
                float best = 0.0f, bw0 = 0.0f, bw1 = 0.0f, bw2 = 0.0f;
                float second = 0.0f, cw0 = 0.0f, cw1 = 0.0f, cw2 = 0.0f;
                int bt = -1, bt2 = -1;
                for (int i = first; i < last; i++) {
                    int ti = tileTris[i];
                    RK_IN(RK_Tri) t = tris + ti;
                    if (px < t->minx || px > t->maxx || py < t->miny || py > t->maxy) continue;
                    float w0 = ((t->px[2] - t->px[1]) * (py - t->py[1]) - (t->py[2] - t->py[1]) * (px - t->px[1])) * t->ia;
                    float w1 = ((t->px[0] - t->px[2]) * (py - t->py[2]) - (t->py[0] - t->py[2]) * (px - t->px[2])) * t->ia;
                    float w2 = 1.0f - w0 - w1;
                    if (w0 < 0.0f || w1 < 0.0f || w2 < 0.0f) continue;
                    float iw = w0 * t->iw[0] + w1 * t->iw[1] + w2 * t->iw[2];
                    if (iw > best) {
                        if (P->glass && bt >= 0 && best < iw * 0.9999f) {
                            second = best; bt2 = bt; cw0 = bw0; cw1 = bw1; cw2 = bw2;
                        }
                        best = iw; bt = ti; bw0 = w0; bw1 = w1; bw2 = w2;
                    } else if (P->glass && iw < best * 0.9999f && iw > second) {
                        second = iw; bt2 = ti; cw0 = w0; cw1 = w1; cw2 = w2;
                    }
                }
                if (bt >= 0) {
                    rk_hit hf;
                    RK_CACHED(bt, bw0, bw1, bw2, best, hf)
                    if (!P->glass) {
                        cr += hf.r * hf.a; cg += hf.g * hf.a; cb += hf.b * hf.a; ca += hf.a;
                    } else {
                        float ox = px + hf.dx, oy = py + hf.dy;
                        rk4 bg;
                        if (bt2 >= 0) {
                            rk_hit hb;
                            RK_CACHED(bt2, cw0, cw1, cw2, second, hb)
                            bg = rk_glass_over(hb, rk_background(P, src, ox + hb.dx, oy + hb.dy));
                        } else {
                            bg = rk_background(P, src, ox, oy);
                        }
                        rk4 g = rk_glass_over(hf, bg);
                        cr += g.r; cg += g.g; cb += g.b; ca += g.a;
                    }
                }
            }
        }
    }

    // Average samples, encode to the working space, apply opacity (premultiplied result).
    float n = invS * invS;
    float ra = rk_minf(ca * n, 1.0f);
    float rr = 0.0f, rg = 0.0f, rb = 0.0f;
    if (ca > 0.0f) {
        rr = rk_enc(cr / ca, P->gamma) * ra;
        rg = rk_enc(cg / ca, P->gamma) * ra;
        rb = rk_enc(cb / ca, P->gamma) * ra;
    }
    rr *= P->opacity; rg *= P->opacity; rb *= P->opacity; ra *= P->opacity;

    int di = (y * P->dstPitch + x) * 4;
    int sx = x - P->srcX, sy = y - P->srcY;
    float sB = 0.0f, sG = 0.0f, sR = 0.0f, sA = 0.0f;
    if (sx >= 0 && sy >= 0 && sx < P->srcW && sy < P->srcH) {
        int si = (sy * P->srcPitch + sx) * 4;
        sB = src[si]; sG = src[si + 1]; sR = src[si + 2]; sA = src[si + 3];
    }
    float oB, oG, oR, oA;
    if (P->composite == RK_COMPOSITE_ALONE) {
        oB = rb; oG = rg; oR = rr; oA = ra;
    } else if (P->composite == RK_COMPOSITE_BEHIND) {
        float k = 1.0f - sA;
        oB = sB + rb * k; oG = sG + rg * k; oR = sR + rr * k; oA = sA + ra * k;
    } else {
        float k = 1.0f - ra;
        oB = rb + sB * k; oG = rg + sG * k; oR = rr + sR * k; oA = ra + sA * k;
    }
    dst[di] = oB; dst[di + 1] = oG; dst[di + 2] = oR; dst[di + 3] = oA;
}

#endif
