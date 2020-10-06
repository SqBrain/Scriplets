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