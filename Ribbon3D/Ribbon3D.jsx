/*
    Ribbon 3D for After Effects
    Realistic 3D ribbons along a path: twist, taper, waves, two-sided color,
    smooth (Gouraud) shading with satin highlights, or texture mapping.

    Install: copy this file to
        <After Effects>/Scripts/ScriptUI Panels/
    and open it from the Window menu (dockable panel), or run it via
    File > Scripts > Run Script File (floating palette).

    How it works: the ribbon is a chain of thin 3D layers (segments). Every
    segment carries one "data" expression (mask "RB Data", mode None) that
    samples the path, builds a rotation-minimizing frame, applies twist/wave/
    taper and lighting, and packs the result into the mask's vertices. The
    segment's transform, shape and colors read that data with tiny expressions.
    All parameters live on the controller null, so everything stays live and
    animatable. AE camera, lights, DOF, motion blur and 3D renderers just work.

    The expression core between the EXPR_CORE markers is generated from
    src/core.expr.js by tools/build.js. Edit the source and rebuild.
*/
(function (thisObj) {
    var RB3D = {};
    RB3D.VERSION = "1.0.0";
    RB3D.NAME = "Ribbon 3D";
    RB3D.DATA_MASK = "RB Data";
    RB3D.QUAD_MASK = "RB Quad";
    RB3D.TAG_CTRL = "RB3D-CTRL";
    RB3D.TAG_SEG = "RB3D-SEG";
    RB3D.TAG_LIGHT = "RB3D-LIGHT";

    // <EXPR_CORE>
    // Generated from src/core.expr.js by tools/build.js. Do not edit by hand.
    RB3D.CORE = [
        "function rb_v3(a) { return [a[0], a[1], a.length > 2 ? a[2] : 0]; }",
        "function rb_add(a, b) { return [a[0] + b[0], a[1] + b[1], a[2] + b[2]]; }",
        "function rb_sub(a, b) { return [a[0] - b[0], a[1] - b[1], a[2] - b[2]]; }",
        "function rb_mul(a, s) { return [a[0] * s, a[1] * s, a[2] * s]; }",
        "function rb_dot(a, b) { return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]; }",
        "function rb_cross(a, b) { return [a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]]; }",
        "function rb_len(a) { return Math.sqrt(rb_dot(a, a)); }",
        "function rb_norm(a, fb) { var l = rb_len(a); return l > 1e-9 ? rb_mul(a, 1 / l) : (fb ? fb : [0, 0, 0]); }",
        "function rb_lerp(a, b, t) { return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t, a[2] + (b[2] - a[2]) * t]; }",
        "function rb_ortho(v, t) { return rb_sub(v, rb_mul(t, rb_dot(v, t))); }",
        "function rb_clamp(x, a, b) { return x < a ? a : (x > b ? b : x); }",
        "function rb_num(x) { return (typeof x === \"number\" && isFinite(x)) ? x : 0; }",
        "function rb_bz(s, t) {",
        "var u = 1 - t, a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;",
        "return [a * s[0][0] + b * s[1][0] + c * s[2][0] + d * s[3][0],",
        "a * s[0][1] + b * s[1][1] + c * s[2][1] + d * s[3][1],",
        "a * s[0][2] + b * s[1][2] + c * s[2][2] + d * s[3][2]];",
        "}",
        "function rb_bzd(s, t) {",
        "var u = 1 - t, a = 3 * u * u, b = 6 * u * t, c = 3 * t * t;",
        "var d0 = rb_sub(s[1], s[0]), d1 = rb_sub(s[2], s[1]), d2 = rb_sub(s[3], s[2]);",
        "return [a * d0[0] + b * d1[0] + c * d2[0], a * d0[1] + b * d1[1] + c * d2[1], a * d0[2] + b * d1[2] + c * d2[2]];",
        "}",
        "function rb_tan(s, t) {",
        "var d = rb_bzd(s, t);",
        "if (rb_len(d) < 1e-6) d = rb_sub(rb_bz(s, Math.min(t + 1e-3, 1)), rb_bz(s, Math.max(t - 1e-3, 0)));",
        "if (rb_len(d) < 1e-9) d = rb_sub(s[3], s[0]);",
        "return rb_norm(d, [1, 0, 0]);",
        "}",
        "function rb_layerUp(L) {",
        "var o = rb_v3(L.toWorld([0, 0])), ex = rb_sub(rb_v3(L.toWorld([1, 0])), o), ey = rb_sub(rb_v3(L.toWorld([0, 1])), o);",
        "return rb_norm(rb_cross(ex, ey), [0, 0, 1]);",
        "}",
        "function rb_fromPath(L, G, P) {",
        "var v = P.points(), ti = P.inTangents(), to = P.outTangents(), closed = P.isClosed();",
        "var o = rb_v3(L.toWorld([0, 0])), ex = rb_sub(rb_v3(L.toWorld([1, 0])), o), ey = rb_sub(rb_v3(L.toWorld([0, 1])), o);",
        "var gx = [], g, q;",
        "for (g = 0; g < G.length; g++) {",
        "q = G[g];",
        "gx.push({ p: q.position, a: q.anchorPoint, s: q.scale, r: q.rotation * Math.PI / 180 });",
        "}",
        "function m(p) {",
        "var x = p[0], y = p[1], k, t, dx, dy, c, sn;",
        "for (k = 0; k < gx.length; k++) {",
        "t = gx[k];",
        "dx = (x - t.a[0]) * t.s[0] / 100;",
        "dy = (y - t.a[1]) * t.s[1] / 100;",
        "c = Math.cos(t.r); sn = Math.sin(t.r);",
        "x = t.p[0] + dx * c - dy * sn;",
        "y = t.p[1] + dx * sn + dy * c;",
        "}",
        "return [o[0] + ex[0] * x + ey[0] * y, o[1] + ex[1] * x + ey[1] * y, o[2] + ex[2] * x + ey[2] * y];",
        "}",
        "var n = v.length, segs = [], cnt = closed ? n : n - 1, i, j;",
        "for (i = 0; i < cnt; i++) {",
        "j = (i + 1) % n;",
        "segs.push([m(v[i]), m([v[i][0] + to[i][0], v[i][1] + to[i][1]]),",
        "m([v[j][0] + ti[j][0], v[j][1] + ti[j][1]]), m(v[j])]);",
        "}",
        "return { segs: segs, closed: closed, up: rb_norm(rb_cross(ex, ey), [0, 0, 1]) };",
        "}",
        "function rb_lpos(L) {",
        "try { return rb_v3(L.toWorld(L.anchorPoint)); } catch (e) { return rb_v3(L.toWorld([0, 0, 0])); }",
        "}",
        "function rb_fromLayers(Ls, closed, smooth) {",
        "var P = [], D = [], segs = [], n, i, j, a, b, dl;",
        "for (i = 0; i < Ls.length; i++) P.push(rb_lpos(Ls[i]));",
        "n = P.length;",
        "if (n < 2) return { segs: [], closed: false, up: [0, 0, 1] };",
        "if (n < 3) closed = false;",
        "for (i = 0; i < n; i++) {",
        "if (closed) { a = P[(i - 1 + n) % n]; b = P[(i + 1) % n]; }",
        "else { a = P[Math.max(i - 1, 0)]; b = P[Math.min(i + 1, n - 1)]; }",
        "D.push(rb_norm(rb_sub(b, a), [1, 0, 0]));",
        "}",
        "for (i = 0; i < (closed ? n : n - 1); i++) {",
        "j = (i + 1) % n;",
        "dl = rb_len(rb_sub(P[j], P[i]));",
        "segs.push([P[i], rb_add(P[i], rb_mul(D[i], dl * smooth)), rb_sub(P[j], rb_mul(D[j], dl * smooth)), P[j]]);",
        "}",
        "return { segs: segs, closed: closed, up: rb_layerUp(Ls[0]) };",
        "}",
        "function rb_build(src) {",
        "var segs = src.segs, SUB = Math.max(8, Math.min(32, Math.ceil(256 / Math.max(segs.length, 1))));",
        "var X = [], Tg = [], S = [], SG = [], TT = [], R = [], acc = 0, k, q, t, p, j, u;",
        "for (k = 0; k < segs.length; k++) {",
        "for (q = (k === 0 ? 0 : 1); q <= SUB; q++) {",
        "t = q / SUB;",
        "p = rb_bz(segs[k], t);",
        "if (X.length) acc += rb_len(rb_sub(p, X[X.length - 1]));",
        "X.push(p); Tg.push(rb_tan(segs[k], t)); S.push(acc); SG.push(k); TT.push(t);",
        "}",
        "}",
        "if (X.length < 2) return null;",
        "u = rb_ortho(src.up, Tg[0]);",
        "if (rb_len(u) < 1e-4) u = rb_ortho([0, -1, 0], Tg[0]);",
        "if (rb_len(u) < 1e-4) u = rb_ortho([1, 0, 0], Tg[0]);",
        "R.push(rb_norm(u, [0, 0, 1]));",
        "for (j = 0; j < X.length - 1; j++) {",
        "var v1 = rb_sub(X[j + 1], X[j]), c1 = rb_dot(v1, v1), r = R[j], tL = Tg[j], v2, c2;",
        "if (c1 > 1e-12) {",
        "r = rb_sub(r, rb_mul(v1, 2 * rb_dot(v1, r) / c1));",
        "tL = rb_sub(tL, rb_mul(v1, 2 * rb_dot(v1, Tg[j]) / c1));",
        "}",
        "v2 = rb_sub(Tg[j + 1], tL); c2 = rb_dot(v2, v2);",
        "if (c2 > 1e-12) r = rb_sub(r, rb_mul(v2, 2 * rb_dot(v2, r) / c2));",
        "R.push(rb_norm(rb_ortho(r, Tg[j + 1]), R[j]));",
        "}",
        "return { segs: segs, X: X, Tg: Tg, S: S, SG: SG, TT: TT, R: R, len: acc, closed: src.closed };",
        "}",
        "function rb_at(B, s) {",
        "var S = B.S, d = s * B.len, lo = 0, hi = S.length - 1, mid, j, span, f, sg, ta, t, p, tg, u;",
        "if (d <= 0) { lo = 0; hi = 1; d = 0; }",
        "else if (d >= B.len) { lo = S.length - 2; hi = lo + 1; d = B.len; }",
        "else {",
        "while (hi - lo > 1) { mid = (lo + hi) >> 1; if (S[mid] <= d) lo = mid; else hi = mid; }",
        "}",
        "j = lo; span = S[j + 1] - S[j];",
        "f = span > 1e-9 ? rb_clamp((d - S[j]) / span, 0, 1) : 0;",
        "sg = B.SG[j + 1];",
        "ta = (B.SG[j] === sg) ? B.TT[j] : 0;",
        "t = ta + (B.TT[j + 1] - ta) * f;",
        "p = rb_bz(B.segs[sg], t);",
        "tg = rb_tan(B.segs[sg], t);",
        "u = rb_norm(rb_ortho(rb_lerp(B.R[j], B.R[j + 1], f), tg), B.R[j]);",
        "return { p: p, t: tg, u: u };",
        "}",
        "function rb_euler(u, v, w) {",
        "var k = 180 / Math.PI, sb = rb_clamp(w[0], -1, 1), b = Math.asin(sb), a, c;",
        "if (Math.abs(Math.cos(b)) > 0.0005) {",
        "c = -Math.atan2(v[0], u[0]);",
        "a = -Math.atan2(w[1], w[2]);",
        "} else {",
        "c = 0;",
        "a = sb > 0 ? Math.atan2(u[1], v[1]) : Math.atan2(-u[1], v[1]);",
        "}",
        "return [a * k, b * k, c * k];",
        "}",
        "function rb_lin(c) { return Math.pow(rb_clamp(c, 0, 1), 2.2); }",
        "function rb_enc(c) { return Math.pow(rb_clamp(c, 0, 1), 1 / 2.2); }",
        "function rb_camPos() {",
        "try {",
        "var c = thisComp.activeCamera;",
        "return rb_v3(c.toWorld([0, 0, 0]));",
        "} catch (e) {",
        "return [thisComp.width / 2, thisComp.height / 2, -thisComp.width * 50 / 36];",
        "}",
        "}",
        "function rb_light(C) {",
        "var L = null, res = { p: [thisComp.width * 0.2, -thisComp.height * 0.2, -thisComp.width * 0.8], c: [1, 1, 1] };",
        "try { L = C.effect(\"Light\")(1); } catch (e) { L = null; }",
        "if (L) {",
        "try { res.p = rb_lpos(L); } catch (e2) {}",
        "try {",
        "var lc = L.lightOption.color, li = L.lightOption.intensity / 100;",
        "res.c = [lc[0] * li, lc[1] * li, lc[2] * li];",
        "} catch (e3) {}",
        "}",
        "return res;",
        "}",
        "function rb_shade(p, n, tg, base, cam, light, m) {",
        "var V = rb_norm(rb_sub(cam, p), [0, 0, -1]), L = rb_norm(rb_sub(light.p, p), [0, 0, -1]);",
        "var ndl = rb_dot(n, L), diff = Math.max(ndl, 0), H = rb_norm(rb_add(L, V), n);",
        "var blinn = Math.pow(Math.max(rb_dot(n, H), 0), m.gloss);",
        "var th = rb_dot(tg, H), kk = Math.pow(Math.sqrt(Math.max(0, 1 - th * th)), m.gloss * 0.5);",
        "var spec = (blinn * (1 - m.satin) + kk * m.satin) * rb_clamp(ndl * 4, 0, 1);",
        "var rim = Math.pow(1 - Math.min(Math.abs(rb_dot(n, V)), 1), 4) * m.rim;",
        "var out = [], i;",
        "for (i = 0; i < 3; i++) {",
        "out.push(rb_enc(rb_lin(base[i]) * (m.amb + m.diff * diff * light.c[i]) + (m.spec * spec + rim) * light.c[i]));",
        "}",
        "return out;",
        "}",
        "function rb_main() {",
        "var C = RB_CTRL;",
        "function E(name) { return C.effect(name)(1); }",
        "var N = RB_N, I = RB_I;",
        "var src = RB_src(), B = (src && src.segs.length) ? rb_build(src) : null;",
        "var start = E(\"Start\") / 100, end = E(\"End\") / 100, off = E(\"Offset\") / 100;",
        "var width = Math.max(0, E(\"Width\")), wS = E(\"Width Start\") / 100, wE = E(\"Width End\") / 100;",
        "var turns = E(\"Twist\"), twA = E(\"Twist Angle\"), twSp = E(\"Twist Speed\"), twLock = E(\"Twist Along Path\");",
        "var amp = E(\"Wave Amp\"), wl = Math.max(1, E(\"Wave Length\")), wsp = E(\"Wave Speed\");",
        "var ov = Math.max(0, E(\"Overlap\"));",
        "var a = start + off, b = end + off, dir = b >= a ? 1 : -1;",
        "var uk = [I / N, (I + 1) / N];",
        "var data = { vis: 0 };",
        "function wrap(s) {",
        "if (B.closed) { s = s % 1; if (s < 0) s += 1; return s; }",
        "return rb_clamp(s, 0, 1);",
        "}",
        "function disp(s) {",
        "var f = rb_at(B, wrap(s)), w = amp * Math.sin(2 * Math.PI * (s * B.len - wsp * time) / wl);",
        "return { p: rb_add(f.p, rb_mul(f.u, w)), u: f.u, t: f.t };",
        "}",
        "if (B && B.len > 1e-6) {",
        "var e = 0.5 / B.len, P = [], T = [], U = [], W = [], Wd = [], Nn = [], k, s, F, Fa, Fb, th, uu, wf, ww;",
        "for (k = 0; k < 2; k++) {",
        "s = a + (b - a) * uk[k];",
        "F = disp(s); Fa = disp(s - e * dir); Fb = disp(s + e * dir);",
        "T[k] = rb_norm(rb_sub(Fb.p, Fa.p), rb_mul(F.t, dir));",
        "uu = rb_norm(rb_ortho(F.u, T[k]), F.u);",
        "wf = rb_cross(uu, T[k]);",
        "th = (twA + 360 * turns * (twLock ? s : uk[k]) + twSp * time) * Math.PI / 180;",
        "W[k] = rb_norm(rb_add(rb_mul(wf, Math.cos(th)), rb_mul(uu, Math.sin(th))), wf);",
        "ww = width * Math.max(0, wS + (wE - wS) * uk[k]);",
        "Wd[k] = ww;",
        "P[k] = F.p;",
        "U[k] = uu;",
        "Nn[k] = rb_norm(rb_cross(T[k], W[k]), uu);",
        "}",
        "var chord = rb_len(rb_sub(P[1], P[0]));",
        "var c = rb_lerp(P[0], P[1], 0.5);",
        "var ax = rb_norm(rb_sub(P[1], P[0]), T[0]);",
        "var ay = rb_ortho(rb_add(rb_mul(W[0], Wd[0]), rb_mul(W[1], Wd[1])), ax);",
        "if (rb_len(ay) < 1e-6) ay = rb_ortho(rb_add(W[0], W[1]), ax);",
        "ay = rb_norm(ay, rb_norm(rb_ortho(U[0], ax), [0, 1, 0]));",
        "var az = rb_cross(ax, ay), cam = rb_camPos(), fit = E(\"Camera Fit\"), maxShift = 2 * width + 10;",
        "if (fit) {",
        "var vd = rb_norm(rb_sub(c, cam), [0, 0, 1]), va = rb_dot(vd, az), vb = rb_dot(vd, ay), MIN = 0.05;",
        "var vr = Math.sqrt(va * va + vb * vb);",
        "if (Math.abs(va) < MIN && vr > 2 * MIN) {",
        "var psi = Math.atan2(vb, va), dlt = Math.acos((va < 0 ? -MIN : MIN) / vr), phi = psi - dlt, phi2 = psi + dlt;",
        "phi = Math.atan2(Math.sin(phi), Math.cos(phi));",
        "phi2 = Math.atan2(Math.sin(phi2), Math.cos(phi2));",
        "if (Math.abs(phi2) < Math.abs(phi)) phi = phi2;",
        "var ay2 = rb_sub(rb_mul(ay, Math.cos(phi)), rb_mul(az, Math.sin(phi)));",
        "az = rb_add(rb_mul(ay, Math.sin(phi)), rb_mul(az, Math.cos(phi)));",
        "ay = ay2;",
        "}",
        "}",
        "var loc = function (pt) {",
        "if (fit) {",
        "var r = rb_sub(pt, cam), den = rb_dot(r, az), t, moved;",
        "if (Math.abs(den) > 0.01 * rb_len(r)) {",
        "t = rb_dot(rb_sub(c, cam), az) / den;",
        "moved = rb_add(cam, rb_mul(r, t));",
        "if (t > 0 && rb_len(rb_sub(moved, pt)) < maxShift) pt = moved;",
        "}",
        "}",
        "var d = rb_sub(pt, c);",
        "return [rb_dot(d, ax), rb_dot(d, ay)];",
        "};",
        "var q0 = loc(rb_sub(P[0], rb_mul(W[0], Wd[0] / 2))), q1 = loc(rb_sub(P[1], rb_mul(W[1], Wd[1] / 2)));",
        "var q2 = loc(rb_add(P[1], rb_mul(W[1], Wd[1] / 2))), q3 = loc(rb_add(P[0], rb_mul(W[0], Wd[0] / 2)));",
        "if (I > 0) { q0[0] -= ov; q3[0] -= ov; }",
        "if (I < N - 1) { q1[0] += ov; q2[0] += ov; }",
        "var front = rb_dot(az, rb_sub(cam, c)) < 0;",
        "var base = front ? E(\"Front Color\") : E(\"Back Color\"), c0 = [base[0], base[1], base[2]], c1 = c0;",
        "if (E(\"Built-in Shading\")) {",
        "var light = rb_light(C), sg = front ? -1 : 1;",
        "var mt = { amb: E(\"Ambient\"), diff: E(\"Diffuse\"), spec: E(\"Specular\"), gloss: Math.max(1, E(\"Gloss\")),",
        "satin: rb_clamp(E(\"Satin\"), 0, 1), rim: E(\"Rim\") };",
        "c0 = rb_shade(P[0], rb_mul(Nn[0], sg), T[0], base, cam, light, mt);",
        "c1 = rb_shade(P[1], rb_mul(Nn[1], sg), T[1], base, cam, light, mt);",
        "}",
        "if (E(\"Flip Gradient\")) { var tmp = c0; c0 = c1; c1 = tmp; }",
        "data = { vis: (chord > 1e-3 && (Wd[0] + Wd[1]) > 1e-3) ? 1 : 0, c: c, o: rb_euler(ax, ay, az),",
        "q: [q0, q1, q2, q3], g: [-chord / 2, chord / 2], c0: c0, c1: c1 };",
        "}",
        "data.op = E(\"Opacity\");",
        "data.u = uk;",
        "data.wb = width;",
        "return rb_pack(data);",
        "}",
        "function rb_pack(d) {",
        "var n = rb_num, pts;",
        "if (d.vis) {",
        "pts = [[d.c[0], d.c[1]], [d.c[2], d.o[0]], [d.o[1], d.o[2]],",
        "d.q[0], d.q[1], d.q[2], d.q[3], [d.g[0], d.g[1]],",
        "[d.c0[0], d.c0[1]], [d.c0[2], d.c1[0]], [d.c1[1], d.c1[2]]];",
        "} else {",
        "pts = [[0, 0], [0, 0], [0, 0], [-1, -1], [1, -1], [1, 1], [-1, 1], [-1, 1], [0, 0], [0, 0], [0, 0]];",
        "}",
        "pts.push([d.vis ? d.op : 0, 0], [d.u[0], d.u[1]], [d.wb, 0]);",
        "for (var i = 0; i < pts.length; i++) pts[i] = [n(pts[i][0]), n(pts[i][1])];",
        "return createPath(pts, [], [], false);",
        "}"
    ].join("\n");
    // </EXPR_CORE>

    // ------------------------------------------------------------------ expression builders (pure, tested in Node)

    RB3D.q = function (s) {
        return "\"" + String(s).replace(/\\/g, "\\\\").replace(/"/g, "\\\"").replace(/\r/g, "\\r").replace(/\n/g, "\\n") + "\"";
    };

    // desc: {type:"shape", layer, items:[group names..., path name]}
    //       {type:"mask",  layer, mask}
    //       {type:"layers", layers:[names], closed}
    RB3D.srcCode = function (desc) {
        var q = RB3D.q, lines = ["function RB_src() {"], i, acc, groups, ls;
        if (desc.type === "mask") {
            lines.push("    var L = thisComp.layer(" + q(desc.layer) + ");");
            lines.push("    return rb_fromPath(L, [], L.mask(" + q(desc.mask) + ").maskPath);");
        } else if (desc.type === "shape") {
            lines.push("    var L = thisComp.layer(" + q(desc.layer) + ");");
            acc = "L";
            groups = [];
            for (i = 0; i < desc.items.length; i++) {
                acc += ".content(" + q(desc.items[i]) + ")";
                if (i < desc.items.length - 1) groups.unshift(acc + ".transform");
            }
            lines.push("    return rb_fromPath(L, [" + groups.join(", ") + "], " + acc + ".path);");
        } else {
            ls = [];
            for (i = 0; i < desc.layers.length; i++) ls.push("thisComp.layer(" + q(desc.layers[i]) + ")");
            lines.push("    return rb_fromLayers([" + ls.join(", ") + "], " + (desc.closed ? "true" : "false") +
                       ", RB_CTRL.effect(\"Spline Smooth\")(1));");
        }
        lines.push("}");
        return lines.join("\n");
    };

    RB3D.SRC_OPEN = "// <RB_SRC>";
    RB3D.SRC_CLOSE = "// </RB_SRC>";

    RB3D.dataExpr = function (ctrlName, src, i, n) {
        return [
            "// " + RB3D.NAME + " " + RB3D.VERSION + " | segment data, generated by Ribbon3D.jsx",
            "var RB_I = " + i + ";",
            "var RB_N = " + n + ";",
            "var RB_CTRL = thisComp.layer(" + RB3D.q(ctrlName) + ");",
            RB3D.SRC_OPEN, src, RB3D.SRC_CLOSE,
            RB3D.CORE,
            "rb_main();"
        ].join("\n");
    };

    RB3D.extractSrc = function (expr) {
        var a = expr.indexOf(RB3D.SRC_OPEN), b = expr.indexOf(RB3D.SRC_CLOSE);
        if (a < 0 || b < a) return null;
        return expr.substring(a + RB3D.SRC_OPEN.length, b).replace(/^\s+|\s+$/g, "");
    };

    // World position of path vertex k (used to bake "Path -> 3D Nulls").
    RB3D.vertexExpr = function (src, k, nVerts, closed) {
        var segExpr = (closed || k < nVerts - 1) ? "rb_g[" + k + "][0]" : "rb_g[" + (nVerts - 2) + "][3]";
        return [RB3D.CORE, src, "var rb_g = RB_src().segs;", segExpr].join("\n");
    };

    (function () {
        var D = "var d = mask(" + RB3D.q(RB3D.DATA_MASK) + ").maskPath.points();\n";
        var T = D +
            "var W = thisLayer.width, H = thisLayer.height, u0 = d[12][0], u1 = d[12][1], g0 = d[7][0], g1 = d[7][1];\n" +
            "var gs = Math.abs(g1 - g0) > 1e-6 ? g1 - g0 : 1, wb = Math.max(d[13][0], 1e-3);\n" +
            "function tx(x) { return (u0 + (x - g0) / gs * (u1 - u0)) * W; }\n" +
            "function ty(y) { return H / 2 + y * H / wb; }\n";
        RB3D.EX = {
            pos: D + "[d[0][0], d[0][1], d[1][0]]",
            ori: D + "[d[1][1], d[2][0], d[2][1]]",
            opa: D + "d[11][0]",
            quad: D + "createPath([d[3], d[4], d[5], d[6]], [], [], true)",
            gradStart: D + "[d[7][0], 0]",
            gradEnd: D + "[d[7][1], 0]",
            tintBlack: D + "[d[9][1], d[10][0], d[10][1], 1]",
            tintWhite: D + "[d[8][0], d[8][1], d[9][0], 1]",
            texAnchor: T + "[tx(0), H / 2, 0]",
            texScale: T + "var du = Math.max(Math.abs(u1 - u0) * W, 1e-6);\n[gs / du * 100, wb / H * 100, 100]",
            texMask: T + "createPath([[tx(d[3][0]), ty(d[3][1])], [tx(d[4][0]), ty(d[4][1])], " +
                     "[tx(d[5][0]), ty(d[5][1])], [tx(d[6][0]), ty(d[6][1])]], [], [], true)"
        };
    })();

    // Controller effects: [name, type, default]. Names are referenced by the expressions.
    RB3D.controls = function (width, shading) {
        return [
            ["Start", "slider", 0],
            ["End", "slider", 100],
            ["Offset", "slider", 0],
            ["Width", "slider", width],
            ["Width Start", "slider", 100],
            ["Width End", "slider", 100],
            ["Twist", "slider", 1],
            ["Twist Angle", "angle", 0],
            ["Twist Speed", "slider", 0],
            ["Twist Along Path", "checkbox", 1],
            ["Wave Amp", "slider", 0],
            ["Wave Length", "slider", 500],
            ["Wave Speed", "slider", 0],
            ["Spline Smooth", "slider", 0.33],
            ["Overlap", "slider", 1],
            ["Camera Fit", "checkbox", 1],
            ["Front Color", "color", [0.80, 0.07, 0.16, 1]],
            ["Back Color", "color", [0.96, 0.80, 0.52, 1]],
            ["Built-in Shading", "checkbox", shading ? 1 : 0],
            ["Light", "layer", null],
            ["Ambient", "slider", 0.18],
            ["Diffuse", "slider", 0.9],
            ["Specular", "slider", 0.55],
            ["Gloss", "slider", 30],
            ["Satin", "slider", 0.5],
            ["Rim", "slider", 0.2],
            ["Opacity", "slider", 100],
            ["Flip Gradient", "checkbox", 0]
        ];
    };

    // Comment <-> info (key=value lines, values URI-encoded).
    RB3D.serialize = function (tag, info) {
        var out = [tag], k;
        for (k in info) if (info.hasOwnProperty(k)) out.push(k + "=" + encodeURIComponent(String(info[k])));
        return out.join("\n");
    };
    RB3D.parse = function (text) {
        var lines = String(text || "").split(/\r\n|\r|\n/), info = { tag: lines[0] }, i, p;
        for (i = 1; i < lines.length; i++) {
            p = lines[i].indexOf("=");
            if (p > 0) info[lines[i].substring(0, p)] = decodeURIComponent(lines[i].substring(p + 1));
        }
        return info;
    };

    RB3D.pad = function (n, w) {
        var s = String(n);
        while (s.length < w) s = "0" + s;
        return s;
    };

    // Running outside After Effects (Node tests): expose the builders and stop.
    if (typeof app === "undefined") {
        thisObj.RB3D = RB3D;
        return;
    }

    // ------------------------------------------------------------------ After Effects side

    var MN = {
        effects: "ADBE Effect Parade",
        masks: "ADBE Mask Parade",
        mask: "ADBE Mask Atom",
        maskShape: "ADBE Mask Shape",
        root: "ADBE Root Vectors Group",
        group: "ADBE Vector Group",
        contents: "ADBE Vectors Group",
        pathGroup: "ADBE Vector Shape - Group",
        path: "ADBE Vector Shape",
        gfill: "ADBE Vector Graphic - G-Fill",
        gStart: "ADBE Vector Grad Start Pt",
        gEnd: "ADBE Vector Grad End Pt",
        transform: "ADBE Transform Group",
        position: "ADBE Position",
        anchor: "ADBE Anchor Point",
        scale: "ADBE Scale",
        orientation: "ADBE Orientation",
        opacity: "ADBE Opacity",
        material: "ADBE Material Options Group",
        acceptsLights: "ADBE Accepts Lights",
        castsShadows: "ADBE Casts Shadows"
    };
    var FX = {
        slider: "ADBE Slider Control",
        angle: "ADBE Angle Control",
        checkbox: "ADBE Checkbox Control",
        color: "ADBE Color Control",
        layer: "ADBE Layer Control"
    };

    function activeComp() {
        var c = app.project.activeItem;
        return (c && c instanceof CompItem) ? c : null;
    }

    function layerByName(comp, name) {
        for (var i = 1; i <= comp.numLayers; i++) if (comp.layer(i).name === name) return comp.layer(i);
        return null;
    }

    // "base k" such that no layer is called "base k" + probe (probe: suffix of derived names).
    function uniqueName(comp, base, probe) {
        var k = 1;
        while (layerByName(comp, base + " " + k + (probe || ""))) k++;
        return base + " " + k;
    }

    function makeId() {
        return String(new Date().getTime()) + String(Math.floor(Math.random() * 1e6));
    }

    // ---------- source detection

    function firstShapePath(group) {
        for (var i = 1; i <= group.numProperties; i++) {
            var p = group.property(i);
            if (p.matchName === MN.pathGroup) return p.property(MN.path);
            if (p.matchName === MN.group || p.matchName === MN.contents) {
                var r = firstShapePath(p);
                if (r) return r;
            }
        }
        return null;
    }

    function pathPropOfLayer(layer) {
        var root = layer.property(MN.root);
        if (root) {
            var p = firstShapePath(root);
            if (p) return p;
        }
        var masks = layer.property(MN.masks);
        if (masks && masks.numProperties > 0) return masks.property(1).property(MN.maskShape);
        return null;
    }

    function selectedPathProp(comp) {
        var props = comp.selectedProperties, i, p;
        for (i = props.length - 1; i >= 0; i--) {
            p = props[i];
            if (p.matchName === MN.pathGroup) p = p.property(MN.path);
            else if (p.matchName === MN.mask) p = p.property(MN.maskShape);
            if (p && (p.matchName === MN.path || p.matchName === MN.maskShape)) return p;
        }
        return null;
    }

    function descFromPathProp(p) {
        var layer = p.propertyGroup(p.propertyDepth);
        if (p.matchName === MN.maskShape) return { type: "mask", layer: layer.name, mask: p.parentProperty.name };
        var items = [], q = p.parentProperty;
        while (q && q.matchName !== MN.root) {
            if (q.matchName !== MN.contents) items.unshift(q.name);
            q = q.parentProperty;
        }
        return { type: "shape", layer: layer.name, items: items };
    }

    function orderLayers(layers, mode) {
        var arr = [], i;
        for (i = 0; i < layers.length; i++) arr.push(layers[i]);
        if (mode === 2) return arr; // selection order
        arr.sort(function (a, b) { return mode === 0 ? a.index - b.index : b.index - a.index; });
        return arr;
    }

    // Returns {desc, label} or throws a user-facing message.
    function detectSource(comp, orderMode, closed) {
        var p = selectedPathProp(comp), layers = comp.selectedLayers, i, names;
        if (!p && layers.length === 1) p = pathPropOfLayer(layers[0]);
        if (p) return { desc: descFromPathProp(p), label: "path" };
        if (layers.length >= 2) {
            layers = orderLayers(layers, orderMode);
            names = [];
            for (i = 0; i < layers.length; i++) names.push(layers[i].name);
            return { desc: { type: "layers", layers: names, closed: closed }, label: "layers" };
        }
        throw "Select a path (shape path or mask), a layer that has one, or 2+ layers/nulls to run the ribbon through.\n" +
              "Rectangle/Ellipse/Star shapes must be converted to a Bezier path first.";
    }

    // ---------- building

    function addEffect(layer, matchName, name) {
        var fx = layer.property(MN.effects).addProperty(matchName);
        fx.name = name;
        return fx;
    }

    function addMask(layer, name, mode) {
        var m = layer.property(MN.masks).addProperty(MN.mask);
        m.name = name;
        m.maskMode = mode;
        var sh = new Shape();
        sh.vertices = [[0, 0], [10, 0], [10, 10]];
        sh.closed = mode !== MaskMode.NONE;
        m.property(MN.maskShape).setValue(sh);
        return layer.property(MN.masks).property(name).property(MN.maskShape);
    }

    function setExpr(prop, expr, what) {
        prop.expression = expr;
        if (prop.expressionError && prop.expressionError !== "") {
            throw "Expression error on " + what + ":\n" + prop.expressionError;
        }
    }

    function setMaterial(layer, builtInShading) {
        var m = layer.property(MN.material);
        if (!m) return;
        try { m.property(MN.acceptsLights).setValue(builtInShading ? 0 : 1); } catch (e) {}
        try { m.property(MN.castsShadows).setValue(1); } catch (e2) {}
    }

    function setTransformExpr(layer, texture) {
        var tr = layer.property(MN.transform);
        setExpr(tr.property(MN.position), RB3D.EX.pos, "Position");
        setExpr(tr.property(MN.orientation), RB3D.EX.ori, "Orientation");
        setExpr(tr.property(MN.opacity), RB3D.EX.opa, "Opacity");
        if (texture) {
            setExpr(tr.property(MN.anchor), RB3D.EX.texAnchor, "Anchor Point");
            setExpr(tr.property(MN.scale), RB3D.EX.texScale, "Scale");
        }
    }

    function dataProp(layer) {
        return layer.property(MN.masks).property(RB3D.DATA_MASK).property(MN.maskShape);
    }

    function makeColorTemplate(comp, dataExpr, shading) {
        var L = comp.layers.addShape();
        L.threeDLayer = true;
        setExpr(addMask(L, RB3D.DATA_MASK, MaskMode.NONE), dataExpr, "segment data");

        var grp = L.property(MN.root).addProperty(MN.group);
        grp.name = "Ribbon";
        var pg = L.property(MN.root).property("Ribbon").property(MN.contents).addProperty(MN.pathGroup);
        setExpr(pg.property(MN.path), RB3D.EX.quad, "segment path");
        var gf = L.property(MN.root).property("Ribbon").property(MN.contents).addProperty(MN.gfill);
        setExpr(gf.property(MN.gStart), RB3D.EX.gradStart, "gradient start");
        setExpr(gf.property(MN.gEnd), RB3D.EX.gradEnd, "gradient end");

        // Gradient (white -> black) + Tint = linear blend of the two vertex colors.
        var tint = L.property(MN.effects).addProperty("ADBE Tint");
        setExpr(tint.property(1), RB3D.EX.tintBlack, "Tint");
        setExpr(L.property(MN.effects).property(1).property(2), RB3D.EX.tintWhite, "Tint");
        L.property(MN.effects).property(1).property(3).setValue(100);

        setTransformExpr(L, false);
        setMaterial(L, shading);
        return L;
    }

    function makeTextureTemplate(comp, item, dataExpr) {
        var L = comp.layers.add(item);
        L.threeDLayer = true;
        setExpr(addMask(L, RB3D.DATA_MASK, MaskMode.NONE), dataExpr, "segment data");
        setExpr(addMask(L, RB3D.QUAD_MASK, MaskMode.ADD), RB3D.EX.texMask, "texture mask");
        setTransformExpr(L, true);
        setMaterial(L, false);
        return L;
    }

    function findItemById(id) {
        for (var i = 1; i <= app.project.numItems; i++) if (String(app.project.item(i).id) === String(id)) return app.project.item(i);
        return null;
    }

    // Creates n segment layers (pushed into `created` as they appear, so a failed
    // build can be cleaned up), then puts the light and the controller on top.
    function buildSegments(comp, ctrl, info, src, n, hideShy, created) {
        var shading = info.mode !== "texture" && ctrl.property(MN.effects).property("Built-in Shading").property(1).value ? 1 : 0;
        var tmpl, texItem, k, L, lights;
        if (info.mode === "texture") {
            texItem = findItemById(info.tex);
            if (!texItem) throw "Texture item not found in the project.";
            tmpl = makeTextureTemplate(comp, texItem, RB3D.dataExpr(ctrl.name, src, 0, n));
        } else {
            tmpl = makeColorTemplate(comp, RB3D.dataExpr(ctrl.name, src, 0, n), shading);
        }
        created.push(tmpl);
        // Each duplicate lands above the previous one: top-to-bottom order is [n] ... [001].
        for (k = 1; k < n; k++) created.push(created[created.length - 1].duplicate());
        for (k = 0; k < n; k++) {
            L = created[created.length - n + k];
            if (k > 0) dataProp(L).expression = RB3D.dataExpr(ctrl.name, src, k, n);
            L.name = ctrl.name + " [" + RB3D.pad(k + 1, 3) + "]";
            L.comment = RB3D.TAG_SEG + " id=" + info.id;
            L.label = ctrl.label;
            L.shy = hideShy;
            L.locked = true;
        }
        if (hideShy) comp.hideShyLayers = true;
        lights = layersWithTag(comp, RB3D.TAG_LIGHT, info.id);
        for (k = 0; k < lights.length; k++) lights[k].moveToBeginning();
        ctrl.moveToBeginning();
    }

    function createRibbon(opts) {
        var comp = activeComp();
        if (!comp) throw "Open a composition first.";
        var source = detectSource(comp, opts.order, opts.closed);
        if (opts.mode === "texture") {
            if (!opts.texId) throw "Choose a texture (comp or footage) in the list. Press Refresh if it is empty.";
            if (String(opts.texId) === String(comp.id)) throw "The texture cannot be the current composition.";
        }
        var src = RB3D.srcCode(source.desc);
        var id = makeId();
        var name = uniqueName(comp, RB3D.NAME);
        var created = [], i, fx;
        try {
            var light = comp.layers.addNull();
            created.push(light);
            light.threeDLayer = true;
            light.name = name + " Light";
            light.comment = RB3D.TAG_LIGHT + " id=" + id;
            light.label = 2;
            light.property(MN.transform).property(MN.position).setValue([comp.width * 0.2, -comp.height * 0.2, -comp.width * 0.8]);

            var ctrl = comp.layers.addNull();
            created.push(ctrl);
            ctrl.name = name;
            ctrl.label = 11;
            var defs = RB3D.controls(opts.width, opts.mode === "texture" ? 0 : opts.shading);
            for (i = 0; i < defs.length; i++) {
                fx = addEffect(ctrl, FX[defs[i][1]], defs[i][0]);
                if (defs[i][1] === "layer") fx.property(1).setValue(light.index);
                else fx.property(1).setValue(defs[i][2]);
            }
            var info = { id: id, mode: opts.mode, tex: opts.texId || "", n: opts.n, src: src };
            ctrl.comment = RB3D.serialize(RB3D.TAG_CTRL, info);

            buildSegments(comp, ctrl, info, src, opts.n, opts.hideShy, created);
        } catch (e) {
            removeLayers(created);
            throw e;
        }
        for (i = 1; i <= comp.numLayers; i++) comp.layer(i).selected = false;
        ctrl.selected = true;
        return ctrl;
    }

    // ---------- ribbon management

    function ribbonId(layer) {
        var info = RB3D.parse(layer.comment), m;
        if (info.tag === RB3D.TAG_CTRL) return info.id;
        m = /^RB3D-(SEG|LIGHT) id=(\S+)/.exec(String(layer.comment));
        return m ? m[2] : null;
    }

    function findCtrl(comp) {
        var sel = comp.selectedLayers, i, id = null, L;
        for (i = 0; i < sel.length && !id; i++) id = ribbonId(sel[i]);
        if (!id) return null;
        for (i = 1; i <= comp.numLayers; i++) {
            L = comp.layer(i);
            if (RB3D.parse(L.comment).tag === RB3D.TAG_CTRL && RB3D.parse(L.comment).id === id) return L;
        }
        return null;
    }

    function layersWithTag(comp, tag, id) {
        var out = [], i, L;
        for (i = 1; i <= comp.numLayers; i++) {
            L = comp.layer(i);
            if (String(L.comment).indexOf(tag + " id=" + id) === 0) out.push(L);
        }
        return out;
    }

    function removeLayers(arr) {
        for (var i = arr.length - 1; i >= 0; i--) {
            arr[i].locked = false;
            arr[i].remove();
        }
    }

    function rebuildRibbon(n, hideShy) {
        var comp = activeComp();
        if (!comp) throw "Open a composition first.";
        var ctrl = findCtrl(comp);
        if (!ctrl) throw "Select a ribbon (its controller or any of its segments).";
        var info = RB3D.parse(ctrl.comment);
        var segs = layersWithTag(comp, RB3D.TAG_SEG, info.id);
        // Prefer the live source from an existing segment: AE keeps it up to date when layers are renamed.
        var src = segs.length ? RB3D.extractSrc(dataProp(segs[0]).expression) : null;
        if (!src) src = info.src;
        removeLayers(segs);
        info.n = n;
        info.src = src;
        delete info.tag;
        ctrl.comment = RB3D.serialize(RB3D.TAG_CTRL, info);
        var created = [];
        try {
            buildSegments(comp, ctrl, info, src, n, hideShy, created);
        } catch (e) {
            removeLayers(created);
            throw e;
        }
    }

    function syncMaterials() {
        var comp = activeComp();
        if (!comp) throw "Open a composition first.";
        var ctrl = findCtrl(comp);
        if (!ctrl) throw "Select a ribbon (its controller or any of its segments).";
        var info = RB3D.parse(ctrl.comment);
        var shading = info.mode !== "texture" && ctrl.property(MN.effects).property("Built-in Shading").property(1).value;
        var segs = layersWithTag(comp, RB3D.TAG_SEG, info.id), i;
        for (i = 0; i < segs.length; i++) {
            segs[i].locked = false;
            setMaterial(segs[i], shading);
            segs[i].locked = true;
        }
    }

    function deleteRibbon() {
        var comp = activeComp();
        if (!comp) throw "Open a composition first.";
        var ctrl = findCtrl(comp);
        if (!ctrl) throw "Select a ribbon (its controller or any of its segments).";
        var id = RB3D.parse(ctrl.comment).id;
        if (!confirm("Delete \"" + ctrl.name + "\" with all its segments and its light null?")) return;
        removeLayers(layersWithTag(comp, RB3D.TAG_SEG, id));
        removeLayers(layersWithTag(comp, RB3D.TAG_LIGHT, id));
        ctrl.locked = false;
        ctrl.remove();
    }

    // Bakes the selected path's vertices into 3D nulls (top layer = first vertex),
    // so the path can be shaped freely in Z and used as a "layers" source.
    function pathToNulls() {
        var comp = activeComp();
        if (!comp) throw "Open a composition first.";
        var p = selectedPathProp(comp), sel = comp.selectedLayers, i;
        if (!p && sel.length === 1) p = pathPropOfLayer(sel[0]);
        if (!p) throw "Select a path (shape path or mask) or a layer that has one.";
        var shape = p.valueAtTime(comp.time, false), nV = shape.vertices.length, closed = shape.closed;
        if (nV < 2) throw "The path needs at least 2 vertices.";
        var src = RB3D.srcCode(descFromPathProp(p)), base = uniqueName(comp, "RB Path", " Pt 01"), nulls = [], k, nl, pos, v;
        for (k = nV - 1; k >= 0; k--) {
            nl = comp.layers.addNull();
            nl.threeDLayer = true;
            nl.name = base + " Pt " + RB3D.pad(k + 1, 2);
            nl.label = 9;
            pos = nl.property(MN.transform).property(MN.position);
            setExpr(pos, RB3D.vertexExpr(src, k, nV, closed), "null position");
            v = pos.valueAtTime(comp.time, false);
            pos.expression = "";
            pos.setValue(v);
            nulls.unshift(nl);
        }
        for (i = 1; i <= comp.numLayers; i++) comp.layer(i).selected = false;
        for (k = 0; k < nulls.length; k++) nulls[k].selected = true;
        return closed;
    }

    // ------------------------------------------------------------------ UI

    function run(undoName, fn) {
        app.beginUndoGroup(RB3D.NAME + ": " + undoName);
        try {
            return fn();
        } catch (e) {
            alert(RB3D.NAME + "\n\n" + (typeof e === "string" ? e : e.toString() + (e.line ? " (line " + e.line + ")" : "")));
        } finally {
            app.endUndoGroup();
        }
        return null;
    }

    function buildUI(thisObj) {
        var win = (thisObj instanceof Panel) ? thisObj :
            new Window("palette", RB3D.NAME + " " + RB3D.VERSION, undefined, { resizeable: true });
        win.orientation = "column";
        win.alignChildren = ["fill", "top"];
        win.spacing = 6;
        win.margins = 8;

        function row(parent, label) {
            var g = parent.add("group");
            g.orientation = "row";
            g.alignChildren = ["left", "center"];
            g.spacing = 4;
            var st = g.add("statictext", undefined, label);
            st.preferredSize.width = 72;
            return g;
        }

        var pNew = win.add("panel", undefined, "New ribbon");
        pNew.orientation = "column";
        pNew.alignChildren = ["fill", "top"];
        pNew.margins = [8, 14, 8, 8];
        pNew.spacing = 4;

        var r1 = row(pNew, "Segments:");
        var etSeg = r1.add("edittext", undefined, "100");
        etSeg.characters = 5;
        r1.add("statictext", undefined, "Width:");
        var etW = r1.add("edittext", undefined, "60");
        etW.characters = 5;

        var r2 = row(pNew, "Type:");
        var ddType = r2.add("dropdownlist", undefined, ["Color (shape layers)", "Texture (comp/footage)"]);
        ddType.selection = 0;

        var r3 = row(pNew, "Texture:");
        var ddTex = r3.add("dropdownlist", undefined, []);
        ddTex.preferredSize.width = 140;
        var btRefresh = r3.add("button", undefined, "Refresh");
        btRefresh.preferredSize.width = 60;

        var r4 = row(pNew, "Shading:");
        var ddShade = r4.add("dropdownlist", undefined, ["Built-in smooth", "AE lights"]);
        ddShade.selection = 0;

        var r5 = row(pNew, "Point order:");
        var ddOrder = r5.add("dropdownlist", undefined, ["Top to bottom", "Bottom to top", "Selection order"]);
        ddOrder.selection = 0;

        var cbClosed = pNew.add("checkbox", undefined, "Closed loop (layers/nulls source)");
        var cbShy = pNew.add("checkbox", undefined, "Hide segments (shy)");
        cbShy.value = true;

        var btCreate = pNew.add("button", undefined, "Create Ribbon");
        var btNulls = pNew.add("button", undefined, "Path -> 3D Nulls");
        var hint = pNew.add("statictext", undefined,
            "Select a path (shape path / mask), a layer with a path, or 2+ layers (nulls) and press Create.",
            { multiline: true });
        hint.preferredSize = [220, 44];

        var pSel = win.add("panel", undefined, "Selected ribbon");
        pSel.orientation = "row";
        pSel.alignChildren = ["fill", "center"];
        pSel.margins = [8, 14, 8, 8];
        pSel.spacing = 4;
        var btRebuild = pSel.add("button", undefined, "Rebuild");
        var btSync = pSel.add("button", undefined, "Sync Materials");
        var btDelete = pSel.add("button", undefined, "Delete");

        var texIds = [];
        function refreshTextures() {
            var comp = activeComp(), i, it;
            ddTex.removeAll();
            texIds = [];
            for (i = 1; i <= app.project.numItems; i++) {
                it = app.project.item(i);
                if ((it instanceof CompItem || (it instanceof FootageItem && it.hasVideo)) && (!comp || it.id !== comp.id)) {
                    ddTex.add("item", it.name);
                    texIds.push(it.id);
                }
            }
            if (texIds.length) ddTex.selection = 0;
        }
        function updateEnabled() {
            var tex = ddType.selection.index === 1;
            ddTex.enabled = tex;
            btRefresh.enabled = tex;
            ddShade.enabled = !tex;
        }
        function readInt(et, def, lo, hi) {
            var v = parseInt(et.text, 10);
            if (isNaN(v)) v = def;
            v = Math.max(lo, Math.min(hi, v));
            et.text = String(v);
            return v;
        }

        ddType.onChange = function () {
            if (ddType.selection.index === 1 && !texIds.length) refreshTextures();
            updateEnabled();
        };
        btRefresh.onClick = refreshTextures;

        btCreate.onClick = function () {
            var n = readInt(etSeg, 100, 2, 1000), w = readInt(etW, 60, 1, 100000);
            var tex = ddType.selection.index === 1;
            run("create", function () {
                return createRibbon({
                    n: n,
                    width: w,
                    mode: tex ? "texture" : "color",
                    texId: tex && ddTex.selection ? texIds[ddTex.selection.index] : null,
                    shading: ddShade.selection.index === 0 ? 1 : 0,
                    order: ddOrder.selection.index,
                    closed: cbClosed.value,
                    hideShy: cbShy.value
                });
            });
        };
        btNulls.onClick = function () {
            var closed = run("path to nulls", pathToNulls);
            if (closed !== null) {
                cbClosed.value = closed;
                ddOrder.selection = 0;
            }
        };
        btRebuild.onClick = function () {
            var n = readInt(etSeg, 100, 2, 1000);
            run("rebuild", function () { rebuildRibbon(n, cbShy.value); });
        };
        btSync.onClick = function () { run("sync materials", syncMaterials); };
        btDelete.onClick = function () { run("delete", deleteRibbon); };

        updateEnabled();
        win.layout.layout(true);
        win.onResizing = win.onResize = function () { this.layout.resize(); };
        if (win instanceof Window) {
            win.center();
            win.show();
        }
        return win;
    }

    buildUI(thisObj);
})(this);
