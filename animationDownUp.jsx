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
    //@include "./lib/constants.jsx"
    //@include "./lib/Ease_Keyframes.jsx"
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