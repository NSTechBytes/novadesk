import { app, widgetWindow } from "novadesk";

console.log("=== LayoutBoxBorderTest Integration ===");

const win = new widgetWindow({
  id: "LayoutBoxBorderTestWindow",
  x: 100,
  y: 100,
  width: 800,
  height: 600,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: LayoutBoxBorderTestWindow");

win.on("close", () => app.exit());
