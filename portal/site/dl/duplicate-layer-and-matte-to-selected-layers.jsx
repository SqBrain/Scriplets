/*  VFX Cave · Copy Layer Into Each Matte (duplicate-layer-and-matte-to-selected-layers) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/Duplicate%20Layer%20and%20Matte%20to%20selected%20layers.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#duplicate-layer-and-matte-to-selected-layers
    Built by VFX Cave from Duplicate Layer and Matte to selected layers.jsx.
    The sha256 of this file is published in the catalog index. */

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
