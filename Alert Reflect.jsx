function alertReftectionObj(){
	var keyState = ScriptUI.environment.keyboardState;
	var comp = app.project.activeItem;
	if(comp == undefined || !(comp instanceof CompItem)){
		alert("Select composition first");
		return;
	}
	if(comp.selectedProperties.length > 0){
		var prop = comp.selectedProperties[0];
		if(comp.selectedProperties.length == 2){
			prop = comp.selectedProperties[1];
		}
		if(keyState.shiftKey === true){
			alert(prop.value.reflect.properties.join("\n"));
		} else if (keyState.ctrlKey === true){
			alert(prop.value.reflect.methods.join("\n"));
		} else if(keyState.altKey === true){
			alert(prop.reflect.methods.join("\n"));
		} else {
			alert(prop.reflect.properties.join("\n"));
		}
	} else {
		alert("No property was selected");
	}
}

alertReftectionObj();