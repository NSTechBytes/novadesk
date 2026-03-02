import { app, widgetWindow } from "novadesk";

console.log("=== CursorTest Integration (Latest API) ===");

var win = new widgetWindow({
    id: "CursorTestWindow",
    x: 180,
    y: 180,
    width: 420,
    height: 180,
    backgroundColor: "rgba(30,30,30,0.85)",
    script: "./script.ui.js",
    show: true
});

win.on("close", function () {
    app.exit();
});
