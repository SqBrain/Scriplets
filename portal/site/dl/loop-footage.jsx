/*  VFX Cave · Loop Footage with Time Remap (loop-footage) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/loopFootage.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#loop-footage
    Built by VFX Cave from loopFootage.jsx.
    The sha256 of this file is published in the catalog index. */

loopFootage();
function loopFootage() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        app.beginUndoGroup( "Loop Footage" );
        var layers = comp.selectedLayers;
        var f = comp.frameDuration;
        for ( var i = 0; i < layers.length; i++ ) {
            var layer = layers[ i ];
            if ( layer instanceof AVLayer && layer.canSetTimeRemapEnabled ) {
                var timeRemap = layer.property( "ADBE Time Remapping" );
                layer.timeRemapEnabled = true;
                var t1 = layer.inPoint;
                var t2 = layer.outPoint;
                if ( t2 > timeRemap.keyTime( 2 ) ) t2 = timeRemap.keyTime( 2 );
                t2 -= f;
                var v1 = timeRemap.valueAtTime( t1, false );
                var v2 = timeRemap.valueAtTime( t2, false );
                timeRemap.setValuesAtTimes( [ t1, t2, t2 + f ], [ v1, v2, v1 ] );
                if ( timeRemap.numKeys > 4 ) {
                    timeRemap.removeKey( timeRemap.numKeys );
                    if ( timeRemap.numKeys > 3 ) {
                        timeRemap.removeKey( 1 );
                    }
                }
                timeRemap.expression = "loopOut()";
            }
        }
        app.endUndoGroup();
    }
}