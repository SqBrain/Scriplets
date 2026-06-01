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