/*  VFX Cave · Precompose Trimmed Layer (precomp) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/precomp.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#precomp
    Built by VFX Cave from precomp.jsx.
    The sha256 of this file is published in the catalog index. */

if ( typeof kbar !== 'undefined' && kbar.button ) {
    var version = kbar.version;       // KBar version like '3.0.0'
    var JSON = kbar.JSON;             // Lets you stringify/parse JSON strings
    var aeq = kbar.aeq;               // Lets you use aequery

    var button = kbar.button;
    var id = button.id;               // A randomly generated string by KBar
    var name = button.name;           // The name the user set for the button

    switch ( button.argument ) {
        case 'Camera':
            precompose( "Camera Tracking" );
            break;

        case 'person':
            precompose( "person" );
            break;

        default:
            alert( "I don't know what the argument " + button.argument + " is supposed to do" );
            break;
    }
}
else {
    precompose();
}

function precompose( name ) {
    var comp = app.project.activeItem;
    if ( comp && comp instanceof CompItem ) {
        var layer = comp.selectedLayers[ 0 ];
        app.beginUndoGroup( "Precomp" );
        if ( layer instanceof AVLayer ) {
            var src = layer.source;
            var folder = src.parentFolder;
            var framerate = src.frameRate;
            var match = layer.name.match( /(.+) 25 fps/ );
            if ( match ) {
                name = match[ 1 ] + " " + name;
            }
            var newComp = folder.items.addComp( name, layer.width, layer.height, layer.source.pixelAspect, layer.outPoint - layer.inPoint, framerate );
            layer.copyToComp( newComp );
            newComp.layer( 1 ).startTime = comp.layer( 1 ).startTime - comp.layer( 1 ).inPoint;
            comp.layers.add( newComp );
            comp.layer( 1 ).startTime = layer.inPoint;
            comp.layer( 1 ).label = 16;
            comp.layer( 1 ).moveBefore( layer );
            layer.remove();
        }
        app.endUndoGroup();
    }
}