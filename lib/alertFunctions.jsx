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