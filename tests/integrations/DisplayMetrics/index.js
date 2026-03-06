import { app } from "novadesk";
import * as system from "system";

console.log("=== DisplayMetrics Integration (Latest API) ===");

var start = Date.now();
var metrics = system.displayMetrics.get();
var elapsed = Date.now() - start;

console.log("[INFO] displayMetrics.get() returned:", JSON.stringify(metrics, null, 2));

function isArea(a) {
    return !!a &&
        typeof a.x === "number" &&
        typeof a.y === "number" &&
        typeof a.width === "number" &&
        typeof a.height === "number";
}

var hasPrimary = !!metrics && !!metrics.primary && isArea(metrics.primary.workArea) && isArea(metrics.primary.screenArea);
var hasVirtual = !!metrics &&
    !!metrics.virtualScreen &&
    isArea(metrics.virtualScreen) &&
    metrics.virtualScreen.width > 0 &&
    metrics.virtualScreen.height > 0;
var hasMonitorsArray = !!metrics && Array.isArray(metrics.monitors);
var monitorsValid = hasMonitorsArray && metrics.monitors.length > 0 && metrics.monitors.every(function (m) {
    return typeof m.id === "number" && isArea(m.workArea) && isArea(m.screenArea);
});

console.log("[PASS] displayMetrics.get() in " + elapsed + "ms");
console.log("[CHECK] primary shape valid: " + hasPrimary);
console.log("[CHECK] virtualScreen shape valid: " + hasVirtual);
console.log("[CHECK] monitors shape valid: " + monitorsValid);
console.log("=== DisplayMetrics Integration Complete ===");
app.exit();
