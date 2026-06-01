createPalette();
function createPalette() {
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        var numStr = prompt( "1-Number Of Colors | 2-Size Value?| 3-Spacing Value?| 4-Round (0/1)?| 5-Colors in Row| 6-Palette Name?", "5|60|5|5|0|Palette" );
        if ( typeof numStr == "string" ) {
            app.beginUndoGroup( "Create Color Palette" );
            var numArr = numStr.split( "|" );
            var num = Math.abs( parseInt( numArr[0] ) );
            if ( isNaN( num ) ) {
                alert( "1 - Number Of Colors | 2 - Size Value?| 3 - Spacing Value?| 4 - Round (0/1)?| 5 - Palette Name?" );
                return;
            }
            var sizeVal = numArr[1] && !isNaN( parseInt( numArr[1] ) ) ? parseInt( numArr[1] ) : 60;
            var spacingVal = numArr[2] && !isNaN( parseInt( numArr[2] ) ) ? parseInt( numArr[2] ) : 5;
            var rowVal = numArr[3] && !isNaN( parseInt( numArr[3] ) ) ? parseInt( numArr[3] ) : 5;
            var roundnessVal = numArr[4] && !isNaN( parseInt( numArr[4] ) ) ? Boolean( parseInt( numArr[4] ) ) : false;
            var paletteName = numArr[5] ? numArr[5] : "Palette";

            const sizeName = "Size";
            const spacingName = "Spacing";
            const rowName = "Row";
            const roundName = "Round?";
            const colorName = "Color ";
            const sliderName = "ADBE Slider Control";
            var palette = comp.layers.addShape();
            palette.name = paletteName;
            palette.guideLayer = true;

            var effGrp = palette( "ADBE Effect Parade" );
            addProperty( effGrp, sizeName, sliderName, sizeVal );
            addProperty( effGrp, spacingName, sliderName, spacingVal );
            addProperty( effGrp, rowName, sliderName, rowVal );
            addProperty( effGrp, roundName, sliderName, roundnessVal );
            for ( var i = 0; i < num; i++ ) {
                addProperty( effGrp, colorName + ( i + 1 ), "ADBE Color Control" );
            }

            var content = palette( "ADBE Root Vectors Group" );
            addColor( content, num );
            app.endUndoGroup();
        }
    }

    function addColor( grp, num ) {
        for ( var i = 0; i < num; i++ ) {
            var colorGrp = grp.addProperty( "ADBE Vector Group" );
            colorGrp.name = colorName + ( i + 1 );
            var content = colorGrp( "ADBE Vectors Group" );

            var rect = addProperty( content, "Rectangle", "ADBE Vector Shape - Rect" );
            rect.size.expression =
                's = effect( "' + sizeName + '" )( 1 );\n' +
                '[s, s];';
            rect.roundness.expression =
                's = effect("' + sizeName + '")(1)/2;\n' +
                'effect("' + roundName + '")(1) == true ? s : 0;';

            var fill = addProperty( content, "Rectangle", "ADBE Vector Graphic - Fill" );
            fill.color.expression =
                'i = thisProperty.propertyGroup( 3 ).propertyIndex;\n' +
                'e = effect( "' + colorGrp.name + '" );\n' +
                '( e !== null ) ? e( 1 ) : [1, 1, 1];';

            colorGrp( "ADBE Vector Transform Group" ).position.expression =
                's = effect( "' + sizeName + '" )( 1 );\n' +
                'sp = effect( "' + spacingName + '" )( 1 );\n' +
                'n = effect( "' + rowName + '" )( 1 );\n' +
                'i = thisProperty.propertyGroup( 2 ).propertyIndex - 1;\n' +
                'ix = i % n;\n' +
                'iy = Math.floor( i / n );\n' +
                'p = thisProperty.propertyGroup( 3 )( 1 ).transform.position;\n' +
                'x = sp * ix + s * ix;\n' +
                'y = sp * iy + s * iy;\n' +
                'p + [x, y];';
        }
    }

    function addProperty( grp, name, matchName, val ) {
        var prop = grp.addProperty( matchName );
        prop.name = name;
        if ( val ) prop( 1 ).setValue( val );
        return prop;
    }
}