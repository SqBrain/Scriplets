// Run: node --test Ribbon3D/tests/
// Optional ES3 syntax check of the jsx and every generated expression:
//   npm install --prefix <somewhere> acorn && ACORN=<somewhere>/node_modules/acorn node --test Ribbon3D/tests/
"use strict";
const test = require("node:test");
const assert = require("node:assert/strict");
const fs = require("fs");
const path = require("path");
const H = require("./harness");
const { V, Layer, Comp, PathProp, ShapeGroup } = H;

const RB3D = H.loadRB3D();
const CTRL = "Ribbon 3D 1";

function scene(over, opt) {
    opt = opt || {};
    const comp = new Comp(1920, 1080, [], opt.camera || null);
    const light = new Layer({ name: CTRL + " Light", position: [384, -216, -1536] });
    const pathProp = opt.pathProp || new PathProp(
        [[-600, 100], [0, -150], [600, 100]],
        [[0, 0], [-250, 0], [-200, -200]],
        [[200, -200], [250, 0], [0, 0]], false);
    const gt = { position: [30, -20], anchorPoint: [10, 5], scale: [110, 90], rotation: 12 };
    const group = new ShapeGroup(gt, { "Path 1": { path: pathProp } });
    const pathLayer = new Layer({ name: "Path Layer", position: [960, 540, 100], orientation: [25, -20, 5], contents: { "Shape 1": group } });
    const ctrl = new Layer({ name: CTRL, threeD: false, effects: H.defaultControls(RB3D, 60, opt.shading === undefined ? 1 : opt.shading, light) });
    Object.assign(ctrl.effects, { "Overlap": 0 }, over || {});
    [light, pathLayer, ctrl].forEach((L) => comp.add(L));
    const desc = { type: "shape", layer: "Path Layer", items: ["Shape 1", "Path 1"] };
    const ref = H.referencePolyline(pathLayer, [group.transform], pathProp, 400);
    return { comp, light, pathLayer, ctrl, pathProp, desc, ref };
}
const rig = (s, n, extra) => H.evaluateRig(RB3D, Object.assign({ comp: s.comp, ctrlName: CTRL, desc: s.desc, n }, extra || {}));
const startMid = (seg) => V.lerp(seg.world[0], seg.world[3], 0.5);
const endMid = (seg) => V.lerp(seg.world[1], seg.world[2], 0.5);
const near = (a, b, tol, msg) => assert.ok(V.len(V.sub(a, b)) <= tol, `${msg}: |${a}| vs |${b}| (d=${V.len(V.sub(a, b))})`);

test("jsx exposes builders and an embedded core", () => {
    assert.ok(RB3D.CORE.length > 5000);
    assert.match(RB3D.CORE, /function rb_main\(\)/);
});

test("AE orientation from frame round-trips (Rx*Ry*Rz)", () => {
    let seed = 7;
    const rnd = () => (seed = (seed * 16807) % 2147483647) / 2147483647 - 0.5;
    const cases = [];
    for (let i = 0; i < 300; i++) {
        const u = V.norm([rnd(), rnd(), rnd()]);
        const v = V.norm(V.sub([rnd(), rnd(), rnd()], V.mul(u, V.dot([rnd(), rnd(), rnd()], u))));
        const vv = V.norm(V.sub(v, V.mul(u, V.dot(v, u))));
        cases.push([u, vv, V.cross(u, vv)]);
    }
    cases.push([[0, 0, -1], [0, 1, 0], [1, 0, 0]], [[0, 0, 1], [0, 1, 0], [-1, 0, 0]]); // gimbal cases
    cases.push([[0, 1, 0], [0, 0, 1], [1, 0, 0]], [[0, -1, 0], [0, 0, 1], [-1, 0, 0]]);
    for (const [u, v, w] of cases) {
        const e = H.evalExpr(RB3D.CORE + `\nrb_euler(${JSON.stringify(u)}, ${JSON.stringify(v)}, ${JSON.stringify(w)});`, {});
        const M = H.orientMatrix(e);
        near([M[0][0], M[1][0], M[2][0]], u, 1e-6, "x axis");
        near([M[0][1], M[1][1], M[2][1]], v, 1e-6, "y axis");
        near([M[0][2], M[1][2], M[2][2]], w, 1e-6, "z axis");
    }
});

