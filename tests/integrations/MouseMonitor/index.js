import { app } from "novadesk";
import * as system from "system";

console.log("=== MouseMonitor Integration (Latest API) ===");

const id = setInterval(function () {
  console.log("Mouse Position: X=" + system.mouse.clientX() + ", Y=" + system.mouse.clientY());
}, 100);

setTimeout(function () {
  clearInterval(id);
  app.exit();
}, 5000);
