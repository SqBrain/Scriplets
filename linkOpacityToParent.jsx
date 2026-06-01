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