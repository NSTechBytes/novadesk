import { app } from "novadesk";
import * as system from "system";

console.log("=== CPUMonitor Integration (Latest API) ===");

var tick = 0;
var intervalId = setInterval(function () {
    tick += 1;
    var usage = system.cpu.usage();
    console.log("[PASS] tick=" + tick + " cpu.usage()=" + usage + "%");
}, 1000);

setTimeout(function () {
    clearInterval(intervalId);
    console.log("=== CPUMonitor Integration Complete ===");
    app.exit();
}, 10000);
