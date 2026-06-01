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