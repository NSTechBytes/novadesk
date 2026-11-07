import { app, widgetWindow } from "novadesk";

console.log("=== ComboBoxTest ===");

const win = new widgetWindow({
    id: "comboBoxTestWindow",
    width: 650,
    height: 700,
    backgroundColor: "#1e1e2e",
    script: "script.ui.js",
});

win.on("close", function () {
    app.exit();
});
