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