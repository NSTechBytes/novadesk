// Background card / title
ui.addShape({
  id: "headerBg",
  type: "rectangle",
  x: 0,
  y: 0,
  width: 580,
  height: 45,
  fillColor: "rgba(30, 38, 60, 0.95)"
});

ui.addText({
  id: "titleText",
  x: 20,
  y: 12,
  width: 540,
  height: 24,
  text: "File Explorer Dialogs Test",
  fontSize: 16,
  fontColor: "#ffffff"
});

// Helper to create clickable UI buttons
function createButton(id, x, y, width, height, text, bg, onClick) {
  ui.addShape({
    id: id + "_bg",
    type: "rectangle",
    x: x,
    y: y,
    width: width,
    height: height,
    radius: 6,
    fillColor: bg,
    strokeColor: "rgba(255, 255, 255, 0.15)",
    strokeWidth: 1,
    mouseEventCursor: "hand",
    onMouseOver: function () {
      ui.setElementProperties(id + "_bg", { strokeColor: "rgba(255, 255, 255, 0.5)" });
    },
    onMouseLeave: function () {
      ui.setElementProperties(id + "_bg", { strokeColor: "rgba(255, 255, 255, 0.15)" });
    },
    onLeftMouseUp: onClick
  });

  ui.addText({
    id: id + "_txt",
    x: x + 10,
    y: y + 10,
    width: width - 20,
    height: height - 15,
    text: text,
    fontSize: 13,
    fontColor: "#ffffff",
    mouseEventCursor: "hand",
    onLeftMouseUp: onClick
  });
}

// 1. Open Single File (Images)
createButton("btnOpenFile", 30, 65, 240, 40, "1. Open Image File", "rgba(45, 95, 180, 0.9)", function () {
  console.log("[UI] Requesting image file dialog via IPC...");
  const res = ipcRenderer.invoke("dialog:open-image");
  console.log("[UI] Received result:", JSON.stringify(res));
  ui.setElementProperties("resultText", {
    text: (!res || res.canceled || !res.filePaths || res.filePaths.length === 0)
      ? "Status: Canceled by user"
      : "Selected file:\n" + res.filePaths[0]
  });
});

// 2. Open Multiple Files
createButton("btnOpenMulti", 290, 65, 240, 40, "2. Open Multiple Files", "rgba(60, 110, 195, 0.9)", function () {
  console.log("[UI] Requesting multi-file dialog via IPC...");
  const res = ipcRenderer.invoke("dialog:open-multi");
  console.log("[UI] Received result:", JSON.stringify(res));
  ui.setElementProperties("resultText", {
    text: (!res || res.canceled || !res.filePaths || res.filePaths.length === 0)
      ? "Status: Canceled by user"
      : "Selected " + res.filePaths.length + " files:\n" + res.filePaths.join("\n")
  });
});

// 3. Choose Folder (Directory)
createButton("btnOpenDir", 30, 120, 240, 40, "3. Choose Folder", "rgba(45, 140, 100, 0.9)", function () {
  console.log("[UI] Requesting folder dialog via IPC...");
  const res = ipcRenderer.invoke("dialog:open-directory");
  console.log("[UI] Received result:", JSON.stringify(res));
  ui.setElementProperties("resultText", {
    text: (res && res.folder)
      ? "Chosen directory:\n" + res.folder
      : "Status: Folder selection canceled"
  });
});

// 4. Save File Dialog
createButton("btnSaveFile", 290, 120, 240, 40, "4. Save File Dialog", "rgba(160, 80, 45, 0.9)", function () {
  console.log("[UI] Requesting save dialog via IPC...");
  const res = ipcRenderer.invoke("dialog:save-file");
  console.log("[UI] Received save result:", JSON.stringify(res));
  ui.setElementProperties("resultText", {
    text: (!res || res.canceled || !res.filePath)
      ? "Status: Save canceled"
      : "Save destination:\n" + res.filePath
  });
});

// 5. Unified showFileExplorerDialog helper
createButton("btnExplorerHelper", 30, 175, 500, 38, "5. showFileExplorerDialog (Unified Helper)", "rgba(100, 60, 160, 0.9)", function () {
  console.log("[UI] Requesting unified dialog via IPC...");
  const res = ipcRenderer.invoke("dialog:unified-explorer", {
    type: "open",
    title: "Unified Explorer Dialog"
  });
  console.log("[UI] Received unified result:", JSON.stringify(res));
  ui.setElementProperties("resultText", {
    text: (!res || res.canceled)
      ? "Status: Canceled"
      : "Selected:\n" + (res.filePath || (res.filePaths && res.filePaths[0]) || "")
  });
});

// Status / Output box
ui.addShape({
  id: "outputBox",
  type: "rectangle",
  x: 30,
  y: 230,
  width: 520,
  height: 160,
  radius: 8,
  fillColor: "rgba(12, 16, 26, 0.9)",
  strokeColor: "rgba(255, 255, 255, 0.12)",
  strokeWidth: 1
});

ui.addText({
  id: "resultText",
  x: 45,
  y: 245,
  width: 490,
  height: 130,
  text: "Click any button above to test File Explorer dialogs via IPC.",
  fontSize: 13,
  fontColor: "#a0d0ff"
});
