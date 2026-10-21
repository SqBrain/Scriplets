/*  VFX Cave · Precompose MP4s at 25 fps (change-comps-precompose) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/changeComps;.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#change-comps-precompose
    Built by VFX Cave from changeComps;.jsx.
    The sha256 of this file is published in the catalog index. */

precompAndChangeFPS();
function changeComps() {
    var items = app.project.selection;
    var len = app.project.numItems;
    var template, src;
    for ( var n = 1; n <= len; n++ ) {
        var it = app.project.item( n );
        if ( it.name == "Scene 2" ) {
            template = app.project.item( n );
        }
        if ( it.name == "Source" ) {
            src = app.project.item( n );
        }
        if ( src && template ) {
            break;
        }
    }
    var adjLayer = template.layer( 1 );
    app.beginUndoGroup( "Change Comps" );
    var width = 1920;
    var height = 1080;
    for ( var i = 0; i < items.length; i++ ) {
        var item = items[ i ];
        if ( item instanceof CompItem ) {
            item.width = width;
            item.height = height;
            var hasSrc = false;
            var hasAdj = false;
            var scene = item.name.match( /Scene (.+)/ );
            for ( var y = 1; y <= item.numLayers; y++ ) {
                var layer = item.layer( y );
                var tr = layer.transform.scale;
                var pos = layer.transform.position;
                if ( scene && layer.name.search( scene[ 1 ] + ".MP4" ) >= 0 ) {
                    hasSrc = true;
                }
                if ( layer.name == "Adjustment Layer 1" ) {
                    hasAdj = true;
                }
                tr.setValue( tr.value * 2 );
                pos.setValue( [ width / 2, height / 2 ] );
            }
            if ( !hasSrc ) {
                for ( var z = 1; z <= src.numItems; z++ ) {
                    var l = src.item( z );
                    if ( l.name == scene[ 1 ] + ".MP4" ) {
                        item.layers.add( l );
                        item.layer( 1 ).transform.opacity.setValue( 50 );
                    }
                }
            }
            if ( !hasAdj ) {
                adjLayer.copyToComp( item );
            }
        }
    }
    app.endUndoGroup();
}

function precompAndChangeFPS() {
    var items = app.project.selection;
    var len = app.project.numItems;
    var scenes;
    for ( var n = 1; n <= len; n++ ) {
        var it = app.project.item( n );
        if ( it.name == "Scenes" ) {
            scenes = app.project.item( n );
        }
    }
    app.beginUndoGroup( "Change Comps" );
    for ( var i = 0; i < items.length; i++ ) {
        var item = items[ i ];
        var folder = scenes.items.addFolder( item.name );
        if ( item instanceof CompItem ) {
            for ( var y = 1; y <= item.numLayers; y++ ) {
                var layer = item.layer( y );
                var time = layer.inPoint;
                if ( layer.name.match( /(.+).MP4/ ) ) {
                    var ind = layer.index;
                    var comp = precompose( layer, folder, "Scene " + layer.name.match( /(.+).MP4/ )[ 1 ] + " 25 fps", 25 );
                    item.layers.add( comp );
                    item.layer( 1 ).startTime = time;
                    item.layer( 1 ).moveBefore( item.layer( ind + 1 ) );
                }
            }
        }
    }
    app.endUndoGroup();

    function precompose( layer, folder, compName, framerate ) {
        var comp = null;
        if ( layer instanceof AVLayer ) {
            if ( !framerate || isNaN( framerate ) ) framerate = layer.source.frameRate;
            comp = folder.items.addComp( compName, layer.width, layer.height, layer.source.pixelAspect, layer.outPoint - layer.inPoint, framerate );
            layer.copyToComp( comp );
            comp.layer( 1 ).startTime = comp.layer( 1 ).startTime - comp.layer( 1 ).inPoint;
            layer.remove();
        }
        return comp;
    }
}