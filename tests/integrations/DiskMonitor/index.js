import { app } from "novadesk";
import * as system from "system";

console.log("=== DiskMonitor Integration (Latest API) ===");

function bytesToGB(v) {
    return (v / (1024 * 1024 * 1024)).toFixed(2);
}

var drivePath = "C:\\";
var tick = 0;
var intervalId = setInterval(function () {
    tick += 1;
    var totalBytesValue = system.disk.totalBytes(drivePath);
    var usedBytesValue = system.disk.usedBytes(drivePath);
    var availableBytesValue = system.disk.availableBytes(drivePath);
    var usagePercentValue = system.disk.usagePercent(drivePath);

    console.log(
        "[PASS] tick=" + tick +
        " path=" + drivePath +
        " total=" + bytesToGB(totalBytesValue) + "GB" +
        " used=" + bytesToGB(usedBytesValue) + "GB" +
        " free=" + bytesToGB(availableBytesValue) + "GB" +
        " usage=" + usagePercentValue + "%"
    );
}, 1000);

setTimeout(function () {
    clearInterval(intervalId);
    console.log("=== DiskMonitor Integration Complete ===");
    app.exit();
}, 10000);