test("centerline lies on the path (3D layer, group transform, bezier handles)", () => {
    const s = scene({ "Twist": 1.5, "Camera Fit": 0 });
    const segs = rig(s, 80);
    let worst = 0;
    for (const seg of segs) worst = Math.max(worst, H.distToPolyline(startMid(seg), s.ref), H.distToPolyline(endMid(seg), s.ref));
    assert.ok(worst < 0.05, "max distance to path " + worst);
    near(startMid(segs[0]), s.ref[0], 1e-6, "ribbon starts at the first vertex");
    near(endMid(segs[segs.length - 1]), s.ref[s.ref.length - 1], 1e-6, "ribbon ends at the last vertex");
});

test("Camera Fit: neighbouring segments meet exactly in the image", () => {
    const s = scene({ "Twist": 2 });
    const segs = rig(s, 100);
    const pr = (p) => H.projectScreen(s.comp, p);
    let worst = 0, worst3d = 0;
    for (let i = 0; i + 1 < segs.length; i++) {
        for (const [a, b] of [[1, 0], [2, 3]]) {
            const A = pr(segs[i].world[a]), B = pr(segs[i + 1].world[b]);
            worst = Math.max(worst, Math.hypot(A[0] - B[0], A[1] - B[1]));
            worst3d = Math.max(worst3d, V.len(V.sub(segs[i].world[a], segs[i + 1].world[b])));
        }
    }
    assert.ok(worst < 1e-6, "max screen gap " + worst + " px");
    assert.ok(worst3d > 0.5, "twisted quads are not coplanar, the fit really moved corners (" + worst3d + ")");
});

test("untwisted planar ribbon is watertight in 3D", () => {
    const segs = rig(scene({ "Twist": 0, "Camera Fit": 0 }), 100);
    let worst = 0;
    for (let i = 0; i + 1 < segs.length; i++) {
        worst = Math.max(worst, V.len(V.sub(segs[i].world[1], segs[i + 1].world[0])), V.len(V.sub(segs[i].world[2], segs[i + 1].world[3])));
    }
    assert.ok(worst < 1e-6, "max edge gap " + worst + " px");
});

test("overlap extends inner edges only", () => {
    const s = scene({ "Overlap": 2 });
    const a = rig(s, 10), b = rig(scene({ "Overlap": 0 }), 10);
    assert.equal(a[0].quad[0][0], b[0].quad[0][0], "first segment start edge untouched");
    assert.ok(Math.abs(a[0].quad[1][0] - b[0].quad[1][0] - 2) < 1e-9);
    assert.ok(Math.abs(a[9].quad[3][0] - b[9].quad[3][0] + 2) < 1e-9);
    assert.equal(a[9].quad[2][0], b[9].quad[2][0], "last segment end edge untouched");
});

test("start/end trim follows arc length", () => {
    const s = scene({ "Start": 20, "End": 60 });
    const segs = rig(s, 100);
    let L = 0;
    for (const seg of segs) L += V.len(V.sub(endMid(seg), startMid(seg)));
    const total = H.polyLength(s.ref);
    assert.ok(Math.abs(L / total - 0.4) < 0.002, "trimmed fraction " + L / total);
});

test("twist, width and taper: every corner lands on the true ribbon edge (screen space)", () => {
    const N = 120;
    const s = scene({ "Twist": 1, "Twist Along Path": 0, "Twist Angle": 30, "Width Start": 0, "Width End": 100 });
    const segs = rig(s, N);
    const up = V.norm(V.cross(s.pathLayer.toWorldVec([1, 0, 0]), s.pathLayer.toWorldVec([0, 1, 0])));
    const truth = (k, side) => {
        const at = H.polylineAt(s.ref, k / N), wf = V.cross(up, at.t), th = (30 + 360 * k / N) * Math.PI / 180;
        const w = V.add(V.mul(wf, Math.cos(th)), V.mul(up, Math.sin(th)));
        return V.add(at.p, V.mul(w, side * 60 * (k / N) / 2));
    };
    let worst = 0;
    for (let i = 0; i < N; i++) {
        const want = [truth(i, -1), truth(i + 1, -1), truth(i + 1, 1), truth(i, 1)];
        for (let c = 0; c < 4; c++) {
            const A = H.projectScreen(s.comp, segs[i].world[c]), B = H.projectScreen(s.comp, want[c]);
            worst = Math.max(worst, Math.hypot(A[0] - B[0], A[1] - B[1]));
        }
    }
    assert.ok(worst < 0.25, "max corner error " + worst + " px");
    assert.ok(V.len(V.sub(segs[0].world[3], segs[0].world[0])) < 1e-6, "zero width at the start");
});

