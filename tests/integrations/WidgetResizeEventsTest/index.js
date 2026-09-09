import { app, widgetWindow } from "novadesk";

console.log("=== WidgetResizeEventsTest Integration ===");

const win = new widgetWindow({
  id: "WidgetResizeEventsTestWindow",
  x: 200,
  y: 200,
  width: 440,
  height: 240,
  backgroundColor: "rgba(25, 30, 48, 0.95)",
  script: "./script.ui.js",
  show: true
});

win.setResizable(true);

let resizeStartCount = 0;
let resizeCount = 0;
let resizeEndCount = 0;

win.on("resizeStart", (e) => {
  resizeStartCount++;
  console.log("[EVENT] resizeStart fired!", "Count:", resizeStartCount, "Client:", e?.clientX, e?.clientY);
  console.log("[STATUS] win.isResizing():", win.isResizing());
});

win.on("resize", (e) => {
  resizeCount++;
  const size = win.getSize();
  console.log("[EVENT] resize fired!", "Count:", resizeCount, "New size:", size?.width, "x", size?.height);
});

win.on("resizeEnd", (e) => {
  resizeEndCount++;
  console.log("[EVENT] resizeEnd fired!", "Count:", resizeEndCount, "Client:", e?.clientX, e?.clientY);
  console.log("[STATUS] win.isResizing():", win.isResizing());
});

win.on("close", () => {
  console.log("[EVENT] close");
  app.exit();
});

console.log("Initial win.isResizable():", win.isResizable());
console.log("Initial win.isResizing():", win.isResizing());
