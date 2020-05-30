function showIerarhyOfPropertyMDS(){
	var comp = app.project.activeItem;
	if(comp != undefined && comp instanceof CompItem){
		var props = comp.selectedProperties;
		if(props.length === 0){
			alert("No prorperty selected");
			return;
		}
		var prop = props[0];
		var arr = [];
		var s = "";
		for(var i = prop.propertyDepth - 1; i > 0; i--){
			var pr = prop.propertyGroup(i);
			arr.push(s + "Name: " + pr.name + "\tIndex: " + pr.propertyIndex + "\tpropertyGroup(" + i + ")"); 
				s += "  ";
		}
		arr.push(s + "Name: " + prop.name + "\tIndex: " + prop.propertyIndex + "\tpropertyGroup(0)"); 
		alert(arr.join("\n\n"));
	} else {
		alert("Select composition");
		return;
	}
}

showIerarhyOfPropertyMDS();