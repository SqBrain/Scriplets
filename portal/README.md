# VFX Cave — prototype

A working prototype of a free catalog for self-written and AI-written After Effects scripts. The seed catalog is the scripts in this repository. "VFX Cave" is a working name, set in `cave.config.json` and the site template.

The product is English-first. Russian is an optional second language for the site and the AE panel.

| Part | Where | What it does |
|---|---|---|
| Manifests | `catalog/<id>.json` | One per script: description in English and Russian, declared capabilities, AI disclosure |
| Build | `tools/build.mjs` | Inlines `//@include` into one file, checks ExtendScript compatibility, finds risky capabilities, hashes every file, writes the open index and the site |
| Site | `site/index.html` | Catalog with search and filters, script pages with checks and source, request board, keeper's path, page for authors |
| Index | `site/data/index.json` | Open JSON for the panel, launchers and AI assistants |
| Downloads | `site/dl/<id>.jsx` | Bundled scripts with a header: author, license, link to the catalog page |
| AE panel | `panel/VFX Cave.jsx` | Search, install with sha256 check, run with a capability prompt |

## Run

```sh
cd portal
npm install
npm run build   # rebuild dl/, data/index.json and index.html
npm test        # check the panel outside AE: ES3, JSON, sha256
```

Open `site/index.html` straight from disk: the data is embedded, no server needed. To publish, serve the `site/` folder from any static host, for example GitHub Pages through Actions.

## What the build checks

- Every `//@include` and `#include` is inlined in place, with paths resolved from the including file as ExtendScript does. A file included twice is inlined once. A missing path marks the script "needs work" and closes its download.
- Syntax is parsed as ES3, the ExtendScript engine. `const` is accepted because ExtendScript supports it. ES5-only syntax is a warning; ES2015+ (`let`, arrow functions, template strings) blocks the script.
- ES5 APIs without a polyfill: `JSON`, `forEach`/`map`/`filter`, `trim`, `Object.keys`. A polyfill anywhere in the bundle is noted.
- Capabilities: files, shell, network, eval, prototype changes, settings, menu commands. What's found is compared with the manifest's `permissions`; an undeclared capability is flagged.
- Shell, network and eval mark a script "needs review". Publishing isn't held up, but the badge shows.
- Each bundled file's sha256 goes into the index. The panel checks it before installing and before every run.

## The AE panel

1. Put `panel/VFX Cave.jsx` into `Scripts/ScriptUI Panels` inside the After Effects folder and restart AE.
2. Turn on `Preferences → Scripting & Expressions → Allow Scripts to Write Files and Access Network`.
3. Open `Window → VFX Cave.jsx`, click **Source…** and enter the index URL or the path to a local `portal/site/data/index.json`.
4. **Refresh**, pick an artifact, **Install**, then **Run**.

Over `https://` the panel downloads with `curl`, which ships with macOS and Windows 10 or later. Installed scripts live in the user data folder (`Folder.userData/VFX Cave`), so no admin rights are needed. The panel speaks English; to switch it to Russian, set `lang` to `ru` in `app.settings` section `VFX Cave`.

**The panel has not been run inside After Effects yet.** `npm test` covers only what can be checked outside AE: ES3 syntax, JSON reading and sha256.

## Add a script

1. Put the script with readable source into the repository.
2. Create `catalog/<id>.json` modelled on any existing manifest. The `id` is lowercase latin with hyphens and matches the file name.
3. Run `npm run build` and read the report in the console.
4. Commit the manifest together with the rebuilt `site/` folder.

Manifest fields: `id`, `version`, `entry` (path from the repository root), `host`, `kind` (`script`, `panel`, `preset`), `category`, `title`, `summary`, `description`, `usage`, `notes` (text fields have `en` and `ru`), `tags`, `hostVersions`, `author`, `license`, `ai` (`level`: `none`, `assisted`, `generated` or `unknown`, plus `model` and `humanReviewed`), `permissions`, `requires`.

## Not real yet

- "Works / broken" marks, XP and requests live only in the browser (`localStorage`). A shared catalog needs a backend or GitHub sign-in.
- Publishing through pull requests is described on the site, but there's no CI for it yet.
- Other hosts (Blender, Houdini, Nuke, Resolve) show as "soon".
- `ai.level` for the scripts in this repository is `unknown`: mark where an AI helped.
