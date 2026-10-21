/*  VFX Cave · Highlight & Shadow Mattes (botya-create-matte) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/BOTYA_createMatte.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#botya-create-matte
    Built by VFX Cave from BOTYA_createMatte.jsx.
    The sha256 of this file is published in the catalog index. */

if ( typeof kbar !== 'undefined' && kbar.button ) {
    var version = kbar.version;       // KBar version like '3.0.0'
    var JSON = kbar.JSON;             // Lets you stringify/parse JSON strings
    var aeq = kbar.aeq;               // Lets you use aequery

    var button = kbar.button;
    var id = button.id;               // A randomly generated string by KBar
    var name = button.name;           // The name the user set for the button

    switch ( button.argument ) {
        case 'Highlight':
            BOTYA_createMatte( button.argument );
            break;

        case 'Shadow':
            BOTYA_createMatte( button.argument );
            break;

        default:
            alert( "I don't know what the argument " + button.argument + " is supposed to do" );
            break;
    }
} else {
    BOTYA_createMatte( "Highlight" );
}
function BOTYA_createMatte( arg ) {
    app.beginUndoGroup( "Add Matte" );
    const mainBlur = 3;
    var comp = app.project.activeItem;
    if ( comp && comp instanceof CompItem ) {
        var layers = comp.selectedLayers;
        var ctrlLayer, cLExists = false;
        if ( layers.length > 1 ) {
            for ( var y = 1; y <= comp.numLayers; y++ ) {
                if ( comp.layer( y ).name == "Controller" ) {
                    cLExists = true;
                    ctrlLayer = comp.layer( y );
                    break;
                }
            }
            if ( !cLExists ) {
                ctrlLayer = comp.layers.addShape();
                ctrlLayer.selected = false;
                ctrlLayer.name = "Controller";
                var eff = ctrlLayer( "ADBE Effect Parade" );
                var hl1Op = eff.addProperty( "ADBE Slider Control" );
                hl1Op.name = "Highlight Main Opacity";
                hl1Op.property( 1 ).setValue( 100 );
                var hl1Blur = eff.addProperty( "ADBE Slider Control" );
                hl1Blur.name = "Highlight Main Blur";
                hl1Blur.property( 1 ).setValue( mainBlur );
                var hl2Op = eff.addProperty( "ADBE Slider Control" );
                hl2Op.name = "Highlight Secondary Opacity";
                hl2Op.property( 1 ).setValue( 100 );
                var hl2Blur = eff.addProperty( "ADBE Slider Control" );
                hl2Blur.name = "Highlight Secondary Blur";
                hl2Blur.property( 1 ).setValue( 30 );
                var shdwOp = eff.addProperty( "ADBE Slider Control" );
                shdwOp.name = "Shadow Opacity";
                shdwOp.property( 1 ).setValue( 100 );
                cLExists = true;
            }
        }
        for ( var i = layers.length - 1; i >= 0; i-- ) {
            var layer = layers[ i ];
            if ( !( layer instanceof AVLayer || layer instanceof ShapeLayer ) ) {
                continue;
            }
            if ( arg == "Highlight" ) {
                AddMatte( mainBlur, arg, layer, cLExists );
            }
            AddMatte( 30, arg, layer, cLExists );
        }
    }
    app.endUndoGroup();

    function AddMatte( blurVal, arg, layer, cLExists ) {
        var matteLayer = layer.duplicate();
        matteLayer.name = matteLayer.name + " Matte";
        matteLayer.label = ( blurVal == mainBlur ? 13 : 10 );
        matteLayer.enabled = false;
        var effects = matteLayer( "ADBE Effect Parade" );
        for ( var y = effects.numProperties; y >= 1; y-- ) {
            effects( y ).remove();
        }
        var blur = effects.addProperty( "Fast Box Blur" );
        blur( 1 ).setValue( cLExists ? 0 : blurVal );
        if ( cLExists ) blur( 1 ).expression = "thisComp.layer( 'Controller' ).effect('" + ( ( blurVal == mainBlur && arg == "Highlight" ) ? "Highlight Main Blur" : "Highlight Secondary Blur" ) + "')(1) + value";
        blur( 4 ).setValue( true );
        var modeLayer = layer.duplicate();
        modeLayer.name = modeLayer.name + " " + arg + ( blurVal == mainBlur ? " 2" : "" );
        effects = modeLayer( "ADBE Effect Parade" );
        for ( var y = effects.numProperties; y >= 1; y-- ) {
            effects( y ).remove();
        }
        var opExpName = "";
        if ( arg == "Highlight" ) {
            modeLayer.label = ( blurVal == mainBlur ? 11 : 14 );
            opExpName = ( blurVal == mainBlur ? "Highlight Main Opacity" : "Highlight Secondary Opacity" );
            modeLayer.blendingMode = BlendingMode.SCREEN;
            modeLayer.setTrackMatte( matteLayer, TrackMatteType.ALPHA_INVERTED );
        } else {
            modeLayer.blendingMode = BlendingMode.MULTIPLY;
            opExpName = "Shadow Opacity";
            modeLayer.label = 0;
            modeLayer.setTrackMatte( matteLayer, TrackMatteType.ALPHA );
        }
        if ( cLExists ) {
            modeLayer( "ADBE Transform Group" ).property( "ADBE Opacity" ).expression = "(thisComp.layer('Controller').effect('" + opExpName + "')(1)/100) * value";
        }
    }
}