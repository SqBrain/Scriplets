/*  VFX Cave · Choked Inverted Self-Matte (make-matte) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/makeMatte.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#make-matte
    Built by VFX Cave from makeMatte.jsx.
    The sha256 of this file is published in the catalog index. */

makeMatte();
function makeMatte() {
    var comp = app.project.activeItem;
    if ( comp && comp instanceof CompItem ) {
        var layer = comp.selectedLayers[ 0 ];
        app.beginUndoGroup( "Make Matte" );
        var copy = layer.duplicate();
        copy.name = copy.name + " Matte";
        copy.label = 13;
        copy.enabled = false;
        var t = copy.effect;
        var eff = t.addProperty( "ADBE Simple Choker" );
        eff( "Choke Matte" ).setValue( -20 );
        for ( var i = layer.effect.numProperties; i > 0; i-- ) {
            layer.effect( i ).remove();
        }
        layer.setTrackMatte( copy, TrackMatteType.ALPHA_INVERTED );
        app.endUndoGroup();
    }
}