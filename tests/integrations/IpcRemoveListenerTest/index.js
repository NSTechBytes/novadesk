import { widgetWindow } from 'novadesk';

console.log("=== IpcRemoveListenerTest (Main) ===");

// Verify methods exist on ipcMain
console.log("typeof ipcMain.removeListener:", typeof ipcMain.removeListener);
console.log("typeof ipcMain.off:", typeof ipcMain.off);
console.log("typeof ipcMain.removeAllListeners:", typeof ipcMain.removeAllListeners);
console.log("typeof ipcMain.removeHandler:", typeof ipcMain.removeHandler);

let mainCountA = 0;
let mainCountB = 0;

function onMainA(event, payload) {
  mainCountA++;
  console.log("[main] onMainA called:", payload);
}

function onMainB(event, payload) {
  mainCountB++;
  console.log("[main] onMainB called:", payload);
}

// 1. Register two listeners on "test-remove"
ipcMain.on("test-remove", onMainA);
ipcMain.on("test-remove", onMainB);

// 2. Remove onMainA via removeListener
ipcMain.removeListener("test-remove", onMainA);

// 3. Test removeHandler
ipcMain.handle("temp-handler", () => {
  return "handler active";
});
ipcMain.removeHandler("temp-handler");

// 4. Listen for UI test results
ipcMain.on("ui-test-results", (event, results) => {
  console.log("[main] UI test results:", JSON.stringify(results));
  const mainPass = (mainCountA === 0 && mainCountB === 1);
  console.log("[main] Main removeListener result:", mainPass ? "PASS" : "FAIL", `(A=${mainCountA}, B=${mainCountB})`);
  
  // Test removeAllListeners
  ipcMain.removeAllListeners("test-remove");
  ipcMain.removeAllListeners();
  console.log("[main] removeAllListeners tested successfully");
});

const win = new widgetWindow({
  id: "ipc-remove-test",
  width: 500,
  height: 380,
  script: "script.ui.js",
  backgroundColor: "rgb(25, 25, 30)"
});
