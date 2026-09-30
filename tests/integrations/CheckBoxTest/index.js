import { app, widgetWindow } from "novadesk";

console.log("=== CheckBoxTest ===");

const win = new widgetWindow({
    id: "checkBoxTestWindow",
    width: 600,
    height: 650,
    backgroundColor: "#1e1e2e",
    script: "script.ui.js",
});

win.on("close", function () {
    app.exit();
});
