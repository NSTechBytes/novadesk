import { app, widgetWindow } from "novadesk";

console.log("=== DropdownTest ===");

const win = new widgetWindow({
    id: "dropdownTestWindow",
    width: 600,
    height: 650,
    backgroundColor: "#1e1e2e",
    script: "script.ui.js",
});

win.on("close", function () {
    app.exit();
});
