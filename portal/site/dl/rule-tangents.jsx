/*  VFX Cave · Rule Tangents (rule-tangents) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/Rule%20tangets.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#rule-tangents
    Built by VFX Cave from Rule tangets.jsx; 16 included file(s) inlined.
    The sha256 of this file is published in the catalog index. */

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

    // [cave] >>> ./lib/index.jsx
 // [cave] >>> ./constants.jsx
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


 // [cave] <<< ./constants.jsx
 // [cave] >>> ./supportingFunctions.jsx
function assignDefVal( variable, defVal, modifiedVal ) {
    if ( variable === undefined ) {
        return defVal;
    } else {
        if ( modifiedVal )
            return modifiedVal;
        else
            return variable;
    }
}

function indexOf( arr, item ) {
    for ( var i = 0; i < arr.length; i++ ) {
        if ( arr[ i ] === item ) return i;
    }
    return -1;
}

function getMatchFromName( elem, name ) {
    if ( searchInName( elem, name ) ) return elem.name.match( name );
    else return null;
}

function sortNumbers( arr, order ) {
    if ( order === undefined || order === null ) order = 1;
    if ( order === 3 ) {
        arr.shuffle();
    } else {
        arr.sort( function ( a, b ) {
            return a - b;
        } );
        if ( order === 2 ) {
            arr.reverse();
        }
    }
    return arr;
}

function compareTwoArrays( arr1, arr2 ) {
    for ( var i = 0; i < arr1.length; i++ ) {
        if ( arr1[ i ] > arr2[ i ] ) {
            return 1;
        } else if ( arr1[ i ] < arr2[ i ] ) {
            return -1;
        } else if ( arr1[ i ] === arr2[ i ] ) {
            continue;
        }
    }
    return 0;
}

function newRegExp( text, exact, flags ) {
    text = assignDefVal( text, ".*?" );
    switch ( exact ) {
        case REGSTART:
            text = "^" + text;
            break;

        case REGEND:
            text += "$";
            break;

        case REGFULL:
            text = "^" + text + "$";
            break;

        case REGNUM:
            text = "^" + text + " *(\\d*)$";
            break;

        case REGNUMEND:
            text = text + " *(\\d*)$";
            break;

        case REGNUMONLY:
            text = "(\\d*)$";
            break;

        default:
            break;
    }

    if ( flags === undefined ) {
        flags = "";
    }
    flags.replace( /[^igmsuy]/g, "" );
    return new RegExp( text, flags );
}

function convertObjToArray( obj ) {
    var arr = [];
    for ( var prop in obj ) {
        arr.push( [ prop, obj[ prop ] ] )
    }
    return arr;
}

function duplicateObj( obj, count ) {
    count = assignDefVal( count, 1 );
    if ( obj && typeof obj.duplicate == 'function' ) {
        for ( var i = 0; i < count; i++ ) {
            obj = obj.duplicate();
        }
    }
    return obj;
}

function toDo() {
    alert( "This function isn't ready" );
}

function getLastItem( arr ) {
    return arr[ arr.length - 1 ];
}
 // [cave] <<< ./supportingFunctions.jsx
 // [cave] >>> ./appLevelFunctions.jsx
function getKeyState() {
    return ScriptUI.environment.keyboardState;
}

function getSetting( name, value ) {
    if ( haveSetting( name ) ) {
        return JSON.parse( app.settings.getSetting( SCRIPT_NAMESafe, name ) );
    } else if ( value ) {
        saveSetting( name, value );
        return value;
    } else {
        return alertError( "getSetting", "Setting " + name + "isn't exists and no default value was passed" );
    }
}

function haveSetting( name ) {
    return app.settings.haveSetting( SCRIPT_NAMESafe, name );
}

function saveSetting( name, value ) {
    if ( value !== undefined ) {
        value = JSON.stringify( value );
    } else {
        alert( "No value was passed to saveSetting" );
        return null;
    }
    app.settings.saveSetting( SCRIPT_NAMESafe, name, value );
}

function reflectProps( obj ) {
    if ( obj ) {
        alert( obj.reflect.properties.join( "\n" ) );
    }
}

