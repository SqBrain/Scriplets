SB_SplitTextOnWords();
function SB_SplitTextOnWords() {
    //@include "./lib/index.jsx"

    const layers = getSelectedLayers();
    app.beginUndoGroup( "Split Text to words" );
    if ( layers ) {
        const contLayer = createControlNull( "Controller", getComp() );
        const slider = contLayer( EFFECTS_GROUP ).addProperty( SLIDER_CONTROL );
        const spaces = [];
        for ( var i = 0; i < layers.length; i++ ) {
            const layer = layers[ i ];
            if ( !layer instanceof TextLayer ) continue;
            var space = splitTextLayer( layer );
            if ( space !== 0 )
                spaces.push( space );
        }
        const averageSpace = getAverage( spaces );
        slider( 1 ).setValue( averageSpace );
    }
    app.endUndoGroup();

    function splitTextLayer( layer ) {
        const comp = layer.containingComp;
        const time = comp.time;
        var reg1 = /^ *(.+?) *$/gm;
        var reg2 = /^(.*?)$/gm;
        var textVal = layer( TEXT_GROUP )( TEXT_SOURCE_TEXT ).value;
        var text = textVal.text;
        const pos = layer( TRANSFORM )( POSITION );
        moveAnchorPoint( layer, 0, 0, time );
        const posValue = pos.valueAtTime( time, false );


        const lines = text.match( reg2 );
        deleteEmptyLinesOnEdges( lines );

        for ( var i = 0; i < lines.length; i++ ) {
            getLine( lines, layer, i );
        }
    }

    function getLine( lines, origLayer, lineIndex, isSplitToWords ) {
        var newLayer = origLayer.duplicate();
        var layerVal = newLayer( TEXT_GROUP )( TEXT_SOURCE_TEXT ).value;
        var newText = "";
        for ( var i = 0; i < lines.length; i++ ) {
            i == lineIndex ? newText += lines[ i ] : newText += "\n";
        }
        layerVal.text = newText;
        newLayer( TEXT_GROUP )( TEXT_SOURCE_TEXT ).setValue( layerVal );
        moveAnchorPoint( newLayer, 0, 0 );
        if ( isSplitToWords ) {
            splitToWords( newLayer );*/
        }

    }

    function splitToWords( lineLayer ) {

    }

    function deleteEmptyLinesOnEdges( lines ) {
        while ( lines[ 0 ].search( /^\s*$/ ) !== -1 ) {
            lines.splice( 0, 1 );
        }
        while ( lines[ lines.length - 1 ].search( /^\s*$/ ) !== -1 ) {
            lines.splice( lines.length - 1, 1 );
        }
    }

    function moveAnchorPoint( layer, x, y, time ) {
        time = time || 0;
        if ( typeof x !== "number" ) x = 0.5;
        if ( typeof y !== "number" ) y = 0.5;
        var BBox = layer.sourceRectAtTime( time, false );
        const pos = layer( TRANSFORM )( POSITION );
        const anchP = layer( TRANSFORM )( ANCHOR_POINT );
        const scale = layer( TRANSFORM )( SCALE );
        const oldPos = pos.valueAtTime( time, false );
        const oldAP = anchP.valueAtTime( time, false );
        const oldScale = scale.valueAtTime( time, false )[ 0 ] / 100;
        anchP.setValue( [ BBox.left + BBox.width * x, BBox.top + BBox.height * y ] );
        pos.setValue( oldPos - ( oldAP - [ BBox.left, BBox.top ] ) * oldScale );
    }
}