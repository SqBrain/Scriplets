masksInAdobeSQB();
function masksInAdobeSQB() {
    //@include "../libs/index.jsx"
    app.beginUndoGroup( "Masks" );
    madeBunchOfMasks();
    app.endUndoGroup();
    function madeBunchOfMasks() {
        var layers = getSelectedLayers();
        if ( layers ) {
            var comp = getComp();
            var time = comp.time;
            var f = comp.frameDuration;
            var dur = 0.5;
            var cols = 10;
            for ( var i = 0; i < layers.length; i++ ) {
                var layer = layers[ i ];
                if ( layer instanceof AVLayer ) {
                    var width = layer.width;
                    var w = width / cols;
                    var wh = w / 4;
                    var rows = Math.round( layer.height / w );
                    var cyc = 1;
                    var cyc0 = 1;
                    var n = 1;
                    while ( n <= cols * rows ) {
                        var n0 = cyc0 - n;
                        var x;
                        var y;
                        if ( cyc == cyc0 ) {
                            x = n0;
                            y = n0;
                        } else {
                            var nH = cyc / 2;
                            if ( n0 < nH ) {
                                y = Math.floor( cyc / 2 );
                            } else {
                                y = Math.floor( cyc / 2 ) - 1 - ( n0 - Math.ceil( cyc / 2 ) );
                            }
                            if ( n0 + 1 > nH ) {
                                x = Math.floor( cyc / 2 );
                            } else {
                                x = n0;
                            }
                        }
                        var dupLayer = layer.duplicate();
                        dupLayer.name = layer.name + " " + x + ":" + y;
                        var newMask = dupLayer( MASKS ).addProperty( MASK_GROUP );
                        var newShape = new Shape();
                        newShape.vertices = [
                            [ x * w, y * w ], [ x * w, y * w ],
                            [ x * w, ( y + 1 ) * w ], [ x * w, ( y + 1 ) * w ],
                            [ ( x + 1 ) * w, ( y + 1 ) * w ], [ ( x + 1 ) * w, ( y + 1 ) * w ],
                            [ ( x + 1 ) * w, y * w ], [ ( x + 1 ) * w, y * w ]
                        ];
                        z = [ 0, 0 ];
                        newShape.inTangents = [
                            z, z,
                            z, z,
                            z, z,
                            z, z
                        ];
                        newShape.outTangents = [
                            z, z,
                            z, z,
                            z, z,
                            z, z
                        ];
                        var newShape2 = new Shape();
                        newShape2.vertices = [
                            [ x * w + wh * 2, y * w ], [ x * w, y * w + wh * 2 ],
                            [ x * w, ( y + 1 ) * w - wh * 2 ], [ x * w + wh * 2, ( y + 1 ) * w ],
                            [ ( x + 1 ) * w - wh * 2, ( y + 1 ) * w ], [ ( x + 1 ) * w, ( y + 1 ) * w - wh * 2 ],
                            [ ( x + 1 ) * w, y * w + wh * 2 ], [ ( x + 1 ) * w - wh * 2, y * w ]
                        ];
                        z = [ 0, 0 ];
                        newShape2.outTangents = [
                            [ -wh, 0 ], z,
                            [ 0, wh ], z,
                            [ wh, 0 ], z,
                            [ 0, -wh ], z
                        ];
                        newShape2.inTangents = [
                            z, [ 0, -wh ],
                            z, [ -wh, 0 ],
                            z, [ 0, wh ],
                            z, [ wh, 0 ]
                        ];
                        var t = time;
                        newMask( MASK_SHAPE ).setValuesAtTimes( [ t, t + dur ], [ newShape, newShape2 ] );
                        moveAnchorToCenter( dupLayer );
                        var dist = [ -50, -30 ];
                        var pos = dupLayer.transform.position;
                        var scale = dupLayer.transform.scale;
                        var val = pos.valueAtTime( t, true );
                        pos.setValuesAtTimes( [ t, t + dur ], [ val, val + dist ] );
                        scale.setValuesAtTimes( [ t, t + dur ], [ [ 100, 100 ], [ 0, 0 ] ] );
                        if ( n0 == 0 ) {
                            cyc += 2;
                            cyc0 += cyc;
                        }
                        n++;
                    }
                    layer.enabled = false;
                }
            }
        }
    }

    function moveALotOfLayers() {
        var layers = getSelectedLayers();
        if ( layers ) {
            var comp = getComp();
            var time = comp.time;
            var f = comp.frameDuration;
            var dur = 1;
            for ( var i = 0; i < layers.length; i++ ) {
                var layer = layers[ i ];
                var dist = [ -100, -100 ];
                // var t = time;
                var t = time + i * f;
                var pos = layer.transform.position;
                var scale = layer.transform.scale;
                var val = pos.valueAtTime( t, true );
                pos.setValuesAtTimes( [ t, t + dur ], [ val, val + dist ] );
                scale.setValuesAtTimes( [ t, t + dur ], [ [ 100, 100 ], [ 0, 0 ] ] );
            }
        }
    }
}