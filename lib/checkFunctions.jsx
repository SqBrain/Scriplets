function checkAndSetName( scope, name ) {
    var reg = new RegExp( "^" + name, 'gi' );
    var count = 0;
    if ( scope instanceof FolderItem || scope instanceof Project ) {
        var length = scope.numItems;
    } else if ( scope instanceof CompItem ) {
        var length = scope.numLayers;
    } else if ( scope instanceof PropertyGroup && scope.propertyType === PropertyType.NAMED_GROUP ) {
        var length = scope.numProperties;
    }
    for ( var i = 1; i < length; i++ ) {
        if ( scope instanceof FolderItem || scope instanceof Project ) {
            var thisScope = scope.item( i );
        } else if ( scope instanceof CompItem ) {
            var thisScope = scope.layer( i );
        } else if ( scope instanceof PropertyGroup && scope.propertyType === PropertyType.NAMED_GROUP ) {
            var thisScope = scope.property( i );
        }
        if ( thisScope.name.search( reg ) !== -1 ) {
            count++;
        }
    }
    if ( count === 0 || count === 1 ) {
        return name;
    } else {
        return ( name + " " + count );
    }
}

function isArrayEmpty( arr, msg ) {
    if ( arr.length === 0 ) {
        if ( msg ) {
            alert( msg );
        }
        return true;
    }
    return false;
}

function isItemInArr( arr, match ) {
    if ( isArray( arr ) ) {
        for ( var i = 0; i < arr.length; i++ ) {
            if ( arr[ i ] === match ) return true;
        }
        return false;
    } else {
        alert( "Not an array" );
    }
}

function isItComp( comp ) {
    return ( comp && comp instanceof CompItem );
}

function isItPath( prop ) {
    return ( prop.matchName === VPATH || prop.matchName === MASKSHAPE );
}

function searchInName( elem, name, reg ) {
    if ( !name ) return false;
    if ( reg ) name = newRegExp( name, reg );
    if ( elem.name && elem.name.search( name ) !== -1 ) return true;
    return false;
}

function isNumeric( array ) {
    if ( !isArray( array ) ) {
        return array;
    }
    for ( var i = 0; i < array.length; i++ ) {
        if ( typeof array[ i ] !== "number" ) return false;
    }
    return true;
}

function isArray( array ) {
    if ( !array instanceof Array ) {
        alert( "array is not an instance of Array" );
        return false;
    } else {
        return true;
    }
}