import { app, widgetWindow } from "novadesk";

console.log("=== OnlineFontTest Integration ===");

// Open the UI window that will run all font-download assertions.
const win = new widgetWindow({
  id: "OnlineFontTestWindow",
  x: 200,
  y: 150,
  width: 620,
  height: 480,
  backgroundColor: "rgba(18,22,32,0.97)",
  script: "./script.ui.js",
  show: true
});

console.log("[PASS] widgetWindow created: OnlineFontTestWindow");

// Give the async download up to 15 s to complete, then signal the UI to
// perform its post-download assertions and exit.
setTimeout(() => {
  ipcMain.send("font:test:check");
}, 15000);
