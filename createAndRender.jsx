function createAndRender(){
    const command = app.findMenuCommandId("Add to Adobe Media Encoder Queue...");
    const proj = app.project;
    var folders = proj.selection;
    for(var i = 0; i < folders.length; i++){
        folders[i].selected = false;
    }
    for(var i = 0; i < folders.length; i++){
        var folder = folders[i];
        if( folder instanceof FolderItem ){
            for(var y = 1; y <= folder.numItems; y+=3){
                if(y + 2 <= folder.numItems){
                    var comp1 = folder.item(y);
                    var comp2 = folder.item(y+1);
                    var comp3 = folder.item(y+2);
                    if( comp1 instanceof CompItem && comp1.name.search(/ 1$/) !== -1 && 
                        comp2 instanceof CompItem && comp2.name.search(/ 2$/) !== -1 &&
                        comp3 instanceof CompItem && comp3.name.search(/ 3$/) !== -1) {
                        var newComp = proj.rootFolder.items.addComp(comp1.name, comp1.width, comp1.height, 1, comp1.duration, comp1.frameRate);
                        newComp.selected = true;
                        var layer1 = newComp.layers.add(comp1);
                        // layer1.transform.opacity.setValuesAtTimes([0, 6, 54, 60], [0, 100, 100, 0]);
                        app.executeCommand(command);
                        layer1.enabled = false;
                        var layer2 = newComp.layers.add(comp2);
                        // layer2.transform.opacity.setValuesAtTimes([0, 6, 54, 60], [0, 100, 100, 0]);
                        newComp.name = comp2.name;
                        app.executeCommand(command);
                        layer2.enabled = false;
                        var layer3 = newComp.layers.add(comp3);
                        // layer3.transform.opacity.setValuesAtTimes([0, 6, 54, 60], [0, 100, 100, 0]);
                        newComp.name = comp3.name;
                        app.executeCommand(command);
                        // newComp.duration = 180;
                        // layer1.enabled = true;
                        // layer2.enabled = true;
                        // layer2.startTime = 60;
                        // layer3.startTime = 120;
                        // newComp.name = comp3.name.slice(0, comp3.name.length - 1) + "3 minutes";
                        // app.executeCommand(command);
                        newComp.selected = false;
                    } else {
                        y-=2;
                    }
                }
            }
        }
    }
}

createAndRender();