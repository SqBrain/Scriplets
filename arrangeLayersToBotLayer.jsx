arrangeLayersToBotLayer()
function arrangeLayersToBotLayer(){
    var comp = app.project.activeItem;
    app.beginUndoGroup("changeOrder");
    if(comp instanceof CompItem){
        var layers = comp.selectedLayers;
        var startIndecies = [];
        var newIndecies = [];
        for(var i = 0; i < layers.length - 1; i++){
            var l1 = layers[i];
            if(l1 instanceof ShapeLayer){
                var rc = l1.content.addProperty("ADBE Vector Filter - RC");
                rc.radius.expression = 'thisComp.layer("Shape Layer 1").effect("Roundness")(1)';
                var fill = l1.effect.addProperty("ADBE Fill");
                fill.color.expression = 'thisComp.layer("Shape Layer 1").effect("Color Control")(1)';
            }
            for(var y = i+1; y < layers.length; y++){
                var l2 = layers[y];
                if(l2.inPoint == l1.inPoint){
                    if(l2.index > l1.index){
                        l1.moveBefore(l2);
                    } else {
                        l2.moveBefore(l1);
                    }
                }
            }
        }

    }
    app.endUndoGroup();
}