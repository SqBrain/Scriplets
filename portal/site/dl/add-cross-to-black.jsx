/*  VFX Cave · Fade In/Out from Black (add-cross-to-black) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/add_cross_to_black.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#add-cross-to-black
    Built by VFX Cave from add_cross_to_black.jsx.
    The sha256 of this file is published in the catalog index. */

SQB_addFadesToBlack();
function SQB_addFadesToBlack() {
    var items = app.project.selection;
    app.beginUndoGroup( "AddFadeToBlack" );
    for ( var i = 0; i < items.length; i++ ) {
        var item = items[ i ];
        if ( item instanceof CompItem ) {
            var newLayer = item.layers.addShape();
            newLayer.name = "Fade to Black";
            var rect = newLayer.property( "ADBE Root Vectors Group" ).addProperty( "ADBE Vector Shape - Rect" );
            rect.size.setValue( [ item.width, item.height ] );
            var fill = newLayer.property( "ADBE Root Vectors Group" ).addProperty( "ADBE Vector Graphic - Fill" );
            fill.property( "ADBE Vector Fill Color" ).setValue( [ 0, 0, 0 ] );
            var f = item.frameDuration;
            newLayer.transform.opacity.setValuesAtTimes( [ 0, 15 * f, item.duration - 15 * f, item.duration ], [ 100, 0, 0, 100 ] );
        }
    }
    app.endUndoGroup();
}