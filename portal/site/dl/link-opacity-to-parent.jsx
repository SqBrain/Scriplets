/*  VFX Cave · Link Opacity to Parent (link-opacity-to-parent) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/linkOpacityToParent.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#link-opacity-to-parent
    Built by VFX Cave from linkOpacityToParent.jsx.
    The sha256 of this file is published in the catalog index. */

linkOpacityToParent();
function linkOpacityToParent(){
    var comp = app.project.activeItem;
    app.beginUndoGroup("Link Opacity to parent");
    if(comp instanceof CompItem){
        var layers = comp.selectedLayers;
        for(var i = 0; i < layers.length; i++){
            var layer = layers[i];
            if(layer.parent){
                var parent = layer.parent;
                layer.transform.opacity.expression = "thisComp.layer('"+ parent.name +"').transform.opacity;";
            }
        }
    }
    app.endUndoGroup();
}