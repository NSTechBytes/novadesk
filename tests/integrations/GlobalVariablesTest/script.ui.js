console.log("=== GlobalVariablesTest (ui) ===");

function isNonEmptyString(v) {
    return typeof v === "string" && v.length > 0;
}

function isAbsoluteLikePath(v) {
    if (!isNonEmptyString(v)) return false;
    return path.isAbsolute(v) || v.indexOf(":\\") === 1 || v.indexOf("\\\\") === 0;
}

ui.beginUpdate();
ui.addText({
    id: "title",
    x: 14,
    y: 14,
    width: 450,
    height: 24,
    text: "Global variables test",
    fontSize: 15,
    fontColor: "rgb(230,230,230)"
});
ui.addText({
    id: "status",
    x: 14,
    y: 48,
    width: 450,
    height: 120,
    text: "Checking __filename / __dirname / __widgetDir ...",
    fontSize: 13,
    fontColor: "rgb(180,220,255)"
});
ui.endUpdate();

var filenameOk = isAbsoluteLikePath(__filename);
var dirnameOk = isAbsoluteLikePath(__dirname);
var widgetDirOk = isAbsoluteLikePath(__widgetDir);

ui.setElementProperties("status", {
    text:
        "__filename: " + filenameOk +
        " | __dirname: " + dirnameOk +
        " | __widgetDir: " + widgetDirOk
});

ipcRenderer.send("ui-globals-report", {
    filename: __filename,
    dirname: __dirname,
    widgetDir: __widgetDir,
    filenameOk: filenameOk,
    dirnameOk: dirnameOk,
    widgetDirOk: widgetDirOk
});

