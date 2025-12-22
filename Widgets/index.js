/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// const { use } = require("react");

// Use novadesk.include to load another script globally
// novadesk.include("testWidget.js");
// novadesk.include("timerDemo.js");
// novadesk.include("systemTest.js");

// function onAppReady() {
//     novadesk.log("App ready, initializing widgets...");
//     createContentDemoWindow();
//     createClockWidget();
//     createSystemWidget();
// }
// !enableDebugging;
// !
// !logToFile
var clockWindow;

function createClockWidget() {
    clockWindow = new widgetWindow({
        id: "clockWindow",
        width: 200,
        height: 100,
        backgroundcolor: "rgb(255,255,255)"
    });

    clockWindow.addImage({
        id: "Background Image",
        path: "assets\\Background.png",
        width: 200,
        height: 100,
        onleftmouseup: "clockWindow.refresh()"
    });

    clockWindow.addText({
        id: "hellotext",
        text: "Hello",
        fontsize: 25
    })

}


function onAppReady() {
    createClockWidget();

    // --- API Examples ---
    // 1. widget.refresh() - Clears all elements (flicker-free)
    // 2. widget.close()   - Closes the widget window
    // 3. novadesk.refresh() - Reloads all scripts and widgets
}