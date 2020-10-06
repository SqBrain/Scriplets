function convertColor( color ) {
    if ( color instanceof Array ) {
        for ( var i = 0; i < color.length; i++ ) {
            color[ i ] = Math.floor( ( color[ i ] / 255 ) * 10000 ) / 10000;
        }
    } else if ( isHexNum( color ) ) {
        if ( color[ 0 ] === "#" && ( color.length === 4 || color.length === 7 ) ) {
            color = color.slice( 1 );
        }
        color = getHexArr( color );
    }
    return color;
}

function convertHexColor( color ) {
    if ( color.length < 6 ) {
        while ( color.length < 6 ) {
            color = "0" + color;
        }
    }
    color = getHexArr( color );
    return color;
}

function convertToHex( arr ) {
    var str = "";
    for ( var i = 0; i < 3; i++ ) {
        var color = Math.floor( arr[ i ] * 255 ).toString( 16 );
        str += color;
    }
    while ( str.length < 6 ) {
        str = "0" + str;
    }
    return str;
}

function getHexArr( color ) {
    if ( color.length === 3 ) {
        color = color.split( "" );
        for ( var i = 0; i < 3; i++ ) {
            color[ i ] = color[ i ] + color[ i ];
        }
    } else {
        var temp = [];
        for ( var i = 0; i < 3; i++ ) {
            temp.push( color.slice( i * 2, i * 2 + 2 ) );
        }
        color = temp;
    }
    for ( var i = 0; i < 3; i++ ) {
        color[ i ] = Math.floor( ( parseInt( color[ i ], 16 ) / 255 ) * 10000 ) / 10000;
    }
    return color;
}

function isHexNum( hex ) {
    if ( typeof hex === "string" && ( ( hex.length === 3 || hex.length === 6 ) || ( ( hex.length === 4 || hex.length === 7 ) && hex[ 0 ] === "#" ) ) ) {
        return ( hex.search( /[^\da-fA-F#]/ === -1 ) ) ? true : false;
    } else {
        return false;
    }
}