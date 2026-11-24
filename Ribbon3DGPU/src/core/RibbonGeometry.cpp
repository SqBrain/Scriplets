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
    for (auto& rc : out.rolls) rc.up = normalize(R * rc.up, rc.up);
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
    // Roll controls: signed angle that turns the transported up R onto the requested up around T.
    // With W0 = R x T, the twisted up is R cos(a) - W0 sin(a).
    for (const Polyline::RollControl& rc : p.rolls) {
        if (rc.index < 0 || size_t(rc.index) >= n) continue;
        const V3& T = T_[size_t(rc.index)];
        const V3& R = R_[size_t(rc.index)];
        V3 d = orthoTo(rc.up, T);
        if (rb::length(d) < 1e-6) continue;
        d = normalize(d);
        double a = std::atan2(-dot(d, cross(R, T)), dot(d, R));
        if (!rollA_.empty()) a += 2 * kPi * std::round((rollA_.back() - a) / (2 * kPi));  // shortest turn
        rollS_.push_back(S_[size_t(rc.index)] / len_);
        rollA_.push_back(a);
    }
}

double FramedCurve::rollAt(double s) const {
    const size_t n = rollS_.size();
    if (n == 0) return 0;
    if (closed_) s = s - std::floor(s);
    if (n == 1 || s <= rollS_[0]) return rollA_[0];
    if (s >= rollS_[n - 1]) return rollA_[n - 1];
    size_t i = size_t(std::upper_bound(rollS_.begin(), rollS_.end(), s) - rollS_.begin()) - 1;
    // Cubic Hermite with Catmull-Rom slopes over arc length: smooth twist between nulls.
    auto slope = [&](size_t k) {
        size_t a = k == 0 ? 0 : k - 1, b = k + 1 >= n ? n - 1 : k + 1;
        double ds = rollS_[b] - rollS_[a];
        return ds > 1e-12 ? (rollA_[b] - rollA_[a]) / ds : 0.0;
    };
    double h = rollS_[i + 1] - rollS_[i];
    if (h <= 1e-12) return rollA_[i + 1];
    double t = (s - rollS_[i]) / h, t2 = t * t, t3 = t2 * t;
    return rollA_[i] * (2 * t3 - 3 * t2 + 1) + slope(i) * h * (t3 - 2 * t2 + t) + rollA_[i + 1] * (-2 * t3 + 3 * t2) +
           slope(i + 1) * h * (t3 - t2);
}

