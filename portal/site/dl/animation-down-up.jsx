/*  VFX Cave · Slide In/Out with Bounce (animation-down-up) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/animationDownUp.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#animation-down-up
    Built by VFX Cave from animationDownUp.jsx; 2 included file(s) inlined.
    The sha256 of this file is published in the catalog index. */

if ( typeof kbar !== 'undefined' && kbar.button ) {
    var version = kbar.version;       // KBar version like '3.0.0'
    var JSON = kbar.JSON;             // Lets you stringify/parse JSON strings
    var aeq = kbar.aeq;               // Lets you use aequery

    var button = kbar.button;
    var id = button.id;               // A randomly generated string by KBar
    var name = button.name;           // The name the user set for the button

    SQB_addAnimationStartEnd( button.argument );
} else {
    SQB_addAnimationStartEnd( "up" );
}

function SQB_addAnimationStartEnd( condition ) {
    app.beginUndoGroup( "Apply Animation" );
    // [cave] >>> ./lib/constants.jsx
//TRANSFORM GROUP MATCH NAMES______________________________________________________________________________________________________________________________________________________________

const TRANSFORM = "ADBE Transform Group";
const POSITION = "ADBE Position";
const POSITION_X = "ADBE Position_0";
const POSITION_Y = "ADBE Position_1";
const POSITION_Z = "ADBE Position_2";
const ANCHOR_POINT = "ADBE Anchor Point";
const ROTATION = "ADBE Rotate Z";
const ROTATION_X = "ADBE Rotate X";
const ROTATION_Y = "ADBE Rotate Y";
const ORIENTATION = "ADBE Orientation";
const SCALE = "ADBE Scale";
const OPACITY = "ADBE Opacity";

//VECTOR MATCH NAMES______________________________________________________________________________________________________________________________________________________________________

const VEC_TRANSFORM = "ADBE Vector Transform Group";
const VEC_POSITION = "ADBE Vector Position";
const VEC_ANCHOR_POINT = "ADBE Vector Anchor";
const VEC_SCALE = "ADBE Vector Scale";
const VEC_OPACITY = "ADBE Vector Group Opacity";
const VEC_ROTATION = "ADBE Vector Rotation";
const VEC_SKEW = "ADBE Vector Skew";
const VEC_SKEW_AXIS = "ADBE Vector Skew Axis";
const VEC_ROOT = "ADBE Root Vectors Group";
const VEC_CONTENT = "ADBE Vectors Group";
const VEC_GROUP = "ADBE Vector Group";
const VEC_ELLIPSE = "ADBE Vector Shape - Ellipse";
const VEC_RECT = "ADBE Vector Shape - Rect";
const VEC_PATH_GROUP = "ADBE Vector Shape - Group";
const VEC_STAR = "ADBE Vector Shape - Star";
const VEC_PATH = "ADBE Vector Shape";

const VEC_MERGE = "ADBE Vector Filter - Merge";
const VEC_REPEAT = "ADBE Vector Filter - Repeater";
const VEC_Wiggle = "ADBE Vector Filter - Wiggler";
const VEC_ROUND_CORNER = "ADBE Vector Filter - RC";
const VEC_ROUND_CORNER_RADIUS = "ADBE Vector RoundCorner Radius";

const VEC_FILL = "ADBE Vector Graphic - Fill";
const VEC_FILL_COLOR = "ADBE Vector Fill Color";
const VEC_FILL_OPACITY = "ADBE Vector Fill Opacity";

const VEC_STROKE = "ADBE Vector Graphic - Stroke";
const VEC_STROKE_COLOR = "ADBE Vector Stroke Color";
const VEC_STROKE_WIDTH = "ADBE Vector Stroke Width";
const VEC_STROKE_CAP = "ADBE Vector Stroke Line Cap";
const VEC_STROKE_JOIN = "ADBE Vector Stroke Line Join";
const VEC_STROKE_DASHES = "ADBE Vector Stroke Dashes";
const VEC_STROKE_DASH = "ADBE Vector Stroke Dash ";
const VEC_STROKE_GAP = "ADBE Vector Stroke Gap ";
const VEC_STROKE_OFF = "ADBE Vector Stroke Offset";

const VEC_TRIM = "ADBE Vector Filter - Trim";
const VEC_TRIM_START = "ADBE Vector Trim Start";
const VEC_TRIM_END = "ADBE Vector Trim End";
const VEC_TRIM_OFF = "ADBE Vector Trim Offset";

const VEC_OFFSET = "ADBE Vector Filter - Offset";
const VEC_OFFSET_JOIN = "ADBE Vector Offset Line Join";
const VEC_OFFSET_AMOUNT = "ADBE Vector Offset Amount";

const VEC_ZIG = "ADBE Vector Filter - Zigzag";
const VEC_ZIG_SIZE = "ADBE Vector Zigzag Size";
const VEC_ZIG_SEG = "ADBE Vector Zigzag Detail";

const VECTORSHAPE = [ VEC_ELLIPSE, VEC_RECT, VEC_PATH, VEC_STAR ];

//DIFFERENT LAYER MATCH NAMES______________________________________________________________________________________________________________________________________________________________

const EFFECTS_GROUP = "ADBE Effect Parade";
const MASKS = "ADBE Mask Parade";
const MASK_GROUP = "ADBE Mask Atom";
const MASK_SHAPE = "ADBE Mask Shape";
const MARKERS_GROUP = "ADBE Marker Group";
const MARKER = "ADBE Marker";
const TEXT_GROUP = "ADBE Text Properties";
const TEXT_SOURCE_TEXT = "ADBE Text Document";

//EXPRESSION CONTROLS______________________________________________________________________________________________________________________________________________________________________

const SLIDER_CONTROL = "ADBE Slider Control";
const CHECKBOX_CONTROL = "ADBE Checkbox Control";
const COLOR_CONTROL = "ADBE Color Control";
const LAYER_CONTROL = "ADBE Layer Control";
const ANGLE_CONTROL = "ADBE Angle Control";
const POINT_CONTROL = "ADBE Point Control";
const POINT3D_CONTROL = "ADBE Point3D Control";
const DROPDOWN_CONTROL = "ADBE Dropdown Control";
const FILL_EFFECT = "ADBE Fill";
const TRANSFORM_EFFECT = "ADBE Geometry2";

//MASK MODES_______________________________________________________________________________________________________________________________________________________________________________

const MASK_MODE_NONE = MaskMode.NONE;
const MASK_MODE_ADD = MaskMode.ADD;
const MASK_MODE_SUB = MaskMode.SUBTRACT;
const MASK_MODE_INT = MaskMode.INTERSECT;
const MASK_MODE_LIGHT = MaskMode.LIGHTEN;
const MASK_MODE_DARK = MaskMode.DARKEN;
const MASK_MODE_DIFF = MaskMode.DIFFERENCE;

//PROPERTY VALUE TYPES_____________________________________________________________________________________________________________________________________________________________________

const NO_VALUE = PropertyValueType.NO_VALUE;
const ThreeD_SPATIAL_VALUE = PropertyValueType.ThreeD_SPATIAL;
const ThreeD_VALUE = PropertyValueType.ThreeD;
const TwoD_SPATIAL_VALUE = PropertyValueType.TwoD_SPATIAL;
const TwoD_VALUE = PropertyValueType.TwoD;
const OneD_VALUE = PropertyValueType.OneD;
const COLOR_VALUE = PropertyValueType.COLOR;
const CUSTOM_VALUE = PropertyValueType.CUSTOM_VALUE;
const MARKER_VALUE = PropertyValueType.MARKER;
const LAYER_INDEX_VALUE = PropertyValueType.LAYER_INDEX;
const MASK_INDEX_VALUE = PropertyValueType.MASK_INDEX;
const SHAPE_VALUE = PropertyValueType.SHAPE;
const TEXT_DOCUMENT_VALUE = PropertyValueType.TEXT_DOCUMENT;

//KEYFRAME INTERPOLATION TYPES____________________________________________________________________________________________________________________________________________________________

const LINEAR = KeyframeInterpolationType.LINEAR;
const HOLD = KeyframeInterpolationType.HOLD;
const BEZIER = KeyframeInterpolationType.BEZIER;

//PROPERTY TYPES__________________________________________________________________________________________________________________________________________________________________________

const PROPERTY = PropertyType.PROPERTY;
const INDEXED_GROUP = PropertyType.INDEXED_GROUP;
const NAMED_GROUP = PropertyType.NAMED_GROUP;

//REGEXP TYPES____________________________________________________________________________________________________________________________________________________________________________

const REG_START = "start";
const REG_END = "end";
const REG_FULL = "full";
const REG_NUM = "num";
const REG_NUM_END = "numEnd";
const REG_NUM_ONLY = "onlyNum";

//CAPS AND JOINS TYPES____________________________________________________________________________________________________________________________________________________________________

const BUT_CAP = 1;
const ROUND_CAP = 2;
const BEVEL_CAP = 3;

//layer types
const SHAPE_LAYER = 'shapeLayer';
const CAMERA_LAYER = 'cameraLayer';
const LIGHT_LAYER = 'lightLayer';
const NULL_LAYER = 'nullLayer';
const COMP_LAYER = 'compLayer';
const VIDEO_LAYER = 'videoLayer';
const SOUND_LAYER = 'soundLayer';
const ADJUSTMENT_LAYER = 'adjustmentLayer';
const TEXT_LAYER = 'textLayer';
const SOLID_LAYER = 'solidLayer'; //10
const STILL_LAYER = 'stillLayer';
const TRACK_MATTE_LAYER = 'trackMatteLayer';

//trackMatteTypes

const MATTE_TYPE_ALPHA = TrackMatteType.ALPHA
const MATTE_TYPE_ALPHA_INVERTED = TrackMatteType.ALPHA_INVERTED
const MATTE_TYPE_LUMA = TrackMatteType.LUMA
const MATTE_TYPE_LUMA_INVERTED = TrackMatteType.LUMA_INVERTED
const MATTE_TYPE_NO_TRACK_MATTE = TrackMatteType.NO_TRACK_MATTE


    // [cave] <<< ./lib/constants.jsx
    // [cave] >>> ./lib/Ease_Keyframes.jsx
function easeKeyframe( number, condition, inTran, outTran ) {
    if ( number === null ) {
        return;
    } else if ( number > 100 ) {
        number = 100;
    } else if ( number < 0 ) {
        number = 0.1;
    }
    var comp = getComp();
    if ( comp ) {
        var selProps = getSelectedProps( comp, NO_ALERT, PROPS_WITH_KEYS );
        for ( var j = 0; j < selProps.length; j++ ) {
            var prop = selProps[ j ];
            var selKeys = prop.selectedKeys;
            var rovings = [];
            for ( var k = 0; k < selKeys.length; k++ ) {
                var isRoving = setInterpolationAtKey( prop, selKeys[ k ], condition, number, inTran, outTran );
                if ( isRoving !== null ) {
                    rovings.push( isRoving );
                }
            }
            for ( var s = 0; s < rovings.length; s++ ) {
                prop.setRovingAtKey( rovings[ s ], true );
            }
        }
    }
}

function setInterpolationAtKey( prop, key, condition, number, inTran, outTran ) {
    if ( isSpatial( prop ) && prop.keyRoving( key ) === true ) {
        return key;
    } else {
        var inInt, outInt, inEase, outEase;
        if ( inTran === true ) {
            if ( condition !== 0 ) {
                inInt = getInterpolation( condition, prop );
            } else {
                inEase = new KeyframeEase( 0, number );
            }
        } else {
            inInt = prop.keyInInterpolationType( key );
            inEase = new KeyframeEase( prop.keyInTemporalEase( key )[ 0 ].speed, prop.keyInTemporalEase( key )[ 0 ].influence );
        }

        if ( outTran === true ) {
            if ( condition !== 0 ) {
                outInt = getInterpolation( condition, prop );
            } else {
                outEase = new KeyframeEase( 0, number );
            }
        } else {
            outInt = prop.keyOutInterpolationType( key );
            outEase = new KeyframeEase( prop.keyOutTemporalEase( key )[ 0 ].speed, prop.keyOutTemporalEase( key )[ 0 ].influence );
        }

        if ( condition !== 0 ) {
            prop.setInterpolationTypeAtKey( key, inInt, outInt );
            if ( condition === 3 && inTran === true && outTran === true && prop.isInterpolationTypeValid( BEZIER ) ) {
                prop.setTemporalContinuousAtKey( key, true );
                prop.setTemporalAutoBezierAtKey( key, true );
            }
        } else {
            if ( prop.isInterpolationTypeValid( BEZIER ) ) {
                if ( !isSpatial( prop ) && ( prop.value.length === 3 ) ) {
                    prop.setTemporalEaseAtKey( key, [ inEase, inEase, inEase ], [ outEase, outEase, outEase ] );
                } else if ( !isSpatial( prop ) && ( prop.value.length === 2 ) ) {
                    prop.setTemporalEaseAtKey( key, [ inEase, inEase ], [ outEase, outEase ] );
                } else {
                    prop.setTemporalEaseAtKey( key, [ inEase ], [ outEase ] );
                }
            }
        }
        return null;
    }

    function getInterpolation( condition, prop ) {
        if ( condition === 1 ) {
            if ( prop.isInterpolationTypeValid( LINEAR ) ) {
                return LINEAR;
            } else if ( prop.isInterpolationTypeValid( BEZIER ) ) {
                return BEZIER;
            } else if ( prop.isInterpolationTypeValid( HOLD ) ) {
                return HOLD;
            }
        } else if ( condition === 2 ) {
            if ( prop.isInterpolationTypeValid( HOLD ) ) {
                return HOLD;
            } else if ( prop.isInterpolationTypeValid( LINEAR ) ) {
                return LINEAR;
            } else if ( prop.isInterpolationTypeValid( BEZIER ) ) {
                return BEZIER;
            }
        } else if ( condition === 3 ) {
            if ( prop.isInterpolationTypeValid( BEZIER ) ) {
                return BEZIER;
            } else if ( prop.isInterpolationTypeValid( LINEAR ) ) {
                return LINEAR;
            } else if ( prop.isInterpolationTypeValid( HOLD ) ) {
                return HOLD;
            }
        }
    }

    function isSpatial( prop ) {
        return prop.propertyValueType === TwoD_SPATIAL_VALUE || prop.propertyValueType === ThreeD_SPATIAL_VALUE;
    }
}
    // [cave] <<< ./lib/Ease_Keyframes.jsx
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        var angVal = 0;
        switch ( condition ) {
            case "down":
                angVal = 180;
                break;
            case "right":
                angVal = 90;
                break;
            case "left":
                angVal = 270;
                break;
            default:
                angVal = 0;
        }
        var layers = comp.selectedLayers;
        var f = comp.frameDuration;
        var timeOff = 10;
        var driverName = "Driver";
        var offsetName = "Offset";
        var ampName = "Amplitude";
        var freqName = "Frequency";
        var decName = "Decay";
        var angName = "Angle";
        for ( var i = 0; i < layers.length; i++ ) {
            var layer = layers[ i ];
            var inP = layer.inPoint;
            var outP = layer.outPoint;
            if ( layer instanceof AVLayer || layer instanceof ShapeLayer || layer instanceof TextLayer ) {
                var eff = layer.effect;
                var amp = eff.addProperty( "ADBE Slider Control" );
                amp.name = ampName;
                amp( 1 ).setValue( 20 );
                var freq = eff.addProperty( "ADBE Slider Control" );
                freq.name = freqName;
                freq( 1 ).setValue( 40 );
                var dec = eff.addProperty( "ADBE Slider Control" );
                dec.name = decName;
                dec( 1 ).setValue( 60 );
                var ang = eff.addProperty( "ADBE Angle Control" );
                ang.name = angName;
                ang( 1 ).setValue( angVal );
                var driver = eff.addProperty( "ADBE Slider Control" );
                driver.name = driverName;
                driver( 1 ).setValuesAtTimes( [ inP, inP + f * timeOff, outP - f * timeOff, outP ], [ -1, 0, 0, 1 ] );
                setInterpolationAtKey( driver( 1 ), 1, 0, 65, false, true );
                setInterpolationAtKey( driver( 1 ), 3, 0, 65, false, true );
                // layer.marker.setValueAtTime( inP + f * timeOff, new MarkerValue( "" ) );
                driver( 1 ).expression =
                    "try {\n  amp = effect(\"" + ampName + "\")(1) / 200;\n" +
                    "  freq = effect(\"" + freqName + "\")(1) / 30;\n" +
                    "  decay = effect(\"" + decName + "\")(1) / 10;\n" +
                    "  n = 0;\n" +
                    "  if (numKeys > 0){\n" +
                    "    n = nearestKey(time).index; \n" +
                    "    if (key(n).time > time){\n" +
                    "      n--;\n" +
                    "    }\n" +
                    "  }\n" +
                    "  if (n == 0){\n" +
                    "    t = 0;\n" +
                    "  } else {\n" +
                    "    t = time - key(n).time;\n" +
                    "  }\n" +
                    "\n" +
                    "  if (n > 0){\n" +
                    "    v = velocityAtTime(key(n).time - thisComp.frameDuration/10);\n" +
                    "    value + v*amp*Math.sin(freq*t*2*Math.PI)/Math.exp(decay*t);\n" +
                    "  } else {\n" +
                    "    value;\n" +
                    "  }\n" +
                    "} catch (e) {\n" +
                    "  value;\n" +
                    "}";

                var offset = eff.addProperty( "ADBE Slider Control" );
                offset.name = offsetName;
                offset( 1 ).setValue( 100 );
                layer.transform.position.expression =
                    "o = effect('" + offsetName + "')(1);\n" +
                    "d = effect('" + driverName + "')(1);\n" +
                    "a = degreesToRadians(effect('" + angName + "')(1) - 90);\n" +
                    "x = o*Math.cos(a);\n" +
                    "y = o*Math.sin(a);\n" +
                    "value + [x*d, y*d];";
                layer.transform.opacity.setValuesAtTimes( [ inP, inP + f * timeOff, outP - f * timeOff, outP ], [ 0, 100, 100, 0 ] );
                setInterpolationAtKey( layer.transform.opacity, 1, 0, 65, false, true );
                setInterpolationAtKey( layer.transform.opacity, 3, 0, 65, false, true );
            }
        }
    }
    app.endUndoGroup();
}