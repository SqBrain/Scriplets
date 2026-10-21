/*  VFX Cave · Trim Comp to Layers (trim-comp-to-layers) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/trimCompToLayers.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#trim-comp-to-layers
    Built by VFX Cave from trimCompToLayers.jsx.
    The sha256 of this file is published in the catalog index. */

function trimToLayersDur(){
	var comp = app.project.activeItem;
	if(comp != undefined && comp instanceof CompItem){
		var layers = comp.numLayers;
		if(layers > 0){
			var inP = comp.layer(1).inPoint;
			var outP = comp.layer(1).outPoint;
			for (var i = 2; i <= layers; i++) {
				if(comp.layer(i).inPoint < inP) inP = comp.layer(i).inPoint;
				if(comp.layer(i).outPoint > outP) outP = comp.layer(i).outPoint;
			}
			comp.displayStartTime = inP;
			comp.duration = outP - inP;
		} else {
			alert("No layers in Composition");
		}
	} else {
		alert("Select composition");
		return;
	}
}

trimToLayersDur();