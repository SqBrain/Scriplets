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

