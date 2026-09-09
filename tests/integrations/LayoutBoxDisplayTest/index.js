import { app, widgetWindow } from "novadesk";

console.log("=== LayoutApiTest Integration ===");

const win = new widgetWindow({
  id: "LayoutApiTestWindow(Display)",
  x: 180,
  y: 140,
  width: 100,
  height: 100,
  backgroundColor: "black",
  script: "./image.ui.js",
  show: true,
  backgroundImage: "https://images.unsplash.com/photo-1511497584788-876760111969",
   backgroundImageFallback: "C:\\Users\\nasirshahbaz\\OneDrive\\Desktop\\Screenshot 2026-05-14 181227.png",
  backgroundSize: "contain",
  backgroundPosition: "left"
});

