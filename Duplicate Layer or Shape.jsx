SQB_duplicateLayerOrShape();

function SQB_duplicateLayerOrShape() {
    try {
        var comp = app.project.activeItem;
        if ( comp instanceof CompItem ) {
            var layers = comp.selectedLayers;
            if ( layers.length == 0 ) {
                alert( "No Layers selected" );
                return;
            } else {
                var props = comp.selectedProperties;
                var num = parseInt( prompt( "How many times you want to duplicate layer(shape)? Input number" ) );
                if ( !num ) {
                    return;
                } else if ( isNaN( num ) ) {
                    alert( "Wrong input" );
                    return;
                }
                app.beginUndoGroup( "Duplicator" );
                if ( layers.length == 1 && props.length > 0 && layers[ 0 ] instanceof ShapeLayer ) {
                    for ( var i = 0; i < props.length; i++ ) {
                        var thisProp = props[ i ];
                        var content = thisProp.parentProperty;
                        if ( content.propertyType == PropertyType.INDEXED_GROUP ) {
                            var grp = [];
                            for ( var z = 1; z <= content.numProperties; z++ ) {
                                var curPr = content( z );
                                if ( curPr.selected ) {
                                    grp.push( curPr.propertyIndex );
                                    for ( var v = 0; v <= props.length; v++ ) {
                                        if ( props[ v ] == curPr ) {
                                            props.splice( v, 1 );
                                            i--;
                                        }
                                    }
                                }
                            }
                            for ( var y = grp.length - 1; y >= 0; y-- ) {
                                var prop = content( grp[ y ] );
                                for ( var k = 0; k < num - 1; k++ ) {
                                    prop.duplicate();
                                    prop = content( grp[ y ] );
                                }
                            }
                        }
                    }
                } else {
                    var grp = [];
                    // debugger;
                    for ( var z = 0; z < layers.length; z++ ) {
                        grp.push( layers[ z ].index );
                    }
                    grp.sort();
                    for ( var y = grp.length - 1; y >= 0; y-- ) {
                        var layer = comp.layer( grp[ y ] );
                        for ( var k = 0; k < num - 1; k++ ) {
                            layer.duplicate();
                            layer = comp.layer( grp[ y ] );
                        }
                    }
                }
                app.endUndoGroup();
            }

        }
    } catch ( e ) {
        alert( e );
        return;
    }
}