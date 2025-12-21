/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

var demoWidget;

function createContentDemoWindow() {
    demoWidget = new widgetWindow({
        id: "testWidget",
        width: 400,
        height: 300,
        backgroundcolor: "rgba(30,30,30,220)",
        zpos: "ondesktop",
        draggable: true,
        keeponscreen: true
    });

    demoWidget.addText({
        id: "statusText",
        text: "Click me to test setProperties!",
        x: 0, y: 0,
        width: 400,
        height: 300,
        fontsize: 14,
        align: "centercenter",
        color: "white",
        onleftmouseup: "toggleTestState()"
    });

    novadesk.log("Test widget created. Click it to toggle properties.");
}

function toggleTestState() {
    if (!demoWidget) return;

    var props = demoWidget.getProperties();
    novadesk.log("Current properties: " + JSON.stringify(props));

    if (props.width === 400) {
        // Change to 'alt' state
        demoWidget.setProperties({
            width: 600,
            height: 400,
            opacity: 0.6,
            backgroundcolor: "rgba(255, 100, 0, 180)", // Orange-ish
            zpos: "ontopmost"
        });
        demoWidget.updateText("statusText", "State: ENLARGED & FADED\nClick again to reset.");
    } else {
        // Reset to original state
        demoWidget.setProperties({
            width: 400,
            height: 300,
            opacity: 1.0,
            backgroundcolor: "rgba(30,30,30,220)",
            zpos: "ondesktop"
        });
        demoWidget.updateText("statusText", "State: ORIGINAL\nClick to test setProperties!");
    }
}

function onAppReady() {
    createContentDemoWindow();
}
