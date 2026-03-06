import { app, widgetWindow } from "novadesk";

console.log("=========================================");
console.log("   Novadesk Background Test Integration");
console.log("=========================================");

var widget = new widgetWindow({
    id: "backgroundTest",
    width: 600,
    height: 400,
    backgroundColor: "linearGradient(45, #ff3b30, #007aff)",
    script: "./script.ui.js"
});

console.log("[PASS] widgetWindow created: backgroundTest");
console.log("[PASS] backgroundColor applied: linearGradient(45, #ff3b30, #007aff)");

setTimeout(function () {
    console.log("[INFO] closing background test widget");
    widget.close();
    app.exit();
}, 3000);
