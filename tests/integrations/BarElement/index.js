import { app, widgetWindow } from "novadesk";

console.log("=== BarElement Integration (Latest API) ===");

var widget = new widgetWindow({
    id: "barElementTest",
    width: 1000,
    height: 800,
    backgroundColor: "rgba(40, 40, 50, 0.95)",
    script: "./script.ui.js"
});

var widget2 = new widgetWindow({
    id: "barElementTest",
    width: 1000,
    height: 800,
    backgroundColor: "rgba(40, 40, 50, 0.95)",
    script: "./hittest.ui.js"
});

console.log("[PASS] widgetWindow created: barElementTest");

widget.on("close", function () {
    app.exit();
});
