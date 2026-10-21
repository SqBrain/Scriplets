/*  VFX Cave · Show Property Path (show-hierarchy-of-property) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/showIerarhyOfProperty.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#show-hierarchy-of-property
    Built by VFX Cave from showIerarhyOfProperty.jsx.
    The sha256 of this file is published in the catalog index. */

function showIerarhyOfPropertyMDS() {
	var comp = app.project.activeItem;
	if ( comp != undefined && comp instanceof CompItem ) {
		var props = comp.selectedProperties;
		if ( props.length === 0 ) {
			alert( "No prorperty selected" );
			return;
		}
		var prop = props[0];
		var arr = [];
		var s = "";
		for ( var i = prop.propertyDepth - 1; i > 0; i-- ) {
			var pr = prop.propertyGroup( i );
			arr.push( s + "Name: " + pr.name + "\tMatch Name: " + pr.matchName + "\tIndex: " + pr.propertyIndex + "\tpropertyGroup(" + i + ")" );
			s += "  ";
		}
		arr.push( s + "Name: " + prop.name + "\tMatch Name: " + prop.matchName + "\tIndex: " + prop.propertyIndex + "\tpropertyGroup(0)" );
		alert( arr.join( "\n\n" ) );
	} else {
		alert( "Select composition" );
		return;
	}
}

showIerarhyOfPropertyMDS();