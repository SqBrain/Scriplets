SQB_addWigglesWithSliders();
function SQB_addWigglesWithSliders() {
    var comp = app.project.activeItem;
    app.beginUndoGroup( "Add Time Expression" );
    if ( comp && comp instanceof CompItem ) {
        var props = comp.selectedProperties;
        var layers = comp.selectedLayers;
        var layer;
        if ( layers.length > 1 ) {
            var hasController = false;
            for ( var y = 1; y <= comp.numLayers; y++ ) {
                if ( comp.layer( y ).name == "Controller" ) {
                    hasController = y;
                    break;
                }
            }
            if ( hasController ) {
                layer = comp.layer( hasController );
            } else {
                layer = comp.layers.addShape();
                layer.name = "Controller";
            }
        } else if ( layers.length == 1 ) {
            layer = layers[ 0 ];
        } else {
            alert( "No properties selected" );
            return;
        }
        var eff = layer( "ADBE Effect Parade" );
        for ( var i = 0; i < props.length; i++ ) {
            var prop = props[ i ];
            if ( prop.canSetExpression ) {
                prop.expression = 'spd = thisComp.layer("' + layer.name + '").effect("' + "Speed" + '")(1);\n' +
                    'time*spd;';
            }
        }
        var speed = eff.addProperty( "ADBE Slider Control" );
        speed.name = "Speed";
        speed( 1 ).setValue( 100 );
    }
    app.endUndoGroup();
}