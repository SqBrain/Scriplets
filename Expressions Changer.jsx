function convertExpressionsToText( path ) {
    const distFolderName = "converted";
    const QUOTE = "\"";
    var folder = new Folder( path );
    if ( Folder.exists ) {
        var files = folder.getFiles( "*.jsx" );
        var destinationFolder = new Folder( path + "/" + distFolderName );
        createIndexFile( files );
        for ( var y = 0; y < files.length; y++ ) {
            var file = files[ y ];
            file.open( "r" );
            var textOfExpressions = file.read().split( "\n" );
            file.close();
            textOfExpressions = removeEmptyLines( textOfExpressions );
        }
    }

    function writeExpressionToNewFile() {
        var newFile = new File( destinationFolder.path + "/" + file.name );
        newFile.open( "w" );
        for ( var z = 0; z < textOfExpressions.length; z++ ) {
            var end = ( z !== textOfExpressions.length - 1 ) ? " +" : ";";
            var line = textOfExpressions[ z ];
            if ( line.search( /^\/\// ) !== -1 ) {
                line = "var " + line.match( /^\/\/(.+)/ )[ 1 ] + "=";
            }
            newFile.writeln( QUOTE + line + "\\n" + QUOTE + end );
        }
    }

    function removeEmptyLines( textArray ) {
        var newArray = [];
        for ( var i = 0; i <= textArray.length; i++ ) {
            if ( textArray[ i ] !== "" ) {
                newArray.push( textArray[ i ] );
            }
        }
        return newArray;
    }

    function createIndexFile( folder, files ) {
        var indexFile = new File( folder.path + "/index.jsx" );
        if ( indexFile.exists ) {
            for ( var i = 0; i < files.length; i++ ) {
                if ( files[ i ] == indexFile ) {
                    files.splice( i, 1 );
                    break;
                }
            }
        }
        indexFile.open( "w" );
        for ( var y = 0; y < files.length; y++ ) {
            indexFile.writeln( "//@include '" + distFolderName + "/" + files[ y ].name + "'" );
        }
        indexFile.close();
        return indexFile;
    }
}