function moveAnchorToCenter( repositingLayer ) {
    var theLayer = repositingLayer;
    var curComp = repositingLayer.containingComp;
    var curTime = curComp.time;
    var noMasks = true;
    if ( theLayer.mask.numProperties !== 0 ) {
        for ( var i = 1; i <= theLayer.mask.numProperties; i++ ) {
            if ( theLayer.mask( i ).maskMode != MaskMode.NONE ) {
                noMasks = false;
            }
        }
    }
    if ( noMasks ) {
        var x = theLayer.sourceRectAtTime( curTime, false ).width / 2;
        var y = theLayer.sourceRectAtTime( curTime, false ).height;

        if ( ( theLayer instanceof TextLayer ) || ( theLayer instanceof ShapeLayer ) ) {
            x += theLayer.sourceRectAtTime( curTime, false ).left;
            y += theLayer.sourceRectAtTime( curTime, false ).top;
        }
    } else {
        var xBounds = Array();
        var yBounds = Array();
        var numMasks = theLayer.mask.numProperties;
        for ( var f = 1; f <= numMasks; f++ ) {
            var numVerts = theLayer.mask( f ).maskShape.value.vertices.length;
            if ( theLayer.mask( f ).maskMode == MaskMode.NONE )
                continue;
            for ( var j = 0; j < numVerts; j++ ) {
                var curVerts = theLayer.mask( f ).maskShape.valueAtTime( curTime, false ).vertices[ j ];
                xBounds.push( curVerts[ 0 ] );
                yBounds.push( curVerts[ 1 ] );
            }
        }
        xBounds.sort( function ( a, b ) {
            return a - b;
        } );

        yBounds.sort( function ( a, b ) {
            return a - b;
        } );

        var xl = xBounds.shift();
        var xh = xBounds.pop();
        var yl = yBounds.shift();
        var yh = yBounds.pop();
        if ( ( theLayer instanceof TextLayer ) || ( theLayer instanceof ShapeLayer ) ) {
            var xl2 = theLayer.sourceRectAtTime( curTime, false ).left;
            var xh2 = xl2 + theLayer.sourceRectAtTime( curTime, false ).width;
            var yl2 = theLayer.sourceRectAtTime( curTime, false ).top;
            var yh2 = yl2 + theLayer.sourceRectAtTime( curTime, false ).height;
            if ( xl < xl2 )
                xl = xl2;
            if ( xh > xh2 )
                xh = xh2;
            if ( yl < yl2 )
                yl = yl2;
            if ( yh > yh2 )
                yh = yh2;
        }
        x = xl + ( ( xh - xl ) / 2 );
        y = yl + ( ( yh - yl ) / 2 );
    }
    try {
        var curAnchor = theLayer.anchorPoint.value;
    } catch ( e ) {
        var curAnchor = theLayer.anchorPoint.valueAtTime( curComp.time, true );
    }
    var xMove = ( x - curAnchor[ 0 ] ) * ( theLayer.scale.value[ 0 ] / 100 );
    var yMove = ( y - curAnchor[ 1 ] ) * ( theLayer.scale.value[ 1 ] / 100 );
    var posEx = false;
    if ( theLayer.position.expressionEnabled ) {
        theLayer.position.expressionEnabled = false;
        posEx = true;
    }

    var dupLayer = theLayer.duplicate();
    var oldParent = theLayer.parent;
    dupLayer.moveToEnd();
    if ( dupLayer.scale.isTimeVarying ) {
        dupLayer.scale.setValueAtTime( curComp.time, [ 100, 100 ] );
    } else {
        dupLayer.scale.setValue( [ 100, 100 ] );
    }

    theLayer.parent = dupLayer;
    if ( theLayer.anchorPoint.isTimeVarying ) {
        var theComp = curComp;
        theLayer.anchorPoint.setValueAtTime( theComp.time, [ x, y ] );
    } else {
        theLayer.anchorPoint.setValue( [ x, y ] );
    }
    if ( theLayer.position.isTimeVarying === true ) {
        var curPos = theLayer.position;
        var numKeys = curPos.numKeys;
        var rovs = [];
        for ( var k = 1; k <= numKeys; k++ ) {
            if ( curPos.keyRoving( k ) === true ) {
                curPos.setRovingAtKey( k, false );
                rovs.push( k );
            }
            curPos.setValueAtKey( k, curPos.keyValue( k ) + [ xMove, yMove ] );
        }
        for ( var k = 0; k < rovs.length; k++ ) {
            curPos.setRovingAtKey( rovs[ k ], true );
        }
    } else {
        var curPos = theLayer.position.value;
        theLayer.position.setValue( [ curPos[ 0 ] + xMove, curPos[ 1 ] + yMove, curPos[ 2 ] ] );
    }
    clearOutput();
    theLayer.parent = oldParent;
    if ( posEx ) {
        theLayer.position.expressionEnabled = true;
    }
    dupLayer.remove();
}