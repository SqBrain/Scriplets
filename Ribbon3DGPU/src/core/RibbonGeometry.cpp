#include "RibbonGeometry.h"

#include <algorithm>

namespace rb {

static const double kPi = 3.14159265358979323846;

M3 rotationXYZ(double xDeg, double yDeg, double zDeg) {
    double a = xDeg * kPi / 180, b = yDeg * kPi / 180, c = zDeg * kPi / 180;
    M3 rx, ry, rz;
    rx.m[1][1] = std::cos(a); rx.m[1][2] = -std::sin(a); rx.m[2][1] = std::sin(a); rx.m[2][2] = std::cos(a);
    ry.m[0][0] = std::cos(b); ry.m[0][2] = std::sin(b); ry.m[2][0] = -std::sin(b); ry.m[2][2] = std::cos(b);
    rz.m[0][0] = std::cos(c); rz.m[0][1] = -std::sin(c); rz.m[1][0] = std::sin(c); rz.m[1][1] = std::cos(c);
    return rx * ry * rz;
}

static V3 bez(const Bezier& s, double t) {
    double u = 1 - t;
    return s[0] * (u * u * u) + s[1] * (3 * u * u * t) + s[2] * (3 * u * t * t) + s[3] * (t * t * t);
}

Polyline bezierToPolyline(const std::vector<Bezier>& segs, bool closed, const V3& up, int samplesPerSegment) {
    Polyline p;
    p.closed = closed;
    p.up = up;
    int n = std::max(1, samplesPerSegment);
    for (size_t k = 0; k < segs.size(); k++)
        for (int q = (k == 0 ? 0 : 1); q <= n; q++) p.pts.push_back(bez(segs[k], double(q) / n));
    return p;
}

Polyline transformPolyline(const Polyline& in, const PathTransform& xf) {
    M3 R = rotationXYZ(xf.rotX, xf.rotY, xf.rotZ);
    Polyline out = in;
    for (auto& p : out.pts) p = xf.pivot + R * ((p - xf.pivot) * xf.scale) + xf.offset;
    out.up = normalize(R * in.up, in.up);
    return out;
}

// ------------------------------------------------------------------ FramedCurve

FramedCurve::FramedCurve(const Polyline& p) : X_(p.pts), closed_(p.closed) {
    size_t n = X_.size();
    if (n < 2) return;
    S_.resize(n);
    S_[0] = 0;
    for (size_t i = 1; i < n; i++) S_[i] = S_[i - 1] + rb::length(X_[i] - X_[i - 1]);
    len_ = S_[n - 1];
    if (len_ <= 1e-9) return;
    // A closed polyline repeats its first point at the end.
    bool loop = closed_ && rb::length(X_[0] - X_[n - 1]) < 1e-6 && n > 3;
    // Tangents: derivative of the quadratic through three neighbours, parametrised by chord
    // length (second order, also at the ends of open paths and for uneven spacing).
    T_.resize(n);
    V3 last(1, 0, 0);
    auto d3 = [](const V3& p0, const V3& p1, const V3& p2, int at) {
        double h1 = rb::length(p1 - p0), h2 = rb::length(p2 - p1);
        if (h1 < 1e-12 || h2 < 1e-12) return p2 - p0;
        double s = h1 + h2;
        if (at == 0) return p0 * (-(2 * h1 + h2) / (h1 * s)) + p1 * (s / (h1 * h2)) + p2 * (-h1 / (h2 * s));
        if (at == 1) return p0 * (-h2 / (h1 * s)) + p1 * ((h2 - h1) / (h1 * h2)) + p2 * (h1 / (h2 * s));
        return p0 * (h2 / (h1 * s)) + p1 * (-s / (h1 * h2)) + p2 * ((2 * h2 + h1) / (h2 * s));
    };
    for (size_t i = 0; i < n; i++) {
        V3 d;
        if (loop) {
            d = d3((i == 0) ? X_[n - 2] : X_[i - 1], X_[i], (i == n - 1) ? X_[1] : X_[i + 1], 1);
        } else if (n == 2) {
            d = X_[1] - X_[0];
        } else if (i == 0) {
            d = d3(X_[0], X_[1], X_[2], 0);
        } else if (i == n - 1) {
            d = d3(X_[n - 3], X_[n - 2], X_[n - 1], 2);
        } else {
            d = d3(X_[i - 1], X_[i], X_[i + 1], 1);
        }
        T_[i] = normalize(d, last);
        last = T_[i];
    }
    // Rotation minimizing frame, double reflection method (Wang et al. 2008).
    V3 u = orthoTo(p.up, T_[0]);
    if (rb::length(u) < 1e-4) u = orthoTo(V3(0, -1, 0), T_[0]);
    if (rb::length(u) < 1e-4) u = orthoTo(V3(1, 0, 0), T_[0]);
    R_.resize(n);
    R_[0] = normalize(u);
    for (size_t j = 0; j + 1 < n; j++) {
        V3 v1 = X_[j + 1] - X_[j], r = R_[j], tL = T_[j];
        double c1 = dot(v1, v1);
        if (c1 > 1e-12) {
            r = r - v1 * (2 * dot(v1, r) / c1);
            tL = tL - v1 * (2 * dot(v1, T_[j]) / c1);
        }
        V3 v2 = T_[j + 1] - tL;
        double c2 = dot(v2, v2);
        if (c2 > 1e-12) r = r - v2 * (2 * dot(v2, r) / c2);
        R_[j + 1] = normalize(orthoTo(r, T_[j + 1]), R_[j]);
    }
}

void FramedCurve::at(double s, V3& pos, V3& tangent, V3& up) const {
    if (closed_) s = s - std::floor(s);
    else s = std::clamp(s, 0.0, 1.0);
    double d = s * len_;
    size_t j = size_t(std::upper_bound(S_.begin(), S_.end(), d) - S_.begin());
    j = (j == 0) ? 0 : j - 1;
    if (j >= S_.size() - 1) j = S_.size() - 2;
    double span = S_[j + 1] - S_[j];
    double f = span > 1e-12 ? std::clamp((d - S_[j]) / span, 0.0, 1.0) : 0.0;
    // Cubic Hermite between samples (tangents scaled by the chord): O(h^4) instead of the chord's O(h^2).
    double f2 = f * f, f3 = f2 * f;
    pos = X_[j] * (2 * f3 - 3 * f2 + 1) + T_[j] * ((f3 - 2 * f2 + f) * span) + X_[j + 1] * (-2 * f3 + 3 * f2) +
          T_[j + 1] * ((f3 - f2) * span);
    tangent = normalize(lerp(T_[j], T_[j + 1], f), T_[j]);
    up = normalize(orthoTo(lerp(R_[j], R_[j + 1], f), tangent), R_[j]);
}

// ------------------------------------------------------------------ ribbon

RibbonMesh buildRibbon(const Polyline& path, const RibbonSettings& rs) {
    RibbonMesh mesh;
    FramedCurve C(path);
    if (!C.valid()) return mesh;
    const int N = std::max(1, rs.segments), M = std::max(1, rs.across);
    const double a = rs.start + rs.offset, b = rs.end + rs.offset;
    if (std::fabs(b - a) < 1e-9) return mesh;
    const double dir = b >= a ? 1 : -1;
    const double L = C.length(), wl = std::max(1.0, rs.waveLength), e = 0.5 / L;

    struct Sample { V3 p, t, u; };
    auto disp = [&](double s) {
        Sample r;
        C.at(s, r.p, r.t, r.u);
        double w = rs.waveAmp * std::sin(2 * kPi * (s * L - rs.waveSpeed * rs.time) / wl);
        r.p = r.p + r.u * w;
        return r;
    };

    mesh.verts.reserve(size_t(N + 1) * (M + 1));
    for (int k = 0; k <= N; k++) {
        double u = double(k) / N, s = a + (b - a) * u;
        Sample F = disp(s), Fa = disp(s - e * dir), Fb = disp(s + e * dir);
        V3 T = normalize(Fb.p - Fa.p, F.t * dir);
        V3 U = normalize(orthoTo(F.u, T), F.u);
        V3 Wf = cross(U, T);
        double th = (rs.twistAngleDeg + 360 * rs.twistTurns * (rs.twistAlongPath ? s : u) + rs.twistSpeedDeg * rs.time) * kPi / 180;
        V3 W = normalize(Wf * std::cos(th) + U * std::sin(th), Wf);
        double w = rs.width * std::max(0.0, rs.widthStart + (rs.widthEnd - rs.widthStart) * u);
        V3 front = normalize(cross(W, T), U * -1.0);
        mesh.center.push_back(F.p);
        mesh.tangent.push_back(T);
        mesh.widthDir.push_back(W);
        mesh.widths.push_back(w);
        for (int j = 0; j <= M; j++) {
            double v = double(j) / M;
            V3 p = F.p + W * ((v - 0.5) * w);
            RK_Vertex vx;
            vx.pos[0] = float(p.x); vx.pos[1] = float(p.y); vx.pos[2] = float(p.z);
            vx.nrm[0] = float(front.x); vx.nrm[1] = float(front.y); vx.nrm[2] = float(front.z);
            vx.tan[0] = float(T.x); vx.tan[1] = float(T.y); vx.tan[2] = float(T.z);
            vx.uv[0] = float(u); vx.uv[1] = float(v);
            vx.pad = 0;
            mesh.verts.push_back(vx);
        }
    }
    mesh.tris.reserve(size_t(N) * M * 6);
    for (int k = 0; k < N; k++) {
        for (int j = 0; j < M; j++) {
            int i00 = k * (M + 1) + j, i10 = (k + 1) * (M + 1) + j, i11 = i10 + 1, i01 = i00 + 1;
            int t[6] = {i00, i10, i11, i00, i11, i01};
            mesh.tris.insert(mesh.tris.end(), t, t + 6);
        }
    }
    return mesh;
}

}  // namespace rb
