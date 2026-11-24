// Renders a few rigs with the software rasterizer to PNG for a visual sanity check.
// Run: node Ribbon3D/tests/preview.js [outDir]   (default: Ribbon3D/tests/out)
"use strict";
const fs = require("fs");
const path = require("path");
const H = require("./harness");
const { Layer, Comp, PathProp, ShapeGroup } = H;

const RB3D = H.loadRB3D();
const CTRL = "Ribbon 3D 1";
const outDir = path.resolve(process.argv[2] || path.join(__dirname, "out"));
fs.mkdirSync(outDir, { recursive: true });

function pathScene(over, n, time) {
    const comp = new Comp(1920, 1080);
    const light = comp.add(new Layer({ name: CTRL + " Light", position: [300, -300, -1400] }));
    const pp = new PathProp(
        [[-780, 160], [-260, -220], [260, 220], [780, -160]],
        [[0, 0], [-260, 40], [-260, -40], [-220, 60]],
        [[220, -60], [260, -40], [260, 40], [0, 0]], false);
    comp.add(new Layer({ name: "Path", position: [960, 540, 0], orientation: [0, 0, 0],
        contents: { "Shape 1": new ShapeGroup({}, { "Path 1": { path: pp } }) } }));
    comp.add(new Layer({ name: CTRL, threeD: false, effects: Object.assign(H.defaultControls(RB3D, 110, 1, light), over) }));
    return H.evaluateRig(RB3D, { comp, ctrlName: CTRL, desc: { type: "shape", layer: "Path", items: ["Shape 1", "Path 1"] }, n, time });
}

function helixScene(over, n) {
    const comp = new Comp(1920, 1080);
    const light = comp.add(new Layer({ name: CTRL + " Light", position: [1500, -200, -1200] }));
    const names = [];
    for (let i = 0; i < 9; i++) {
        const a = i * Math.PI * 0.5;
        names.push(comp.add(new Layer({ name: "Pt " + i, position: [400 + i * 140, 540 + Math.sin(a) * 260, Math.cos(a) * 260], anchor: [50, 50, 0] })).name);
    }
    comp.add(new Layer({ name: CTRL, threeD: false, effects: Object.assign(H.defaultControls(RB3D, 70, 1, light), over) }));
    return { comp, segs: H.evaluateRig(RB3D, { comp, ctrlName: CTRL, desc: { type: "layers", layers: names, closed: false }, n }) };
}

function render(name, segs, comp) {
    const img = H.renderSegments(segs, comp || new Comp(1920, 1080), { width: 960, height: 540, supersample: 3 });
    const file = path.join(outDir, name + ".png");
    H.writePNG(file, img);
    console.log("wrote " + file);
}

const t0 = Date.now();
const hero = pathScene({ "Twist": 1.5, "Twist Along Path": 0, "Wave Amp": 70, "Wave Length": 700, "Satin": 0.6,
    "Width Start": 15, "Width End": 100 }, 160);
const perSeg = (Date.now() - t0) / 160;
render("01_satin_twist", hero);
render("02_ae_lights_flat", pathScene({ "Twist": 1.5, "Twist Along Path": 0, "Wave Amp": 70, "Wave Length": 700, "Built-in Shading": 0 }, 160));
const hx = helixScene({ "Twist": 3, "Front Color": [0.15, 0.45, 0.95, 1], "Back Color": [0.95, 0.95, 0.98, 1], "Gloss": 60, "Satin": 0.2 }, 220);
render("03_nulls_helix", hx.segs, hx.comp);
render("04_camera_fit_off_steps", pathScene({ "Twist": 3, "Twist Along Path": 0, "Camera Fit": 0, "Overlap": 0 }, 50));
render("05_camera_fit_on", pathScene({ "Twist": 3, "Twist Along Path": 0, "Camera Fit": 1, "Overlap": 0 }, 50));
console.log("data expression eval in Node: ~" + perSeg.toFixed(2) + " ms per segment (incl. 9 property expressions)");
