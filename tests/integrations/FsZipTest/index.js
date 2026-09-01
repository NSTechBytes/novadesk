import { app, widgetWindow } from "novadesk";
import * as fs from "fs";

console.log("=== FS Zip Methods Integration Test ===");

// 1. Setup sample test files — use __dirname so files land next to the test script
const testDir = __dirname + "/tmp_test_dir";
const testArchive = __dirname + "/tmp_test_archive.zip";
const extractedDir = __dirname + "/tmp_extracted_dir";
const singleZip = __dirname + "/tmp_single.zip";

fs.mkdir(testDir + "/nested/deep", true);
fs.writeFile(testDir + "/sample.txt", "Hello Novadesk Zip System!\nLine 2 of test data.");
fs.writeFile(testDir + "/nested/data.json", JSON.stringify({ name: "Novadesk", version: "2.0", ok: true }, null, 2));
fs.writeFile(testDir + "/nested/deep/note.txt", "Deeply nested file test content.");

let results = [];

// 2. Test fs.zip (directory compression)
try {
  const zipOk = fs.zip(testDir, testArchive, { compressionLevel: 9, overwrite: true });
  console.log("[Test 1] fs.zip (Directory):", zipOk);
  results.push({ name: "fs.zip (Directory)", passed: zipOk === true });
} catch (e) {
  console.error("[Test 1] Error:", e);
  results.push({ name: "fs.zip (Directory)", passed: false, error: String(e) });
}

// 3. Test fs.listZip (inspect zip entries)
try {
  const entries = fs.listZip(testArchive);
  console.log("[Test 2] fs.listZip entries count:", entries ? entries.length : 0);
  console.log("Entries:", JSON.stringify(entries, null, 2));
  const hasSample = entries && entries.some(e => e.name.includes("sample.txt"));
  const hasNested = entries && entries.some(e => e.name.includes("data.json"));
  results.push({ name: "fs.listZip (Inspection)", passed: !!(entries && hasSample && hasNested), count: entries ? entries.length : 0 });
} catch (e) {
  console.error("[Test 2] Error:", e);
  results.push({ name: "fs.listZip (Inspection)", passed: false, error: String(e) });
}

// 4. Test fs.readZipFile (read single file directly from archive)
try {
  const content = fs.readZipFile(testArchive, "sample.txt");
  console.log("[Test 3] fs.readZipFile content:", content);
  const contentOk = content && content.includes("Hello Novadesk Zip System!");
  results.push({ name: "fs.readZipFile (Direct Read)", passed: !!contentOk });
} catch (e) {
  console.error("[Test 3] Error:", e);
  results.push({ name: "fs.readZipFile (Direct Read)", passed: false, error: String(e) });
}

// 5. Test fs.unzip (extract archive)
try {
  const unzipOk = fs.unzip(testArchive, extractedDir, { overwrite: true });
  console.log("[Test 4] fs.unzip:", unzipOk);
  const extractedSample = fs.readFile(extractedDir + "/sample.txt");
  const extractedData = fs.readFile(extractedDir + "/nested/data.json");
  const matches = (extractedSample && extractedSample.includes("Hello Novadesk")) && (extractedData && extractedData.includes("Novadesk"));
  results.push({ name: "fs.unzip (Extraction)", passed: !!(unzipOk && matches) });
} catch (e) {
  console.error("[Test 4] Error:", e);
  results.push({ name: "fs.unzip (Extraction)", passed: false, error: String(e) });
}

// 6. Test fs.zip on a single file
try {
  const singleZipOk = fs.zip(testDir + "/sample.txt", singleZip, { overwrite: true });
  const singleEntries = fs.listZip(singleZip);
  console.log("[Test 5] Single file zip:", singleZipOk, singleEntries);
  results.push({ name: "fs.zip (Single File)", passed: !!(singleZipOk && singleEntries && singleEntries.length >= 1) });
} catch (e) {
  console.error("[Test 5] Error:", e);
  results.push({ name: "fs.zip (Single File)", passed: false, error: String(e) });
}

// Pass results to UI
ipcMain.handle("get-results", () => {
  return results;
});

new widgetWindow({
  id: "FsZipTestWindow",
  x: 260,
  y: 180,
  width: 580,
  height: 400,
  backgroundColor: "rgba(18, 22, 34, 0.95)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
