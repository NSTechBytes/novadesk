import { app } from "novadesk";
import * as system from "system";

console.log("=== IconExtractAPI Integration (Latest API) ===");

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

call("fileIcon.extractFileIcon(notepad)", () =>
  system.fileIcon.extractFileIcon("C:\\Windows\\System32\\notepad.exe", __dirname + "\\notepad_48.ico", 48)
);

call("fileIcon.extractFileIcon(calc)", () =>
  system.fileIcon.extractFileIcon("C:\\Windows\\System32\\calc.exe", __dirname + "\\calc_48.ico", 48)
);

call("fileIcon.extractFileIcon(invalid)", () =>
  system.fileIcon.extractFileIcon("C:\\this\\path\\does-not-exist.exe", __dirname + "\\missing.ico", 48)
);

console.log("=== IconExtractAPI Complete ===");
app.exit();
