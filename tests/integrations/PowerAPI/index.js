import { app } from "novadesk";
import * as system from "system";

console.log("=== PowerAPI Integration (Latest API) ===");

const start = Date.now();
const powerStatus = system.power.getStatus();
console.log("[PASS] power.getStatus(): " + JSON.stringify(powerStatus));
console.log("[INFO] execution ms: " + (Date.now() - start));
app.exit();
