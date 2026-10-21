/*  VFX Cave · Toggle Reference Opacity (set-opacity) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/Set%20Opacity.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#set-opacity
    Built by VFX Cave from Set Opacity.jsx.
    The sha256 of this file is published in the catalog index. */

setOpacity();

function setOpacity(){
  var op = 40;
  var keyState = ScriptUI.environment.keyboardState;
  if (keyState.altKey){
    op = 100;
  } else if(keyState.shiftKey){
    op = 0;
  }
  var comp = app.project.activeItem;
  if(comp !== undefined && comp instanceof CompItem){
    var layers = comp.selectedLayers;
    for (var i = 0; i < layers.length; i++) {
      var opc = layers[i].property("ADBE Transform Group").property("ADBE Opacity");
      if(opc.numKeys === 0){
        if(opc.value < 100){
          opc.setValue(100);
        } else {
          opc.setValue(op);
        }
      }
    }
  } else {
    alert("Select composition");
  }
}