test("texture mapping lands on the same 3D quad as the color geometry", () => {
    const s = scene({ "Twist": 1 });
    const col = rig(s, 40), tex = rig(s, 40, { texture: { width: 2000, height: 200 } });
    for (let i = 0; i < col.length; i++) {
        for (let k = 0; k < 4; k++) near(tex[i].world[k], col[i].world[k], 1e-6, `segment ${i} corner ${k}`);
        // The chord ends (local x = g0 / g1) map to u0 / u1 of the texture.
        const t = tex[i], g0 = t.d[7][0], g1 = t.d[7][1];
        assert.ok(Math.abs(t.anchor[0] + g0 * 100 / t.scale[0] - 2000 * i / 40) < 1e-6, "texture u0");
        assert.ok(Math.abs(t.anchor[0] + g1 * 100 / t.scale[0] - 2000 * (i + 1) / 40) < 1e-6, "texture u1");
        assert.ok(Math.abs(t.anchor[1] - 100) < 1e-9 && Math.abs(t.scale[1] - 30) < 1e-9, "texture height = base width");
    }
});

test("AE-lights mode: flat front/back colors chosen by facing", () => {
    const s = scene({ "Twist": 1.5, "Twist Along Path": 0 }, { shading: 0 });
    const segs = rig(s, 60);
    const cam = s.comp.cameraPos();
    let fronts = 0, backs = 0;
    for (const seg of segs) {
        const az = [seg.M[0][2], seg.M[1][2], seg.M[2][2]];
        const front = V.dot(az, V.sub(cam, seg.pos)) < 0;
        const want = s.ctrl.effects[front ? "Front Color" : "Back Color"];
        near(seg.cWhite.slice(0, 3), want.slice(0, 3), 1e-9, "start color");
        near(seg.cBlack.slice(0, 3), want.slice(0, 3), 1e-9, "end color");
        front ? fronts++ : backs++;
    }
    assert.ok(fronts > 5 && backs > 5, `both sides visible (front ${fronts}, back ${backs})`);
});

test("built-in shading is continuous across segments and in range", () => {
    const s = scene({ "Twist": 1.5 });
    const segs = rig(s, 100);
    let distinct = new Set();
    for (let i = 0; i < segs.length; i++) {
        for (const c of [segs[i].cWhite, segs[i].cBlack]) for (let k = 0; k < 3; k++) assert.ok(c[k] >= 0 && c[k] <= 1);
        distinct.add(segs[i].cWhite.slice(0, 3).map((x) => x.toFixed(3)).join());
        if (i + 1 < segs.length) {
            const f0 = V.dot([segs[i].M[0][2], segs[i].M[1][2], segs[i].M[2][2]], V.sub(s.comp.cameraPos(), segs[i].pos)) < 0;
            const f1 = V.dot([segs[i + 1].M[0][2], segs[i + 1].M[1][2], segs[i + 1].M[2][2]], V.sub(s.comp.cameraPos(), segs[i + 1].pos)) < 0;
            if (f0 === f1) near(segs[i].cBlack.slice(0, 3), segs[i + 1].cWhite.slice(0, 3), 1e-9, "Gouraud continuity at joint " + i);
        }
    }
    assert.ok(distinct.size > 50, "shading varies along the ribbon");
});

test("Flip Gradient swaps the vertex colors", () => {
    const a = rig(scene({ "Twist": 1 }), 20), b = rig(scene({ "Twist": 1, "Flip Gradient": 1 }), 20);
    for (let i = 0; i < 20; i++) {
        near(a[i].cWhite, b[i].cBlack, 1e-12, "flip");
        near(a[i].cBlack, b[i].cWhite, 1e-12, "flip");
    }
});

