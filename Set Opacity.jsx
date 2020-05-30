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
