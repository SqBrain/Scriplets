function getRandomFloat( min, max ) {
    return ( Math.random() * ( max - min ) ) + min;
}

function getRandomInt( min, max ) {
    return Math.round( ( Math.random() * ( max - min ) ) + min );
}

function toRadians( degrees ) {
    return degrees * Math.PI / 180;
}

function toDegrees( radians ) {
    return radians * 180 / Math.PI;
}

function cos( radians ) {
    return Math.cos( radians );
}

function sin( radians ) {
    return Math.sin( radians );
}

function tan( radians ) {
    return Math.tan( radians );
}

function atan( radians ) {
    return Math.atan( radians );
}

function atan2( x, y ) {
    return Math.atan2( y, x );
}

function timeToFrames( time ) {
    var comp = getComp();
    if ( isItComp( comp ) ) {
        return time / curComp.frameDuration;
    } else {
        return alertSelectComp( "Time to Frames" );
    }
}

function sqrt( num ) {
    return Math.sqrt( num );
}

function round( num ) {
    return Math.round( num );
}

function abs( num ) {
    return Math.abs( num );
}

function check( num ) {
    return Math.round( num * 1000 );
}

function pow( num, degree ) {
    return Math.pow( num, degree );
}

function floor( num ) {
    return Math.floor( num );
}

function findHypotenuse( num1, num2 ) {
    return sqrt( pow( num1, 2 ) + pow( num2, 2 ) );
}


function getAverage( array ) {
    if ( !isNumeric( array ) ) {
        alert( "Not a numeric array" );
        return null;
    }
    var sum = 0;
    for ( var i = 0; i < array.length; i++ ) {
        sum += array[ i ];
    }
    return sum / array.length;
}