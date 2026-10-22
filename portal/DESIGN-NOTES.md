# VFX Cave: inspiration brief (Oct 2026)

Goal: replace "dark charcoal + one amber + pixel font + ALL-CAPS mono eyebrows" with a design language that only a motion/VFX script catalog could have.

## Part A: reference portals

Each entry gives what is distinctive, what to borrow, and what to avoid.

1. **PICO-8 BBS**, lexaloffle.com/bbs. Each cart is a `.p8.png`: the label image *is* the file, with 32 KB of code packed into a cartridge-shaped PNG. Listings become walls of identical cartridge silhouettes with 128×128 pixel labels; cart pages embed a playable player, and the source is one click away ([manual](https://www.lexaloffle.com/dl/docs/pico-8_manual.html)). **Borrow:** one fixed object silhouette for every script (a layer chip or cartridge) holding an author-supplied 16:9 "label frame"; "Source" as a first-class tab. **Avoid:** forum-thread listings and pagination.

2. **Shadertoy**, shadertoy.com. Thumbnails stay static until hover, then the shader compiles and runs live at low resolution ([pouët](https://www.pouet.net/topic.php?which=10194)). The shader page puts the canvas on the left and the code on the right, with Image/Common/Buffer tabs, iChannel0–3 input slots, and a time/fps/resolution readout. **Borrow:** a hover-to-play WebM of the script's result; an "inputs" strip (needs: active comp · selected layers · footage). **Avoid:** one failing preview blacking out the grid; degrade each card on its own.

3. **Dwitter + tixy.land.** Dwitter is 140 characters of JS: canvas above, code below, a character count, and remixes that link to their parent ([about](https://dwitter.net/about)). tixy is a single 16×16 dot grid driven by a 32-character `(t,i,x,y)=>` input, shared by URL ([write-up](https://discuss.pixls.us/t/discovering-tixy-land/21175)). **Borrow:** a visible fix/fork lineage ("fix of #0412 by @x"); show source size; a tiny generative "signature" per script. **Avoid:** code-golf obscurity.

4. **Lospec Palette List**, lospec.com/palette-list. Each entry is a horizontal swatch strip with colour count, downloads and tags; there is a monthly contest, a random-palette button, and many export formats (PNG, .ase, .gpl, .hex) ([guide](https://lospec.com/palette-list/importing-palettes)). **Borrow:** a *hash strip*: 16 cells generated from each version's sha256 and mapped to the AE label colours, so tampering or a new version visibly changes the fingerprint. Offer .jsx/.jsxbin/.zip the way Lospec offers palette formats. **Avoid:** chrome colour that competes with content.

5. **Poly Haven**, polyhaven.com. CC0 is stamped everywhere. The grid is uniform; asset pages pair a big preview with a right column of technical facts (author, date, dynamic range for HDRIs) and a split download control (resolution × format). **Borrow:** a split "host version × format" download, plus a dense spec sidebar. **Avoid:** image-only tiles that look identical at small sizes; scripts need text-forward cards.

6. **itch.io.** Creators theme their own pages (colours, banner, fonts). Browse covers are 315×250 crops of 630×500 art, and animated GIFs play only on hover ([admin](https://itch.io/t/497387/how-do-you-enable-a-moving-picture-for-a-cover-image)). Jams show countdowns and entry grids. **Borrow:** run the request board as jams, with a deadline countdown and an entries grid. **Avoid:** per-page theming chaos; allow one label colour per script, nothing more.

7. **Civitai**, civitai.com. Image-led portrait cards carry overlaid type and base-model chips. The Buzz currency drives tips, Early Access and bounties with deadlines and entry caps ([bounty guide](https://education.civitai.com/civitais-guide-to-bounties/)). **Borrow:** bounty states (open, entries, awarded) and base-host chips (`AE 2025`, `ES3`). **Avoid:** confusing multi-colour currencies and overloaded cards; XP must never look like money.

8. **uiverse.io.** Cards render the live element itself, not a screenshot, on a background the author picks; 7k+ MIT elements; a "Get code" button on every card ([repo](https://github.com/uiverse-io/galaxy)). **Borrow:** render a faithful HTML mock of the script's ScriptUI panel right on the card. **Avoid:** neon-gradient gallery noise.

9. **Raycast Store**, raycast.com/store. Every extension page lists each command it adds, with icon, screenshots, author and install count. It is keyboard-first: ⏎ installs ([manual](https://manual.raycast.com/store)). **Borrow:** a "What it adds" list (panels, menu items, effects); ⌘K navigation. **Avoid:** glossy glow gradients, the generic dev-tool look.

10. **Blender Extensions**, extensions.blender.org. The manifest declares permissions (`files`, `network`, `clipboard`, `camera`, `microphone`), each with a justification of 64 characters or fewer ([manual](https://docs.blender.org/manual/en/dev/advanced/extensions/getting_started.html)). Compatibility is a version range. **Borrow:** this exact model for capability badges: an auto-detected badge plus the author's one-line reason ("shell: calls ffmpeg to transcode"), flagged when detection and declaration disagree. **Avoid:** its generic web-kit look.

11. **Nukepedia**, nukepedia.com. The taxonomy mirrors Nuke's own toolbar (Image, Draw, Time, Channel, Colour, Filter, Keyer, Merge, Transform, 3D). Every tool lists compatible versions, OS, downloads and rating ([example](https://www.nukepedia.com/tools/gizmos/other/slate)). **Borrow:** categories that mirror AE's own menus (Layer ›, Effect ›, Window ›). **Avoid:** login-gated downloads and 2010-era CMS chrome.

12. **Are.na / Cosmos**, are.na and cosmos.so. Are.na is a monochrome equal-square block grid where text, link, image and file weigh the same, and every block shows its connected channels. Cosmos is chrome-free black masonry. **Borrow:** user-curated collections ("channels") and a "connected to N collections" count. **Avoid:** zero metadata, which kills trust.

13. **Teenage Engineering**, teenage.engineering. Full-bleed bands of off-white (#F5F5F5) and near-black sit on a rigid hairline grid, with tiny light grotesk type; orange appears only on the products ([analysis](https://blakecrosley.com/es/guides/design/teenage-engineering)). **Borrow:** install guides styled as numbered, diagrammed field-manual pages; hairline cell dividers. **Avoid:** hair-thin, tiny type.

14. **Playdate Catalog**, play.date/games. A curated store where every shot is a 1-bit 400×240 screen, so the constraint is the identity ([Wikipedia](https://en.wikipedia.org/wiki/Playdate_(console))). **Borrow:** one strict preview format and editorial staff picks. **Avoid:** curation as a bottleneck.

15. **cables.gl.** Ops are small boxes wired with cables, and every op has a dotted namespace (`Ops.Gl.Texture`) and its own doc page ([walkthrough](https://cables.gl/docs/0_howtouse/ui_walkthrough/ui_walkthrough)). **Borrow:** namespaced IDs (`ae.rig.bounce`), and request → script → fix lineage drawn as wires. **Avoid:** a node graph as primary navigation.

16. **Motion-industry sites (aescripts, Video Copilot, Buck/Ordinary Folk).** aescripts is dense e-commerce with explicit Requirements/Compatibility blocks; Video Copilot uses dark chrome product boxes; Buck and Ordinary Folk are reel-first video portfolios. **Borrow:** the compatibility block and confident full-bleed motion. **Avoid:** lens-flare, chrome and storefront clichés.

## Part B: the vernacular

**AE label colours** (defaults; users can retheme them; [get-ae-labels](https://github.com/motiondeveloper/get-ae-labels)): 1 Red #B53838 · 2 Yellow #E4D84C · 3 Aqua #A9CBC7 · 4 Pink #E5BCC9 · 5 Lavender #A9A9CA · 6 Peach #E7C19E · 7 Sea Foam #B3C7B3 · 8 Blue #677DE0 · 9 Green #4AA44C · 10 Purple #8E2C9A · 11 Orange #E8920D · 12 Brown #7F452A · 13 Fuchsia #F46DD6 · 14 Cyan #3DA2A5 · 15 Sandstone #A89677 · 16 Dark Green #1E401E (plus 0 None).

**Timeline:** numbered layer rows; switch columns (eye, solo, lock, shy, fx, motion blur, 3D); label-coloured duration bars; work-area bar; a blue playhead (CTI) with a current-time readout. **Keyframes:** ◆ linear, ● auto-Bezier, hourglass for Bezier/Easy Ease, a square-ish shape for hold. Mixed keys draw the left half from the incoming interpolation and the right half from the outgoing ([Adobe](https://helpx.adobe.com/after-effects/desktop/animate-in-after-effects/animation-keyframes/keyframe-interpolation.html)).

**Time formats:** `0;00;05;12` (semicolons mean 29.97 drop-frame), `0:00:05:12` non-drop, frames `00132`, feet+frames `8+04` for 35 mm ([Adobe](https://helpx.adobe.com/after-effects/using/timecode-time-display-units.html)).

**Comp viewer:** transparency checkerboard; safe margins (AE defaults are 10% action-safe and 20% title-safe; centre-cut 32.5%/40%; [PVC](https://www.provideocoalition.com/unsafe_areas/)); rulers; channel toggles (RGB/R/G/B/A); `50% ▾` zoom; Full/Half/Quarter resolution.

**Render-queue statuses:** Unqueued · Queued · Needs Output · Rendering · Will Continue · User Stopped · Err Stopped · Done ([docs](https://ae-scripting.docsforadobe.dev/renderqueue/renderqueueitem/)). This is a ready-made status vocabulary: *Needs Output* means install notes are missing, *Err Stopped* means broken in 2025, *Done* means verified.

**Script culture:** `#target aftereffects`; `app.beginUndoGroup("Bounce It")`, whose name is what appears in Edit › Undo, so show it as the script's "undo name"; `.jsx/.jsxbin/.ffx`; docked panels in `ScriptUI Panels/`.

**Node graphs:** Nuke's DAG has grey tiles, Backdrops (tinted, titled rectangles that group nodes), Dots and postage stamps. Channels are `layer.channel` (`rgba.red`, `depth.Z`; [Foundry](https://learn.foundry.com/nuke/content/comp_environment/channels/understanding_channels.html)), and EXR names can nest, as in `light1.specular.R` ([OpenEXR](https://cary-ilm-openexr.readthedocs.io/en/latest/MultiViewOpenEXR.html)). Houdini flags: bypass is yellow, display is blue, template is purple ([SideFX](https://www.sidefx.com/docs/houdini/network/flags.html)).

**Film and broadcast:** SMPTE 75% bars #BFBFBF #BFBF00 #00BFBF #00BF00 #BF00BF #BF0000 #0000BF, plus PLUGE near-blacks at −4/0/+4 IRE ([Wikipedia](https://en.wikipedia.org/wiki/SMPTE_color_bars)). Slate fields: PROD · ROLL · SCENE · TAKE · DIRECTOR · CAMERA · DATE, with diagonal-striped sticks. Leaders: the SMPTE Universal leader counts 8→2 with a clock sweep and a 2-pop; the Academy leader counts in feet with upside-down numerals ([Wikipedia](https://en.wikipedia.org/wiki/Film_leader)). Also: BITC burn-ins (white mono timecode in a black box in a corner) and the 24-patch ColorChecker.

### Direction 1: "Timeline" (the catalog is a composition)
The listing is an AE timeline. Each script is a layer row: index, label chip, name, and switch columns repurposed as capability lights (files / shell / network / eval). The time ruler's ticks are **host versions** (CC 2019 … 2025, 2026). Each script's label-coloured bar is solid where votes say "works", transparency-checkerboard where untested, and red-hatched where "broken". Keyframe glyphs on the bar mark events: ◆ release, hourglass = verified, hold-square = broken report. **Palette:** bg #1B1B1B, panel #232323, row #2C2C2C, hairline #3A3A3A, text #D6D6D6 / #8C8C8C, playhead #2D8CEB. Categories use the 16 labels (Purple rigging, Yellow text, Aqua shapes, Sea Foam expressions, Red render/export, Sandstone utilities…). **Type:** Onest for UI and JetBrains Mono for code and readouts (both Latin + Cyrillic). **Signature:** a draggable blue playhead reading "AE 25.x". Drop it on your version and every row re-sorts and dims by compatibility.

### Direction 2: "Slate & Leader" (every script is a take)
The script page header is a clapper slate with SCRIPT / TAKE (version) / HOST / AUTHOR / DATE / SHA-256 in chalk-white fields. The sticks are black-and-white stripes that turn into SMPTE bar colours when every trust check passes. The listing is a contact sheet of 16:9 frames with burn-ins: filename top-left, version top-right, capability boxes bottom-left. Install is a 3·2·1 leader countdown with a clock sweep; copying the command fires a "2-pop" flash. Empty and 404 states show bars with PLUGE. **Palette:** #0D0D0D, slate #F1EFE8, chalk #CFCAB8, the seven bar colours above, burn-in #000000CC on #FFFFFF. **Type:** Sofia Sans Extra Condensed (slate lettering), Martian Mono (burn-ins), Handjet (LED-style timecode); all three cover Latin and Cyrillic. **Signature:** the sticks snap shut when you copy or install.

### Direction 3: "Node Tree" (the community is a comp graph)
Script cards are node tiles with ports: inputs on the left (comp, selected layers, footage) and outputs on the right (keys, layers, files). Capability flags sit on the tile edge like Houdini flags and light up when used. Home-page categories are Nuke Backdrops: large tinted, titled panels. On the request board, request nodes are wired to the scripts that answered them, and fixes and forks are drawn as wires. Metadata uses EXR syntax (`caps.network`, `ae.2025.works = 41`). **Palette:** canvas #2A2A2A with a #343434 dot grid, tiles #3D3D3D, wires #8F8F8F, selected wire #F2C230, backdrops as label colours at 18% opacity, flag yellow #E8C547, flag blue #3E8BD8. **Type:** Tektur for node titles, Geologica for UI, Geist Mono for code (all Latin + Cyrillic). **Signature:** hover a tile and its lineage lights up through the graph: what it answered, forked and fixed.

### Pick: Direction 1, with Direction 2's slate as the script-page header
Here the metaphor *is the data*: compatibility per host version, category and capabilities become bars, labels and switches. Every AE user reads it instantly, and it ports cleanly, since Blender, Nuke and Resolve each just get their own ruler of versions. The 16 label colours replace the lone amber with a native multi-colour system no SaaS template has; the hash strip from #4 can end each bar.

**Risks:** Adobe pastiche (redraw the glyphs; no Adobe icons or logos); contrast (Dark Green, Brown and Purple only as fills under light text; check 4.5:1); mobile (collapse the ruler into a segmented mini-bar).
