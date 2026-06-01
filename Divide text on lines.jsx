SQ_divideTextOnLines();

function SQ_divideTextOnLines() {
    //@include "./lib/index.jsx"
    var comp = app.project.activeItem;
    if ( comp instanceof CompItem ) {
        var layers = comp.selectedLayers;
        for ( var i = 0; i < layers.length; i++ ) {
            var layer = layers[i];
            if ( layer instanceof TextLayer ) {
                var text = layer( TEXT_ );
            }
        }
    }
}