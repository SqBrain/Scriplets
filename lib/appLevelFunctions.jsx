function getKeyState() {
    return ScriptUI.environment.keyboardState;
}

function getSetting( name, value ) {
    if ( haveSetting( name ) ) {
        return JSON.parse( app.settings.getSetting( SCRIPT_NAMESafe, name ) );
    } else if ( value ) {
        saveSetting( name, value );
        return value;
    } else {
        return alertError( "getSetting", "Setting " + name + "isn't exists and no default value was passed" );
    }
}

function haveSetting( name ) {
    return app.settings.haveSetting( SCRIPT_NAMESafe, name );
}

function saveSetting( name, value ) {
    if ( value !== undefined ) {
        value = JSON.stringify( value );
    } else {
        alert( "No value was passed to saveSetting" );
        return null;
    }
    app.settings.saveSetting( SCRIPT_NAMESafe, name, value );
}

function reflectProps( obj ) {
    if ( obj ) {
        alert( obj.reflect.properties.join( "\n" ) );
    }
}

function reflectMethods( obj ) {
    if ( obj ) {
        alert( obj.reflect.methods.join( "\n" ) );
    }
}
