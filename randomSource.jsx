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