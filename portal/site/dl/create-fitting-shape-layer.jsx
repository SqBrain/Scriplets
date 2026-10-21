/*  VFX Cave · Shape Layer Spanning Selection (create-fitting-shape-layer) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/CreateFittingShapeLayer.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#create-fitting-shape-layer
    Built by VFX Cave from CreateFittingShapeLayer.jsx.
    The sha256 of this file is published in the catalog index. */

SQB_createFittingShapeLayers();
function SQB_createFittingShapeLayers() {
    var comp = app.project.activeItem;
    app.beginUndoGroup( "Create Fitting Shape Layers" );
    if ( comp && comp instanceof CompItem ) {
        var layers = comp.selectedLayers;
        var inPoint = 0;
        var outPoint = 0;
        if ( layers.length == 0 ) {
            outPoint = comp.displayStartTime + comp.duration;
        }
        var index = 0;
        for ( var i = 0; i < layers.length; i++ ) {
            var layer = layers[ i ];
            if ( i == 0 ) {
                inPoint = layer.inPoint;
                outPoint = layer.outPoint;
                index = layer.index;
            } else {
                if ( inPoint > layer.inPoint ) {
                    inPoint = layer.inPoint;
                }
                if ( outPoint < layer.outPoint ) {
                    outPoint = layer.outPoint;
                }
                if ( index > layer.index ) {
                    index = layer.index;
                }
            }
        }
        var shape = comp.layers.addShape();
        shape.inPoint = inPoint;
        shape.outPoint = outPoint;
        shape.moveBefore( comp.layer( index + 1 ) );
    }
    app.endUndoGroup();
}