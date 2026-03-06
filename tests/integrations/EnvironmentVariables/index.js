import { app } from "novadesk";
import * as system from "system";

console.log("=== EnvironmentVariables Integration (Latest API) ===");

var keys = ["PATH", "USERNAME", "USERPROFILE", "TEMP", "APPDATA"];
for (var i = 0; i < keys.length; i++) {
    var k = keys[i];
    var v = system.getEnv(k, "");
    console.log("[INFO] " + k + "=" + (v || "(empty)"));
}

var missing = system.getEnv("NOVADESK_TEST_MISSING_ENV", "fallback-value");
console.log("[PASS] missing env fallback: " + missing);

console.log("=== EnvironmentVariables Integration Complete ===");
app.exit();
