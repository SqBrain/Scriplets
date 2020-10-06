function getComp() {
    return app.project.activeItem;
}

function getSelectedLayers( comp ) {
    comp = comp || getComp();
    if ( isItComp( comp ) ) {
        const layers = comp.selectedLayers;
        if ( layers.length > 0 )
            return comp.selectedLayers;
        else
            return alertError( "getSelectedLayers: ", "No layers selected" );
    } else {
        return alertSelectComp( "getSelectedLayers: " );
    }
}

function getSelectedProps( comp ) {
    comp = comp || getComp();
    if ( isItComp( comp ) ) {
        return comp.selectedProperties;
    } else {
        return alertSelectComp( "getSelectedProps: " );
    }
}

function deleteLayer( layer ) {
    layer.locked = false;
    layer.remove();
}

function setSelected( arr, bool ) {
    if ( arr instanceof Array ) {
        for ( var i = 0; i < arr.length; i++ ) {
            arr[ i ].selected = bool;
        }
    } else if ( arr instanceof PropertyGroup ) {
        for ( var i = arr.numProperties; i > 0; i-- ) {
            arr.property( i ).selected = bool;
        }
    } else if ( arr instanceof Layer ) {
        for ( var i = arr.numLayers; i > 0; i-- ) {
            arr.layer( i ).selected = bool;
        }
    }
}

function sortLayers( layers ) {
    layers.sort( function ( a, b ) {
        return a.index - b.index;
    } );
}