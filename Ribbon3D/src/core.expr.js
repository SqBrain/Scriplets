// Ribbon3D expression core.
// Embedded into Ribbon3D.jsx by tools/build.js and injected into every segment's
// "RB Data" mask path expression, after a generated header that defines:
//   RB_I     segment index (0..RB_N-1)
//   RB_N     number of segments
//   RB_CTRL  controller layer
//   RB_src() returns the path source: {segs:[[p0,p1,p2,p3],...], closed:bool, up:[x,y,z]}
// Written in ES3 so it runs in both the JavaScript and the Legacy ExtendScript
// expression engines. Vectors are plain arrays: "+" on arrays is NOT used,
// because the JavaScript engine would concatenate them as strings.
//
// Output: a path whose vertices pack the per-segment data (see rb_pack).

function rb_v3(a) { return [a[0], a[1], a.length > 2 ? a[2] : 0]; }
function rb_add(a, b) { return [a[0] + b[0], a[1] + b[1], a[2] + b[2]]; }
function rb_sub(a, b) { return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]; }
function rb_mul(a, s) { return [a[0] * s, a[1] * s, a[2] * s]; }
function rb_dot(a, b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }
function rb_cross(a, b) { return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]; }
function rb_len(a) { return Math.sqrt(rb_dot(a, a)); }
function rb_norm(a, fb) { var l = rb_len(a); return l > 1e-9 ? rb_mul(a, 1 / l) : (fb ? fb : [0, 0, 0]); }
function rb_lerp(a, b, t) { return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t]; }
function rb_ortho(v, t) { return rb_sub(v, rb_mul(t, rb_dot(v, t))); }
function rb_clamp(x, a, b) { return x < a ? a : (x > b ? b : x); }
function rb_num(x) { return (typeof x === "number" && isFinite(x)) ? x : 0; }

// ---------------------------------------------------------------- bezier
function rb_bz(s, t) {
    var u = 1 - t, a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
    return [a * s[0][0] + b * s[1][0] + c * s[2][0] + d * s[3][0],
            a * s[0][1] + b * s[1][1] + c * s[2][1] + d * s[3][1],
            a * s[0][2] + b * s[1][2] + c * s[2][2] + d * s[3][2]];
}
function rb_bzd(s, t) {
    var u = 1 - t, a = 3 * u * u, b = 6 * u * t, c = 3 * t * t;
    var d0 = rb_sub(s[1], s[0]), d1 = rb_sub(s[2], s[1]), d2 = rb_sub(s[3], s[2]);
    return [a * d0[0] + b * d1[0] + c * d2[0], a * d0[1] + b * d1[1] + c * d2[1], a * d0[2] + b * d1[2] + c * d2[2]];
}
// Unit tangent; survives zero-length handles (straight AE path segments).
function rb_tan(s, t) {
    var d = rb_bzd(s, t);
    if (rb_len(d) < 1e-6) d = rb_sub(rb_bz(s, Math.min(t + 1e-3, 1)), rb_bz(s, Math.max(t - 1e-3, 0)));
    if (rb_len(d) < 1e-9) d = rb_sub(s[3], s[0]);
    return rb_norm(d, [1, 0, 0]);
}

// ---------------------------------------------------------------- sources
// Up vector of a layer = its local Z axis in world space (works for 2D layers too).
function rb_layerUp(L) {
    var o = rb_v3(L.toWorld([0, 0])), ex = rb_sub(rb_v3(L.toWorld([1, 0])), o), ey = rb_sub(rb_v3(L.toWorld([0, 1])), o);
    return rb_norm(rb_cross(ex, ey), [0, 0, 1]);
}

