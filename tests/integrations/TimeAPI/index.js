import { app } from "novadesk";
import * as system from "system";

console.log("=== TimeAPI Integration ===");

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

const now = call("time.time()", () => system.time.time());
call("time.time('%Y-%m-%d %H:%M:%S')", () => system.time.time("%Y-%m-%d %H:%M:%S"));
call("time.time('%A, %d %B %Y', 'en_US.UTF-8')", () => system.time.time("%A, %d %B %Y", "en_US.UTF-8"));

const ts = call("time.timeStamp()", () => system.time.timeStamp());

if (typeof ts === "number") {
    call("time.timeStampFormat(ts, '%Y-%m-%d %H:%M:%S')", () => system.time.timeStampFormat(ts, "%Y-%m-%d %H:%M:%S"));
    call("time.formatLocale(ts, '%A %d %B %Y', 'en_US.UTF-8')", () => system.time.formatLocale(ts, "%A %d %B %Y", "en_US.UTF-8"));
}

if (typeof now === "string" && typeof ts === "number") {
    call("time.timeStampLocale('2026-05-01 08:30:00', '%Y-%m-%d %H:%M:%S', 'en_US.UTF-8')", () =>
        system.time.timeStampLocale("2026-05-01 08:30:00", "%Y-%m-%d %H:%M:%S", "en_US.UTF-8")
    );
}

call("time.daylightSavingTime()", () => system.time.daylightSavingTime());

app.exit();