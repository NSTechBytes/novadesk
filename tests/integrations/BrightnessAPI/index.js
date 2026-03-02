import { app } from "novadesk";
import * as system from "system";

console.log("=========================================");
console.log("   Novadesk Brightness API Integration");
console.log("=========================================");

function call(name, fn) {
    try {
        var out = fn();
        console.log("[PASS] " + name + ": " + JSON.stringify(out));
        return out;
    } catch (err) {
        console.error("[FAIL] " + name + ": " + String(err));
        return null;
    }
}

var info = call("brightness.getValue()", function () {
    return system.brightness.getValue();
});

if (!info || typeof info !== "object") {
    console.error("[FAIL] brightness.getValue() did not return an object");
    app.exit();
} else if (!info.supported) {
    console.log("[SKIP] Brightness control is not supported on this display/system.");
    console.log("[INFO] getValue response: " + JSON.stringify(info));
    app.exit();
} else {
    var currentPercent = typeof info.percent === "number"
        ? info.percent
        : (typeof info.current === "number" && typeof info.min === "number" && typeof info.max === "number" && info.max > info.min
            ? ((info.current - info.min) * 100) / (info.max - info.min)
            : 50);

    var target = currentPercent >= 95 ? Math.max(0, currentPercent - 20) : Math.min(100, currentPercent + 5);
    console.log("[INFO] Brightness test => current=" + currentPercent + "%, target=" + target + "%");

    call("brightness.setValue({ percent: target })", function () {
        return system.brightness.setValue({ percent: target });
    });

    call("brightness.getValue() after set", function () {
        return system.brightness.getValue();
    });

    call("brightness.setValue({ percent: current }) [restore]", function () {
        return system.brightness.setValue({ percent: currentPercent });
    });

    call("brightness.getValue() after restore", function () {
        return system.brightness.getValue();
    });

    console.log("=== Brightness API Integration Complete ===");
    app.exit();
}
