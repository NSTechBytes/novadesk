import { app, widgetWindow } from "novadesk";

console.log("=== LayoutBoxBorderTest Integration ===");

// const win = new widgetWindow({
//   id: "LayoutBoxBorderTestWindow",
//   x: 100,
//   y: 100,
//   width: 800,
//   height: 600,
//   backgroundColor: "rgba(240, 240, 240, 1.0)",
//   script: "./script.ui.js",
//   show: true
// });

const solidTest = new widgetWindow({
  id: "solidTest",
  width: 800,
  height: 600,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./solid.ui.js",
  show: true
});

const insetTest = new widgetWindow({
  id: "insetTest",
  width: 800,
  height: 600,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./inset.ui.js",
  show: true
});

const outsetTest = new widgetWindow({
  id: "outsetTest",
  width: 800,
  height: 600,
  backgroundColor: "rgba(240, 240, 240, 1.0)",
  script: "./outset.ui.js",
  show: true
});
