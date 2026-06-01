if ( typeof kbar !== 'undefined' && kbar.button ) {
    var version = kbar.version;       // KBar version like '3.0.0'
    var JSON = kbar.JSON;             // Lets you stringify/parse JSON strings
    var aeq = kbar.aeq;               // Lets you use aequery

    var button = kbar.button;
    var id = button.id;               // A randomly generated string by KBar
    var name = button.name;           // The name the user set for the button

    SQB_add_to_expression( button.argument );
} else {
    SQB_add_to_expression( "shapeIndexExpr" );
}

function SQB_add_to_expression( option ) {
    var comp = app.project.activeItem;
    const SLIDER = "ADBE Slider Control";
    const SEED = "Seed";
    const MIN_VAL = "Min ";
    const MAX_VAL = "Max ";
    const S = "\\s*";
    const REVERS = true;
    if ( comp instanceof CompItem ) {
        var props = leaveOnlyCorrectProps( comp.selectedProperties );
        if ( props.length > 0 ) {
            app.beginUndoGroup( "SQB Add to expression" );
            var lastProp = props[ props.length - 1 ];
            var lastLayer = comp.selectedLayers[ comp.selectedLayers.length - 1 ];
            for ( var i = 0; i < props.length; i++ ) {
                var prop = props[ i ];
                var layer = prop.propertyGroup( prop.propertyDepth );
                var layerLink = "thisComp.layer('" + layer.name + "')";
                if ( prop.canSetExpression ) {
                    switch ( option ) {
                        case "random":
                            addRandomExpression( prop, layer, layerLink );
                            break;
                        case "frameDur":
                            addFrameDur( prop );
                            break;
                        case "compHalf":
                            addCompHalf( prop );
                            break;
                        case "mainLayer":
                            addMainLayer( prop, lastLayer );
                            break;
                        case "pathPoints":
                            propPoints( prop );
                            break;
                        case "relativeShapePath":
                            getRelativeShapePath( prop, lastProp );
                            break;
                        case "indexExpr":
                            addIndexExpr( prop, lastLayer, !REVERS );
                            break;
                        case "reverseIndexExpr":
                            addIndexExpr( prop, lastLayer, REVERS );
                            break;
                        case "shapeIndexExpr":
                            addShapeIndexExpr( prop );
                            break;
                        case "mainProps":
                            addMainProps( prop );
                            break;
                    }
                }
            }
            app.endUndoGroup();
        } else {
            alert( "Select some properties" );
            return null;
        }
    }

    function getNameForEffect( layer, name ) {
        var eff = layer( "ADBE Effect Parade" );
        var iter = 1;
        var appendix = "";
        while ( effectExists( eff, name ) ) {
            iter++;
            appendix = " " + iter;
            name = name + appendix;
        }
        return name;
    }

    function effectExists( eff, name ) {
        for ( var i = 1; i <= eff.numProperties; i++ ) {
            var e = eff( i );
            if ( e.name == name ) {
                return true;
            }
        }
        return false;
    }

    function getVarName( name ) {
        var newName = name.replace( /[^\w\d]/g, "_" ).toLowerCase();
        if ( newName[ 0 ].match( /\d/ ) )
            newName = "_" + newName;
        return newName;
    }

    function getPropExpressionName( name ) {
        var newName = name.replace( /\t/g, "" );
        newName[ 0 ] = newName[ 0 ].toLowerCase();
        return newName;
    }

    function addRandomExpression( prop, layer, layerLink ) {
        var cont;
        var eff = layer( "ADBE Effect Parade" );
        if ( !effectExists( eff, SEED ) ) {
            cont = eff.addProperty( SLIDER );
            cont.name = SEED;
        }
        var name = prompt( "Input random effect name", "Val" );
        if ( !name ) name = "Val";
        cont = eff.addProperty( SLIDER );
        var minValName = getNameForEffect( layer, MIN_VAL + name );
        cont.name = minValName;

        cont = eff.addProperty( SLIDER );
        var maxValName = getNameForEffect( layer, MAX_VAL + name );
        cont.name = maxValName;

        var expression = prop.expression;
        var seedMatch = expression.match( /seedRandom\s*.+/ );
        var seedContMatch = expression.match( /seed\s*=.+/ );
        var part1 = "";
        var part2 = "";
        if ( !seedMatch && !seedContMatch ) {
            part1 += "seed = " + layerLink + ".effect(\"" + SEED + "\")(1);\n";
            part1 += "seedRandom( seed, true );\n";
            part2 = expression;
        } else if ( !seedMatch ) {
            var splitIndex = seedContMatch.index + seedContMatch[ 0 ].length;
            part1 = expression.slice( 0, splitIndex ) + "seedRandom( seed, true );\n";
            part2 = expression.slice( splitIndex + 1 );
        } else {
            var splitIndex = seedMatch.index + seedMatch[ 0 ].length;
            part1 = "seed = " + layerLink + ".effect(\"" + SEED + "\")(1);\n" + expression.slice( 0, splitIndex );
            part2 = expression.slice( splitIndex + 1 );
        }
        var minVar = getVarName( minValName );
        var maxVar = getVarName( maxValName );
        var varName = getVarName( name );
        part1 += minVar + " = " + layerLink + ".effect('" + minValName + "')(1);\n";
        part1 += maxVar + " = " + layerLink + ".effect('" + maxValName + "')(1);\n";
        part1 += varName + " = random(" + minVar + ", " + maxVar + ");\n";
        prop.expression = part1 + part2;
    }

    function addFrameDur( prop ) {
        var frameDurMatch = prop.expression.match( /\s*=\s*thisComp\.frameDuration/ );
        var fVar = prop.expression.match( /\s+f\s*=/ );
        var str = " = thisComp.frameDuration;\n";
        var expression;
        if ( !frameDurMatch && !fVar ) {
            expression = prop.expression.replace( /thisComp\.frameDuration/g, "f" );
            prop.expression = "f" + str + expression;
        } else if ( fVar ) {
            expression = prop.expression.replace( /thisComp\.frameDuration/g, "f" );
            prop.expression = "frmDur" + str + expression;
        }
    }

    function addCompHalf( prop ) {
        var w = "thisComp\\.width";
        var h = "thisComp\\.height";
        var reg =
            "(\\[" + S + w + S + "," + S + h + S + "\\]" + S + "\/" + S + "2)|" +
            "(\\[" + S + w + S + "\/ " + S + "2" + S + "," + S + h + S + "\/" + S + "2" + S + "\\])";
        var compHalfMatch = prop.expression.match( new RegExp( "=" + S + reg ) );
        var cVar = prop.expression.match( /\s*c\s*=/ );
        var str = " = [ thisComp.width, thisComp.height ]/2;\n";
        var expression;
        if ( !compHalfMatch && !cVar ) {
            expression = prop.expression.replace( new RegExp( reg, "g" ) );
            prop.expression = "c" + str + expression;
        } else if ( fVar ) {
            expression = prop.expression.replace( new RegExp( reg, "g" ) );
            prop.expression = "compHalf" + str + expression;
        }
    }

    function propPoints( prop ) {
        if ( prop.matchName == "ADBE Vector Shape" ) {
            var exp = "p = points();\n";
            exp += "for( var i = 0; i < p.length; i++){\n";
            exp += "}\n";
            insertExpr( prop, exp );
        } else {
            return null;
        }
    }

    function addMainLayer( prop, lastLayer ) {
        var exp = "l = thisComp.layer(\"" + lastLayer.name + "\");\n";
        insertExpr( prop, exp );
    }

    function addIndexExpr( prop, lastLayer, reverse ) {
        var exp;
        if ( reverse ) {
            exp = "i = index - thisComp.layer(\"" + lastLayer.name + "\").index;\n";
        } else {
            exp = "i = thisComp.layer(\"" + lastLayer.name + "\").index - index;\n";
        }
        insertExpr( prop, exp );
    }

    function addShapeIndexExpr( prop ) {
        for ( var i = 1; i < prop.propertyDepth; i++ ) {
            var parent = prop.propertyGroup( i );
            if ( parent.matchName == "ADBE Vectors Group" || parent.matchName == "ADBE Root Vectors Group" ) {
                break;
            }
        }
        i = ( prop.propertyGroup( i - 1 ).matchName == "ADBE Vector Group" ) ? i - 1 : i + 1;
        insertExpr( prop, "i = thisProperty.propertyGroup(" + ( i ) + ").propertyIndex;" );
    }

    function addMainProps( prop ) {
        addFrameDur( prop );
        addCompHalf( prop );
    }

    function getRelativeShapePath( prop, target ) {
        var parentProp = getCommonParent( prop, target );
        if ( parentProp ) {
            var depth = prop.propertyDepth - parentProp.propertyDepth;
            var exp = "p = propertyGroup(" + depth + ")";
            depth = target.propertyDepth - parentProp.propertyDepth;
            while ( depth > 0 ) {
                var subParent = target.propertyGroup( depth );
                depth--;
                if ( depth == 0 ) {
                    exp += "." + getPropExpressionName( target.name );
                } else if ( isNamedGr( subParent ) ) {
                    exp += "(" + target.propertyGroup( depth ).propertyIndex + ")";
                } else {
                    exp += "(\"" + target.propertyGroup( depth ).name + "\")";
                }
            }
            exp += ";\n";
            insertExpr( prop, exp );
        } else {
            return null;
        }
    }

    function getCommonParent( prop, target ) {
        if (
            getPropLayer( prop ) == getPropLayer( target ) &&
            ( isSingleProp( prop ) && isSingleProp( target ) ) &&
            getPropLayer( prop ) instanceof ShapeLayer
        ) {
            var tDepth = target.propertyDepth;
            var pDepth = prop.propertyDepth;
            var depth = ( pDepth > tDepth ) ? tDepth - 1 : pDepth - 1;
            while ( prop.propertyGroup( pDepth - depth ) != target.propertyGroup( tDepth - depth ) && depth > 0 ) {
                depth--;
            }
            return prop.propertyGroup( pDepth - depth );
        } else {
            return null;
        }
    }

    function getPropLayer( prop ) {
        return prop.propertyGroup( prop.propertyDepth );
    }

    function isSingleProp( prop ) {
        return prop.propertyType == PropertyType.PROPERTY;
    }

    function isNamedGr( prop ) {
        return prop.propertyType == PropertyType.NAMED_GROUP;
    }

    function isIndexedGr( prop ) {
        return prop.propertyType == PropertyType.INDEXED_GROUP;
    }

    function insertExpr( prop, expr ) {
        prop.expression = expr + prop.expression;
    }

    function leaveOnlyCorrectProps( props ) {
        var corrProps = [];
        for ( var i = props.length - 1; i >= 0; i-- ) {
            if ( props[ i ].propertyType == PropertyType.PROPERTY ) {
                corrProps.push( props[ i ] );
            }
        }
        return corrProps;
    }
}
