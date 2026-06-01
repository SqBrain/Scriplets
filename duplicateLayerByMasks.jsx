SQB_duplicateLayerByMasks();
function SQB_duplicateLayerByMasks() {
    var comp = app.project.activeItem;
    const MASKS = "ADBE Mask Parade";
    if ( comp instanceof CompItem ) {
        app.beginUndoGroup( 'Duplicate Layer by masks' );
        var layer = comp.selectedLayers[ 0 ];
        var masks = layer( MASKS );
        for ( var i = 1; i <= masks.numProperties; i++ ) {
            var newLayer = layer.duplicate();
            for ( var y = masks.numProperties; y > 0; y-- ) {
                if ( y !== i )
                    newLayer( MASKS )( y ).remove();
            }
        }
        layer.remove();
        app.endUndoGroup();
    }
}