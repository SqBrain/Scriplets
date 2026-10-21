/*  VFX Cave · Inspect Property (Reflect) (alert-reflect) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/Alert%20Reflect.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#alert-reflect
    Built by VFX Cave from Alert Reflect.jsx.
    The sha256 of this file is published in the catalog index. */

function alertReftectionObj() {
	var keyState = ScriptUI.environment.keyboardState;
	var comp = app.project.activeItem;
	if ( comp == undefined || !( comp instanceof CompItem ) ) {
		alert( "Select composition first" );
		return;
	}
	if ( comp.selectedProperties.length > 0 ) {
		var prop = comp.selectedProperties[ 0 ];
		if ( comp.selectedProperties.length == 2 ) {
			prop = comp.selectedProperties[ 1 ];
		}
		if ( keyState.shiftKey === true ) {
			alert( prop.value.reflect.properties.join( "\n" ) );
		} else if ( keyState.ctrlKey === true ) {
			if ( prop.propertyType == PropertyType.PROPERTY )
				alert( prop.matchName + "\n" + prop.propertyValueType );
			else
				alert( prop.matchName );

		} else if ( keyState.altKey === true ) {
			alert( prop.reflect.properties.join( "\n" ) );
		} else {
			var methods = prop.reflect.methods;
			var props = prop.reflect.properties;
			var str = "Property: " + prop.name + "\n\nProperties:\n\n";
			for ( var i = 0; i < props.length; i++ ) {
				var name = props[ i ].toString();
				var t;
				if ( name.length < 9 ) t = "\t\t\t";
				else if ( name.length < 16 ) t = "\t\t";
				else t = "\t";
				try {
					if ( prop[ name ] instanceof PropertyGroup )
						str += name + ":" + t + prop[ name ].name + "\n";
					else if ( name == "propertyValueType" )
						str += name + ":" + t + getStringValueType( prop[ name ].toString() ) + "\n";
					else if ( name == "propertyType" )
						str += name + ":" + t + getStringPropertyType( prop[ name ].toString() ) + "\n";
					else if ( !prop[ name ] )
						str += name + ":" + t + String( prop[ name ] ) + "\n";
					else
						str += name + ":" + t + prop[ name ].toString() + "\n";
				} catch ( error ) {

				}
			}
			str += "\nMethods:\n\n";
			for ( var i = 0; i < methods.length; i++ ) {
				var name = methods[ i ].toString();
				str += name + "()\n";
			}
			alert( str );
		}
	} else {
		alert( "No property was selected" );
	}

	function getStringPropertyType( num ) {
		if ( num == 6214 ) {
			return "INDEXED_GROUP";
		} else if ( num == 6213 ) {
			return "NAMED_GROUP";
		} else {
			return "PROPERTY";
		}
	}

	function getStringValueType( num ) {
		if ( num == 6418 ) {
			return "COLOR";
		} else if ( num == 6419 ) {
			return "CUSTOM_VALUE";
		} else if ( num == 6421 ) {
			return "LAYER_INDEX";
		} else if ( num == 6420 ) {
			return "MARKER";
		} else if ( num == 6422 ) {
			return "MASK_INDEX";
		} else if ( num == 6417 ) {
			return "OneD";
		} else if ( num == 6423 ) {
			return "SHAPE";
		} else if ( num == 6424 ) {
			return "TEXT_DOCUMENT";
		} else if ( num == 6414 ) {
			return "ThreeD";
		} else if ( num == 6413 ) {
			return "ThreeD_SPATIAL";
		} else if ( num == 6416 ) {
			return "TwoD";
		} else if ( num == 6415 ) {
			return "TwoD_SPATIAL";
		} else {
			return "NO_VALUE";
		}
	}
}

alertReftectionObj();