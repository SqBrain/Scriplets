/*  VFX Cave · Trim Layers to Last Selected (trim-layers) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/trimLayers.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#trim-layers
    Built by VFX Cave from trimLayers.jsx.
    The sha256 of this file is published in the catalog index. */

trimLayers();
function trimLayers() {
    var comp = app.project.activeItem;
    if ( comp && comp instanceof CompItem ) {
        app.beginUndoGroup( "Trim Layers" );
        var layers = comp.selectedLayers;
        var main = layers[ layers.length - 1 ];
        for ( var i = 0; i < layers.length - 1; i++ ) {
            var layer = layers[ i ];
            // layer.inPoint = comp.displayStartTime;
            // layer.outPoint = layer.inPoint + comp.duration;
            layer.inPoint = main.inPoint;
            layer.outPoint = main.outPoint;
            // layer.transform.opacity.setValue( 100 );
            // if ( layer.hasAudio ) layer.audioEnabled = false;
        }
        app.endUndoGroup();
    }
}