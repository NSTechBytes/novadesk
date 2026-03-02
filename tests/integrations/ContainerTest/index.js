import { app, widgetWindow } from "novadesk";

console.log("=== ContainerTest Integration (Latest API) ===");

var win = new widgetWindow({
    id: "ContainerTestWindow",
    x: 120,
    y: 120,
    width: 520,
    height: 360,
    backgroundColor: "#ffffff",
    script: "./script.ui.js"
});

console.log("[PASS] widgetWindow created: ContainerTestWindow");

win.on("close", function () {
    app.exit();
});
