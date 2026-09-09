import { app, widgetWindow } from "novadesk";

console.log("=== Element Drag and Drop Area Test ===");

new widgetWindow({
  id: "ElementDragAndDropWindow",
  x: 260,
  y: 180,
  width: 560,
  height: 380,
  draggable: false, // Window draggable is false so titlebar dragArea is tested
  backgroundColor: "rgba(18, 22, 36, 0.95)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