// Bezier path of a shape layer (with its parent groups' transforms) or a mask.
// G: shape group transform groups, innermost first. P: path property.
function rb_fromPath(L, G, P) {
    var v = P.points(), ti = P.inTangents(), to = P.outTangents(), closed = P.isClosed();
    var o = rb_v3(L.toWorld([0, 0])), ex = rb_sub(rb_v3(L.toWorld([1, 0])), o), ey = rb_sub(rb_v3(L.toWorld([0, 1])), o);
    var gx = [], g, q;
    for (g = 0; g < G.length; g++) {
        q = G[g];
        gx.push({ p: q.position, a: q.anchorPoint, s: q.scale, r: q.rotation * Math.PI / 180 });
    }
    function m(p) {
        var x = p[0], y = p[1], k, t, dx, dy, c, sn;
        for (k = 0; k < gx.length; k++) {
            t = gx[k];
            dx = (x - t.a[0]) * t.s[0] / 100;
            dy = (y - t.a[1]) * t.s[1] / 100;
            c = Math.cos(t.r); sn = Math.sin(t.r);
            x = t.p[0] + dx * c - dy * sn;
            y = t.p[1] + dx * sn + dy * c;
        }
        return [o[0] + ex[0] * x + ey[0] * y, o[1] + ex[1] * x + ey[1] * y, o[2] + ex[2] * x + ey[2] * y];
    }
    var n = v.length, segs = [], cnt = closed ? n : n - 1, i, j;
    for (i = 0; i < cnt; i++) {
        j = (i + 1) % n;
        segs.push([m(v[i]), m([v[i][0] + to[i][0], v[i][1] + to[i][1]]),
                   m([v[j][0] + ti[j][0], v[j][1] + ti[j][1]]), m(v[j])]);
    }
    return { segs: segs, closed: closed, up: rb_norm(rb_cross(ex, ey), [0, 0, 1]) };
}

function rb_lpos(L) {
    try { return rb_v3(L.toWorld(L.anchorPoint)); } catch (e) { return rb_v3(L.toWorld([0, 0, 0])); }
}

// Smooth spline through layers (nulls). Handles follow the neighbour chord,
// scaled by the segment length ("Spline Smooth", 1/3 ~ Catmull-Rom).
function rb_fromLayers(Ls, closed, smooth) {
    var P = [], D = [], segs = [], n, i, j, a, b, dl;
    for (i = 0; i < Ls.length; i++) P.push(rb_lpos(Ls[i]));
    n = P.length;
    if (n < 2) return { segs: [], closed: false, up: [0, 0, 1] };
    if (n < 3) closed = false;
    for (i = 0; i < n; i++) {
        if (closed) { a = P[(i - 1 + n) % n]; b = P[(i + 1) % n]; }
        else { a = P[Math.max(i - 1, 0)]; b = P[Math.min(i + 1, n - 1)]; }
        D.push(rb_norm(rb_sub(b, a), [1, 0, 0]));
    }
    for (i = 0; i < (closed ? n : n - 1); i++) {
        j = (i + 1) % n;
        dl = rb_len(rb_sub(P[j], P[i]));
        segs.push([P[i], rb_add(P[i], rb_mul(D[i], dl * smooth)), rb_sub(P[j], rb_mul(D[j], dl * smooth)), P[j]]);
    }
    return { segs: segs, closed: closed, up: rb_layerUp(Ls[0]) };
}

// ---------------------------------------------------------------- arc length + rotation minimizing frame
function rb_build(src) {
    var segs = src.segs, SUB = Math.max(8, Math.min(32, Math.ceil(256 / Math.max(segs.length, 1))));
    var X = [], Tg = [], S = [], SG = [], TT = [], R = [], acc = 0, k, q, t, p, j, u;
    for (k = 0; k < segs.length; k++) {
        for (q = (k === 0 ? 0 : 1); q <= SUB; q++) {
            t = q / SUB;
            p = rb_bz(segs[k], t);
            if (X.length) acc += rb_len(rb_sub(p, X[X.length - 1]));
            X.push(p); Tg.push(rb_tan(segs[k], t)); S.push(acc); SG.push(k); TT.push(t);
        }
    }
    if (X.length < 2) return null;
    // Seed: the source "up" (layer normal), made perpendicular to the first tangent.
    u = rb_ortho(src.up, Tg[0]);
    if (rb_len(u) < 1e-4) u = rb_ortho([0, -1, 0], Tg[0]);
    if (rb_len(u) < 1e-4) u = rb_ortho([1, 0, 0], Tg[0]);
    R.push(rb_norm(u, [0, 0, 1]));
    // Double reflection method (Wang et al. 2008).
    for (j = 0; j < X.length - 1; j++) {
        var v1 = rb_sub(X[j + 1], X[j]), c1 = rb_dot(v1, v1), r = R[j], tL = Tg[j], v2, c2;
        if (c1 > 1e-12) {
            r = rb_sub(r, rb_mul(v1, 2 * rb_dot(v1, r) / c1));
            tL = rb_sub(tL, rb_mul(v1, 2 * rb_dot(v1, Tg[j]) / c1));
        }
        v2 = rb_sub(Tg[j + 1], tL); c2 = rb_dot(v2, v2);
        if (c2 > 1e-12) r = rb_sub(r, rb_mul(v2, 2 * rb_dot(v2, r) / c2));
        R.push(rb_norm(rb_ortho(r, Tg[j + 1]), R[j]));
    }
    return { segs: segs, X: X, Tg: Tg, S: S, SG: SG, TT: TT, R: R, len: acc, closed: src.closed };
}

