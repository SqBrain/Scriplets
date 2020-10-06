SB_addControllableMatte();
function SB_addControllableMatte() {
    const authorName = "Rig_Factory";
    const scriptName = "Add_Controllable_Matte";
    const scriptVersion = "1.0.0";

    //@include "./lib/index.jsx"
    //@include "./MattePresetBinnaryCode.jsx"

    const alt = getKeyState().altKey;
    const presetName = "SB Matte Preset";
    const preset = createResourceFile( "mattePreset.ffx", BIN_PRESET_MATTE, getUserDataFolder() )

    const comp = app.project.activeItem;
    if ( comp && comp instanceof CompItem ) {
        app.beginUndoGroup( "Create Matte" );
        const layers = comp.selectedLayers;
        if ( layers.length === 0 ) {
            alert( "No layers selected" );
            return;
        }
        const sortedLayers = sort( layers );
        for ( var i = sortedLayers.length - 1; i >= 0; i-- ) {
            var layer = sortedLayers[ i ];
            if ( alt ) createMatte( layer );
            else createLowerThird( layer );
        }
        app.endUndoGroup();
    } else {
        alert( "Select composition" );
    }

    function sort( array, callback ) {
        if ( !isArray( array ) ) return array;
        if ( callback == undefined || typeof callback !== "function" ) {
            if ( isNumeric( array ) ) {
                callback = function ( item ) { return item; }
            } else {
                callback = function ( item ) {
                    if ( item.index )
                        return item.index;
                    else if ( item.propertyIndex )
                        return item.propertyIndex;
                    else {
                        alert( "Cannot return any index and array doesn't contain only numbers" );

                    }
                }
            }
        }
        const arr = [];
        for ( var i = 0; i < array.length; i++ ) {
            var item = array[ i ];
            var comparable = callback( item );
            if ( arr.length == 0 ) {
                arr.push( item );
            } else {
                var y = 0;
                while ( y < arr.length && comparable > callback( arr[ y ] ) ) {
                    y++;
                }
                arr.splice( y, 0, item );
            }
        }
        return arr;
    }

    function createLowerThird( layer ) {
        if ( layer instanceof CameraLayer && layer instanceof LightLayer && !layer.source.hasVideo && layer.nullLayer ) return false;
        const comp = layer.containingComp;
        const layerEff = layer( EFFECTS_GROUP );
        if ( !layerEff( "xAxis" ) )
            createSliderControl( layerEff, "xAxis", 0 );
        if ( !layerEff( "yAxis" ) )
            createSliderControl( layerEff, "yAxis", 0 );

        const anchP = layer( TRANSFORM )( ANCHOR_POINT );
        const layerAnchorPointVal = anchP.valueAtTime( comp.time, false );
        const layerBBox = layer.sourceRectAtTime( comp.time, false );
        const xVal = 100 * ( layerAnchorPointVal[ 0 ] - layerBBox.left ) / layerBBox.width;
        const yVal = 100 * ( layerAnchorPointVal[ 1 ] - layerBBox.top ) / layerBBox.height;
        anchP.expression =
            "xAxis = effect( 'xAxis' )( 1 ) / 100;\n" +
            "yAxis = effect( 'yAxis' )( 1 ) / 100;\n" +
            "b = sourceRectAtTime();\n" +
            "[ b.left + b.width * xAxis, b.top + b.height * yAxis ];\n"

        const lowerThird = comp.layers.addShape();
        lowerThird.label = 11;
        lowerThird.moveAfter( layer );
        const eff = lowerThird( EFFECTS_GROUP )
        const rect = lowerThird( 2 ).addProperty( VEC_RECT );
        if ( layer instanceof ShapeLayer || layer instanceof TextLayer ) {
            layerAnchorPointVal[ 0 ] += comp.width / 2;
            layerAnchorPointVal[ 1 ] += comp.height / 2;
        }

        createSliderControl( eff, "Minimal Width", 100 );
        if ( !preset ) {
            createSliderControl( eff, "Top", 1 );
            createSliderControl( eff, "Bottom", 1 );
            createSliderControl( eff, "Left", 1 );
            createSliderControl( eff, "Right", 1 );
            const xAxis = createSliderControl( eff, "xAxis", xVal );
            xAxis.expression = "thisComp.layer( index - 1 ).effect( 'xAxis' )( 1 );";
            const yAxis = createSliderControl( eff, "yAxis", yVal );
            yAxis.expression = "thisComp.layer( index - 1 ).effect( 'yAxis' )( 1 );";
            lowerThird( TRANSFORM )( ANCHOR_POINT ).expression =
                "top = effect( 'Top' )( 1 );\n" +
                "bot = effect( 'Bottom' )( 1 );\n" +
                "left = effect( 'Left' )( 1 );\n" +
                "right = effect( 'Right' )( 1 );\n" +
                "x = effect( 'xAxis' )( 1 )/100;\n" +
                "y = effect( 'xAxis' )( 1 )/100;\n" +
                "b = sourceRectAtTime();\n" +
                "[ b.left + b.width * x + left * ( 1 - x ) - right * x, b.top + b.height * y - bot * y + top * ( 1 - y ) ];";
            rect( 2 ).expression =
                "top = effect( 'Top' )( 1 );\n" +
                "bot = effect( 'Bottom' )( 1 );\n" +
                "left = effect( 'Left' )( 1 );\n" +
                "right = effect( 'Right' )( 1 );\n" +
                "minW = effect('Minimal Width')(1);\n" +
                "\n" +
                "s = thisComp.layer( index - 1 ).sourceRectAtTime();\n" +
                "w = s.width + left + right;\n" +
                "if(w < minW) w = minW;\n" +
                "[ w, s.height + bot + top ];"
        } else {
            lowerThird.applyPreset( preset );
            const lowerThirdEff = eff( 2 );
            lowerThirdEff.name = presetName;
            xAx = lowerThirdEff( "X Axis" );
            xAx.expression = "thisComp.layer( index - 1 ).effect( 'xAxis' )( 1 );";
            lowerThirdEff( "Y Axis" ).expression = "thisComp.layer( index - 1 ).effect( 'yAxis' )( 1 );";

            lowerThird( TRANSFORM )( ANCHOR_POINT ).expression =
                "e = effect( '" + presetName + "' );\n" +
                "top = e( 'Top' );\n" +
                "bot = e( 'Bottom' );\n" +
                "left = e( 'Left' );\n" +
                "right = e( 'Right' );\n" +
                "x = e( 'X Axis' )/100;\n" +
                "y = e( 'Y Axis' )/100;\n" +
                "b = sourceRectAtTime();\n" +
                "[ b.left + b.width * x + left * ( 1 - x ) - right * x, b.top + b.height * y - bot * y + top * ( 1 - y ) ];";
            rect( 2 ).expression =
                "e = effect( '" + presetName + "' );\n" +
                "top = e( 'Top' );\n" +
                "bot = e( 'Bottom' );\n" +
                "left = e( 'Left' );\n" +
                "right = e( 'Right' );\n" +
                "minW = effect('Minimal Width')(1);\n" +
                "\n" +
                "s = thisComp.layer( index - 1 ).sourceRectAtTime();\n" +
                "w = s.width + left + right;\n" +
                "if(w < minW) w = minW;\n" +
                "[ w, s.height + bot + top ];"
        }
        const fill = lowerThird( 2 ).addProperty( VEC_FILL );
        fill( 4 ).setValue( [ 1, 1, 1 ] );
        lowerThird( TRANSFORM )( POSITION ).expression = "thisComp.layer( index - 1 ).transform.position;";
    }

    function createMatte( layer ) {
        if ( layer instanceof CameraLayer && layer instanceof LightLayer && !layer.source.hasVideo && layer.nullLayer ) return false;
        const comp = layer.containingComp;
        const layerEff = layer( EFFECTS_GROUP );
        if ( !layerEff( "xAxis" ) )
            createSliderControl( layerEff, "xAxis", 0 );
        if ( !layerEff( "yAxis" ) )
            createSliderControl( layerEff, "yAxis", 0 );
        const matte = comp.layers.addShape();
        matte.label = 13;
        matte.enabled = false;
        matte.moveBefore( layer );
        const eff = matte( EFFECTS_GROUP )
        const rect = matte( 2 ).addProperty( VEC_RECT );
        const layerAnchorPointVal = layer( TRANSFORM )( ANCHOR_POINT ).valueAtTime( comp.time, false );
        // const layerPositionVal = layer( TRANSFORM )( POSITION ).valueAtTime( comp.time, false );
        const layerBBox = layer.sourceRectAtTime( comp.time, false );
        const xVal = 100 * ( layerAnchorPointVal[ 0 ] - layerBBox.left ) / layerBBox.width;
        const yVal = 100 * ( layerAnchorPointVal[ 1 ] - layerBBox.top ) / layerBBox.height;
        if ( layer instanceof ShapeLayer || layer instanceof TextLayer ) {
            layerAnchorPointVal[ 0 ] += comp.width / 2;
            layerAnchorPointVal[ 1 ] += comp.height / 2;
        }
        createSliderControl( eff, "Minimal Width", 100 );

        if ( !preset ) {
            createSliderControl( eff, "Top", 1 );
            createSliderControl( eff, "Bottom", 1 );
            createSliderControl( eff, "Left", 1 );
            createSliderControl( eff, "Right", 1 );
            const xAxis = createSliderControl( eff, "xAxis", xVal );
            xAxis.expression = "thisComp.layer( index + 1 ).effect( 'xAxis' )( 1 );";
            const yAxis = createSliderControl( eff, "yAxis", yVal );
            yAxis.expression = "thisComp.layer( index + 1 ).effect( 'yAxis' )( 1 );";
            matte( TRANSFORM )( ANCHOR_POINT ).expression =
                "top = effect( 'Top' )( 1 );\n" +
                "bot = effect( 'Bottom' )( 1 );\n" +
                "left = effect( 'Left' )( 1 );\n" +
                "right = effect( 'Right' )( 1 );\n" +
                "x = effect( 'xAxis' )( 1 )/100;\n" +
                "y = effect( 'xAxis' )( 1 )/100;\n" +
                "b = sourceRectAtTime();\n" +
                "[ b.left + b.width * x + left * ( 1 - x ) - right * x, b.top + b.height * y - bot * y + top * ( 1 - y ) ];";
            rect( 2 ).expression =
                "top = effect( 'Top' )( 1 );\n" +
                "bot = effect( 'Bottom' )( 1 );\n" +
                "left = effect( 'Left' )( 1 );\n" +
                "right = effect( 'Right' )( 1 );\n" +
                "minW = effect('Minimal Width')(1);\n" +
                "\n" +
                "s = thisComp.layer( index + 1 ).sourceRectAtTime();\n" +
                "w = s.width + left + right;\n" +
                "if(w < minW) w = minW;\n" +
                "[ w, s.height + bot + top ];"
        } else {
            matte.applyPreset( preset );
            const matteEff = eff( 2 );
            matteEff.name = presetName;
            matteEff( "X Axis" ).expression = "thisComp.layer( index + 1 ).effect( 'xAxis' )( 1 );";
            matteEff( "Y Axis" ).expression = "thisComp.layer( index + 1 ).effect( 'yAxis' )( 1 );";

            matte( TRANSFORM )( ANCHOR_POINT ).expression =
                "e = effect( '" + presetName + "' );\n" +
                "top = e( 'Top' );\n" +
                "bot = e( 'Bottom' );\n" +
                "left = e( 'Left' );\n" +
                "right = e( 'Right' );\n" +
                "x = e( 'X Axis' )/100;\n" +
                "y = e( 'Y Axis' )/100;\n" +
                "b = sourceRectAtTime();\n" +
                "[ b.left + b.width * x + left * ( 1 - x ) - right * x, b.top + b.height * y - bot * y + top * ( 1 - y ) ];";
            rect( 2 ).expression =
                "e = effect( '" + presetName + "' );\n" +
                "top = e( 'Top' );\n" +
                "bot = e( 'Bottom' );\n" +
                "left = e( 'Left' );\n" +
                "right = e( 'Right' );\n" +
                "minW = effect('Minimal Width')(1);\n" +
                "\n" +
                "s = thisComp.layer( index + 1 ).sourceRectAtTime();\n" +
                "w = s.width + left + right;\n" +
                "if(w < minW) w = minW;\n" +
                "[ w, s.height + bot + top ];"
        }
        const fill = matte( 2 ).addProperty( VEC_FILL );
        fill( 4 ).setValue( [ 1, 1, 1 ] );
        matte( TRANSFORM )( POSITION ).expression =
            "l = thisComp.layer( index+1 );\n" +
            "l.toComp( [ 0, 0, 0 ] ) + l( 'ADBE Transform Group' )( 'ADBE Anchor Point' );";
        layer.trackMatteType = TrackMatteType.ALPHA;
        const transformEff = layer( EFFECTS_GROUP ).addProperty( "ADBE Geometry2" );
        // transformEff( "Anchor Point" ).setValue( [ Math.floor( layerAnchorPointVal[ 0 ] * 100 ) / 100, Math.floor( layerAnchorPointVal[ 1 ] * 100 ) / 100 ] );
        // transformEff( "Position" ).setValue( [ Math.floor( layerPositionVal[ 0 ] * 100 ) / 100, Math.floor( layerPositionVal[ 1 ] * 100 ) / 100 ] );
    }

    function createSliderControl( eff, name, value ) {
        name = name || "Slider";
        if ( !value || typeof value !== "number" ) value = 0;
        const slider = eff.addProperty( SLIDER_CONTROL );
        slider.name = name;
        slider( 1 ).setValue( value );
        return slider;
    }
}