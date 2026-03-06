import { app, widgetWindow } from "novadesk";

console.log("=== GroupTest Integration (Latest API) ===");
new widgetWindow({
  id: "GroupTestWindow",
  x: 160,
  y: 160,
  width: 520,
  height: 260,
  backgroundColor: "rgba(20,20,20,0.9)",
  script: "./script.ui.js",
  show: true
}).on("close", function () { app.exit(); });
