/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

!logToFile;
!enableDebugging;

var testWidget = new widgetWindow({
    id: "testProps",
    width: 200,
    x: 300,
    y: 100,
    height: 100,
    backgroundcolor: "rgba(50, 50, 60, 0.8)",
});

testWidget.addText({
    id: "info",
    text: "Testing Properties...",
    x: 10, y: 10,
    fontsize: 14,
    color: "rgb(255, 255, 255)",
    onleftmouseup: "novadesk.log('Initial Properties: ' + JSON.stringify(testWidget.getProperties()));"
});

// Check initial properties
var props = testWidget.getProperties();
novadesk.log("Initial Properties: " + JSON.stringify(props));

if (props.x === -2147483648 || props.y === -2147483648) {
    novadesk.error("FAIL: Initial properties contain CW_USEDEFAULT!");
} else {
    novadesk.log("SUCCESS: Initial properties are set: x=" + props.x + ", y=" + props.y);
}

// Move the widget
testWidget.setProperties({ x: 100, y: 100 });

// Wait a bit for the message to be processed (though it should be synchronous in our current implementation)
setTimeout(function () {
    var newProps = testWidget.getProperties();
    novadesk.log("Updated Properties: " + JSON.stringify(newProps));

    if (newProps.x === 100 && newProps.y === 100) {
        novadesk.log("SUCCESS: Properties updated correctly after setProperties.");
        testWidget.updateText("info", "Tests Passed!\nx=" + newProps.x + ", y=" + newProps.y);
    } else {
        novadesk.error("FAIL: Properties NOT updated! Expected 100,100 but got " + newProps.x + "," + newProps.y);
    }
}, 2000);
