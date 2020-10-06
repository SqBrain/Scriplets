function createControlLayer( name, comp ) {
    try {
        comp = comp || getComp();
        name = name || CONTROL_LAYER_NAME;
        var controlLayer = comp.layers.addShape();
        controlLayer.label = 9;
        controlLayer.name = checkAndSetName( comp, name );
        return controlLayer;
    } catch ( e ) {
        return alertError( "createControlLayer", e.toString() );
    }
}

function applyPreset( layer, file, name ) {
    try {
        layer.selected = true;
        layer.applyPreset( file );
        layer.selected = false;
        var eff = getLayerEffects( layer );
        var curEff = eff.property( eff.numProperties );
        if ( name !== undefined ) {
            curEff.name = checkAndSetName( eff, name );
        }
        return curEff;
    } catch ( e ) {
        return alertError( "applyPreset", e.toString() );
    }
}

function addSliderController( layer, name, value, expression ) {
    if ( !layer || !( layer.instance === 'Layer' ) ) return alertError( "addSliderController: ", "Object isn't a layer" );
    name = name || "Slider Control";
    value = value || 1;
    const controller = layer( EFFECTS_GROUP ).addProperty( SLIDER_CONTROL );
    controller.name = name;
    controller( 1 ).setValue( value );
    if ( expression ) controller( 1 ).expression = expression;
    return controller;
}