Polyline controlPointsToPolyline(const std::vector<ControlPoint>& cps, bool closed, int samplesPerSegment) {
    const size_t n = cps.size();
    if (n < 3) closed = false;
    std::vector<Bezier> segs;
    for (size_t i = 0; i + 1 < n || (closed && i < n); i++) {
        const ControlPoint& a = cps[i];
        const ControlPoint& b = cps[(i + 1) % n];
        double chord = rb::length(b.pos - a.pos) / 3;
        segs.push_back({a.pos, a.pos + a.axisX * (a.handleScale * chord), b.pos - b.axisX * (b.handleScale * chord), b.pos});
    }
    const int sps = std::max(1, samplesPerSegment);
    Polyline p = bezierToPolyline(segs, closed, n ? cps[0].axisZ : V3(0, 0, 1), sps);
    for (size_t i = 0; i < n; i++) p.rolls.push_back({int(i) * sps, cps[i].axisZ});
    if (closed && n) p.rolls.push_back({int(n) * sps, cps[0].axisZ});
    return p;
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

std::vector<ProfilePoint> ribbonProfile(double width, double thickness, double bevel, int bevelSegments, bool chamfer,
                                        int faceSegments) {
    const double hx = std::max(0.0, width) / 2, hy = std::max(0.0, thickness) / 2;
    const double r = std::clamp(bevel, 0.0, std::min(hx, hy));
    const int fs = std::max(1, faceSegments), as = chamfer ? 1 : std::max(1, bevelSegments);
    std::vector<ProfilePoint> out;
    auto add = [&](double x, double y, double nx, double ny) { out.push_back({x, y, nx, ny}); };
    // Corner around (cx, cy) from angle a0 to a1 (degrees). Round: smooth normals. Chamfer: one flat
    // facet with its own normal (duplicated points give hard edges against the faces).
    auto corner = [&](double cx, double cy, double a0, double a1) {
        double r0 = a0 * kPi / 180, r1 = a1 * kPi / 180;
        if (chamfer) {
            double m = (r0 + r1) / 2;
            add(cx + r * std::cos(r0), cy + r * std::sin(r0), std::cos(m), std::sin(m));
            add(cx + r * std::cos(r1), cy + r * std::sin(r1), std::cos(m), std::sin(m));
            return;
        }
        for (int j = 0; j <= as; j++) {
            double t = r0 + (r1 - r0) * j / as;
            add(cx + r * std::cos(t), cy + r * std::sin(t), std::cos(t), std::sin(t));
        }
    };
    const double ix = hx - r, iy = hy - r;
    for (int i = 0; i <= fs; i++) add(-ix + 2 * ix * i / fs, hy, 0, 1);   // front face, left to right
    corner(ix, iy, 90, 0);
    add(hx, iy, 1, 0);                                                    // right edge
    add(hx, -iy, 1, 0);
    corner(ix, -iy, 0, -90);
    for (int i = 0; i <= fs; i++) add(ix - 2 * ix * i / fs, -hy, 0, -1);  // back face, right to left
    corner(-ix, -iy, -90, -180);
    add(-hx, -iy, -1, 0);                                                 // left edge
    add(-hx, iy, -1, 0);
    corner(-ix, iy, 180, 90);
    return out;
}

static RK_Vertex makeVertex(const V3& p, const V3& n, const V3& t, double u, double v, double side) {
    RK_Vertex vx;
    vx.pos[0] = float(p.x); vx.pos[1] = float(p.y); vx.pos[2] = float(p.z);
    vx.nrm[0] = float(n.x); vx.nrm[1] = float(n.y); vx.nrm[2] = float(n.z);
    vx.tan[0] = float(t.x); vx.tan[1] = float(t.y); vx.tan[2] = float(t.z);
    vx.uv[0] = float(u); vx.uv[1] = float(v);
    vx.side = float(side);
    return vx;
}

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

    // Cross sections: center, tangent, width direction W, front normal F = W x T, width.
    std::vector<V3> fronts;
    std::vector<double> us;
    for (int k = 0; k <= N; k++) {
        double u = double(k) / N, s = a + (b - a) * u;
        Sample F = disp(s), Fa = disp(s - e * dir), Fb = disp(s + e * dir);
        V3 T = normalize(Fb.p - Fa.p, F.t * dir);
        V3 U = normalize(orthoTo(F.u, T), F.u);
        V3 Wf = cross(U, T);
        double th = (rs.twistAngleDeg + 360 * rs.twistTurns * (rs.twistAlongPath ? s : u) + rs.twistSpeedDeg * rs.time) * kPi / 180;
        th += C.rollAt(s);  // control nulls' rotation
        V3 W = normalize(Wf * std::cos(th) + U * std::sin(th), Wf);
        double w = rs.width * std::max(0.0, rs.widthStart + (rs.widthEnd - rs.widthStart) * u);
        mesh.center.push_back(F.p);
        mesh.tangent.push_back(T);
        mesh.widthDir.push_back(W);
        mesh.widths.push_back(w);
        fronts.push_back(normalize(cross(W, T), U * -1.0));
        us.push_back(u);
    }

    if (rs.thickness <= 0) {
        // Thin two-sided sheet.
        mesh.verts.reserve(size_t(N + 1) * (M + 1));
        for (int k = 0; k <= N; k++)
            for (int j = 0; j <= M; j++) {
                double v = double(j) / M;
                mesh.verts.push_back(makeVertex(mesh.center[k] + mesh.widthDir[k] * ((v - 0.5) * mesh.widths[k]), fronts[k],
                                                mesh.tangent[k], us[k], v, 0));
            }
        mesh.tris.reserve(size_t(N) * M * 6);
        for (int k = 0; k < N; k++)
            for (int j = 0; j < M; j++) {
                int i00 = k * (M + 1) + j, i10 = (k + 1) * (M + 1) + j, i11 = i10 + 1, i01 = i00 + 1;
                int t[6] = {i00, i10, i11, i00, i11, i01};
                mesh.tris.insert(mesh.tris.end(), t, t + 6);
            }
        return mesh;
    }

    // Solid: the beveled profile swept along the path. Each section uses its own (tapered) width.
    mesh.solid = true;
    const int R = int(ribbonProfile(1, 1, 0, rs.bevelSegments, rs.chamfer, M).size());
    mesh.verts.reserve(size_t(N + 1) * R + 2 * (R + 1));
    for (int k = 0; k <= N; k++) {
        const double w = mesh.widths[k];
        std::vector<ProfilePoint> prof = ribbonProfile(w, rs.thickness, rs.bevel, rs.bevelSegments, rs.chamfer, M);
        for (const ProfilePoint& q : prof) {
            V3 p = mesh.center[k] + mesh.widthDir[k] * q.x + fronts[k] * q.y;
            V3 n = normalize(mesh.widthDir[k] * q.nx + fronts[k] * q.ny, fronts[k]);
            double v = w > 1e-9 ? std::clamp(q.x / w + 0.5, 0.0, 1.0) : 0.5;
            mesh.verts.push_back(makeVertex(p, n, mesh.tangent[k], us[k], v, q.ny));
        }
    }
    mesh.tris.reserve(size_t(N) * R * 6 + 6 * R);
    for (int k = 0; k < N; k++)
        for (int i = 0; i < R; i++) {
            int i2 = (i + 1) % R;
            int i00 = k * R + i, i01 = k * R + i2, i10 = (k + 1) * R + i, i11 = (k + 1) * R + i2;
            int t[6] = {i00, i10, i11, i00, i11, i01};
            mesh.tris.insert(mesh.tris.end(), t, t + 6);
        }
    if (rs.caps) {
        // Flat end caps: fan around the section center, normal along -T (start) / +T (end), edge color.
        for (int end = 0; end < 2; end++) {
            const int k = end ? N : 0;
            const V3 n = mesh.tangent[k] * (end ? 1.0 : -1.0);
            const int c = int(mesh.verts.size());
            mesh.verts.push_back(makeVertex(mesh.center[k], n, mesh.widthDir[k], us[k], 0.5, 0));
            for (int i = 0; i < R; i++) {
                RK_Vertex vx = mesh.verts[size_t(k * R + i)];
                vx.nrm[0] = float(n.x); vx.nrm[1] = float(n.y); vx.nrm[2] = float(n.z);
                vx.side = 0;
                mesh.verts.push_back(vx);
            }
            for (int i = 0; i < R; i++) {
                int t[3] = {c, c + 1 + i, c + 1 + (i + 1) % R};
                mesh.tris.insert(mesh.tris.end(), t, t + 3);
            }
        }
    }
    return mesh;
}

}  // namespace rb
