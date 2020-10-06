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
