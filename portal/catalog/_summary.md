# VFX Cave seed catalog: summary

Source: `/home/user/Scriplets` (the user's AE scripts, repo root + `true/`). One manifest per entry in `portal/catalog/<id>.json`.
**43 entries**: 41 scripts, 1 panel, 1 preset. `vfx-cave-panel.json` was already in the folder and was not touched.
Everything below comes from reading the code. Nothing was run inside After Effects, so a "likely bug" means it needs a test in AE.

## Entries

| id | entry | kind | category | permissions | requires | min AE |
|---|---|---|---|---|---|---|
| `add-controllable-matte` | `addControllableMatte.jsx` | script | mattes | `files`, `shell`, `prototypes`, `settings`, `menu` | `lib`, `MattePresetBinnaryCode.jsx` | — |
| `add-cross-to-black` | `add_cross_to_black.jsx` | script | comp | — | — | — |
| `add-flicker` | `addFlicker.jsx` | script | animation | — | — | — |
| `add-time-with-slider` | `addTimeWithSlider.jsx` | script | expressions | — | — | — |
| `add-wiggle` | `add wiggle.jsx` | script | animation | — | — | — |
| `add-wiggle-with-sliders` | `Add_Wiggle_with_Sliders.jsx` | script | animation | — | — | — |
| `alert-reflect` | `Alert Reflect.jsx` | script | utility | — | — | — |
| `align-points` | `alignPoints.jsx` | script | masks-shapes | — | — | — |
| `animation-down-up` | `animationDownUp.jsx` | script | animation | — | `lib` | — |
| `arrange-layers-to-bot-layer` | `arrangeLayersToBotLayer.jsx` | script | layers | — | — | — |
| `bake-still-expression` | `bakeStillExpression.jsx` | script | expressions | — | — | — |
| `botya-create-matte` | `BOTYA_createMatte.jsx` | script | mattes | — | — | 2023 (23.0) |
| `change-comp-resolution` | `changeCompResolution.jsx` | script | comp | — | — | — |
| `change-comps-duration` | `Change Comps.jsx` | script | comp | — | — | — |
| `change-comps-precompose` | `changeComps;.jsx` | script | comp | — | — | — |
| `convert-masks-to-shapes` | `ConvertMasksToShapes.jsx` | script | masks-shapes | — | `lib` | — |
| `create-and-render` | `createAndRender.jsx` | script | render | `menu` | — | — |
| `create-fitting-shape-layer` | `CreateFittingShapeLayer.jsx` | script | layers | — | — | — |
| `create-palette` | `сreatePalette.jsx` | script | color | — | — | — |
| `duplicate-layer-and-matte-to-selected-layers` | `Duplicate Layer and Matte to selected layers.jsx` | script | mattes | — | — | 2023 (23.0) |
| `duplicate-layer-by-masks` | `duplicateLayerByMasks.jsx` | script | masks-shapes | — | — | — |
| `duplicate-layer-or-shape` | `Duplicate Layer or Shape.jsx` | script | layers | — | — | — |
| `expressions-helper` | `Expressions Helper.jsx` | script | expressions | — | — | — |
| `link-opacity-to-parent` | `linkOpacityToParent.jsx` | script | expressions | — | — | — |
| `loop-footage` | `loopFootage.jsx` | script | layers | — | — | — |
| `made-bunch-of-masks` | `madeBunchOfMasks.js` | script | animation | — | `../libs/index.jsx` | — |
| `make-matte` | `makeMatte.jsx` | script | mattes | — | — | 2023 (23.0) |
| `make-shadow` | `makeShadow.jsx` | script | layers | — | — | — |
| `matte-layers` | `matteLayers.jsx` | script | mattes | — | — | 2023 (23.0) |
| `plashka-creator` | `Plashka creator.jsx` | script | masks-shapes | — | `lib` | 2020 (17.0.1) |
| `precomp` | `precomp.jsx` | script | comp | — | — | — |
| `ps-rect-controller` | `true/PS_Rect_Controller.ffx` | preset | masks-shapes | — | — | — |
| `random-source` | `randomSource.jsx` | script | layers | — | — | — |
| `rearrange-keys` | `rearangeKeys.jsx` | script | keyframes | — | — | 2022 (22.6) |
| `rename-layers` | `rename_layers.jsx` | script | layers | — | — | — |
| `round-path-points` | `Round pathPoints.jsx` | script | masks-shapes | — | — | — |
| `rule-tangents` | `Rule tangets.jsx` | panel | keyframes | `files`, `shell`, `prototypes`, `settings`, `menu` | `lib` | — |
| `set-opacity` | `Set Opacity.jsx` | script | layers | — | — | — |
| `shift-channels` | `ShiftChannels.jsx` | script | color | — | — | — |
| `show-hierarchy-of-property` | `showIerarhyOfProperty.jsx` | script | utility | — | — | — |
| `split-text-on-words` | `SplitTextOnWords.jsx` | script | text | `files`, `shell`, `prototypes`, `settings`, `menu` | `lib` | — |
| `trim-comp-to-layers` | `trimCompToLayers.jsx` | script | comp | — | — | — |
| `trim-layers` | `trimLayers.jsx` | script | layers | — | — | — |

### How permissions were counted

- The build (`portal/tools/build.mjs`) inlines `//@include` files and scans the whole bundle with regexes, so every function in `lib/index.jsx` counts, even ones the script never calls. The manifests declare what is **in the shipped bundle**, so the build shows no undeclared "!" capabilities. I checked this with a read-only copy of the build's bundle, syntax and capability checks. For every entry, declared = detected.
- For the three scripts that include `lib/index.jsx` (`rule-tangents`, `split-text-on-words`, `add-controllable-matte`), the bundle brings in `files`, `shell`, `prototypes`, `settings` and `menu`. What actually runs:
  - `prototypes`: always. `lib/prototypeLevelFunctions.jsx` assigns `AVLayer.prototype.instance`, `Property.prototype.containingComp` and others at include time.
  - `files` + `menu`: only `add-controllable-matte`. It writes `mattePreset.ffx` into `Folder.userData/…`, and if file access is off it runs `app.executeCommand(2359)` to open Preferences.
  - `shell` (`openURL` → `system.callSystem`) and `settings` (`saveSetting` → `app.settings.saveSetting`): dormant. No catalog script calls them.
  The entries' notes say this. `shell` is detected, so the build will mark these three "review". To get honest minimal permissions, include only the lib modules a script needs instead of `lib/index.jsx`, or tree-shake the bundle.
- `made-bunch-of-masks` has `permissions: []` because its include does not resolve, so nothing gets inlined. Once it is pointed at `lib/index.jsx`, it needs the same five as the others.
- `requires: ["lib"]` marks any include of `lib/index.jsx` or a single `lib/*` file. `add-controllable-matte` also requires `MattePresetBinnaryCode.jsx`. `made-bunch-of-masks` lists its literal, broken include `../libs/index.jsx`.

### Minimum AE versions (`hostVersions.min`)

| min | why | entries |
|---|---|---|
| 2023 (23.0) | `Layer.setTrackMatte()` arrived with the new track matte model in AE 23.0 | `botya-create-matte`, `duplicate-layer-and-matte-to-selected-layers`, `make-matte`, `matte-layers` |
| 2022 (22.6) | `Property.keyLabel()` / `setLabelAtKey()` (keyframe color labels) | `rearrange-keys` |
| 2020 (17.0.1) | Dropdown Menu Control + `Property.setPropertyParameters()` | `plashka-creator` |

Every other entry is `null`. They use long-standing APIs. The `parseFloat(app.version) >= 8` check in `Rule tangets.jsx` only rules out pre-CS3, so it is not a real requirement. `plashka-creator` and `add-controllable-matte` set `trackMatteType` the legacy way, which still works in 23.0+.

## Skipped files

| file | reason |
|---|---|
| `lib/*.jsx` (17 files) | Shared library. The build inlines it into the scripts that include it. |
| `MattePresetBinnaryCode.jsx` | A data module: one line, `const BIN_PRESET_MATTE = "RIFX…"` (a binary `.ffx` stored as a string). Only `addControllableMatte.jsx` uses it (checked with grep), and it appears in that entry's `requires`. |
| `Rule tangets.jsxbin`, `addControllableMatte.jsxbin` | Compiled JSXBIN: can't be reviewed, and I could not check that it matches the sources. |
| `addControllableMatte.rar` | RAR v5 archive. Not extracted. |
| `сreatePalette.txt` | Byte-identical to `сreatePalette.jsx`: `diff` shows no output and both are 3973 bytes. |
| `rearangeKeys.js` | Older copy of `rearangeKeys.jsx` (see Duplicates). |
| `Divide text on lines.jsx` | **Not a working script**, an unfinished stub. It includes `lib/index.jsx`, loops over the selected text layers and reads `layer(TEXT_)`, where `TEXT_` is not defined anywhere. It either does nothing or throws a ReferenceError, and it never creates anything. `SplitTextOnWords.jsx` does the job it was meant for. Add a manifest only if you want to show stubs. |
| `Expressions Changer.jsx` | **Not a working script.** It defines `convertExpressionsToText(path)` but never calls it, so running the file does nothing. The function is also broken: `createIndexFile(files)` is called with the wrong arguments, it checks `Folder.exists` on the class instead of the instance, and the writer function is never called. Finished, it would need `files`. |
| `package.json`, `.vscode/launch.json`, `reports/`, `research_notes/`, `portal/` | Not AE scripts. |

## Duplicates and overlaps

- **`сreatePalette.txt` = `сreatePalette.jsx`**, an exact copy. Only the `.jsx` has an entry (`create-palette`).
- **`rearangeKeys.js` vs `rearangeKeys.jsx`**: the only difference is line 25, `t1 + f * e` (.js) vs `t1 + f * e * num` (.jsx). The `.js` asks for the frame step and then ignores it, always using 1 frame, so it is the older version. One entry: `rearrange-keys` → `rearangeKeys.jsx`.
- **`Change Comps.jsx` vs `changeComps;.jsx`**: different scripts, so two entries.
  - `change-comps-duration`: `Change Comps.jsx` sets every selected comp to 90 s. Its rename and resize code is commented out.
  - `change-comps-precompose`: `changeComps;.jsx` precomposes `.MP4` layers into 25 fps comps under a `Scenes` folder. It also contains an uncalled 1920×1080 resize function.
- Overlaps worth merging or cross-linking later:
  - `round-path-points` ≈ `align-points`. Both snap to 0.5 px. `align-points` also handles keyframes and has an undo group.
  - `add-wiggle` vs `add-wiggle-with-sliders`: sliders on each layer vs a shared "Controller". `add-time-with-slider` copies the same controller code (its function is even named `SQB_addWigglesWithSliders`).
  - `plashka-creator` vs `add-controllable-matte`: two plate/matte rigs.
  - `precomp` vs the `precompose()` helper inside `changeComps;.jsx`.
  - `botya-create-matte` (Shadow mode) vs `make-shadow`.

## Suspicious code, bugs, platform notes

**Build-level findings (for `tools/build.mjs`):**
- **The build will block 12 working scripts because of `const`.** ExtendScript supports `const`, and those scripts run in AE. The build parses with acorn at ES3/ES5, and `const` only parses at ES2015. With `const` → `var`, all 12 parse as ES3, so `const` is the only reason. Affected: `botya-create-matte`, `convert-masks-to-shapes` (via `lib/constants.jsx`), `expressions-helper`, `plashka-creator`, `rule-tangents`, `split-text-on-words`, `add-controllable-matte`, `animation-down-up` (via `lib/constants.jsx`), `change-comp-resolution`, `create-and-render`, `duplicate-layer-by-masks`, `create-palette`. Suggestion: allow `const`, for example by rewriting it to `var` before the ES3 parse, and keep blocking `let`, arrows, template strings and classes.
- **Broken include (intended demo):** `madeBunchOfMasks.js` → `//@include "../libs/index.jsx"`, which resolves outside the repo and does not exist. It was probably meant to be `lib/index.jsx`. The build will mark it blocked.
- **Dormant capabilities in `lib/index.jsx`** (see permissions above) send the three lib-index scripts to "review" because of `shell`.
- The `json` runtime warning will fire for the lib-index scripts: `lib/appLevelFunctions.jsx` uses `JSON.parse`/`stringify` with no polyfill. These functions are never called. `add-controllable-matte` will also get `embedded-data`, from the ~14 KB string in `MattePresetBinnaryCode.jsx`.

**Scripts that likely fail as written (test in AE before publishing):**
- `add-controllable-matte`: `lib/fileDataFunctions.jsx#getUserDataFolder()` uses `AUTHOR_NAME` and `SCRIPT_NAME`, which are defined nowhere. The script defines `authorName` and `scriptName` instead. With file access enabled, it stops with a ReferenceError before doing anything. `checkPrefs()` can also return `undefined`.
- `made-bunch-of-masks`: the broken include. Then `val + dist` does array math. The tile pattern covers a square, so non-square layers are only partly covered.
- `plashka-creator`: `center - anch` and `pos + offset` do array math, plus a reassigned `const`.
- `rule-tangents`: the degrees→radians conversion reassigns `const` variables. The Out Angle handlers check `lockLength` instead of `lockAngle`. It crashes when no spatial property is selected.
- `split-text-on-words`: the lib's `moveAnchorToCenter()` puts the anchor at bottom-center (`y = height`, not `height / 2`), and it uses array math when Position is keyframed.
- `change-comp-resolution`: Cancel → TypeError on `null`. The `heigth` typo breaks the `--` option. `mul` in Alt mode does array math.

Background: ExtendScript has no vector math on arrays (`[1,2] + [3,4]` gives a string, `-` gives NaN), unlike the expression engine. A `const` in ExtendScript can't be reassigned. Both are flagged as "likely" because I could not run anything.

**Smaller bugs noted in the entries:**
- `add-wiggle`: the inner loop reuses `i`, so multi-layer selections skip layers.
- `rearrange-keys`: `obj.spatial == true` is a comparison, not an assignment, so spatial tangents are never restored. Cancel breaks the run.
- `expressions-helper`: `compHalf` calls `.replace(regex)` with no replacement, which inserts "undefined", and it references the undefined `fVar`.
- `duplicate-layer-or-shape`: `grp.sort()` sorts indices as strings.
- `duplicate-layer-by-masks`: deletes the layer if it has no masks.
- `make-matte`: strips all effects from the original.
- `precomp`: the comp name is `undefined` outside KBar, and the time offset comes from `comp.layer(1)` instead of the selected layer.
- `random-source`: errors when no comps are selected in Project.
- `create-palette`: the prompt labels for values 4 and 5 are swapped relative to the code.
- `create-fitting-shape-layer`: with nothing selected it moves the new layer before itself.
- Leftover debug alerts: `round-path-points`, `align-points`.
- No undo group: `set-opacity`, `round-path-points`, `trim-comp-to-layers`, `create-and-render`.

**Localization (matters for RU users with a Russian AE UI):** these scripts use English display names instead of match names, so they may fail in localized AE:
- `"Fast Box Blur"` (should be `ADBE Box Blur2`): `botya-create-matte`, `make-shadow`
- `"Choke Matte"`: `make-matte`
- the `"Add to Adobe Media Encoder Queue..."` menu name: `create-and-render`

**Project-specific / hard-coded:** `change-comps-duration` (90 s), `change-comps-precompose` (a `Scenes` folder, uppercase `.MP4`, 25 fps), `arrange-layers-to-bot-layer` (a layer named "Shape Layer 1" with "Roundness" and "Color Control" effects), `create-and-render` (comps named "… 1/2/3").

**Windows-only code:** none in the catalog scripts. The only OS-specific code is `lib/fileDataFunctions.jsx#openURL()`, which runs `cmd /c start` on Windows and `open` on macOS through `system.callSystem`. It covers both platforms and nothing calls it.

**Latent bugs in `lib/`** (unreachable from the current entries, but worth fixing before more scripts use the library):
- Undefined globals: `SCRIPT_NAMESafe` (appLevelFunctions), `CONTROL_LAYER_NAME` (controllerFunctions), `logActions`/`logFile` (highOrderFunctions), `curComp` in `timeToFrames` (mathModule), `NO_ALERT`/`PROPS_WITH_KEYS` (Ease_Keyframes), `VPATH`/`MASKSHAPE` (checkFunctions).
- `REGSTART`… in `newRegExp` (supportingFunctions), while `constants.jsx` defines `REG_START`…
- `isArray()` is always true (`!array instanceof Array`).
- `createResourceFile()` treats `binary` as always true (`binary || true`).
- `copyKeys()` references the undefined `keyObj`.

## Naming notes

- ids follow the file names. Spelling was fixed where the file name has a typo: `rule-tangents` (`Rule tangets.jsx`; the script calls itself "Rule Tangents"), `show-hierarchy-of-property` (`showIerarhyOfProperty.jsx`), `rearrange-keys` (`rearangeKeys.jsx`).
- `create-palette` for `сreatePalette.jsx`, whose first letter is Cyrillic.
- `change-comps-duration` and `change-comps-precompose` tell apart the two "change comps" files.
- `split-text-on-words` keeps the file-name id, but the script splits text into **lines**, and its title and description say so.
- `entry` paths match the files on disk exactly (spaces, Cyrillic `с`, `;`).
- The preset sits in a folder named `true/`, which looks accidental.
- `ai.level` is `unknown` everywhere. The author should fill it in.
