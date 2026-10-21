/*  VFX Cave · Randomize Precomp Sources (random-source) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/randomSource.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#random-source
    Built by VFX Cave from randomSource.jsx.
    The sha256 of this file is published in the catalog index. */

SQB_randomSource();
function SQB_randomSource() {
    app.beginUndoGroup( "Random Source" );
    var sel = app.project.selection;
    var comps = [];
    for ( var i = 0; i < sel.length; i++ ) {
        if ( sel[ i ] instanceof CompItem ) {
            comps.push( sel[ i ] );
        }
    }
    var comp = app.project.activeItem;
    // debugger;
    if ( comp instanceof CompItem ) {
        for ( var i = 0; i < comp.selectedLayers.length; i++ ) {
            var layer = comp.selectedLayers[ i ];
            var r = Math.floor( Math.random() * comps.length );
            if ( layer.source && layer.source instanceof CompItem ) {
                layer.replaceSource( comps[ r ], false );
            }
        }
    }
    app.endUndoGroup();
}