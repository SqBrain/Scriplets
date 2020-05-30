function adjustDotsOnThePath() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        var props = comp.selectedProperties;
        for ( var i = 0; i < props.length; i++ ) {
            var prop = props[ i ];
            if ( prop.matchName == "ADBE Vector Shape" ) {
                if ( prop.numKeys > 0 ) {

                } else {
                    var value = prop.value;
                    alert( value.reflect.properties.join( "\n" ) )
                    var verts = value.vertices;
                    for ( var y = 0; y < verts.length; y++ ) {
                        var vert = verts[ y ];
                        verts[ y ] = [ adjustNumber( vert[ 0 ] ), adjustNumber( vert[ 1 ] ) ];
                    }
                    value.vertices = verts;
                    prop.setValue( value );
                }
            }
        }
    }

    function adjustNumber( num ) {
        if ( typeof num !== "number" ) {
            return;
        }
        var fullNum = Math.floor( num );
        var end = Math.round( num * 100 ) / 100 - fullNum;
        if ( end < 0.25 ) {
            num = fullNum;
        } else if ( end < 0.75 ) {
            num = fullNum + 0.5;
        } else {
            num = Math.ceil( num );
        }
        return num;
    }
}

function addDotsOnThePath() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        var props = comp.selectedProperties;
        for ( var i = 0; i < props.length; i++ ) {
            var prop = props[ i ];
            if ( prop.matchName == "ADBE Vector Shape" ) {
                if ( prop.numKeys > 0 ) {

                } else {
                    var value = prop.value;
                    var verts = value.vertices;
                    var inTan = value.inTangents;
                    var outTan = value.outTangents;
                    var addArr = [];
                    var addArrInTan = [];
                    var addArrOutTan = [];
                    for ( var y = 0; y < verts.length; y++ ) {
                        var vert = verts[ y ];
                        var next = ( y == verts.length - 1 ) ? 0 : y + 1;
                        var nextVert = verts[ next ];
                        var difX = Math.round( vert[ 0 ] - nextVert[ 0 ] );
                        var difY = Math.round( vert[ 1 ] - nextVert[ 1 ] );
                        var dirX = ( difX / Math.abs( difX ) );
                        var dirY = ( difY / Math.abs( difY ) );
                        var xNum = vert[ 0 ];
                        var yNum = vert[ 1 ];
                        if ( Math.abs( difX ) == Math.abs( difY ) && difY !== 0 ) {
                            for ( var s = 0; s <= difX; s++ ) {
                                var dx = xNum - s * dirX;
                                var dy = yNum - s * dirY;
                                var dx2 = xNum - ( s + 1 ) * dirX;
                                if ( s == 0 ) {
                                    addArr.push( vert, [ dx2, dy ] );
                                    addArrInTan.push( inTan[ y ], [ 0, 0 ] );
                                    addArrOutTan.push( outTan[ y ], [ 0, 0 ] );
                                } else if ( s < difX ) {
                                    addArr.push( [ dx, dy ], [ dx2, dy ] );
                                    addArrInTan.push( [ 0, 0 ], [ 0, 0 ] );
                                    addArrOutTan.push( [ 0, 0 ], [ 0, 0 ] );
                                } else {
                                    addArr.push( nextVert );
                                    addArrInTan.push( inTan[ next ] );
                                    addArrOutTan.push( outTan[ next ] );
                                }
                            }
                        } else {
                            addArr.push( vert );
                            addArrInTan.push( inTan[ y ] );
                            addArrOutTan.push( outTan[ y ] );
                        }
                    }
                    value.vertices = addArr;
                    value.inTangents = addArrInTan;
                    value.outTangents = addArrOutTan;
                    prop.setValue( value );
                }
            }
        }
    }

    function adjustNumber( num ) {
        if ( typeof num !== "number" ) {
            return;
        }
        var fullNum = Math.floor( num );
        var end = Math.round( num * 100 ) / 100 - fullNum;
        if ( end < 0.25 ) {
            num = fullNum;
        } else if ( end < 0.75 ) {
            num = fullNum + 0.5;
        } else {
            num = Math.ceil( num );
        }
        return num;
    }
}

adjustDotsOnThePath();