function reflectMethods( obj ) {
    if ( obj ) {
        alert( obj.reflect.methods.join( "\n" ) );
    }
}

 // [cave] <<< ./appLevelFunctions.jsx
 // [cave] >>> ./compLevelFunctions.jsx
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
 // [cave] <<< ./compLevelFunctions.jsx
 // [cave] >>> ./propertyLevelFunctions.jsx
function changeExp( prop, reg, newText ) {
    if ( prop.expression.search( reg ) !== -1 ) {
        prop.expression = prop.expression.replace( reg, newText );
    }
}

function copyKeys( prop, prop2, keyIndex, roving ) {
    var keyTime = prop.keyTime( keyIndex );
    prop2.setValueAtTime( prop.keyValue( keyIndex ) );
    prop2.setInterpolationTypeAtKey( keyIndex, prop.keyInInterpolationType( keyIndex ), prop.keyOutInterpolationType( keyIndex ) );

    if ( prop.propertyValueType == PropertyValueType.ThreeD_SPATIAL || prop.propertyValueType == PropertyValueType.TwoD_SPATIAL ) {
        prop2.setSpatialTangentsAtKey( keyIndex, prop.keyInSpatialTangent( keyIndex ), prop.keyOutSpatialTangent( keyIndex ) );
        prop2.setSpatialAutoBezierAtKey( keyIndex, prop.keySpatialAutoBezier( keyIndex ) );
        prop2.setSpatialContinuousAtKey( keyIndex, prop.keySpatialContinuous( keyIndex ) );
        roving.push( prop.keyRoving( keyIndex ) )
    }

    if ( keyObj.inInterpolation == KeyframeInterpolationType.BEZIER || keyObj.outInterpolation == KeyframeInterpolationType.BEZIER ) {
        prop2.setTemporalEaseAtKey( keyIndex, prop.keyInTemporalEase( keyIndex ), prop.keyOutTemporalEase( keyIndex ) );
        prop2.setTemporalAutoBezierAtKey( keyIndex, prop.keyTemporalAutoBezier( keyIndex ) );
        prop2.setTemporalContinuousAtKey( keyIndex, prop.keyTemporalContinuous( keyIndex ) );
    }
}

function copyValue( prop, newProp, exp ) {
    if ( prop.numKeys > 0 ) {
        var roving = [];
        for ( var i = 1; i <= prop.numKeys; i++ ) {
            copyKeys( prop, newProp, i, roving )
        }
        for ( var y = 1; y <= prop.numKeys; y++ ) {
            setRovings( newProp, i, roving[ y ] )
        }
    } else {
        newProp.setValue( prop.value );
    }
    if ( prop.canSetExpression && prop.expression !== "" && exp ) {
        newProp.expression = prop.expression;
    }
}

function deleteKeys( prop ) {
    for ( var i = prop.numKeys; i > 0; i-- ) {
        prop.removeKey( i );
    }
}

function getLinkToProp( prop ) {
    link = "";
    while ( prop.propertyDepth > 0 ) {
        var name = prop.name;
        if ( prop.propertyType === NAMED_GROUP || prop.propertyType === PROPERTY ) {
            name = prop.propertyIndex;
        }
        link = '("' + name + '")' + link;
        prop = prop.propertyGroup( 1 );
    }
    link = 'layer("' + prop.containingComp + '")' + link;
    return link;
}

function setRovings( prop, keyIndex, roving ) {
    prop.setRovingAtKey( keyIndex, roving );
}

