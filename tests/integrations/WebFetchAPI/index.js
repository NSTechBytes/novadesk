import { app } from "novadesk";
import * as system from "system";

console.log("=== WebFetchAPI Integration (Latest API) ===");

function fetchAndLog(label, target) {
  try {
    const data = system.webFetch(target);
    console.log("[PASS] " + label + ": " + (data ? ("len=" + data.length) : "null"));
  } catch (e) {
    console.error("[FAIL] " + label + ": " + String(e));
  }
}

fetchAndLog("local merge_base", __dirname + "\\..\\JSONAPI\\merge_base.json");
fetchAndLog("httpbin", "https://httpbin.org/get");
fetchAndLog("invalid", "https://nonexistent.invalid");

app.exit();
