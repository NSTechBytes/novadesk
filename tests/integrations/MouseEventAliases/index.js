import { app, widgetWindow } from "novadesk";

console.log("=== MouseEventAliases Integration (Latest API) ===");
new widgetWindow({
  id: "MouseEventAliasesWindow",
  x: 260,
  y: 220,
  width: 360,
  height: 180,
  backgroundColor: "rgba(24,24,24,0.9)",
  script: "./script.ui.js",
  show: true
}).on("close", function () { app.exit(); });