function replacePartOfExp( mainExp, reg, exp ) {
    if ( mainExp.indexOf( reg ) !== -1 ) {
        var specSumbls = /([\[\\\^\$\.\|\?\*\+\(\)])/gi;
        if ( reg.match( specSumbls ) !== -1 ) {
            reg = reg.replace( specSumbls, "\\$1" );
        }
        var newReg = new RegExp( reg, 'gi' );
        mainExp = mainExp.replace( newReg, exp );
    }
    return mainExp;
}

function selectKeysOnProp( prop, keys, value ) {
    if ( !isNumeric( keys ) ) return alertError( "selectKeysOnProp: ", "Keys array contain not only numbers" );
    if ( prop.propertyType !== PROPERTY ) return alertError( "selectKeysOnProp: ", "Cant perform select keys on this obj because it's type not a property" );
    if ( prop.numKeys < keys.length ) return alertError( "selectKeysOnProp: ", "Cant perform select keys on this property because it has lesser number of keys than it's number in keys array" );
    if ( value === undefined ) value = true;
    try {
        for ( var i = 0; i < keys.length; i++ ) {
            var index = Math.floor( keys[ i ] );
            prop.setSelectedAtKey( index, value );
        }
    } catch ( err ) {
        alert( "selectKeysOnProp: \n\n" + err.toString() );
        return null;
    }
}

function deleteSingleKey( prop ) {
    try {
        if ( prop.propertyType === PROPERTY && prop.numKeys == 1 ) {
            prop.removeKey( 1 );
        }
    } catch ( err ) {
        alert( "deleteSingleKey: \n\n" + err.toString() );
        return null;
    }
}

function getOnlySpatialProperties( props ) {
    var arr = [];
    for ( var i = 0; i < props.length; i++ ) {
        if ( props[ i ].isSpatial === true )
            arr.push( props[ i ] );
    }
    if ( arr.length === 0 ) return null;
    return arr;
}

function getPropsArr( props, withKeys ) {
    if ( withKeys === undefined ) {
        withKeys = false;
    }
    var arrOfProps = [];
    if ( props.length !== 0 ) {
        if ( props[ 0 ].instance === "Layer" ) {
            for ( var i = props.length - 1; i >= 0; i-- ) {
                var curProps = props[ i ].selectedProperties;
                for ( var y = 0; y < curProps.length; y++ ) {
                    if ( curProps[ y ].propertyType === PropertyType.PROPERTY ) {
                        if ( curProps[ y ].selectedKeys.length > 0 ) {
                            var keys = sortNumbers( curProps[ y ].selectedKeys );
                            var firstTime = curProps[ y ].keyTime( keys[ 0 ] );
                            var lastTime = curProps[ y ].keyTime( keys[ keys.length - 1 ] );
                        } else {
                            var keys = null;
                            var firstTime = 0;
                            var lastTime = 0;
                        }
                        if ( withKeys === true ) {
                            if ( keys !== null ) {
                                arrOfProps.push( {
                                    propNumber: getNumberFromProp( curProps[ y ] ),
                                    keys: keys,
                                    firstKeyTime: firstTime,
                                    lastKeyTime: lastTime,
                                    comp: curProps[ y ].containingComp()
                                } );
                            }
                        } else {
                            arrOfProps.push( {
                                propNumber: getNumberFromProp( curProps[ y ] ),
                                keys: keys,
                                firstKeyTime: firstTime,
                                lastKeyTime: lastTime,
                                comp: curProps[ y ].containingComp()
                            } );
                        }
                    }
                }
            }
        } else {
            for ( var i = props.length - 1; i >= 0; i-- ) {
                var prop = props[ i ];
                if ( prop.propertyType === PropertyType.PROPERTY ) {
                    if ( prop.selectedKeys.length > 0 ) {
                        var keys = sortNumbers( prop.selectedKeys );
                        var firstTime = prop.keyTime( keys[ 0 ] );
                        var lastTime = prop.keyTime( keys[ keys.length - 1 ] );
                    } else {
                        var keys = null;
                        var firstTime = 0;
                        var lastTime = 0;
                    }
                    if ( withKeys === true ) {
                        if ( keys !== null ) {
                            arrOfProps.push( {
                                propNumber: getNumberFromProp( prop ),
                                keys: keys,
                                firstKeyTime: firstTime,
                                lastKeyTime: lastTime,
                                comp: prop.containingComp()
                            } );
                        }
                    } else {
                        arrOfProps.push( {
                            propNumber: getNumberFromProp( prop ),
                            keys: keys,
                            firstKeyTime: firstTime,
                            lastKeyTime: lastTime,
                            comp: prop.containingComp()
                        } );
                    }
                }
            }
            if ( arrOfProps.length === 0 ) return null;
        }
    } else return null;
    return arrOfProps;
}

function getNumberFromProp( prop ) {
    var arrOfIndices = [];
    while ( prop.propertyDepth != 0 ) {
        arrOfIndices.unshift( parseInt( prop.propertyIndex ) );
        prop = prop.parentProperty;
        if ( prop.propertyDepth === 0 ) {
            arrOfIndices.unshift( parseInt( prop.index ) );
            break;
        } else continue;
    }
    return arrOfIndices;
}

function getPropertyFromNumber( arr, comp, delta ) {
    var arrOfIndices = arr;
    if ( delta === undefined ) delta = 0;
    var curComp = app.project.activeItem;
    if ( comp === undefined ) {
        comp = curComp;
    }
    if ( curComp !== comp ) {
        curComp = comp;
    }
    var layer = curComp.layer( arrOfIndices[ 0 ] + delta );
    var length = arrOfIndices.length;
    var i = 1;
    while ( i !== length ) {
        var prop = layer.property( arrOfIndices[ i ] );
        i++;
        layer = prop;
    }
    return layer;
}

function reflect(obj, param) {
    var ref = obj.reflect;
    if(param == 0) {
        alert(ref.properties.join("\n"));
    } else if(param == 1) {
        alert(ref.properties.join("\n"));
    } else {
        alert("PROPERTIES:\n" + ref.properties.join("\n") + "\n" + "METHODS:\n" + ref.methods.join("\n"));
    }
}
 // [cave] <<< ./propertyLevelFunctions.jsx
 // [cave] >>> ./alertFunctions.jsx
function alertSelectComp( funcName ) {
    funcName = funcName || "";
    alert( funcName + "\nNo composition is selected. Select Composition" );
    return null;
}

function alertError( funcName, msg ) {
    funcName = funcName || "";
    msg = ( msg === undefined ) ? "" : "\n" + msg;
    alert( funcName + msg );
    return null;
}

function alertNoAcess() {
    alert( "Network access disabled. To allow, please go to Preferences > General and check off 'Allow Scripts to Write Files and Access Network' to resolve." );
    return null;
}
 // [cave] <<< ./alertFunctions.jsx
 // [cave] >>> ./fileDataFunctions.jsx
function checkPrefs() {
    if ( !isSecurityPrefSet() ) {
        alertNoAcess();
        try {
            app.executeCommand( 2359 );
        } catch ( e ) {

            alert( e );
        }
        if ( !isSecurityPrefSet() ) {
            return false;
        }
    } else {
        return true;
    }
}

function getUserDataFolder() {
    if ( !checkPrefs() ) return null;
    var userDataFolder = Folder.userData;
    var motionScriptFolder = new Folder( userDataFolder.toString() + "/" + AUTHOR_NAME );
    if ( !motionScriptFolder.exists ) {
        motionScriptFolder = motionScriptFolder.create();
        if ( !motionScriptFolder ) {
            alert( "Error creating " + SCRIPT_NAME + " folder. The folder will be created in the\n" + Folder.temp.toString() + "\nfolder" );
            motionScriptFolder = new Folder( Folder.temp.toString() + "/" + AUTHOR_NAME ).create();

        }
    }
    motionScriptFolder = new Folder( motionScriptFolder.toString() + "/" + updateName( SCRIPT_NAME ) );
    if ( !motionScriptFolder.exists ) {
        motionScriptFolder = motionScriptFolder.create();
    }
    return motionScriptFolder.toString();
}

function updateName( name ) {
    return name.replace( " ", "_" );
}

function createResourceFile( filename, binaryString, resourceFolder, binary, remove ) {
    if ( checkPrefs() === null ) return null;
    binary = binary || true;
    var myFolder = new Folder( resourceFolder );
    if ( !myFolder.exists && !myFolder.create() ) {
        alert( "Error:\nCouldn't create resource folder in\n" + myFolder.toString() );
        return null;
    }
    var myFile = new File( myFolder.toString() + "/" + filename );
    if ( File( myFile ).exists ) {
        if ( remove ) {
            myFile.remove();
        } else {
            return myFile;
        }
    }
    if ( binary ) {
        myFile.encoding = "BINARY";
    }
    if ( myFile.open( "w" ) ) {
        myFile.write( binaryString );
        myFile.close();
        return myFile;
    } else {
        alert( "Error:\nCouldn't open resource file in\n" + myFile.toString() );
        return null;
    }
}

function isSecurityPrefSet() {
    var securitySetting;
    try {
        securitySetting = app.preferences.getPrefAsLong( "Main Pref Section", "Pref_SCRIPTING_FILE_NETWORK_SECURITY" );
        return securitySetting;
    } catch ( e ) {
        return false;
    }
}

function openURL( url ) {
    if ( $.os.indexOf( "Windows" ) != -1 )
        system.callSystem( 'cmd /c start "q" "' + url + '"' );
    else {
        var cmd = 'open "' + url + '"';
        system.callSystem( cmd );
    }
}
 // [cave] <<< ./fileDataFunctions.jsx
 // [cave] >>> ./checkFunctions.jsx
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
 // [cave] <<< ./checkFunctions.jsx
 // [cave] >>> ./prototypeLevelFunctions.jsx
AVLayer.prototype.instance = "Layer";
TextLayer.prototype.instance = "Layer";
ShapeLayer.prototype.instance = "Layer";
CameraLayer.prototype.instance = "Layer";
LightLayer.prototype.instance = "Layer";
Property.prototype.instance = "Property";
PropertyGroup.prototype.instance = "Property";

Property.prototype.containingComp = function () {
    return this.propertyGroup( this.propertyDepth ).containingComp;
}
Property.prototype.containingLayer = function () {
    return this.propertyGroup( this.propertyDepth );
}
PropertyGroup.prototype.containingComp = function () {
    return this.propertyGroup( this.propertyDepth ).containingComp;
}
PropertyGroup.prototype.containingLayer = function () {
    return this.propertyGroup( this.propertyDepth );
}
 // [cave] <<< ./prototypeLevelFunctions.jsx
 // [cave] >>> ./controllerFunctions.jsx
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
 // [cave] <<< ./controllerFunctions.jsx
 // [cave] >>> ./moveAnchorPoint.jsx
function moveAnchorToCenter( repositingLayer ) {
    var theLayer = repositingLayer;
    var curComp = repositingLayer.containingComp;
    var curTime = curComp.time;
    var noMasks = true;
    if ( theLayer.mask.numProperties !== 0 ) {
        for ( var i = 1; i <= theLayer.mask.numProperties; i++ ) {
            if ( theLayer.mask( i ).maskMode != MaskMode.NONE ) {
                noMasks = false;
            }
        }
    }
    if ( noMasks ) {
        var x = theLayer.sourceRectAtTime( curTime, false ).width / 2;
        var y = theLayer.sourceRectAtTime( curTime, false ).height;

        if ( ( theLayer instanceof TextLayer ) || ( theLayer instanceof ShapeLayer ) ) {
            x += theLayer.sourceRectAtTime( curTime, false ).left;
            y += theLayer.sourceRectAtTime( curTime, false ).top;
        }
    } else {
        var xBounds = Array();
        var yBounds = Array();
        var numMasks = theLayer.mask.numProperties;
        for ( var f = 1; f <= numMasks; f++ ) {
            var numVerts = theLayer.mask( f ).maskShape.value.vertices.length;
            if ( theLayer.mask( f ).maskMode == MaskMode.NONE )
                continue;
            for ( var j = 0; j < numVerts; j++ ) {
                var curVerts = theLayer.mask( f ).maskShape.valueAtTime( curTime, false ).vertices[j];
                xBounds.push( curVerts[0] );
                yBounds.push( curVerts[1] );
            }
        }
        xBounds.sort( function ( a, b ) {
            return a - b;
        } );

        yBounds.sort( function ( a, b ) {
            return a - b;
        } );

        var xl = xBounds.shift();
        var xh = xBounds.pop();
        var yl = yBounds.shift();
        var yh = yBounds.pop();
        if ( ( theLayer instanceof TextLayer ) || ( theLayer instanceof ShapeLayer ) ) {
            var xl2 = theLayer.sourceRectAtTime( curTime, false ).left;
            var xh2 = xl2 + theLayer.sourceRectAtTime( curTime, false ).width;
            var yl2 = theLayer.sourceRectAtTime( curTime, false ).top;
            var yh2 = yl2 + theLayer.sourceRectAtTime( curTime, false ).height;
            if ( xl < xl2 )
                xl = xl2;
            if ( xh > xh2 )
                xh = xh2;
            if ( yl < yl2 )
                yl = yl2;
            if ( yh > yh2 )
                yh = yh2;
        }
        x = xl + ( ( xh - xl ) / 2 );
        y = yl + ( ( yh - yl ) / 2 );
    }
    try {
        var curAnchor = theLayer.anchorPoint.value;
    } catch ( e ) {
        var curAnchor = theLayer.anchorPoint.valueAtTime( curComp.time, true );
    }
    var xMove = ( x - curAnchor[0] ) * ( theLayer.scale.value[0] / 100 );
    var yMove = ( y - curAnchor[1] ) * ( theLayer.scale.value[1] / 100 );
    var posEx = false;
    if ( theLayer.position.expressionEnabled ) {
        theLayer.position.expressionEnabled = false;
        posEx = true;
    }

    var dupLayer = theLayer.duplicate();
    var oldParent = theLayer.parent;
    dupLayer.moveToEnd();
    if ( dupLayer.scale.isTimeVarying ) {
        dupLayer.scale.setValueAtTime( curComp.time, [100, 100] );
    } else {
        dupLayer.scale.setValue( [100, 100] );
    }

    theLayer.parent = dupLayer;
    if ( theLayer.anchorPoint.isTimeVarying ) {
        var theComp = curComp;
        theLayer.anchorPoint.setValueAtTime( theComp.time, [x, y] );
    } else {
        theLayer.anchorPoint.setValue( [x, y] );
    }
    if ( theLayer.position.isTimeVarying === true ) {
        var curPos = theLayer.position;
        var numKeys = curPos.numKeys;
        var rovs = [];
        for ( var k = 1; k <= numKeys; k++ ) {
            if ( curPos.keyRoving( k ) === true ) {
                curPos.setRovingAtKey( k, false );
                rovs.push( k );
            }
            curPos.setValueAtKey( k, curPos.keyValue( k ) + [xMove, yMove] );
        }
        for ( var k = 0; k < rovs.length; k++ ) {
            curPos.setRovingAtKey( rovs[k], true );
        }
    } else {
        var curPos = theLayer.position.value;
        theLayer.position.setValue( [curPos[0] + xMove, curPos[1] + yMove, curPos[2]] );
    }
    clearOutput();
    theLayer.parent = oldParent;
    if ( posEx ) {
        theLayer.position.expressionEnabled = true;
    }
    dupLayer.remove();
}
 // [cave] <<< ./moveAnchorPoint.jsx
 // [cave] >>> ./timeRefactor.jsx
function linearTiming( timeFraction ) {
    return timeFraction;
}

function quadTiming( timeFraction ) {
    return Math.pow( timeFraction, 2 )
}

function circTiming( timeFraction ) {
    return 1 - Math.sin( Math.acos( timeFraction ) );
}

function backTiming( x, timeFraction ) {
    return Math.pow( timeFraction, 2 ) * ( ( x + 1 ) * timeFraction - x )
}

function elasticTiming( x, timeFraction ) {
    return Math.pow( 2, 10 * ( timeFraction - 1 ) ) * Math.cos( 20 * Math.PI * x / 3 * timeFraction )
}

function makeEaseOutTiming( timing ) {
    return function ( timeFraction ) {
        return 1 - timing( 1 - timeFraction );
    }
}

function makeEaseInOutTiming( timing ) {
    return function ( timeFraction ) {
        if ( timeFraction < .5 )
            return timing( 2 * timeFraction ) / 2;
        else
            return ( 2 - timing( 2 * ( 1 - timeFraction ) ) ) / 2;
    }
}
 // [cave] <<< ./timeRefactor.jsx
 // [cave] >>> ./mathModule.jsx
function getRandomFloat( min, max ) {
    return ( Math.random() * ( max - min ) ) + min;
}

function getRandomInt( min, max ) {
    return Math.round( ( Math.random() * ( max - min ) ) + min );
}

function toRadians( degrees ) {
    return degrees * Math.PI / 180;
}

function toDegrees( radians ) {
    return radians * 180 / Math.PI;
}

function cos( radians ) {
    return Math.cos( radians );
}

function sin( radians ) {
    return Math.sin( radians );
}

function tan( radians ) {
    return Math.tan( radians );
}

function atan( radians ) {
    return Math.atan( radians );
}

function atan2( x, y ) {
    return Math.atan2( y, x );
}

function timeToFrames( time ) {
    var comp = getComp();
    if ( isItComp( comp ) ) {
        return time / curComp.frameDuration;
    } else {
        return alertSelectComp( "Time to Frames" );
    }
}

function sqrt( num ) {
    return Math.sqrt( num );
}

function round( num ) {
    return Math.round( num );
}

function abs( num ) {
    return Math.abs( num );
}

function check( num ) {
    return Math.round( num * 1000 );
}

function pow( num, degree ) {
    return Math.pow( num, degree );
}

function floor( num ) {
    return Math.floor( num );
}

function findHypotenuse( num1, num2 ) {
    return sqrt( pow( num1, 2 ) + pow( num2, 2 ) );
}


function getAverage( array ) {
    if ( !isNumeric( array ) ) {
        alert( "Not a numeric array" );
        return null;
    }
    var sum = 0;
    for ( var i = 0; i < array.length; i++ ) {
        sum += array[ i ];
    }
    return sum / array.length;
}
 // [cave] <<< ./mathModule.jsx
 // [cave] >>> ./colorModule.jsx
function convertColor( color ) {
    if ( color instanceof Array ) {
        for ( var i = 0; i < color.length; i++ ) {
            color[ i ] = Math.floor( ( color[ i ] / 255 ) * 10000 ) / 10000;
        }
    } else if ( isHexNum( color ) ) {
        if ( color[ 0 ] === "#" && ( color.length === 4 || color.length === 7 ) ) {
            color = color.slice( 1 );
        }
        color = getHexArr( color );
    }
    return color;
}

function convertHexColor( color ) {
    if ( color.length < 6 ) {
        while ( color.length < 6 ) {
            color = "0" + color;
        }
    }
    color = getHexArr( color );
    return color;
}

function convertToHex( arr ) {
    var str = "";
    for ( var i = 0; i < 3; i++ ) {
        var color = Math.floor( arr[ i ] * 255 ).toString( 16 );
        str += color;
    }
    while ( str.length < 6 ) {
        str = "0" + str;
    }
    return str;
}

function getHexArr( color ) {
    if ( color.length === 3 ) {
        color = color.split( "" );
        for ( var i = 0; i < 3; i++ ) {
            color[ i ] = color[ i ] + color[ i ];
        }
    } else {
        var temp = [];
        for ( var i = 0; i < 3; i++ ) {
            temp.push( color.slice( i * 2, i * 2 + 2 ) );
        }
        color = temp;
    }
    for ( var i = 0; i < 3; i++ ) {
        color[ i ] = Math.floor( ( parseInt( color[ i ], 16 ) / 255 ) * 10000 ) / 10000;
    }
    return color;
}

function isHexNum( hex ) {
    if ( typeof hex === "string" && ( ( hex.length === 3 || hex.length === 6 ) || ( ( hex.length === 4 || hex.length === 7 ) && hex[ 0 ] === "#" ) ) ) {
        return ( hex.search( /[^\da-fA-F#]/ === -1 ) ) ? true : false;
    } else {
        return false;
    }
}
 // [cave] <<< ./colorModule.jsx
 // [cave] >>> ./highOrderFunctions.jsx
function wrapToUndoGroup( undoName, func, args ) {
    if ( args == undefined ) {
        args = [];
    } else if ( !( args instanceof Array ) ) {
        args = [ args ];
    }
    app.beginUndoGroup( undoName );
    try {
        func.apply( null, args );
    } catch ( e ) {
        alert( undoName + "\n\n" + e.toString() );
        logActions( logFile, undoName, "\nError\n\n" + e.toString() );
    } finally {
        app.endUndoGroup();
    }
}
 // [cave] <<< ./highOrderFunctions.jsx
    // [cave] <<< ./lib/index.jsx

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