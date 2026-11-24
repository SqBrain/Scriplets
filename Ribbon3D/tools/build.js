#!/usr/bin/env node
// Embeds src/core.expr.js into Ribbon3D.jsx between the EXPR_CORE markers.
// Usage: node Ribbon3D/tools/build.js        (rewrites Ribbon3D.jsx)
//        node Ribbon3D/tools/build.js --check (exit 1 if Ribbon3D.jsx is stale)
"use strict";
const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..");
const corePath = path.join(root, "src", "core.expr.js");
const jsxPath = path.join(root, "Ribbon3D.jsx");

// Full-line comments and indentation are dropped: the core is copied into every
// segment's expression, so its size matters.
const lines = fs.readFileSync(corePath, "utf8")
    .split(/\r?\n/)
    .map((l) => l.trim())
    .filter((l) => l !== "" && !l.startsWith("//"));

for (const l of lines) {
    if (/[^\x20-\x7e]/.test(l)) throw new Error("Non-ASCII character in core line: " + l);
}

const indent = "        ";
const block = [
    "    // <EXPR_CORE>",
    "    // Generated from src/core.expr.js by tools/build.js. Do not edit by hand.",
    "    RB3D.CORE = [",
    lines.map((l) => indent + JSON.stringify(l)).join(",\n"),
    "    ].join(\"\\n\");",
    "    // </EXPR_CORE>",
].join("\n");

const jsx = fs.readFileSync(jsxPath, "utf8");
const re = /[ \t]*\/\/ <EXPR_CORE>[\s\S]*?\/\/ <\/EXPR_CORE>/;
if (!re.test(jsx)) throw new Error("EXPR_CORE markers not found in " + jsxPath);
const out = jsx.replace(re, () => block);

if (process.argv.includes("--check")) {
    if (out !== jsx) {
        console.error("Ribbon3D.jsx is out of date: run node Ribbon3D/tools/build.js");
        process.exit(1);
    }
    console.log("Ribbon3D.jsx is up to date.");
} else {
    fs.writeFileSync(jsxPath, out);
    console.log("Embedded " + lines.length + " core lines (" + lines.join("\n").length + " chars) into Ribbon3D.jsx");
}
