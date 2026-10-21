/*  VFX Cave · Track Matte from Layer Above (matte-layers) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/matteLayers.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#matte-layers
    Built by VFX Cave from matteLayers.jsx.
    The sha256 of this file is published in the catalog index. */

(function() {
    var matteTypes = {
        "alpha": TrackMatteType.ALPHA,
        "luma": TrackMatteType.LUMA,
        "alpha_inverted": TrackMatteType.ALPHA_INVERTED,
        "luma_inverted": TrackMatteType.LUMA_INVERTED
    };

    function applyMatte(arg) {
        var matteType = matteTypes[arg];
        if (!matteType) {
            alert("Invalid argument: " + arg);
            return;
        }

        var comp = app.project.activeItem;
        if (!comp || !(comp instanceof CompItem)) {
            alert("Open a comp first.");
            return;
        }
        var sel = comp.selectedLayers;
        if (!sel.length) {
            alert("Select at least one layer.");
            return;
        }

        app.beginUndoGroup("Set " + arg + " Matte");
        for (var i = 0; i < sel.length; i++) {
            var layer = sel[i];
            if (layer.index === 1) continue;
            layer.setTrackMatte(comp.layer(layer.index - 1), matteType);
        }
        app.endUndoGroup();
    }

    if (typeof kbar !== "undefined" && kbar.button) {
        switch (kbar.button.argument) {
            case "alpha":
                applyMatte("alpha");
                break;
            case "luma":
                applyMatte("luma");
                break;
            case "alpha_inverted":
                applyMatte("alpha_inverted");
                break;
            case "luma_inverted":
                applyMatte("luma_inverted");
                break;
            default:
                alert("Unknown argument: " + kbar.button.argument);
                break;
        }
    } else {
        applyMatte("alpha"); // fallback when run outside KBar
    }
})();