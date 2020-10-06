function assignDefVal( variable, defVal, modifiedVal ) {
    if ( variable === undefined ) {
        return defVal;
    } else {
        if ( modifiedVal )
            return modifiedVal;
        else
            return variable;
    }
}

function indexOf( arr, item ) {
    for ( var i = 0; i < arr.length; i++ ) {
        if ( arr[ i ] === item ) return i;
    }
    return -1;
}

function getMatchFromName( elem, name ) {
    if ( searchInName( elem, name ) ) return elem.name.match( name );
    else return null;
}

function sortNumbers( arr, order ) {
    if ( order === undefined || order === null ) order = 1;
    if ( order === 3 ) {
        arr.shuffle();
    } else {
        arr.sort( function ( a, b ) {
            return a - b;
        } );
        if ( order === 2 ) {
            arr.reverse();
        }
    }
    return arr;
}

function compareTwoArrays( arr1, arr2 ) {
    for ( var i = 0; i < arr1.length; i++ ) {
        if ( arr1[ i ] > arr2[ i ] ) {
            return 1;
        } else if ( arr1[ i ] < arr2[ i ] ) {
            return -1;
        } else if ( arr1[ i ] === arr2[ i ] ) {
            continue;
        }
    }
    return 0;
}

function newRegExp( text, exact, flags ) {
    text = assignDefVal( text, ".*?" );
    switch ( exact ) {
        case REGSTART:
            text = "^" + text;
            break;

        case REGEND:
            text += "$";
            break;

        case REGFULL:
            text = "^" + text + "$";
            break;

        case REGNUM:
            text = "^" + text + " *(\\d*)$";
            break;

        case REGNUMEND:
            text = text + " *(\\d*)$";
            break;

        case REGNUMONLY:
            text = "(\\d*)$";
            break;

        default:
            break;
    }

    if ( flags === undefined ) {
        flags = "";
    }
    flags.replace( /[^igmsuy]/g, "" );
    return new RegExp( text, flags );
}

function convertObjToArray( obj ) {
    var arr = [];
    for ( var prop in obj ) {
        arr.push( [ prop, obj[ prop ] ] )
    }
    return arr;
}

function duplicateObj( obj, count ) {
    count = assignDefVal( count, 1 );
    if ( obj && typeof obj.duplicate == 'function' ) {
        for ( var i = 0; i < count; i++ ) {
            obj = obj.duplicate();
        }
    }
    return obj;
}

function toDo() {
    alert( "This function isn't ready" );
}

function getLastItem( arr ) {
    return arr[ arr.length - 1 ];
}