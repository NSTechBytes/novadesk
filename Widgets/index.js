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

// Create monitors once and keep them alive
var cpu = new novadesk.system.CPU();
var mem = new novadesk.system.Memory();
var net = new novadesk.system.Network();
var mouse = new novadesk.system.Mouse();

function testMonitors() {
    // Use existing monitors (don't recreate each time)
    var usage = cpu.usage();
    novadesk.log("CPU Usage: " + usage + "%");

    var stats = mem.stats();
    novadesk.log("Memory Used: " + stats.used + " / " + stats.total + " (" + stats.percent + "%)");

    var netStats = net.stats();
    novadesk.log("Network In: " + (netStats.netIn / 1024).toFixed(2) + " KB/s, Out: " + (netStats.netOut / 1024).toFixed(2) + " KB/s");

    var mousePos = mouse.position();
    novadesk.log("Mouse Position: X=" + mousePos.x + ", Y=" + mousePos.y);
}

novadesk.onReady(function () {
    createClockWidget();

    // Call every second - monitors stay alive and track changes
    setInterval(function () {
        testMonitors();
    }, 1000);
});