// Point, unit tangent and up vector at arc-length fraction s (0..1).
function rb_at(B, s) {
    var S = B.S, d = s * B.len, lo = 0, hi = S.length - 1, mid, j, span, f, sg, ta, t, p, tg, u;
    if (d <= 0) { lo = 0; hi = 1; d = 0; }
    else if (d >= B.len) { lo = S.length - 2; hi = lo + 1; d = B.len; }
    else {
        while (hi - lo > 1) { mid = (lo + hi) >> 1; if (S[mid] <= d) lo = mid; else hi = mid; }
    }
    j = lo; span = S[j + 1] - S[j];
    f = span > 1e-9 ? rb_clamp((d - S[j]) / span, 0, 1) : 0;
    sg = B.SG[j + 1];
    ta = (B.SG[j] === sg) ? B.TT[j] : 0;
    t = ta + (B.TT[j + 1] - ta) * f;
    p = rb_bz(B.segs[sg], t);
    tg = rb_tan(B.segs[sg], t);
    u = rb_norm(rb_ortho(rb_lerp(B.R[j], B.R[j + 1], f), tg), B.R[j]);
    return { p: p, t: tg, u: u };
}

// ---------------------------------------------------------------- orientation
// Layer axes (x along the segment, y across, z normal) -> AE Orientation [x,y,z]
// (AE composes orientation as Rx * Ry * Rz).
function rb_euler(u, v, w) {
    var k = 180 / Math.PI, sb = rb_clamp(w[0], -1, 1), b = Math.asin(sb), a, c;
    if (Math.abs(Math.cos(b)) > 0.0005) {
        c = -Math.atan2(v[0], u[0]);
        a = -Math.atan2(w[1], w[2]);
    } else {
        c = 0;
        a = sb > 0 ? Math.atan2(u[1], v[1]) : Math.atan2(-u[1], v[1]);
    }
    return [a * k, b * k, c * k];
}

// ---------------------------------------------------------------- shading
function rb_lin(c) { return Math.pow(rb_clamp(c, 0, 1), 2.2); }
function rb_enc(c) { return Math.pow(rb_clamp(c, 0, 1), 1 / 2.2); }

function rb_camPos() {
    try {
        var c = thisComp.activeCamera;
        return rb_v3(c.toWorld([0, 0, 0]));
    } catch (e) {
        return [thisComp.width / 2, thisComp.height / 2, -thisComp.width * 50 / 36];
    }
}

function rb_light(C) {
    var L = null, res = { p: [thisComp.width * 0.2, -thisComp.height * 0.2, -thisComp.width * 0.8], c: [1, 1, 1] };
    try { L = C.effect("Light")(1); } catch (e) { L = null; }
    if (L) {
        try { res.p = rb_lpos(L); } catch (e2) {}
        try {
            var lc = L.lightOption.color, li = L.lightOption.intensity / 100;
            res.c = [lc[0] * li, lc[1] * li, lc[2] * li];
        } catch (e3) {}
    }
    return res;
}

// Two-sided Blinn-Phong + anisotropic "satin" (Kajiya-Kay) highlight + rim, in linear light.
function rb_shade(p, n, tg, base, cam, light, m) {
    var V = rb_norm(rb_sub(cam, p), [0, 0, -1]), L = rb_norm(rb_sub(light.p, p), [0, 0, -1]);
    var ndl = rb_dot(n, L), diff = Math.max(ndl, 0), H = rb_norm(rb_add(L, V), n);
    var blinn = Math.pow(Math.max(rb_dot(n, H), 0), m.gloss);
    var th = rb_dot(tg, H), kk = Math.pow(Math.sqrt(Math.max(0, 1 - th * th)), m.gloss * 0.5);
    var spec = (blinn * (1 - m.satin) + kk * m.satin) * rb_clamp(ndl * 4, 0, 1);
    var rim = Math.pow(1 - Math.min(Math.abs(rb_dot(n, V)), 1), 4) * m.rim;
    var out = [], i;
    for (i = 0; i < 3; i++) {
        out.push(rb_enc(rb_lin(base[i]) * (m.amb + m.diff * diff * light.c[i]) + (m.spec * spec + rim) * light.c[i]));
    }
    return out;
}

