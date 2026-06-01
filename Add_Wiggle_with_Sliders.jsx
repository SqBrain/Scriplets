SQB_addWigglesWithSliders();
function SQB_addWigglesWithSliders() {
    var comp = app.project.activeItem;
    app.beginUndoGroup( "Add Wiggle" );
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
        var amp = eff.addProperty( "ADBE Slider Control" );
        amp.name = "Amp";
        amp( 1 ).setValue( 10 );
        var freq = eff.addProperty( "ADBE Slider Control" );
        freq.name = "Freq";
        freq( 1 ).setValue( 3 );
        for ( var i = 0; i < props.length; i++ ) {
            var prop = props[ i ];
            if ( prop.canSetExpression ) {
                prop.expression = 'amp = thisComp.layer("' + layer.name + '").effect("' + "Amp" + '")(1);\n' +
                    'freq = thisComp.layer("' + layer.name + '").effect("Freq")(1);\n' +
                    'wiggle( freq, amp );';
            }
        }
    }
    app.endUndoGroup();
}