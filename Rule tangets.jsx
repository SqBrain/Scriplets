SB_rule_tangents( this );

function SB_rule_tangents( obj ) {
    const WIN_WIDTH = 300;
    const WIN_HEIGHT = 400;

    if ( parseFloat( app.version ) >= 8 ) {
        var myPalette = rule_tangets( obj );
        if ( myPalette != null && myPalette instanceof Window ) {
            myPalette.show();
            myPalette.center();
            myPalette.size = [ WIN_WIDTH, WIN_HEIGHT ];
        }
    } else {
        alert( "This script requires After Effects CS3 or later" );
    }

    //@include "./lib/index.jsx"

    function rule_tangets( thisObj ) {
        var scriptName = "Rule Tangents";
        var version = "1.0.0";


        function buildUI( obj ) {
            const SPC = 2;
            const MRG = 2;

            var win;
            if ( obj instanceof Panel ) {
                win = obj;
            } else {
                win = new Window( "palette", scriptName + " " + version, [ 0, 0, WIN_WIDTH, WIN_HEIGHT ], { resizeable: true } );
            }
            const HEIGHT = 30;
            const SLIDER_WIDTH = 100;
            const SLIDER_DESCR_WIDTH = 60;
            const TEXT_WIDTH = 50;
            const CHECK_WIDTH = 20;
            var grpHeight = 0;

            const grp1Width = SLIDER_DESCR_WIDTH + SLIDER_WIDTH + TEXT_WIDTH + CHECK_WIDTH + SPC * 3 + MRG * 2;
            const grp1Height = MRG * 2 + HEIGHT * 4 + SPC * 3;
            const group1 = win.add( "group", [ MRG, MRG, grp1Width, grp1Height ] );
            const inAngle = createSliderLine( "In Angle", group1, MRG, grpHeight, 0, 360, 0 );

            grpHeight += HEIGHT + SPC;
            const outAngle = createSliderLine( "Out Angle", group1, MRG, grpHeight, 0, 360, 0 );

            grpHeight += HEIGHT + SPC;
            const inLength = createSliderLine( "In Tangent", group1, MRG, grpHeight, 0, 2000, 0 );

            grpHeight += HEIGHT + SPC;
            const outLength = createSliderLine( "Out Tangent", group1, MRG, grpHeight, 0, 2000, 0 );

            const lockAngle = group1.add( "checkbox", [ grp1Width - CHECK_WIDTH - MRG, grp1Height / 4 - 10, grp1Width - MRG, grp1Height / 4 + 10 ] );
            if ( lockAngle.value ) {
                outAngle.slider.enabled = false;
                outAngle.text.enabled = false;
                outAngle.staticText.enabled = false;
            }
            const lockLength = group1.add( "checkbox", [ grp1Width - CHECK_WIDTH - MRG, grp1Height * 3 / 4 - 10, grp1Width - MRG, grp1Height * 3 / 4 + 10 ] );
            if ( lockLength.value ) {
                outLength.slider.enabled = false;
                outLength.text.enabled = false;
                outLength.staticText.enabled = false;
            }
            inAngle.slider.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }
            outAngle.slider.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }
            inLength.slider.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }
            outLength.slider.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }
            inAngle.text.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }
            outAngle.text.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }
            inLength.text.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }
            outLength.text.onChange = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength )
            }

            lockAngle.onClick = function () {
                outAngle.slider.enabled = !lockAngle.value;
                outAngle.text.enabled = !lockAngle.value;
                outAngle.staticText.enabled = !lockAngle.value;
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength );
            }
            lockLength.onClick = function () {
                outLength.slider.enabled = !lockLength.value;
                outLength.text.enabled = !lockLength.value;
                outLength.staticText.enabled = !lockLength.value;
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength );
            }

            grpHeight += HEIGHT + SPC;
            var butAnalyze = win.add( "button", [ MRG, grpHeight, 220, grpHeight + 30 ], "Analyze Spatial" );
            butAnalyze.onClick = function () {
                analyseSpatial( inLength, outLength, inAngle, outAngle, lockAngle, lockLength );
            }

            function createSliderLine( name, group, startWidth, startHeight, minValue, maxValue, value ) {
                const staticText = group.add( "statictext", [ startWidth, startHeight, SLIDER_DESCR_WIDTH, startHeight + HEIGHT ], name );
                startWidth += SLIDER_DESCR_WIDTH + SPC;
                const slider = group.add( "slider", [ startWidth, startHeight, startWidth + SLIDER_WIDTH, startHeight + HEIGHT ] );
                startWidth += SLIDER_WIDTH + SPC;
                const text = group.add( "edittext", [ startWidth, startHeight, startWidth + TEXT_WIDTH, startHeight + HEIGHT ], undefined, { enterKeySignalsOnChange: true } );
                slider.value = value;
                slider.minvalue = minValue;
                slider.maxvalue = maxValue;
                return {
                    staticText: staticText,
                    slider: slider,
                    text: text
                }
            }


            return win;
        }

        function analyseSpatial( lengthIn, lengthOut, angleIn, angleOut, lockAngle, lockLength ) {
            const SLIDER_VALUE = 0;
            const TEXT_VALUE = 1;

            const props = getOnlySpatialProperties( getSelectedProps() );
            if ( props.length > 1 ) return alertError( "", " To many properties selected" );;
            const prop = props[ 0 ];
            const keys = prop.selectedKeys;
            if ( keys.length > 1 ) return alertError( "", " To many keys selected" );
            if ( keys.length === 0 ) return alertError( "", " No keys selected" );
            const key = keys[ 0 ];

            const keyInTangent = prop.keyInSpatialTangent( key );
            const keyOutTangent = prop.keyOutSpatialTangent( key );

            const inAngle = atan2( keyInTangent[ 0 ], keyInTangent[ 1 ] );
            const inAngleDegrees = toDegrees( inAngle ) + 180;
            const outAngle = atan2( keyOutTangent[ 0 ], keyOutTangent[ 1 ] );
            const outAngleDegrees = toDegrees( outAngle ) + 180;
            if ( inAngleDegrees < 0 ) inAngleDegrees += 360;
            if ( outAngleDegrees < 0 ) outAngleDegrees += 360;

            const inLength = findHypotenuse( keyInTangent[ 0 ], keyInTangent[ 1 ] );
            const outLength = findHypotenuse( keyOutTangent[ 0 ], keyOutTangent[ 1 ] );

            lengthIn.slider.value = inLength;
            lengthIn.text.text = inLength;
            if ( lockLength.value ) {
                lengthOut.slider.value = inLength;
                lengthOut.text.text = inLength;
            } else {
                lengthOut.slider.value = outLength;
                lengthOut.text.text = outLength;
            }
            angleIn.slider.value = inAngleDegrees;
            angleIn.text.text = inAngleDegrees;
            if ( lockAngle.value ) {
                angleOut.slider.value = inAngleDegrees;
                angleOut.text.text = inAngleDegrees;
            } else {
                angleOut.slider.value = outAngleDegrees;
                angleOut.text.text = outAngleDegrees;
            }

            lengthIn.slider.onChange = lengthIn.slider.onChanging = setTangent( lengthIn, SLIDER_VALUE );
            lengthIn.text.onChange = setTangent( lengthIn, TEXT_VALUE );
            if ( !lockLength.value ) {
                lengthOut.slider.onChange = lengthOut.slider.onChanging = setTangent( lengthOut, SLIDER_VALUE );
                lengthOut.text.onChange = setTangent( lengthOut, TEXT_VALUE );
            }

            angleIn.slider.onChange = angleIn.slider.onChanging = setTangent( angleIn, SLIDER_VALUE );
            angleIn.text.onChange = setTangent( angleIn, TEXT_VALUE );
            if ( !lockLength.value ) {
                angleOut.slider.onChange = angleOut.slider.onChanging = setTangent( angleOut, SLIDER_VALUE );
                angleOut.text.onChange = setTangent( angleOut, TEXT_VALUE );
            }

            function setTangent( obj, id ) {
                return function () {
                    if ( id === SLIDER_VALUE ) {
                        obj.text.text = floor( obj.slider.value );
                    } else {
                        const value = parseFloat( obj.text.text );
                        if ( isNaN( value ) ) return alertError( "", "Input is not a number" );
                        obj.slider.value = value;
                    }
                    const lengthInValue = floor( lengthIn.slider.value );
                    const angleInValue = ( floor( angleIn.slider.value ) - 180 ) % 360;
                    var lengthOutValue, angleOutValue;
                    if ( !lockLength.value ) {
                        lengthOutValue = floor( lengthOut.slider.value );
                    } else {
                        lengthOutValue = floor( lengthIn.slider.value );
                    }
                    if ( !lockAngle.value ) {
                        angleOutValue = ( floor( angleOut.slider.value ) - 180 ) % 360;
                    } else {
                        angleOutValue = floor( angleIn.slider.value ) % 360;
                    }

                    angleInValue = toRadians( angleInValue );
                    angleOutValue = toRadians( angleOutValue );

                    const xIn = lengthInValue * cos( angleInValue );
                    const yIn = lengthInValue * sin( angleInValue );
                    const xOut = lengthOutValue * cos( angleOutValue );
                    const yOut = lengthOutValue * sin( angleOutValue );

                    prop.setSpatialTangentsAtKey( key, [ xIn, yIn, 0 ], [ xOut, yOut, 0 ] );
                }
            }


        }

        return buildUI( thisObj );
    }
}