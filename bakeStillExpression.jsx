SQB_bakeExpressions();
function SQB_bakeExpressions() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        app.beginUndoGroup( "Bake Expressions" );
        var props = comp.selectedProperties;
        for ( var i = 0; i < props.length; i++ ) {
            var prop = props[ i ];
            if ( prop.canSetExpression && prop.expression !== "" ) {
                if ( prop.numKeys > 0 ) {
                    for ( var y = 1; y <= prop.numKeys; y++ ) {
                        prop.setValueAtTime( prop.keyTime( y ), prop.valueAtTime( prop.keyTime( y ), false ) );
                    }
                } else {
                    var val = prop.valueAtTime( prop.propertyGroup( prop.propertyDepth ).inPoint, false );
                    prop.setValue( val );
                }
                prop.expression = "";
            }
        }
        app.endUndoGroup();
    }
}