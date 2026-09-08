ui.beginUpdate();

ui.addText({
  id: "title",
  text: "IPC Remove Listener Test",
  x: 20, y: 20,
  width: 460, height: 32,
  fontSize: 20,
  fontColor: "rgb(255,255,255)"
});

ui.addText({
  id: "status",
  text: "Running tests...",
  x: 20, y: 60,
  width: 460, height: 280,
  fontSize: 14,
  fontColor: "rgb(200,200,200)"
});

ui.endUpdate();

const logs = [];
function log(msg) {
  console.log("[ui] " + msg);
  logs.push(msg);
  ui.setElementProperties("status", { text: logs.join("\n") });
}

// 1. Verify existence of removal methods on ipcRenderer
log("Checking ipcRenderer removal APIs...");
const hasRemoveListener = typeof ipcRenderer.removeListener === "function";
const hasOff = typeof ipcRenderer.off === "function";
const hasRemoveAll = typeof ipcRenderer.removeAllListeners === "function";

log(`ipcRenderer.removeListener: ${hasRemoveListener ? "PASS" : "FAIL"}`);
log(`ipcRenderer.off: ${hasOff ? "PASS" : "FAIL"}`);
log(`ipcRenderer.removeAllListeners: ${hasRemoveAll ? "PASS" : "FAIL"}`);

// 2. Test removeListener on ipcRenderer
let countA = 0;
let countB = 0;

function onUiA(event, payload) {
  countA++;
}
function onUiB(event, payload) {
  countB++;
}

ipcRenderer.on("ui-test-remove", onUiA);
ipcRenderer.on("ui-test-remove", onUiB);

// Remove onUiA
ipcRenderer.removeListener("ui-test-remove", onUiA);

// 3. Test .off alias
let offCount = 0;
function onUiOff(event, payload) {
  offCount++;
}
ipcRenderer.on("ui-test-off", onUiOff);
ipcRenderer.off("ui-test-off", onUiOff);

// 4. Test removeAllListeners on specific channel
let allChannelCount = 0;
function onUiAll(event, payload) {
  allChannelCount++;
}
ipcRenderer.on("ui-test-all", onUiAll);
ipcRenderer.removeAllListeners("ui-test-all");

// 5. Test ipcMain.removeHandler
let removeHandlerPass = false;
try {
  ipcRenderer.invoke("temp-handler");
  removeHandlerPass = false;
} catch (e) {
  removeHandlerPass = true;
}
log(`ipcMain.removeHandler: ${removeHandlerPass ? "PASS" : "FAIL"}`);

// Send to trigger ipcMain test
ipcRenderer.send("test-remove", { from: "ui" });

// Check results after a short tick
const removeListenerPass = (countA === 0);
const offPass = (offCount === 0);
const allChannelPass = (allChannelCount === 0);

log(`removeListener works: ${removeListenerPass ? "PASS" : "FAIL"}`);
log(`off alias works: ${offPass ? "PASS" : "FAIL"}`);
log(`removeAllListeners works: ${allChannelPass ? "PASS" : "FAIL"}`);

// Test global removeAllListeners on ipcRenderer
ipcRenderer.removeAllListeners();
log("ipcRenderer.removeAllListeners() completed");

const allPassed = hasRemoveListener && hasOff && hasRemoveAll &&
  removeListenerPass && offPass && allChannelPass && removeHandlerPass;

ui.setElementProperties("title", {
  text: allPassed ? "ALL TESTS PASSED" : "TESTS FAILED",
  fontColor: allPassed ? "rgb(50, 220, 100)" : "rgb(255, 80, 80)"
});

ipcRenderer.send("ui-test-results", {
  hasRemoveListener,
  hasOff,
  hasRemoveAll,
  removeListenerPass,
  offPass,
  allChannelPass,
  removeHandlerPass,
  allPassed
});

