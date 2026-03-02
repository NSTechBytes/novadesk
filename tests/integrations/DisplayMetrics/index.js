import { app } from "novadesk";
import * as system from "system";

console.log("=== DisplayMetrics Integration (Latest API) ===");

var start = Date.now();
var metrics = system.displayMetrics.get();
var elapsed = Date.now() - start;

if (!metrics) {
    console.error("[FAIL] displayMetrics.get() returned null");
    app.exit();
} else {
    console.log("[PASS] displayMetrics.get() in " + elapsed + "ms");
    console.log("[INFO] virtual=" + metrics.virtualWidth + "x" + metrics.virtualHeight + " at (" + metrics.virtualLeft + "," + metrics.virtualTop + ")");
    console.log("[INFO] monitorCount=" + metrics.count + ", primaryIndex=" + metrics.primaryIndex);

    var monitors = metrics.monitors || [];
    for (var i = 0; i < monitors.length; i++) {
        var m = monitors[i];
        var s = m.screen || {};
        console.log(
            "[MONITOR #" + i + "] active=" + m.active +
            " deviceName=" + m.deviceName +
            " monitorName=" + m.monitorName +
            " screen=(" + s.left + "," + s.top + ")-(" + s.right + "," + s.bottom + ")"
        );
    }
}

console.log("=== DisplayMetrics Integration Complete ===");
app.exit();
