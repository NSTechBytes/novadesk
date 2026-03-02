import { app } from "novadesk";
import * as system from "system";

console.log("=== MemoryMonitor Integration (Latest API) ===");

let tick = 0;
const id = setInterval(function () {
  tick += 1;
  const total = system.memory.totalBytes();
  const used = system.memory.usedBytes();
  const available = system.memory.availableBytes();
  const percent = system.memory.usagePercent();
  console.log("[PASS] t=" + tick + " total=" + total + " used=" + used + " available=" + available + " percent=" + percent);
}, 1000);

setTimeout(function () {
  clearInterval(id);
  app.exit();
}, 10000);
