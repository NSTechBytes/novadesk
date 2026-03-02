import { app, widgetWindow } from "novadesk";

console.log("=== ShapeTest Integration (Latest API) ===");
console.log("AppData Path: " + app.getAppDataPath());

new widgetWindow({
  id: "ShapeTestWindow",
  x: 100,
  y: 100,
  width: 700,
  height: 900,
  backgroundColor: "#ffffff",
  script: "./script.ui.js"
}).on("close", function () { app.exit(); });