// ---------------------------------------------------------------- main
function rb_main() {
    var C = RB_CTRL;
    function E(name) { return C.effect(name)(1); }
    var N = RB_N, I = RB_I;
    var src = RB_src(), B = (src && src.segs.length) ? rb_build(src) : null;

    var start = E("Start") / 100, end = E("End") / 100, off = E("Offset") / 100;
    var width = Math.max(0, E("Width")), wS = E("Width Start") / 100, wE = E("Width End") / 100;
    var turns = E("Twist"), twA = E("Twist Angle"), twSp = E("Twist Speed"), twLock = E("Twist Along Path");
    var amp = E("Wave Amp"), wl = Math.max(1, E("Wave Length")), wsp = E("Wave Speed");
    var ov = Math.max(0, E("Overlap"));
    var a = start + off, b = end + off, dir = b >= a ? 1 : -1;
    var uk = [I / N, (I + 1) / N];
    var data = { vis: 0 };

    function wrap(s) {
        if (B.closed) { s = s % 1; if (s < 0) s += 1; return s; }
        return rb_clamp(s, 0, 1);
    }
    function disp(s) {
        var f = rb_at(B, wrap(s)), w = amp * Math.sin(2 * Math.PI * (s * B.len - wsp * time) / wl);
        return { p: rb_add(f.p, rb_mul(f.u, w)), u: f.u, t: f.t };
    }

    if (B && B.len > 1e-6) {
        var e = 0.5 / B.len, P = [], T = [], U = [], W = [], Wd = [], Nn = [], k, s, F, Fa, Fb, th, uu, wf, ww;
        for (k = 0; k < 2; k++) {
            s = a + (b - a) * uk[k];
            F = disp(s); Fa = disp(s - e * dir); Fb = disp(s + e * dir);
            T[k] = rb_norm(rb_sub(Fb.p, Fa.p), rb_mul(F.t, dir));
            uu = rb_norm(rb_ortho(F.u, T[k]), F.u);
            wf = rb_cross(uu, T[k]);
            th = (twA + 360 * turns * (twLock ? s : uk[k]) + twSp * time) * Math.PI / 180;
            W[k] = rb_norm(rb_add(rb_mul(wf, Math.cos(th)), rb_mul(uu, Math.sin(th))), wf);
            ww = width * Math.max(0, wS + (wE - wS) * uk[k]);
            Wd[k] = ww;
            P[k] = F.p;
            U[k] = uu;
            Nn[k] = rb_norm(rb_cross(T[k], W[k]), uu);
        }
        var chord = rb_len(rb_sub(P[1], P[0]));
        var c = rb_lerp(P[0], P[1], 0.5);
        var ax = rb_norm(rb_sub(P[1], P[0]), T[0]);
        var ay = rb_ortho(rb_add(rb_mul(W[0], Wd[0]), rb_mul(W[1], Wd[1])), ax);
        if (rb_len(ay) < 1e-6) ay = rb_ortho(rb_add(W[0], W[1]), ax);
        ay = rb_norm(ay, rb_norm(rb_ortho(U[0], ax), [0, 1, 0]));
        var az = rb_cross(ax, ay), cam = rb_camPos(), fit = E("Camera Fit"), maxShift = 2 * width + 10;
        // A twisted quad is not planar. "Camera Fit" moves each true corner along the camera ray
        // onto the segment plane, so neighbours meet exactly in the image (no stair-steps).
        // A segment seen almost edge-on is first tilted about its chord (by a few degrees at most)
        // so the rays meet its plane at a usable angle; orthogonal projection is the last resort.
        if (fit) {
            var vd = rb_norm(rb_sub(c, cam), [0, 0, 1]), va = rb_dot(vd, az), vb = rb_dot(vd, ay), MIN = 0.05;
            var vr = Math.sqrt(va * va + vb * vb);
            if (Math.abs(va) < MIN && vr > 2 * MIN) {
                var psi = Math.atan2(vb, va), dlt = Math.acos((va < 0 ? -MIN : MIN) / vr), phi = psi - dlt, phi2 = psi + dlt;
                phi = Math.atan2(Math.sin(phi), Math.cos(phi));
                phi2 = Math.atan2(Math.sin(phi2), Math.cos(phi2));
                if (Math.abs(phi2) < Math.abs(phi)) phi = phi2;
                var ay2 = rb_sub(rb_mul(ay, Math.cos(phi)), rb_mul(az, Math.sin(phi)));
                az = rb_add(rb_mul(ay, Math.sin(phi)), rb_mul(az, Math.cos(phi)));
                ay = ay2;
            }
        }
        var loc = function (pt) {
            if (fit) {
                var r = rb_sub(pt, cam), den = rb_dot(r, az), t, moved;
                if (Math.abs(den) > 0.01 * rb_len(r)) {
                    t = rb_dot(rb_sub(c, cam), az) / den;
                    moved = rb_add(cam, rb_mul(r, t));
                    if (t > 0 && rb_len(rb_sub(moved, pt)) < maxShift) pt = moved;
                }
            }
            var d = rb_sub(pt, c);
            return [rb_dot(d, ax), rb_dot(d, ay)];
        };
        var q0 = loc(rb_sub(P[0], rb_mul(W[0], Wd[0] / 2))), q1 = loc(rb_sub(P[1], rb_mul(W[1], Wd[1] / 2)));
        var q2 = loc(rb_add(P[1], rb_mul(W[1], Wd[1] / 2))), q3 = loc(rb_add(P[0], rb_mul(W[0], Wd[0] / 2)));
        if (I > 0) { q0[0] -= ov; q3[0] -= ov; }
        if (I < N - 1) { q1[0] += ov; q2[0] += ov; }

        // Colors: front/back by facing, optional built-in smooth (per-vertex) shading.
        var front = rb_dot(az, rb_sub(cam, c)) < 0;
        var base = front ? E("Front Color") : E("Back Color"), c0 = [base[0], base[1], base[2]], c1 = c0;
        if (E("Built-in Shading")) {
            var light = rb_light(C), sg = front ? -1 : 1;
            var mt = { amb: E("Ambient"), diff: E("Diffuse"), spec: E("Specular"), gloss: Math.max(1, E("Gloss")),
                       satin: rb_clamp(E("Satin"), 0, 1), rim: E("Rim") };
            c0 = rb_shade(P[0], rb_mul(Nn[0], sg), T[0], base, cam, light, mt);
            c1 = rb_shade(P[1], rb_mul(Nn[1], sg), T[1], base, cam, light, mt);
        }
        if (E("Flip Gradient")) { var tmp = c0; c0 = c1; c1 = tmp; }

        data = { vis: (chord > 1e-3 && (Wd[0] + Wd[1]) > 1e-3) ? 1 : 0, c: c, o: rb_euler(ax, ay, az),
                 q: [q0, q1, q2, q3], g: [-chord / 2, chord / 2], c0: c0, c1: c1 };
    }
    data.op = E("Opacity");
    data.u = uk;
    data.wb = width;
    return rb_pack(data);
}

