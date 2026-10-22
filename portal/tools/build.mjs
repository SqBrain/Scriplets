// VFX Cave build: reads catalog manifests, bundles //@include into one file,
// checks ExtendScript compatibility and capabilities, writes the open index
// and the static site.
//
//   node tools/build.mjs
//
// Inputs:  catalog/*.json (one manifest per script), the scripts themselves
//          in the repository root, site/src/template.html.
// Outputs: site/dl/<id>.jsx|.ffx, site/data/index.json, site/index.html.

import fs from "node:fs";
import path from "node:path";
import crypto from "node:crypto";
import { execFileSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import * as acorn from "acorn";

const PORTAL = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const ROOT = path.resolve(PORTAL, "..");
const CATALOG = path.join(PORTAL, "catalog");
const SITE = path.join(PORTAL, "site");
const DL = path.join(SITE, "dl");
const DATA = path.join(SITE, "data");
const CONFIG = JSON.parse(fs.readFileSync(path.join(PORTAL, "cave.config.json"), "utf8"));

const rel = (abs) => path.relative(ROOT, abs).split(path.sep).join("/");

// ---------------------------------------------------------------- bundling

const INCLUDE_RE = /^([ \t]*)(?:\/\/@include|#include)[ \t]+["']([^"']+)["'][ \t]*;?[ \t]*$/gm;

function readText(abs) {
  return fs.readFileSync(abs, "utf8").replace(/^﻿/, "");
}

// Inlines every //@include / #include in place, resolving paths relative to
// the including file like ExtendScript does. A file already inlined once is
// skipped, so a shared lib pulled in from two places lands in the bundle once.
function bundle(abs, ctx) {
  if (ctx.seen.has(abs)) return `// [cave] already inlined above: ${rel(abs)}`;
  ctx.seen.add(abs);
  ctx.files.push(rel(abs));
  return readText(abs).replace(INCLUDE_RE, (line, indent, spec) => {
    const target = path.resolve(path.dirname(abs), spec);
    if (!fs.existsSync(target)) {
      ctx.missing.push({ from: rel(abs), spec });
      return `${indent}// [cave] MISSING INCLUDE: ${spec}`;
    }
    return `${indent}// [cave] >>> ${spec}\n${bundle(target, ctx)}\n${indent}// [cave] <<< ${spec}`;
  });
}

function header(m, inlined) {
  const lines = [
    `/*  VFX Cave · ${m.title.en} (${m.id}) v${m.version}`,
    `    Author: ${m.author.name} · License: ${m.license}`,
    `    Source: ${CONFIG.repoUrl}/blob/${CONFIG.repoBranch}/${encodeURI(m.entry)}`,
    `    Catalog: ${CONFIG.siteUrl}#${m.id}`,
    inlined > 0
      ? `    Built by VFX Cave from ${m.entry}; ${inlined} included file(s) inlined.`
      : `    Built by VFX Cave from ${m.entry}.`,
    `    The sha256 of this file is published in the catalog index. */`,
  ];
  return lines.join("\n") + "\n\n";
}

// ------------------------------------------------------------------ checks

// Preprocessor directives are ExtendScript-only; blank them out (keeping
// offsets) so a standard parser can read the rest.
function blankDirectives(code) {
  return code.replace(/^[ \t]*#(?:target|targetengine|include|includepath|script|strict|engine)\b.*$/gm, (m) =>
    " ".repeat(m.length)
  );
}

function tryParse(code, ecmaVersion) {
  const comments = [];
  acorn.parse(code, {
    ecmaVersion,
    allowReturnOutsideFunction: true,
    allowReserved: true,
    allowHashBang: true,
    onComment: (block, text, start, end) => comments.push([start, end]),
  });
  return comments;
}

function lineOf(code, index) {
  let line = 1;
  for (let i = 0; i < index && i < code.length; i++) if (code.charCodeAt(i) === 10) line++;
  return line;
}

// ExtendScript is an ES3 engine. ES3 is the target; code that only parses as
// ES5 usually still runs, code that needs ES2015+ (let, const, arrows,
// template strings) does not.
function checkSyntax(code) {
  const prepared = blankDirectives(code);
  let es3Error = null;
  // ExtendScript accepts `const` on top of ES3. Retry with const read as var
  // (same length, offsets stay valid) before calling the file non-ES3.
  try {
    tryParse(prepared, 3);
  } catch (e) {
    if (/\bconst\b/.test(prepared)) {
      try {
        const comments = tryParse(prepared.replace(/\bconst\b/g, "var  "), 3);
        return { level: "es3", constExtension: true, comments };
      } catch {
        /* not just const: fall through to the full ladder */
      }
    }
  }
  for (const [version, level] of [[3, "es3"], [5, "es5"], [2015, "es2015"], ["latest", "modern"]]) {
    try {
      const comments = tryParse(prepared, version);
      const result = { level, comments };
      if (es3Error) result.es3Error = es3Error;
      return result;
    } catch (e) {
      if (version === 3) es3Error = { message: e.message.replace(/\s*\(\d+:\d+\)$/, ""), line: e.loc?.line ?? null };
      if (version === "latest") return { level: "invalid", comments: [], es3Error, error: { message: e.message, line: e.loc?.line ?? null } };
    }
  }
}

// Replaces comments with spaces (offsets stay valid) so the scanners below
// don't react to commented-out code.
function stripComments(code, ranges) {
  if (!ranges.length) return code;
  const chars = code.split("");
  for (const [start, end] of ranges)
    for (let i = start; i < end; i++) if (chars[i] !== "\n") chars[i] = " ";
  return chars.join("");
}

const CAPABILITIES = [
  { id: "shell", re: [/\bsystem\.callSystem\s*\(/g, /\.execute\s*\(\s*\)/g] },
  { id: "network", re: [/\bnew\s+Socket\s*\(/g, /["'`][^"'`\n]*\b(?:curl|wget|Invoke-WebRequest)\b/g] },
  {
    id: "files",
    re: [
      /\bnew\s+(?:File|Folder)\s*\(/g,
      /(?<![\w.])(?<!new\s+)(?:File|Folder)\s*\(/g,
      /\b(?:File|Folder)\.(?:openDialog|saveDialog|selectDialog|desktop|temp|userData|myDocuments|appData|current)\b/g,
      /\.(?:openDlg|saveDlg|selectDlg)\s*\(/g,
    ],
  },
  { id: "eval", re: [/(?<![\w.])eval\s*\(/g, /\$\.evalFile\s*\(/g, /\bnew\s+Function\s*\(/g] },
  { id: "prototypes", re: [/\b[A-Z]\w*\.prototype\.\w+\s*=(?!=)/g] },
  { id: "settings", re: [/\bapp\.settings\.saveSetting\s*\(/g, /\bapp\.preferences\.save\w*\s*\(/g] },
  { id: "menu", re: [/\bapp\.executeCommand\s*\(/g] },
];

// Runtime APIs missing from the ExtendScript engine unless something in the
// bundle polyfills them.
const RUNTIME = [
  { code: "json", use: /\bJSON\.(?:parse|stringify)\s*\(/g, polyfill: /\bJSON\s*=|typeof\s+JSON\s*[!=]==?|JSON\.(?:parse|stringify)\s*=/ },
  {
    code: "es5-array",
    use: /\.(?:forEach|map|filter|reduce|some|every)\s*\(/g,
    polyfill: /Array\.prototype\.(?:forEach|map|filter|reduce|some|every)\s*=/,
  },
  { code: "es5-trim", use: /\.trim\s*\(\s*\)/g, polyfill: /String\.prototype\.trim\s*=/ },
  { code: "es5-object", use: /\bObject\.(?:keys|create|defineProperty|freeze)\s*\(|\bArray\.isArray\s*\(/g, polyfill: /Object\.keys\s*=|Array\.isArray\s*=/ },
];

function findAll(code, res) {
  const hits = [];
  for (const re of res) {
    re.lastIndex = 0;
    let m;
    while ((m = re.exec(code))) hits.push(m.index);
  }
  return [...new Set(hits)].sort((a, b) => a - b);
}

function sampleLines(original, code, hits) {
  return hits.slice(0, 3).map((i) => {
    const line = lineOf(code, i);
    const text = original.split("\n")[line - 1] ?? "";
    return { line, code: text.trim().slice(0, 140) };
  });
}

function longestStringLiteral(code) {
  let max = 0;
  const re = /"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*'/g;
  let m;
  while ((m = re.exec(code))) if (m[0].length > max) max = m[0].length;
  return max;
}

function analyze(bundled, manifest) {
  const syntax = checkSyntax(bundled);
  const code = stripComments(bundled, syntax.comments);
  const capabilities = [];
  for (const cap of CAPABILITIES) {
    const hits = findAll(code, cap.re);
    const declared = manifest.permissions.includes(cap.id);
    if (hits.length || declared)
      capabilities.push({ id: cap.id, declared, detected: hits.length > 0, count: hits.length, samples: sampleLines(bundled, code, hits) });
  }
  const warnings = [];
  for (const api of RUNTIME) {
    const hits = findAll(code, [api.use]);
    if (!hits.length) continue;
    const polyfilled = api.polyfill.test(code);
    warnings.push({ code: api.code, polyfilled, count: hits.length, samples: sampleLines(bundled, code, hits) });
  }
  if (longestStringLiteral(code) > 2000) warnings.push({ code: "embedded-data", polyfilled: false, count: 1, samples: [] });
  return { syntax, capabilities, warnings };
}

// -------------------------------------------------------------------- git

function gitDate(entry, first) {
  try {
    const args = first
      ? ["log", "--diff-filter=A", "--follow", "--format=%aI", "--", entry]
      : ["log", "-1", "--format=%aI", "--", entry];
    const out = execFileSync("git", args, { cwd: ROOT, encoding: "utf8" }).trim().split("\n");
    return (first ? out[out.length - 1] : out[0]) || null;
  } catch {
    return null;
  }
}

// ------------------------------------------------------------------- build

function sha256(buf) {
  return crypto.createHash("sha256").update(buf).digest("hex");
}

function loadManifests() {
  const files = fs.readdirSync(CATALOG).filter((f) => f.endsWith(".json") && !f.startsWith("_"));
  const manifests = files.map((f) => ({ file: f, ...JSON.parse(fs.readFileSync(path.join(CATALOG, f), "utf8")) }));
  const ids = new Set();
  for (const m of manifests) {
    const problems = [];
    if (!/^[a-z0-9]+(?:-[a-z0-9]+)*$/.test(m.id ?? "")) problems.push("id must be latin kebab-case");
    if (`${m.id}.json` !== m.file) problems.push(`file name must be ${m.id}.json`);
    if (ids.has(m.id)) problems.push("duplicate id");
    for (const key of ["title", "summary"]) if (!m[key]?.ru || !m[key]?.en) problems.push(`${key}.ru and ${key}.en are required`);
    if (!Array.isArray(m.permissions)) problems.push("permissions must be an array");
    if (!fs.existsSync(path.join(ROOT, m.entry ?? ""))) problems.push(`entry not found: ${m.entry}`);
    if (problems.length) throw new Error(`catalog/${m.file}: ${problems.join("; ")}`);
    ids.add(m.id);
  }
  return manifests.sort((a, b) => a.id.localeCompare(b.id));
}

function buildItem(m) {
  const entryAbs = path.join(ROOT, m.entry);
  const ext = path.extname(m.entry).toLowerCase();
  const added = gitDate(m.entry, true);
  const updated = gitDate(m.entry, false);
  const base = {
    id: m.id,
    version: m.version,
    host: m.host,
    kind: m.kind,
    category: m.category,
    title: m.title,
    summary: m.summary,
    description: m.description,
    usage: m.usage,
    notes: m.notes,
    tags: m.tags,
    hostVersions: m.hostVersions,
    author: m.author,
    license: m.license,
    ai: m.ai,
    entry: m.entry,
    added,
    updated,
  };

  if (m.kind === "preset" || ext === ".ffx") {
    const buf = fs.readFileSync(entryAbs);
    const file = `dl/${m.id}.ffx`;
    fs.writeFileSync(path.join(SITE, file), buf);
    return {
      item: { ...base, file, size: buf.length, sha256: sha256(buf), lines: null, inlined: [], checks: { status: "ok", syntax: null, capabilities: [], warnings: [], missingIncludes: [] } },
      source: null,
    };
  }

  const ctx = { seen: new Set(), files: [], missing: [] };
  const body = bundle(entryAbs, ctx);
  const inlined = ctx.files.slice(1);
  const bundled = header(m, inlined.length) + body;
  const { syntax, capabilities, warnings } = analyze(bundled, m);

  let status = "ok";
  if (ctx.missing.length || syntax.level === "invalid" || syntax.level === "es2015" || syntax.level === "modern") status = "blocked";
  else if (capabilities.some((c) => ["shell", "network", "eval"].includes(c.id) && c.detected)) status = "review";

  const file = `dl/${m.id}.jsx`;
  const buf = Buffer.from(bundled, "utf8");
  fs.writeFileSync(path.join(SITE, file), buf);
  return {
    item: {
      ...base,
      file,
      size: buf.length,
      sha256: sha256(buf),
      lines: bundled.split("\n").length,
      inlined,
      checks: {
        status,
        syntax: { level: syntax.level, constExtension: !!syntax.constExtension, es3Error: syntax.es3Error ?? null, error: syntax.error ?? null },
        capabilities,
        warnings,
        missingIncludes: ctx.missing,
      },
    },
    source: bundled,
  };
}

function main() {
  fs.rmSync(DL, { recursive: true, force: true });
  fs.mkdirSync(DL, { recursive: true });
  fs.mkdirSync(DATA, { recursive: true });

  const manifests = loadManifests();
  const built = manifests.map(buildItem);
  const items = built.map((b) => b.item);

  const index = {
    schema: 1,
    name: CONFIG.name,
    generated: new Date().toISOString(),
    repo: CONFIG.repoUrl,
    site: CONFIG.siteUrl,
    hosts: [{ id: "aftereffects", name: "After Effects", status: "open" }, ...["blender", "houdini", "nuke", "resolve"].map((id) => ({ id, status: "soon" }))],
    items,
    revoked: [],
  };
  fs.writeFileSync(path.join(DATA, "index.json"), JSON.stringify(index, null, 2) + "\n");

  // The site is a single file: the index and every bundled source are inlined
  // so it works from disk, from GitHub Pages and as a preview without fetch.
  const sources = Object.fromEntries(built.filter((b) => b.source).map((b) => [b.item.id, b.source]));
  const payload = JSON.stringify({ index, sources }).replace(/</g, "\\u003c");
  const template = fs.readFileSync(path.join(SITE, "src", "template.html"), "utf8");
  if (!template.includes("/*CAVE_DATA*/")) throw new Error("site/src/template.html has no /*CAVE_DATA*/ marker");
  const page = template.replace("/*CAVE_DATA*/", () => payload);
  // index.html is a full document for GitHub Pages and local use; the bare
  // fragment in .preview/ is for hosts that add their own document skeleton.
  fs.writeFileSync(
    path.join(SITE, "index.html"),
    `<!doctype html>\n<html lang="en">\n<head>\n<meta charset="utf-8">\n<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n</head>\n<body>\n${page}\n</body>\n</html>\n`
  );
  fs.mkdirSync(path.join(PORTAL, ".preview"), { recursive: true });
  fs.writeFileSync(path.join(PORTAL, ".preview", "index.html"), page);

  const rows = items.map((i) => {
    const caps = i.checks.capabilities.filter((c) => c.detected).map((c) => c.id + (c.declared ? "" : "!")).join(",");
    const warns = i.checks.warnings.filter((w) => !w.polyfilled).map((w) => w.code).join(",");
    return `${i.checks.status.padEnd(8)} ${(i.checks.syntax?.level ?? "-").padEnd(8)} ${i.id.padEnd(40)} ${caps}${warns ? "  warn:" + warns : ""}`;
  });
  console.log(rows.join("\n"));
  const count = (s) => items.filter((i) => i.checks.status === s).length;
  console.log(`\n${items.length} items · ok ${count("ok")} · review ${count("review")} · blocked ${count("blocked")}`);
  console.log("capability marked with ! = detected but not declared in the manifest");
}

main();
