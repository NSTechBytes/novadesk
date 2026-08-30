import { app, widgetWindow } from "novadesk";

console.log("=== ContainerNativeScrollTest Integration ===");

const win = new widgetWindow({
  id: "ContainerNativeScrollTestWindow",
  x: 100,
  y: 100,
  width: 600,
  height: 480,
  backgroundColor: "rgba(18, 22, 34, 0.96)",
  script: "./script.ui.js",
  show: true
});

win.on("close", () => {
  app.exit();
});

// setTimeout(() => {
//   win.close();
// }, 2000);

