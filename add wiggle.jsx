function addWiggle(){
	var comp = app.project.activeItem;
	if(comp != undefined && comp instanceof CompItem){
		app.beginUndoGroup("add wiggle");
		var layers = comp.selectedLayers;
		for (var i = 0; i < layers.length; i++) {
			var layer = layers[i];
			var props = layer.selectedProperties;
			var effs = layer.property("ADBE Effect Parade");
			var freq, amp;
			for(var y = 1; y <= effs.numProperties; y++){
				if(effs.property(y).name === "Freq"){
					freq = effs.property(y);
				}
			}
			if(freq == undefined){
				freq = effs.addProperty("ADBE Slider Control");	
				freq.property(1).setValue(3);
				freq.name = "Freq";
			}
			for(var z = 1; z <= effs.numProperties; z++){
				if(effs.property(z).name === "Amp"){
					amp = effs.property(z);
				}
			}
			if(amp == undefined){
				amp = effs.addProperty("ADBE Slider Control");
				amp.property(1).setValue(10);
				amp.name = "Amp";
			}
			for (var i = 0; i < props.length; i++) {
				var prop = props[i];
				if(prop.canSetExpression){
					prop.expression = "wiggle(thisLayer.effect(\"Freq\")(1), thisLayer.effect(\"Amp\")(1));";
				}
			}
		}
		app.endUndoGroup();
	} else {
		alert("Select composition");
		return;
	}
}

addWiggle();