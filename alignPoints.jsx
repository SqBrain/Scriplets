alignPoints();
function alignPoints() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        app.beginUndoGroup( "Align Points" );
        var props = comp.selectedProperties;
        for ( var i = 0; i < props.length; i++ ) {
            var prop = props[ i ];
            if ( prop.matchName == "ADBE Vector Shape" ) {
                var val;
                if ( prop.numKeys > 0 ) {
                    for ( var k = 1; k <= prop.numKeys; k++ ) {
                        val = prop.keyValue( k );
                        prop.setValueAtKey( k, equalize( val ) );
                    }
                } else {
                    val = prop.value;
                    prop.setValue( equalize( val ) );
                }
            }
        }
        app.endUndoGroup();
    }

    function equalize( value ) {
        var verts = value.vertices;
        var threshold = 0.9;
        // debugger;
        for ( var i = 0; i < verts.length; i++ ) {
            var v1 = verts[ i ];
            var w1, w2;
            w1 = getPreciceNum( v1[ 0 ] );
            v1[ 0 ] = w1;
            w2 = getPreciceNum( v1[ 1 ] );
            v1[ 1 ] = w2;
            verts[ i ] = v1;
        }
        alert( verts.join( "\n" ) );
        value.vertices = verts;
        return value;
    }
    function getPreciceNum( p ) {
        var w = Math.round( p );
        var d = p - w;
        if ( d > 0.25 ) w += 0.5;
        if ( d < -0.25 ) w -= 0.5;
        return w;
    }
}