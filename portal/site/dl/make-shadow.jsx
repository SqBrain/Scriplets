/*  VFX Cave · Blurred Multiply Shadow (make-shadow) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/makeShadow.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#make-shadow
    Built by VFX Cave from makeShadow.jsx.
    The sha256 of this file is published in the catalog index. */

BOTYA_createShadow( "Shadow" );
function BOTYA_createShadow( arg ) {
    app.beginUndoGroup( "Create Shadow" );
    var comp = app.project.activeItem;
    if ( comp && comp instanceof CompItem ) {
        var layers = comp.selectedLayers;
        for ( var i = layers.length - 1; i >= 0; i-- ) {
            var layer = layers[ i ];
            if ( !( layer instanceof AVLayer || layer instanceof ShapeLayer ) ) {
                continue;
            }
            createShadow( 30, arg, layer );
        }
    }
    app.endUndoGroup();

    function createShadow( blurVal, arg, layer ) {
        var modeLayer = layer.duplicate();
        modeLayer.moveAfter( layer );
        layer.selected = false;
        modeLayer.selected = true;
        modeLayer.name = modeLayer.name + " " + arg;
        effects = modeLayer( "ADBE Effect Parade" );
        for ( var y = effects.numProperties; y >= 1; y-- ) {
            effects( y ).remove();
        }
        var blur = effects.addProperty( "Fast Box Blur" );
        blur( 1 ).setValue( blurVal );
        blur( 4 ).setValue( false );
        modeLayer.blendingMode = BlendingMode.MULTIPLY;
        modeLayer.label = 0;
        modeLayer.preserveTransparency = true;
    }
}