// Vertex layout (each vertex = 2 numbers):
//  0 [cx, cy]  1 [cz, ox]  2 [oy, oz]          position / orientation
//  3..6        quad corners in layer space (start-, end-, end+, start+)
//  7 [g0, g1]                                  gradient start / end x
//  8 [r0, g0]  9 [b0, r1]  10 [g1, b1]          colors at segment start / end
// 11 [opacity, 0]  12 [u0, u1]  13 [baseWidth, 0]
function rb_pack(d) {
    var n = rb_num, pts;
    if (d.vis) {
        pts = [[d.c[0], d.c[1]], [d.c[2], d.o[0]], [d.o[1], d.o[2]],
               d.q[0], d.q[1], d.q[2], d.q[3], [d.g[0], d.g[1]],
               [d.c0[0], d.c0[1]], [d.c0[2], d.c1[0]], [d.c1[1], d.c1[2]]];
    } else {
        pts = [[0, 0], [0, 0], [0, 0], [-1, -1], [1, -1], [1, 1], [-1, 1], [-1, 1], [0, 0], [0, 0], [0, 0]];
    }
    pts.push([d.vis ? d.op : 0, 0], [d.u[0], d.u[1]], [d.wb, 0]);
    for (var i = 0; i < pts.length; i++) pts[i] = [n(pts[i][0]), n(pts[i][1])];
    return createPath(pts, [], [], false);
}
