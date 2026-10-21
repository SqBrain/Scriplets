// Checks the pure parts of the AE panel outside After Effects:
// ES3 syntax, the JSON reader and SHA-256 against Node's own implementations.
//
//   node tools/test-panel.mjs   (run after npm run build)

import fs from "node:fs";
import path from "node:path";
import vm from "node:vm";
import crypto from "node:crypto";
import assert from "node:assert/strict";
import { fileURLToPath } from "node:url";
import * as acorn from "acorn";

const PORTAL = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const source = fs.readFileSync(path.join(PORTAL, "panel", "VFX Cave.jsx"), "utf8");

acorn.parse(source, { ecmaVersion: 3, allowReserved: true });
console.log("ok   panel parses as ES3");

const ctx = { CAVE_TEST: {} };
vm.runInNewContext(source, ctx);
const lib = ctx.CAVE_TEST.lib;
assert.ok(lib, "panel did not expose its helpers to the test harness");

const sha = (buf) => crypto.createHash("sha256").update(buf).digest("hex");
const latin1 = (buf) => buf.toString("latin1");

for (const sample of ["", "abc", "a".repeat(55), "a".repeat(56), "a".repeat(64), "пещера ▲ cave", crypto.randomBytes(5000)]) {
  const buf = Buffer.isBuffer(sample) ? sample : Buffer.from(sample, "utf8");
  assert.equal(lib.sha256(latin1(buf)), sha(buf), `sha256 mismatch for a ${buf.length}-byte sample`);
}
console.log("ok   sha256 matches node:crypto on edge-case samples");

const indexText = fs.readFileSync(path.join(PORTAL, "site", "data", "index.json"), "utf8");
const index = JSON.parse(indexText);
// Objects from the vm context have that context's prototypes; compare as data.
const parse = (text) => JSON.parse(JSON.stringify(lib.parseJSON(text)));
assert.deepEqual(parse(indexText), index);
assert.deepEqual(parse('{"a":[1,-2.5e3,true,false,null],"b":"\\u043f\\"\\n"}'), { a: [1, -2500, true, false, null], b: 'п"\n' });
assert.throws(() => lib.parseJSON('{"a":1,}'));
console.log(`ok   JSON reader matches JSON.parse on index.json (${index.items.length} items)`);

for (const item of index.items) {
  const buf = fs.readFileSync(path.join(PORTAL, "site", item.file));
  assert.equal(lib.sha256(latin1(buf)), item.sha256, `${item.id}: panel sha256 differs from the index`);
}
console.log("ok   panel sha256 matches the index for every downloadable file");

assert.equal(lib.matches(index.items[0], ""), true);
assert.equal(lib.matches(index.items[0], "zzzz-no-such-word"), false);
assert.equal(lib.SAFE_URL.test("https://sqbrain.github.io/Scriplets/dl/a.jsx"), true);
assert.equal(lib.SAFE_URL.test('https://x.io/a" & calc'), false);
assert.equal(lib.SAFE_URL.test("http://x.io/a"), false);
console.log("ok   search and URL guard");
