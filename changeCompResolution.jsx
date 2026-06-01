function changeCompResolution(){
    var alt = ScriptUI.environment.keyboardState.altKey;
    const MUL = "mul";
    const skip = "--";
    var proj = app.project;
    if(alt) {
        changeScale( proj );
    } else {
        changeRes( proj );
    }

    function changeRes (proj) {
        var sel = proj.selection;
        var res = prompt("Input new Resolution separated by space", "width heigth fps");
        if(res) res = res.split(" ");
        if( res.length < 2 || (res[0] !== MUL && isNaN(res[0])) || isNaN(res[1]) ) {
            alert("Wrong input");
            return;
        }
        var width = (res[0] == MUL || res[0] == skip) ? res[0] : Math.abs(parseInt(res[0]));
        var height;
        if( res[1] !== skip){
            height = (res[0] == MUL || res[1] == skip) ? Math.abs(Number(res[1])) : Math.abs(parseInt(res[1]));
        } else {
            heigth = skip;
        }
        var fps = (res.length > 2 && !isNaN(res[2])) ? Number(res[2]) : null;
        app.beginUndoGroup("Change Resolutions");
        for(var i = 0; i < sel.length; i++){
            var item = sel[i];
            if(item instanceof CompItem) {
                if(width == MUL) {
                    item.width = Math.floor(item.width*height);
                    item.height = Math.floor(item.height*height);
                } else {
                    item.width = width;
                    item.height = height;
                }
                if(fps !== null){
                    item.frameRate = fps;
                }
            }
        }
        app.endUndoGroup();
    }

    function changeScale(proj) {
        var comp = proj.activeItem;
        if(comp instanceof CompItem){
            var sel = comp.selectedLayers;
            var res = prompt("Input new Scale separated by space", "prop scale");
            if(res) res = res.split(" ");
            if( res.length < 2 || (res[0] !== MUL && isNaN(res[0])) || isNaN(res[1]) ) {
                alert("Wrong input");
                return;
            }
            var width = res[0] == MUL ? res[0] : Math.abs(parseInt(res[0]));
            var height = res[0] == MUL ? Math.abs(Number(res[1])) : Math.abs(parseInt(res[1]));
            app.beginUndoGroup("Change Resolutions");
            for(var i = 0; i < sel.length; i++){
                var item = sel[i];
                var scale = item.transform.scale;
                if(scale.numKeys == 0){
                    if(width == MUL) {
                        scale.setValue(scale.value*height);
                    } else {
                        scale.setValue([width, height]);
                    }
                }
            }
            app.endUndoGroup();
        }
    }
}

changeCompResolution();