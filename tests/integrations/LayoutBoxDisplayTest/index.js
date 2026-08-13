import { app, widgetWindow } from "novadesk";

console.log("=== LayoutApiTest Integration ===");

const win = new widgetWindow({
  id: "LayoutApiTestWindow(Display)",
  x: 180,
  y: 140,
  width: 420,
  height: 400,
  backgroundColor: "black",
  script: "./image.ui.js",
  show: true
});

