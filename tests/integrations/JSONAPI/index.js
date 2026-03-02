import { app } from "novadesk";
import * as system from "system";

console.log("=== JSONAPI Integration (Latest API) ===");

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

const fileA = __dirname + "\\output_test.json";
const fileB = __dirname + "\\verify.json";

call("json.write(output_test)", () => system.json.write(fileA, { name: "Test Config", version: 1 }, true));
call("json.read(output_test)", () => system.json.read(fileA));
call("json.write(verify)", () => system.json.write(fileB, { test: "verification", number: 42 }, true));
call("json.read(verify)", () => system.json.read(fileB));

const patchText = JSON.stringify({ obj: { b: 20, d: 4 }, arr: [9, 8], newKey: true });
call("json.write(merge_base patch)", () => system.json.write(__dirname + "\\merge_base.json", patchText, false));
call("webFetch(merge_base)", () => system.webFetch(__dirname + "\\merge_base.json"));

console.log("=== JSONAPI Complete ===");
app.exit();
