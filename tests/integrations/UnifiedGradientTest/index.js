import { app, widgetWindow } from "novadesk";

console.log("=== UnifiedGradientTest Integration (Latest API) ===");
new widgetWindow({
  id: "gradientTest",
  width: 600,
  height: 400,
  backgroundColor: "linearGradient(45, #121212, #242424)",
  script: "./script.ui.js"
}).on("close", function () { app.exit(); });
