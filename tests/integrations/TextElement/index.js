import { app, widgetWindow } from "novadesk";

console.log("=== TextElement Integration (Latest API) ===");

new widgetWindow({
  id: "textElementTest",
  width: 1000,
  height: 1200,
  backgroundColor: "rgba(40, 40, 50, 0.95)",
  script: "inlineStyle.ui.js"
}).on("close", function () { app.exit(); });
