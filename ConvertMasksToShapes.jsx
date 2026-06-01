function SB_ConvertMasksToShapes() {
    //@include "lib/constants.jsx"
    var comp = app.project.activeItem;
    if(comp instanceof CompItem) {
        var props = comp.selectedProperties;
        if(props.length > 0){
            app.beginUndoGroup("Convert masks to shapes");
            var shapeLayer = comp.layers.addShape();
            var content = shapeLayer(VEC_ROOT);
            var shapeAdded = false;
            for(var i = 0; i < props.length; i++) {
                var prop = props[i];
                if(prop.matchName === MASK_SHAPE) {
                    var shape = content.addProperty(VEC_PATH_GROUP);
                    var val = new Shape();
                    val.vertices = prop.value.vertices;
                    val.inTangents = prop.value.inTangents;
                    val.outTangents = prop.value.outTangents;
                    shape(VEC_PATH).setValue(val);
                    if(!shapeAdded) shapeAdded = true;
                }
            }
            if(shapeAdded){
                var fill = content.addProperty(VEC_FILL);
                fill(VEC_FILL_COLOR).setValue([1, 1, 1]);
            } else {
                shapeLayer.remove();
            }
            app.endUndoGroup();
        }
    } else {
        alert("Select Comp");
    }
}

SB_ConvertMasksToShapes();