test("camera is used when present; facing flips when the camera goes behind", () => {
    const camFront = new Layer({ name: "Camera 1", position: [960, 540, -2666.7] });
    const camBack = new Layer({ name: "Camera 1", position: [960, 540, 3000], orientation: [0, 180, 0] });
    const opts = { "Twist": 0, "Twist Angle": 0 };
    const a = rig(scene(opts, { shading: 0, camera: camFront }), 10), b = rig(scene(opts, { shading: 0, camera: camBack }), 10);
    const front = scene(opts).ctrl.effects["Front Color"], back = scene(opts).ctrl.effects["Back Color"];
    const isCol = (c, ref) => V.len(V.sub(c.slice(0, 3), ref.slice(0, 3))) < 1e-9;
    assert.ok(a.every((s) => isCol(s.cWhite, front)) || a.every((s) => isCol(s.cWhite, back)));
    assert.notDeepEqual(a.map((s) => s.cWhite[0]), b.map((s) => s.cWhite[0]), "other side seen from behind");
});

test("wave displaces the ribbon along the up vector", () => {
    const flat = rig(scene({ "Camera Fit": 0 }), 60), wavy = rig(scene({ "Camera Fit": 0, "Wave Amp": 50, "Wave Length": 300 }), 60);
    let maxD = 0;
    for (let i = 0; i < 60; i++) maxD = Math.max(maxD, V.len(V.sub(startMid(flat[i]), startMid(wavy[i]))));
    assert.ok(maxD > 45 && maxD <= 50 + 1e-6, "max displacement " + maxD);
    const t1 = rig(scene({ "Wave Amp": 50, "Wave Speed": 100 }), 10, { time: 0 }), t2 = rig(scene({ "Wave Amp": 50, "Wave Speed": 100 }), 10, { time: 0.5 });
    assert.notDeepEqual(t1[3].pos, t2[3].pos, "wave speed animates over time");
});

test("degenerate ribbon (start == end) is hidden and finite", () => {
    const segs = rig(scene({ "Start": 50, "End": 50 }), 10);
    for (const s of segs) {
        assert.equal(s.opa, 0);
        for (const p of s.d) for (const x of p) assert.ok(Number.isFinite(x));
    }
});

test("closed path: ribbon closes on itself", () => {
    const k = 0.5523 * 300;
    const circle = new PathProp([[0, -300], [300, 0], [0, 300], [-300, 0]],
        [[-k, 0], [0, -k], [k, 0], [0, k]], [[k, 0], [0, k], [-k, 0], [0, -k]], true);
    const s = scene({ "Offset": 13, "Camera Fit": 0 }, { pathProp: circle });
    const segs = rig(s, 64);
    near(endMid(segs[63]), startMid(segs[0]), 1e-6, "loop closure");
    for (const seg of segs) assert.ok(H.distToPolyline(startMid(seg), s.ref) < 0.05);
});

test("mask on a 2D layer as source", () => {
    const comp = new Comp(1920, 1080);
    const light = new Layer({ name: CTRL + " Light", position: [0, 0, -1000] });
    const mp = new PathProp([[100, 500], [900, 300], [1700, 700]], [[0, 0], [-200, 0], [0, 0]], [[200, 0], [200, 0], [0, 0]], false);
    const solid = new Layer({ name: "Solid \"A\" \\ 1", threeD: false, position: [960, 540], anchor: [960, 540], rotation: 10, masks: { "Mask 1": mp } });
    const ctrl = new Layer({ name: CTRL, threeD: false, effects: Object.assign(H.defaultControls(RB3D, 40, 1, light), { "Overlap": 0, "Camera Fit": 0 }) });
    [light, solid, ctrl].forEach((L) => comp.add(L));
    const desc = { type: "mask", layer: solid.name, mask: "Mask 1" };
    const segs = H.evaluateRig(RB3D, { comp, ctrlName: CTRL, desc, n: 50 });
    const ref = H.referencePolyline(solid, [], mp, 400);
    for (const seg of segs) {
        assert.ok(H.distToPolyline(startMid(seg), ref) < 0.05);
        assert.ok(Math.abs(startMid(seg)[2]) < 1e-9, "stays in the comp plane");
    }
});

