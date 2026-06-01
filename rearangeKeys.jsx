SQB_rearangeKeys();
function SQB_rearangeKeys() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        app.beginUndoGroup( "rearange keys" );
        var f = comp.frameDuration;
        var props = comp.selectedProperties;
        var num = prompt( "Insert number of frames between keys", 2 );
        if ( isNaN( num ) ) num = 1;
        else num = parseInt( num );
        // debugger;
        for ( var i = 0; i < props.length; i++ ) {
            var prop = props[ i ];
            var keys = prop.selectedKeys;
            if ( keys.length > 1 ) {
                var t1 = prop.keyTime( keys[ 0 ] );
                var values = [];
                for ( var y = 0; y < keys.length; y++ ) {
                    values.push( copyKeyValue( prop, keys[ y ] ) );
                }
                for ( var z = keys.length - 1; z >= 0; z-- ) {
                    prop.removeKey( keys[ z ] );
                }
                for ( var e = 0; e < values.length; e++ ) {
                    pasteKeyValue( prop, t1 + f * e * num, values[ e ] );
                }
            }
        }
        app.endUndoGroup();
    }
    function copyKeyValue( prop, key ) {
        var obj = {};
        obj.keyTemporalAutoBezier = prop.keyTemporalAutoBezier( key );
        obj.keyTemporalContinuous = prop.keyTemporalContinuous( key );
        obj.keyInInterpolationType = prop.keyInInterpolationType( key );
        obj.keyOutInterpolationType = prop.keyOutInterpolationType( key );
        obj.keyInTemporalEase = prop.keyInTemporalEase( key );
        obj.keyOutTemporalEase = prop.keyOutTemporalEase( key );
        obj.value = prop.keyValue( key );
        obj.keyLabel = prop.keyLabel( key );
        if ( prop.propertyValueType == PropertyValueType.TwoD_SPATIAL || prop.propertyValueType == PropertyValueType.ThreeD_SPATIAL ) {
            obj.spatial == true;
            obj.keyInSpatialTangent = prop.keyInSpatialTangent( key );
            obj.keyOutSpatialTangent = prop.keyOutSpatialTangent( key );
            obj.keySpatialAutoBezier = prop.keySpatialAutoBezier( key );
            obj.keySpatialContinuous = prop.keySpatialContinuous( key );
        } else {
            obj.spatial == false;
        }
        return obj;
    }
    function pasteKeyValue( prop, time, obj ) {
        var key = prop.addKey( time );
        prop.setTemporalAutoBezierAtKey( key, obj.keyTemporalAutoBezier );
        prop.setTemporalContinuousAtKey( key, obj.keyTemporalContinuous );
        prop.setValueAtKey( key, obj.value );
        prop.setLabelAtKey( key, obj.keyLabel );
        prop.setTemporalEaseAtKey( key, obj.keyInTemporalEase, obj.keyOutTemporalEase );
        prop.setInterpolationTypeAtKey( key, obj.keyInInterpolationType, obj.keyOutInterpolationType );
        if ( obj.spatial ) {
            prop.setInSpatialTangentAtKey( key, obj.keyInSpatialTangent );
            prop.setOutSpatialTangentAtKey( key, obj.keyOutSpatialTangent );
            prop.setSpatialAutoBezierAtKey( key, obj.keySpatialAutoBezier );
            prop.setSpatialContinuousAtKey( key, obj.keySpatialContinuous );
        }
    }
}