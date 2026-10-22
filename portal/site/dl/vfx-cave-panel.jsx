/*  VFX Cave · VFX Cave panel (vfx-cave-panel) v0.1.0
    Author: VFX Cave · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/portal/panel/VFX%20Cave.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#vfx-cave-panel
    Built by VFX Cave from portal/panel/VFX Cave.jsx.
    The sha256 of this file is published in the catalog index. */

/*  VFX Cave panel (prototype)
    Finds, installs and runs scripts from the VFX Cave catalog inside After Effects.

    Install: put this file into Scripts/ScriptUI Panels inside the After Effects
    folder, restart AE, open Window > VFX Cave.jsx.

    What it does and why it needs these permissions:
    - reads the open catalog index (data/index.json) from a URL or a local folder;
    - downloads with curl (macOS, Windows 10+) because ExtendScript sockets
      can't speak HTTPS; this needs "Allow Scripts to Write Files and Access
      Network" in Preferences > Scripting & Expressions;
    - checks every downloaded file against the sha256 in the index and refuses
      files that don't match or are revoked;
    - shows what a script can do (files, shell, network, eval) and asks once
      before running such a script;
    - runs installed scripts with $.evalFile from the user data folder, so no
      admin rights are needed.

    ES3 only: the ExtendScript engine has no JSON, let/const or arrow functions. */