test("layers (nulls) source: spline passes through every null", () => {
    const comp = new Comp(1920, 1080);
    const light = new Layer({ name: CTRL + " Light", position: [0, 0, -1000] });
    const pts = [[300, 700, 0], [700, 300, -400], [1100, 600, 200], [1500, 350, -100], [1700, 800, 300]];
    const nulls = pts.map((p, i) => comp.add(new Layer({ name: "Null " + (i + 1), position: p, anchor: [50, 50, 0], orientation: [0, i * 10, 0] })));
    const ctrl = comp.add(new Layer({ name: CTRL, threeD: false, effects: Object.assign(H.defaultControls(RB3D, 50, 1, light), { "Overlap": 0, "Twist": 2, "Camera Fit": 0 }) }));
    comp.add(light);
    const desc = { type: "layers", layers: nulls.map((l) => l.name), closed: false };
    const segs = H.evaluateRig(RB3D, { comp, ctrlName: CTRL, desc, n: 200 });
    const center = segs.map(startMid).concat([endMid(segs[segs.length - 1])]);
    for (const p of pts) assert.ok(H.distToPolyline(p, center) < 0.5, "null " + p + " at " + H.distToPolyline(p, center));
    near(center[0], pts[0], 1e-6, "starts at the first null");
    near(center[center.length - 1], pts[pts.length - 1], 1e-6, "ends at the last null");
    assert.ok(ctrl.effects["Spline Smooth"] > 0);
});

test("vertexExpr bakes path vertices to world positions", () => {
    const s = scene({});
    const src = RB3D.srcCode(s.desc);
    const refV = H.referencePolyline(s.pathLayer, [s.pathLayer.content("Shape 1").transform], s.pathProp, 1);
    for (let k = 0; k < 3; k++) near(H.evalExpr(RB3D.vertexExpr(src, k, 3, false), { thisComp: s.comp }), refV[k], 1e-9, "vertex " + k);
});

test("controller comment serialization round-trips", () => {
    const info = { id: "123", mode: "color", tex: "", n: 100, src: RB3D.srcCode({ type: "shape", layer: "Odd \"name\"\n\\=x", items: ["G=1", "Path 1"] }) };
    const back = RB3D.parse(RB3D.serialize(RB3D.TAG_CTRL, info));
    assert.equal(back.tag, RB3D.TAG_CTRL);
    assert.equal(back.src, info.src);
    assert.equal(back.n, "100");
    const expr = RB3D.dataExpr(CTRL, info.src, 3, 10);
    assert.equal(RB3D.extractSrc(expr), info.src);
});

test("ES3 syntax: jsx and generated expressions (needs ACORN env)", { skip: !process.env.ACORN && "set ACORN=<path to acorn> to enable" }, () => {
    const acorn = require(process.env.ACORN);
    const parse = (code, what) => {
        try { acorn.parse(code, { ecmaVersion: 3, allowReturnOutsideFunction: false, allowReserved: true }); }
        catch (e) { assert.fail(what + ": " + e.message); }
    };
    parse(fs.readFileSync(path.join(__dirname, "..", "Ribbon3D.jsx"), "utf8"), "Ribbon3D.jsx");
    const s = scene({});
    parse(RB3D.dataExpr(CTRL, RB3D.srcCode(s.desc), 0, 10), "data expr (shape)");
    parse(RB3D.dataExpr(CTRL, RB3D.srcCode({ type: "mask", layer: "L", mask: "M" }), 0, 10), "data expr (mask)");
    parse(RB3D.dataExpr(CTRL, RB3D.srcCode({ type: "layers", layers: ["a", "b"], closed: true }), 0, 10), "data expr (layers)");
    parse(RB3D.vertexExpr(RB3D.srcCode(s.desc), 1, 3, false), "vertex expr");
    for (const k of Object.keys(RB3D.EX)) parse(RB3D.EX[k], "EX." + k);
});
