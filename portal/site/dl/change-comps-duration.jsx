/*  VFX Cave · 90-Second Comp Duration (change-comps-duration) v1.0.0
    Author: Mihail · License: MIT
    Source: https://github.com/SqBrain/Scriplets/blob/main/Change%20Comps.jsx
    Catalog: https://sqbrain.github.io/Scriplets/#change-comps-duration
    Built by VFX Cave from Change Comps.jsx.
    The sha256 of this file is published in the catalog index. */

SQB_renameItems();

function SQB_renameItems() {
    var items = app.project.selection;
    if (items.length > 0) {
        // var name = prompt( "Enter new name", "$2" );
        var name = "Footage - ";
        app.beginUndoGroup("SQB rename items");
        try {
            if (name) {
                var prec;
                var match = name.match(/(.+)\$(\d+)$/);
                if (match) {
                    prec = parseInt(match[2]);
                    if (isNaN(prec)) prec = 2;
                    match = match[1];
                } else {
                    match = name;
                    prec = 2;
                }
                for (var i = 0; i < items.length; i++) {
                    var item = items[i];
                    // item.name = match + fillName(i + 1, prec);
                    SQB_changeDurAndFPS(item);
                }
            }
            app.endUndoGroup();
        } catch (e) {
            app.endUndoGroup();
        }
    }

    function fillName(num, prec) {
        var n = num.toString();
        while (n.length < prec) {
            n = "0" + n;
        }
        return n;
    }
}

function SQB_changeDurAndFPS(item) {
    if (item instanceof CompItem) {
        // var f = 1 / 25;
        item.duration = 90;
        // item.frameDuration = f;
        // item.width = 1920;
        // item.height = 1080;
        // item.width = Math.floor( item.width / 2 );
        // item.height = Math.floor( item.height / 2 );
        // for ( var y = 1; y <= item.numLayers; y++ ) {
        //     item.layer( y ).outPoint = item.displayStartTime + item.duration;
        // }
    }
}