(function (thisObj) {
    var SECTION = "VFX Cave";
    var DEFAULT_SOURCE = "https://sqbrain.github.io/Scriplets/data/index.json";
    var RISKY = { shell: true, network: true, eval: true };

    // ------------------------------------------------------------ JSON
    // A small JSON reader so the panel doesn't depend on a JSON object that
    // a clean AE install may lack, and never evals downloaded text.
    var ESCAPES = { '"': '"', "\\": "\\", "/": "/", b: "\b", f: "\f", n: "\n", r: "\r", t: "\t" };

    function parseJSON(text) {
        var i = 0, n = text.length;

        function fail(msg) { throw new Error("index.json: " + msg + " at " + i); }
        function ws() {
            var c;
            while (i < n) {
                c = text.charCodeAt(i);
                if (c === 32 || c === 9 || c === 10 || c === 13) i++;
                else break;
            }
        }
        function str() {
            var out = "", q, b, e;
            i++;
            for (;;) {
                q = text.indexOf('"', i);
                if (q === -1) fail("unterminated string");
                b = text.indexOf("\\", i);
                if (b === -1 || b > q) {
                    out += text.substring(i, q);
                    i = q + 1;
                    return out;
                }
                out += text.substring(i, b);
                e = text.charAt(b + 1);
                if (e === "u") {
                    out += String.fromCharCode(parseInt(text.substr(b + 2, 4), 16));
                    i = b + 6;
                } else {
                    if (!ESCAPES.hasOwnProperty(e)) fail("bad escape");
                    out += ESCAPES[e];
                    i = b + 2;
                }
            }
        }
        function num() {
            var m = /^-?\d+(?:\.\d+)?(?:[eE][+\-]?\d+)?/.exec(text.substr(i, 40));
            if (!m) fail("unexpected '" + text.charAt(i) + "'");
            i += m[0].length;
            return parseFloat(m[0]);
        }
        function arr() {
            var a = [], c;
            i++;
            ws();
            if (text.charAt(i) === "]") { i++; return a; }
            for (;;) {
                a.push(value());
                ws();
                c = text.charAt(i++);
                if (c === "]") return a;
                if (c !== ",") fail("expected , or ]");
            }
        }
        function obj() {
            var o = {}, k, c;
            i++;
            ws();
            if (text.charAt(i) === "}") { i++; return o; }
            for (;;) {
                ws();
                if (text.charAt(i) !== '"') fail("expected a key");
                k = str();
                ws();
                if (text.charAt(i++) !== ":") fail("expected :");
                o[k] = value();
                ws();
                c = text.charAt(i++);
                if (c === "}") return o;
                if (c !== ",") fail("expected , or }");
            }
        }
        function value() {
            ws();
            var c = text.charAt(i);
            if (c === "{") return obj();
            if (c === "[") return arr();
            if (c === '"') return str();
            if (text.substr(i, 4) === "true") { i += 4; return true; }
            if (text.substr(i, 5) === "false") { i += 5; return false; }
            if (text.substr(i, 4) === "null") { i += 4; return null; }
            return num();
        }

        if (text.charCodeAt(0) === 0xfeff) i = 1;
        var v = value();
        ws();
        if (i < n) fail("trailing data");
        return v;
    }

    // ---------------------------------------------------------- SHA-256
    // Input is a byte string (one char per byte, as read with BINARY encoding).
    var K = [
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    ];

    function ror(x, s) { return (x >>> s) | (x << (32 - s)); }

    function sha256(bytes) {
        var len = bytes.length, words = [], i, j, t;
        var blocks = ((len + 8) >> 6) + 1, total = blocks * 16;
        for (i = 0; i < total; i++) words[i] = 0;
        for (i = 0; i < len; i++) words[i >> 2] |= (bytes.charCodeAt(i) & 0xff) << (24 - (i % 4) * 8);
        words[len >> 2] |= 0x80 << (24 - (len % 4) * 8);
        words[total - 2] = Math.floor((len * 8) / 0x100000000);
        words[total - 1] = (len * 8) >>> 0;

        var H = [0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19];
        var W = [];
        for (j = 0; j < total; j += 16) {
            var a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];
            for (t = 0; t < 64; t++) {
                if (t < 16) {
                    W[t] = words[j + t] | 0;
                } else {
                    var x = W[t - 15], y = W[t - 2];
                    W[t] = (W[t - 16] + (ror(x, 7) ^ ror(x, 18) ^ (x >>> 3)) + W[t - 7] + (ror(y, 17) ^ ror(y, 19) ^ (y >>> 10))) | 0;
                }
                var t1 = (h + (ror(e, 6) ^ ror(e, 11) ^ ror(e, 25)) + ((e & f) ^ (~e & g)) + K[t] + W[t]) | 0;
                var t2 = ((ror(a, 2) ^ ror(a, 13) ^ ror(a, 22)) + ((a & b) ^ (a & c) ^ (b & c))) | 0;
                h = g; g = f; f = e; e = (d + t1) | 0;
                d = c; c = b; b = a; a = (t1 + t2) | 0;
            }
            H[0] = (H[0] + a) | 0; H[1] = (H[1] + b) | 0; H[2] = (H[2] + c) | 0; H[3] = (H[3] + d) | 0;
            H[4] = (H[4] + e) | 0; H[5] = (H[5] + f) | 0; H[6] = (H[6] + g) | 0; H[7] = (H[7] + h) | 0;
        }
        var hex = "";
        for (i = 0; i < 8; i++) hex += ("00000000" + (H[i] >>> 0).toString(16)).slice(-8);
        return hex;
    }

    // ------------------------------------------------------- catalog logic
    function matches(item, query) {
        if (!query) return true;
        var hay = [item.id, item.title.ru, item.title.en, item.summary.ru, item.summary.en, (item.tags || []).join(" ")].join(" ").toLowerCase();
        var words = query.toLowerCase().split(/\s+/);
        for (var i = 0; i < words.length; i++) if (words[i] && hay.indexOf(words[i]) === -1) return false;
        return true;
    }

    function detectedCaps(item) {
        var out = [], caps = item.checks.capabilities;
        for (var i = 0; i < caps.length; i++) if (caps[i].detected) out.push(caps[i].id);
        return out;
    }

    function isRevoked(index, id, sha) {
        var list = index.revoked || [];
        for (var i = 0; i < list.length; i++) if (list[i].id === id && (!list[i].sha256 || list[i].sha256 === sha)) return true;
        return false;
    }

    function baseOf(source) { return source.replace(/data\/index\.json$/, ""); }

    var SAFE_URL = /^https:\/\/[A-Za-z0-9.\-]+(?::\d+)?(?:\/[A-Za-z0-9._~%!$&'()*+,;=:@\/\-]*)?$/;

    var lib = { parseJSON: parseJSON, sha256: sha256, matches: matches, detectedCaps: detectedCaps, isRevoked: isRevoked, baseOf: baseOf, SAFE_URL: SAFE_URL };

    // Outside ExtendScript (the Node test harness) stop here.
    if (typeof $ === "undefined" || typeof app === "undefined") {
        if (typeof CAVE_TEST !== "undefined") CAVE_TEST.lib = lib;
        return;
    }

    // ---------------------------------------------------------- settings
    function getSetting(key, fallback) {
        try { if (app.settings.haveSetting(SECTION, key)) return app.settings.getSetting(SECTION, key); } catch (e) {}
        return fallback;
    }
    function setSetting(key, value) {
        try { app.settings.saveSetting(SECTION, key, value); } catch (e) {}
    }

    // ----------------------------------------------------------- strings
    // English first: most artists run AE in English and expect its terms.
    // Russian is opt-in through the "lang" setting (app.settings, section "VFX Cave").
    var LANG = getSetting("lang", "en") === "ru" ? "ru" : "en";
    function localize(o) {
        var s = typeof o === "string" ? o : (o[LANG] || o.en);
        for (var i = 1; i < arguments.length; i++) s = s.split("%" + i).join(String(arguments[i]));
        return s;
    }
    var S = {
        search: localize({ en: "Search", ru: "Поиск" }),
        refresh: localize({ en: "Refresh", ru: "Обновить" }),
        source: localize({ en: "Source…", ru: "Источник…" }),
        install: localize({ en: "Install", ru: "Установить" }),
        update: localize({ en: "Update", ru: "Обновить" }),
        run: localize({ en: "Run", ru: "Запустить" }),
        remove: localize({ en: "Remove", ru: "Удалить" }),
        page: localize({ en: "Page", ru: "Страница" }),
        colTitle: localize({ en: "Artifact", ru: "Артефакт" }),
        colCat: localize({ en: "Category", ru: "Категория" }),
        colState: localize({ en: "State", ru: "Статус" }),
        ok: localize({ en: "checked", ru: "проверен" }),
        review: localize({ en: "review", ru: "ревью" }),
        blocked: localize({ en: "needs work", ru: "доработка" }),
        installed: localize({ en: "installed", ru: "установлен" }),
        outdated: localize({ en: "update", ru: "есть обновление" }),
        revoked: localize({ en: "revoked", ru: "отозван" }),
        noIndex: localize({ en: "No catalog yet. Click Refresh.", ru: "Каталог не загружен. Нажмите «Обновить»." }),
        loaded: localize({ en: "Catalog: %1 artifacts", ru: "Каталог: %1 артефактов" }),
        needPref: localize({
            en: "Turn on Preferences > Scripting & Expressions > Allow Scripts to Write Files and Access Network, then try again.",
            ru: "Включите Preferences > Scripting & Expressions > Allow Scripts to Write Files and Access Network и повторите."
        }),
        badUrl: localize({ en: "Only https:// addresses without spaces or quotes are allowed.", ru: "Разрешены только адреса https:// без пробелов и кавычек." }),
        downloadFailed: localize({ en: "Download failed: %1", ru: "Не удалось скачать: %1" }),
        hashMismatch: localize({ en: "The file doesn't match the catalog (sha256). It was not installed.", ru: "Файл не совпадает с каталогом (sha256). Он не установлен." }),
        isRevoked: localize({ en: "This version was revoked by the catalog and won't run.", ru: "Эта версия отозвана каталогом и не будет запущена." }),
        isBlocked: localize({ en: "This artifact failed the catalog checks and can't be installed yet.", ru: "Этот артефакт не прошёл проверки каталога, установить его пока нельзя." }),
        doneInstall: localize({ en: "Installed: %1", ru: "Установлено: %1" }),
        doneRemove: localize({ en: "Removed: %1", ru: "Удалено: %1" }),
        notInstalled: localize({ en: "Install it first.", ru: "Сначала установите." }),
        askTitle: localize({ en: "Before the first run", ru: "Перед первым запуском" }),
        askLead: localize({ en: "%1 can:", ru: "%1 умеет:" }),
        askRun: localize({ en: "Run", ru: "Запустить" }),
        askCancel: localize({ en: "Cancel", ru: "Отмена" }),
        sourcePrompt: localize({ en: "Catalog index: an https:// URL or a path to data/index.json", ru: "Индекс каталога: адрес https:// или путь к data/index.json" }),
        capsLine: localize({ en: "Can: %1", ru: "Умеет: %1" }),
        capsNone: localize({ en: "Only changes the project.", ru: "Меняет только проект." }),
        caps: {
            files: localize({ en: "read and write files", ru: "читать и писать файлы" }),
            shell: localize({ en: "run system commands", ru: "запускать системные команды" }),
            network: localize({ en: "use the network", ru: "ходить в сеть" }),
            eval: localize({ en: "run code from text", ru: "выполнять код из строки" }),
            prototypes: localize({ en: "change built-in objects", ru: "менять встроенные объекты" }),
            settings: localize({ en: "save AE settings", ru: "сохранять настройки AE" }),
            menu: localize({ en: "call menu commands", ru: "вызывать команды меню" })
        }
    };
    function L(o) { return o ? (o[LANG] || o.en || o.ru || "") : ""; }
    // installed: "id=sha|id=sha"
    function readMap(key) {
        var out = {}, raw = getSetting(key, ""), parts = raw ? raw.split("|") : [];
        for (var i = 0; i < parts.length; i++) {
            var kv = parts[i].split("=");
            if (kv.length === 2) out[kv[0]] = kv[1];
        }
        return out;
    }
    function writeMap(key, map) {
        var parts = [];
        for (var k in map) if (map.hasOwnProperty(k)) parts.push(k + "=" + map[k]);
        setSetting(key, parts.join("|"));
    }

    // ------------------------------------------------------------- files
    var isWindows = $.os.indexOf("Windows") !== -1;
    var home = new Folder(Folder.userData.fsName + "/VFX Cave");
    var artifacts = new Folder(home.fsName + "/artifacts");

    function ensure(folder) { if (!folder.exists) folder.create(); return folder; }

    function readFile(file, encoding) {
        file.encoding = encoding;
        if (!file.open("r")) throw new Error(file.error || file.fsName);
        var s = file.read();
        file.close();
        return s;
    }

    function canUseFiles() {
        try { return app.preferences.getPrefAsLong("Main Pref Section", "Pref_SCRIPTING_FILE_NETWORK_SECURITY") === 1; } catch (e) { return true; }
    }

    function download(url, file) {
        if (!SAFE_URL.test(url)) throw new Error(S.badUrl);
        if (file.exists) file.remove();
        var cmd = 'curl -sSfL --max-time 60 -o "' + file.fsName + '" "' + url + '"';
        var out = system.callSystem(isWindows ? "cmd /c " + cmd : cmd);
        if (!file.exists || file.length === 0) throw new Error(localize(S.downloadFailed, out || url));
    }

    // Copies or downloads an item's file (relative to the index base) to dest.
    function fetchItemFile(source, relPath, dest) {
        if (/^https:\/\//i.test(source)) download(baseOf(source) + relPath, dest);
        else {
            var src = new File(new File(source).parent.parent.fsName + "/" + relPath);
            if (!src.exists) throw new Error(localize(S.downloadFailed, src.fsName));
            if (dest.exists) dest.remove();
            if (!src.copy(dest.fsName)) throw new Error(localize(S.downloadFailed, src.error));
        }
    }

    function loadIndex() {
        var source = getSetting("source", DEFAULT_SOURCE);
        var cache = new File(ensure(home).fsName + "/index.json");
        if (/^https:\/\//i.test(source)) download(source, cache);
        else {
            var local = new File(source);
            if (!local.exists) throw new Error(localize(S.downloadFailed, source));
            if (cache.exists) cache.remove();
            local.copy(cache.fsName);
        }
        return parseJSON(readFile(cache, "UTF-8"));
    }

    function cachedIndex() {
        var cache = new File(home.fsName + "/index.json");
        if (!cache.exists) return null;
        try { return parseJSON(readFile(cache, "UTF-8")); } catch (e) { return null; }
    }

    // ---------------------------------------------------------------- UI
    var index = null;
    var shown = [];
    var installed = readMap("installed");
    var consent = readMap("consent");

    var win = thisObj instanceof Panel ? thisObj : new Window("palette", "VFX Cave", undefined, { resizeable: true });
    win.orientation = "column";
    win.alignChildren = ["fill", "top"];
    win.spacing = 6;
    win.margins = 8;

    var top = win.add("group");
    top.orientation = "row";
    top.alignChildren = ["fill", "center"];
    top.add("statictext", undefined, S.search + ":");
    var q = top.add("edittext", undefined, "");
    q.characters = 18;
    q.alignment = ["fill", "center"];
    var refreshBtn = top.add("button", undefined, S.refresh);
    var sourceBtn = top.add("button", undefined, S.source);

    var list = win.add("listbox", undefined, [], {
        numberOfColumns: 3,
        showHeaders: true,
        columnTitles: [S.colTitle, S.colCat, S.colState],
        columnWidths: [220, 110, 110]
    });
    list.preferredSize = [460, 260];
    list.alignment = ["fill", "fill"];

    var info = win.add("statictext", undefined, S.noIndex, { multiline: true });
    info.preferredSize = [460, 92];
    info.alignment = ["fill", "bottom"];

    var buttons = win.add("group");
    buttons.alignment = ["fill", "bottom"];
    var installBtn = buttons.add("button", undefined, S.install);
    var runBtn = buttons.add("button", undefined, S.run);
    var removeBtn = buttons.add("button", undefined, S.remove);
    var pageBtn = buttons.add("button", undefined, S.page);

    var statusLine = win.add("statictext", undefined, "");
    statusLine.alignment = ["fill", "bottom"];

    function stateOf(item) {
        if (isRevoked(index, item.id, installed[item.id])) return S.revoked;
        if (installed[item.id]) return installed[item.id] === item.sha256 ? S.installed : S.outdated;
        return S[item.checks.status] || item.checks.status;
    }

    function fill() {
        list.removeAll();
        shown = [];
        if (!index) { info.text = S.noIndex; return; }
        var query = q.text;
        for (var i = 0; i < index.items.length; i++) {
            var item = index.items[i];
            if (!matches(item, query)) continue;
            var row = list.add("item", L(item.title));
            row.subItems[0].text = item.category;
            row.subItems[1].text = stateOf(item);
            shown.push(item);
        }
        statusLine.text = localize(S.loaded, index.items.length);
        describe();
    }

    function selected() { return list.selection ? shown[list.selection.index] : null; }

    function capsText(item) {
        var caps = detectedCaps(item), names = [];
        for (var i = 0; i < caps.length; i++) names.push(S.caps[caps[i]] || caps[i]);
        return names.length ? localize(S.capsLine, names.join(", ")) : S.capsNone;
    }

    function describe() {
        var item = selected();
        installBtn.text = item && installed[item.id] && installed[item.id] !== item.sha256 ? S.update : S.install;
        installBtn.enabled = !!item && item.checks.status !== "blocked";
        runBtn.enabled = removeBtn.enabled = !!item && !!installed[item.id];
        pageBtn.enabled = !!item;
        if (!item) return;
        info.text = L(item.summary) + "\n" + capsText(item) + "\n" + L(item.usage) + "\nsha256 " + item.sha256.substr(0, 16) + "…";
    }

    function artifactFile(item) {
        var ext = item.kind === "preset" ? ".ffx" : ".jsx";
        return new File(artifacts.fsName + "/" + item.id + ext);
    }

    function guard(fn) {
        return function () {
            try { fn(); } catch (e) { alert(e.message || String(e), "VFX Cave"); }
        };
    }

    function needsAccess() {
        if (!canUseFiles()) throw new Error(S.needPref);
    }

    function doInstall() {
        var item = selected();
        if (!item) return;
        if (item.checks.status === "blocked") throw new Error(S.isBlocked);
        if (isRevoked(index, item.id, item.sha256)) throw new Error(S.isRevoked);
        needsAccess();
        ensure(artifacts);
        var tmp = new File(artifacts.fsName + "/" + item.id + ".download");
        fetchItemFile(getSetting("source", DEFAULT_SOURCE), item.file, tmp);
        if (sha256(readFile(tmp, "BINARY")) !== item.sha256) {
            tmp.remove();
            throw new Error(S.hashMismatch);
        }
        var dest = artifactFile(item);
        if (dest.exists) dest.remove();
        tmp.rename(dest.name);
        installed[item.id] = item.sha256;
        writeMap("installed", installed);
        fill();
        statusLine.text = localize(S.doneInstall, L(item.title));
    }

    function ask(item, caps) {
        var d = new Window("dialog", S.askTitle);
        d.orientation = "column";
        d.alignChildren = ["fill", "top"];
        d.add("statictext", undefined, localize(S.askLead, L(item.title)));
        for (var i = 0; i < caps.length; i++) d.add("statictext", undefined, "  • " + (S.caps[caps[i]] || caps[i]));
        var g = d.add("group");
        g.alignment = ["right", "bottom"];
        g.add("button", undefined, S.askCancel, { name: "cancel" });
        g.add("button", undefined, S.askRun, { name: "ok" });
        return d.show() === 1;
    }

    function doRun() {
        var item = selected();
        if (!item || !installed[item.id]) throw new Error(S.notInstalled);
        var file = artifactFile(item);
        if (!file.exists) throw new Error(S.notInstalled);
        var sha = sha256(readFile(file, "BINARY"));
        if (sha !== installed[item.id] || isRevoked(index, item.id, sha)) throw new Error(S.isRevoked);
        if (item.kind === "preset") {
            var layer = app.project.activeItem && app.project.activeItem.selectedLayers && app.project.activeItem.selectedLayers[0];
            if (!layer) throw new Error(L(item.usage));
            layer.applyPreset(file);
            return;
        }
        var caps = detectedCaps(item), worrying = [];
        for (var i = 0; i < caps.length; i++) if (RISKY[caps[i]] || caps[i] === "files") worrying.push(caps[i]);
        if (worrying.length && consent[item.id] !== sha) {
            if (!ask(item, worrying)) return;
            consent[item.id] = sha;
            writeMap("consent", consent);
        }
        $.evalFile(file);
    }

    function doRemove() {
        var item = selected();
        if (!item) return;
        var file = artifactFile(item);
        if (file.exists) file.remove();
        delete installed[item.id];
        delete consent[item.id];
        writeMap("installed", installed);
        writeMap("consent", consent);
        fill();
        statusLine.text = localize(S.doneRemove, L(item.title));
    }

    function doPage() {
        var item = selected();
        if (!item) return;
        var url = (index.site || baseOf(getSetting("source", DEFAULT_SOURCE))) + "#" + item.id;
        if (!SAFE_URL.test(url.replace("#", "/"))) throw new Error(S.badUrl);
        system.callSystem(isWindows ? 'cmd /c start "" "' + url + '"' : 'open "' + url + '"');
    }

    function doRefresh() {
        needsAccess();
        index = loadIndex();
        fill();
    }

    function doSource() {
        var value = prompt(S.sourcePrompt, getSetting("source", DEFAULT_SOURCE), "VFX Cave");
        if (!value) return;
        if (/^https?:\/\//i.test(value) && !SAFE_URL.test(value)) throw new Error(S.badUrl);
        setSetting("source", value);
        doRefresh();
    }

    q.onChanging = function () { fill(); };
    list.onChange = function () { describe(); };
    list.onDoubleClick = guard(doRun);
    refreshBtn.onClick = guard(doRefresh);
    sourceBtn.onClick = guard(doSource);
    installBtn.onClick = guard(doInstall);
    runBtn.onClick = guard(doRun);
    removeBtn.onClick = guard(doRemove);
    pageBtn.onClick = guard(doPage);
    win.onResizing = win.onResize = function () { this.layout.resize(); };

    index = cachedIndex();
    fill();

    if (win instanceof Window) { win.center(); win.show(); }
    else { win.layout.layout(true); win.layout.resize(); }
})(this);
