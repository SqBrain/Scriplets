function checkPrefs() {
    if ( !isSecurityPrefSet() ) {
        alertNoAcess();
        try {
            app.executeCommand( 2359 );
        } catch ( e ) {

            alert( e );
        }
        if ( !isSecurityPrefSet() ) {
            return false;
        }
    } else {
        return true;
    }
}

function getUserDataFolder() {
    if ( !checkPrefs() ) return null;
    var userDataFolder = Folder.userData;
    var motionScriptFolder = new Folder( userDataFolder.toString() + "/" + AUTHOR_NAME );
    if ( !motionScriptFolder.exists ) {
        motionScriptFolder = motionScriptFolder.create();
        if ( !motionScriptFolder ) {
            alert( "Error creating " + SCRIPT_NAME + " folder. The folder will be created in the\n" + Folder.temp.toString() + "\nfolder" );
            motionScriptFolder = new Folder( Folder.temp.toString() + "/" + AUTHOR_NAME ).create();

        }
    }
    motionScriptFolder = new Folder( motionScriptFolder.toString() + "/" + updateName( SCRIPT_NAME ) );
    if ( !motionScriptFolder.exists ) {
        motionScriptFolder = motionScriptFolder.create();
    }
    return motionScriptFolder.toString();
}

function updateName( name ) {
    return name.replace( " ", "_" );
}

function createResourceFile( filename, binaryString, resourceFolder, binary, remove ) {
    if ( checkPrefs() === null ) return null;
    binary = binary || true;
    var myFolder = new Folder( resourceFolder );
    if ( !myFolder.exists && !myFolder.create() ) {
        alert( "Error:\nCouldn't create resource folder in\n" + myFolder.toString() );
        return null;
    }
    var myFile = new File( myFolder.toString() + "/" + filename );
    if ( File( myFile ).exists ) {
        if ( remove ) {
            myFile.remove();
        } else {
            return myFile;
        }
    }
    if ( binary ) {
        myFile.encoding = "BINARY";
    }
    if ( myFile.open( "w" ) ) {
        myFile.write( binaryString );
        myFile.close();
        return myFile;
    } else {
        alert( "Error:\nCouldn't open resource file in\n" + myFile.toString() );
        return null;
    }
}

function isSecurityPrefSet() {
    var securitySetting;
    try {
        securitySetting = app.preferences.getPrefAsLong( "Main Pref Section", "Pref_SCRIPTING_FILE_NETWORK_SECURITY" );
        return securitySetting;
    } catch ( e ) {
        return false;
    }
}

function openURL( url ) {
    if ( $.os.indexOf( "Windows" ) != -1 )
        system.callSystem( 'cmd /c start "q" "' + url + '"' );
    else {
        var cmd = 'open "' + url + '"';
        system.callSystem( cmd );
    }
}