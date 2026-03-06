import { app, widgetWindow } from "novadesk";

console.log("=== GlobalVariablesTest (main) ===");

function isNonEmptyString(v) {
    return typeof v === "string" && v.length > 0;
}

function isAbsoluteLikePath(v) {
    if (!isNonEmptyString(v)) return false;
    return path.isAbsolute(v) || v.indexOf(":\\") === 1 || v.indexOf("\\\\") === 0;
}

var mainFilenameOk = isAbsoluteLikePath(__filename);
var mainDirnameOk = isAbsoluteLikePath(__dirname);
var mainWidgetDirOk = isAbsoluteLikePath(__widgetDir);

console.log("[main] __filename=" + __filename);
console.log("[main] __dirname=" + __dirname);
console.log("[main] __widgetDir=" + __widgetDir);
console.log("[main] checks: filename=" + mainFilenameOk + ", dirname=" + mainDirnameOk + ", widgetDir=" + mainWidgetDirOk);

ipcMain.on("ui-globals-report", function (event, payload) {
    console.log("[ui] __filename=" + payload.filename);
    console.log("[ui] __dirname=" + payload.dirname);
    console.log("[ui] __widgetDir=" + payload.widgetDir);
    console.log("[ui] checks: filename=" + payload.filenameOk + ", dirname=" + payload.dirnameOk + ", widgetDir=" + payload.widgetDirOk);
});

var win = new widgetWindow({
    id: "GlobalVariablesTestWindow",
    x: 260,
    y: 220,
    width: 480,
    height: 220,
    backgroundColor: "rgba(20,20,20,0.92)",
    script: "./script.ui.js",
    show: true
});

setTimeout(function () {
    win.close();
    app.exit();
}, 2500);

