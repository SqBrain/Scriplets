SQB_dupLayerAndMatteToSelectedLayers();
function SQB_dupLayerAndMatteToSelectedLayers() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        app.beginUndoGroup( "Dup Layer and matte to Selected Layers" );
        var layers = comp.selectedLayers;
        var mainLayer = layers.splice( layers.length - 1, 1 )[ 0 ];
        sort( layers );
        for ( var i = layers.length - 1; i >= 0; i-- ) {
            var layer = layers[ i ];
            var matte = mainLayer.duplicate();
            matte.moveBefore( layer );
            matte.setTrackMatte( layer, TrackMatteType.ALPHA );
            layer.enabled = true;
        }
        mainLayer.remove();
        app.endUndoGroup();
    }
    function sort( arr ) {
        for ( var i = 0; i < arr.length; i++ ) {
            var minI = -1;
            for ( var y = i + 1; y < arr.length; y++ ) {
                if ( arr[ y ].index < arr[ i ].index ) {
                    minI = y;
                }
            }
            if ( minI > -1 ) {
                var temp = arr[ i ];
                arr[ i ] = arr[ minI ];
                arr[ minI ] = temp;
            }
        }
        return arr;
    }
}
