/*  VFX Cave · RGB Split Layers (shift-channels) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/ShiftChannels.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#shift-channels
    Built by VFX Cave from ShiftChannels.jsx.
    The sha256 of this file is published in the catalog index. */

SQB_ShiftChannels();
function SQB_ShiftChannels() {
    app.beginUndoGroup( "Shift Channels" );
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        var layer = comp.selectedLayers[ 0 ];
        var turnOff = 10;
        if ( layer instanceof ShapeLayer || layer instanceof AVLayer || layer instanceof TextLayer ) {
            var shft = layer.effect.addProperty( "ADBE Shift Channels" );
            shft( 2 ).setValue( turnOff );
            shft( 3 ).setValue( turnOff );
            var green = layer.duplicate();
            shft = green.effect( green.effect.numProperties );
            shft( 4 ).setValue( turnOff );
            shft( 3 ).setValue( 3 );
            var red = layer.duplicate();
            shft = red.effect( green.effect.numProperties );
            shft( 4 ).setValue( turnOff );
            shft( 3 ).setValue( turnOff );
            shft( 2 ).setValue( 2 );
            green.blendingMode = BlendingMode.SCREEN;
            red.blendingMode = BlendingMode.SCREEN;
        }
    }
    app.endUndoGroup();
}