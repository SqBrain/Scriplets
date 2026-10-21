/*  VFX Cave · Batch Rename Layers (rename-layers) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/rename_layers.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#rename-layers
    Built by VFX Cave from rename_layers.jsx.
    The sha256 of this file is published in the catalog index. */

var comp = app.project.activeItem;
if (comp && comp instanceof CompItem) {
    app.beginUndoGroup("Rename Layers");
    var layers = comp.selectedLayers;
    var name = prompt("Enter new name");
    if (name) {
        for (var i = 0; i < layers.length; i++) {
            var layer = layers[i];
            layer.name = name + " " + (i + 1);
        }
    }
    app.endUndoGroup();
} else {
    alert("Please select a composition.");
}