if ( typeof kbar !== 'undefined' && kbar.button ) {
    var version = kbar.version;       // KBar version like '3.0.0'
    var JSON = kbar.JSON;             // Lets you stringify/parse JSON strings
    var aeq = kbar.aeq;               // Lets you use aequery

    var button = kbar.button;
    var id = button.id;               // A randomly generated string by KBar
    var name = button.name;           // The name the user set for the button

    SQBaddFlicker( button.argument );
} else {
    SQBaddFlicker();
}

function SQBaddFlicker( arg ) {
    var comp = app.project.activeItem;
    if ( comp && comp instanceof CompItem ) {
        app.beginUndoGroup( "Add Flicker" );
        var layers = comp.selectedLayers;
        var props = comp.selectedProperties;
        var f = comp.frameDuration;
        var time = comp.time;
        if ( props.length == 0 ) {
            if ( arg == "both" ) {
                for ( var i = 0; i < layers.length; i++ ) {
                    var l = layers[ i ];
                    var t = l.inPoint;
                    var t2 = l.outPoint - f;
                    l.transform.opacity.setValuesAtTimes( [ t, t + f, t + 2 * f, t + 3 * f, t + 4 * f, t + 5 * f, t + 6 * f, t + 7 * f, t + 8 * f ], [ 100, 0, 100, 0, 100, 0, 100, 0, 100 ] );
                    l.transform.opacity.setValuesAtTimes( [ t2, t2 - f, t2 - 2 * f, t2 - 3 * f, t2 - 4 * f, t2 - 5 * f, t2 - 6 * f, t2 - 7 * f, t2 - 8 * f ], [ 100, 0, 100, 0, 100, 0, 100, 0, 100 ] );
                }
            } else if ( arg == "start" ) {
                for ( var i = 0; i < layers.length; i++ ) {
                    var l = layers[ i ];
                    var t = l.inPoint;
                    l.transform.opacity.setValuesAtTimes( [ t, t + f, t + 2 * f, t + 3 * f, t + 4 * f, t + 5 * f, t + 6 * f, t + 7 * f, t + 8 * f ], [ 100, 0, 100, 0, 100, 0, 100, 0, 100 ] );
                }
            } else if ( arg == "end" ) {
                for ( var i = 0; i < layers.length; i++ ) {
                    var l = layers[ i ];
                    var t = l.outPoint - f;
                    l.transform.opacity.setValuesAtTimes( [ t, t - f, t - 2 * f, t - 3 * f, t - 4 * f, t - 5 * f, t - 6 * f, t - 7 * f, t - 8 * f ], [ 100, 0, 100, 0, 100, 0, 100, 0, 100 ] );
                }
            } else {
                for ( var i = 0; i < layers.length; i++ ) {
                    var l = layers[ i ];
                    t = time;
                    l.transform.opacity.setValuesAtTimes( [ t, t + f, t + 2 * f, t + 3 * f, t + 4 * f, t + 5 * f, t + 6 * f, t + 7 * f, t + 8 * f ], [ 100, 0, 100, 0, 100, 0, 100, 0, 100 ] );
                }
            }
        } else {
            for ( var i = 0; i < props.length; i++ ) {
                var p = props[ i ];
                try {
                    p.setValuesAtTimes( [ time, time + f, time + 2 * f, time + 3 * f, time + 4 * f, time + 5 * f, time + 6 * f, time + 7 * f, time + 8 * f, time + 9 * f ], [ 0, 100, 0, 100, 0, 100, 0, 100, 0, 100 ] );
                } catch ( e ) { };
            }
        }
        app.endUndoGroup();
    }
}