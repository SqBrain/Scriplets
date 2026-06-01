createPlashka();
function createPlashka() {
    //@include "./lib/constants.jsx"

    const WIDTH_NAME = "Width";
    const HEIGHT_NAME = "Height";
    const VER_ANCHOR = "Vertical Anchor";
    const HOR_ANCHOR = "Horizontal Anchor";
    const PLASHKA = "plashka";
    const MATTE = "matte";

    var keyState = ScriptUI.environment.keyboardState;
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        app.beginUndoGroup( "Create Plashka" );
        var layers = comp.selectedLayers;
        for ( var i = 0; i < layers.length; i++ ) {
            var layer = layers[i];
            if ( keyState.altKey ) {
                addPlashka( layer, PLASHKA );
            } else if ( keyState.shiftKey ) {
                addPlashkaAndMatte( layer );
            } else {
                addPlashka( layer, MATTE );
            }
        }
        app.endUndoGroup();
    }

    function addPlashka( layer, matter ) {
        const plashka = comp.layers.addShape();
        const bBox = layer.sourceRectAtTime( comp.time, false );
        const trans = layer( TRANSFORM );
        const scale = trans( SCALE ).valueAtTime( comp.time, false );
        const pos = trans( POSITION ).valueAtTime( comp.time, false );
        const rot = trans( ROTATION ).valueAtTime( comp.time, false );
        const or = trans( ORIENTATION ).valueAtTime( comp.time, false );
        const anch = trans( ANCHOR_POINT ).valueAtTime( comp.time, false );
        const center = [bBox.left + bBox.width / 2, bBox.top + bBox.height / 2];
        const offset = center - anch;
        offset = [offset[0] * scale[0] / 100, offset[1] * scale[1] / 100];

        if ( matter == MATTE ) {
            plashka.label = 13;
            plashka.name = layer.name + " MATTE";
            plashka.moveBefore( layer );
            layer.trackMatteType = MATTE_TYPE_ALPHA;
            layer( EFFECTS_GROUP ).addProperty( TRANSFORM_EFFECT );
        } else {
            plashka.name = layer.name + " PLASHKA";
            plashka.moveAfter( layer );
        }

        createShapeRectangle( plashka, bBox );
        if ( matter == PLASHKA ) {
            const effects = plashka( EFFECTS_GROUP );
            effects.addProperty( FILL_EFFECT );
        }
        plashka.parent = layer.parent;
        plashka.threeDLayer = layer.threeDLayer;
        plashka( TRANSFORM )( POSITION ).setValue( pos + offset );
        plashka( TRANSFORM )( ROTATION ).setValue( rot );
        if ( plashka.threeDLayer ) {
            plashka( TRANSFORM )( ORIENTATION ).setValue( or );
        }
        plashka( TRANSFORM )( SCALE ).setValue( scale );
        plashka.inPoint = layer.inPoint;
        plashka.outPoint = layer.outPoint;
        return plashka;
    }

    function addPlashkaAndMatte( layer ) {
        const matte = addPlashka( layer, MATTE );
        const plashka = addPlashka( layer, PLASHKA );
        matte.parent = plashka;
        const exp = "thisComp.layer(index + 2).effect(thisProperty.propertyGroup(1).name)(1);";
        matte( EFFECTS_GROUP )( WIDTH_NAME )( 1 ).expression = exp;
        matte( EFFECTS_GROUP )( HEIGHT_NAME )( 1 ).expression = exp;
        matte( EFFECTS_GROUP )( VER_ANCHOR )( 1 ).expression = exp;
        matte( EFFECTS_GROUP )( HOR_ANCHOR )( 1 ).expression = exp;
    }

    function createShapeRectangle( layer, bBox ) {
        const effects = layer( EFFECTS_GROUP );
        addRectControls( effects, bBox );

        const rect = layer( VEC_ROOT ).addProperty( VEC_RECT );
        rect( 2 ).expression =
            'w = effect( "' + WIDTH_NAME + '" )( 1 );\n' +
            'h = effect( "' + HEIGHT_NAME + '" )( 1 );\n' +
            '[ w, h ]; ';
        rect( 3 ).expression =
            'horDir = effect( "' + HOR_ANCHOR + '" )( 1 );\n' +
            'verDir = effect( "' + VER_ANCHOR + '" )( 1 );\n' +
            'w = effect( "' + WIDTH_NAME + '" )( 1 );\n' +
            'h = effect( "' + HEIGHT_NAME + '" )( 1 );\n' +
            'if ( horDir == 1 ) {\n' +
            '    x = w / 2;\n' +
            '} else if ( horDir == 3 ) {\n' +
            '    x = -w / 2;\n' +
            '} else {\n' +
            '    x = 0;\n' +
            '}\n' +
            'if ( verDir == 1 ) {\n' +
            '    y = h / 2;\n' +
            '} else if ( verDir == 3 ) {\n' +
            '    y = -h / 2;\n' +
            '} else {\n' +
            '    y = 0;\n' +
            '}\n' +
            '[ x, y ];\n';

        const fill = layer( VEC_ROOT ).addProperty( VEC_FILL );
    }

    function addRectControls( effects, bBox ) {
        const width = effects.addProperty( SLIDER_CONTROL );
        width( 1 ).setValue( bBox.width );
        width.name = WIDTH_NAME;
        const height = effects.addProperty( SLIDER_CONTROL );
        height( 1 ).setValue( bBox.height );
        height.name = HEIGHT_NAME;
        const vertAnchor = effects.addProperty( DROPDOWN_CONTROL );
        var index = vertAnchor.propertyIndex;
        vertAnchor( 1 ).setPropertyParameters( ['top', 'center', 'bottom'] );
        vertAnchor = effects( index );
        vertAnchor.name = VER_ANCHOR;
        vertAnchor( 1 ).setValue( 2 );
        const horAnchor = effects.addProperty( DROPDOWN_CONTROL );
        index = horAnchor.propertyIndex;
        horAnchor( 1 ).setPropertyParameters( ['left', 'center', 'right'] );
        horAnchor = effects( index );
        horAnchor.name = HOR_ANCHOR;
        horAnchor( 1 ).setValue( 2 );
    }
}