import { app } from "novadesk";
import * as system from "system";

console.log("=== RecycleBinAPI Integration ===");

function call(name, fn) {
    try {
        const out = fn();
        console.log("[PASS] " + name + ": " + JSON.stringify(out));
        return out;
    } catch (err) {
        console.error("[FAIL] " + name + ": " + String(err));
        return null;
    }
}

call("recycleBin.getStats()", () => system.recycleBin.getStats());

if (typeof system.recycleBin.openBin === "function") {
    call("recycleBin.openBin()", () => system.recycleBin.openBin());
}

if (typeof system.recycleBin.emptyBin === "function") {
    console.log("[INFO] emptyBin() available (not executed in integration test).");
}

if (typeof system.recycleBin.emptyBinSilent === "function") {
    console.log("[INFO] emptyBinSilent() available (not executed in integration test).");
}

app.exit();
