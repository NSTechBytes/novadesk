import { app } from "novadesk";
import * as system from "system";

console.log("=== ExecuteCommand Integration (Latest API) ===");

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

call("execute(notepad.exe)", function () {
    return system.execute("notepad.exe");
});

call("execute(cmd /c echo ...)", function () {
    return system.execute("cmd.exe", "/c echo Novadesk ExecuteCommand", "C:\\", 1);
});

call("execute(url)", function () {
    return system.execute("https://www.google.com");
});

call("execute(hidden)", function () {
    return system.execute("cmd.exe", "/c echo Hidden Window", "", 0);
});

call("execute(minimized)", function () {
    return system.execute("notepad.exe", "", "", 2);
});

call("execute(nonexistent.exe)", function () {
    return system.execute("nonexistent.exe");
});

console.log("=== ExecuteCommand Integration Complete ===");
app.exit();
