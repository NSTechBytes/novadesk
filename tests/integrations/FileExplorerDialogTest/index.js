import { app, widgetWindow, dialog } from "novadesk";

console.log("=== FileExplorerDialog Integration Test (IPC) ===");

// Handle IPC dialog requests from UI script
ipcMain.handle("dialog:open-image", () => {
  return dialog.showOpenDialog({
    title: "Select an Image File",
    buttonLabel: "Select Image",
    filters: [
      { name: "Image Files", extensions: ["png", "jpg", "jpeg", "webp", "gif"] },
      { name: "All Files", extensions: ["*"] }
    ]
  });
});

ipcMain.handle("dialog:open-multi", () => {
  return dialog.showOpenDialog({
    title: "Select Multiple Files",
    properties: ["multiSelections"]
  });
});

ipcMain.handle("dialog:open-directory", () => {
  const folder = dialog.openDirectory({
    title: "Select a Directory"
  });
  return { folder };
});

ipcMain.handle("dialog:save-file", () => {
  return dialog.showSaveDialog({
    title: "Save Output Document",
    defaultPath: "my_export.json",
    defaultExtension: "json",
    buttonLabel: "Export",
    filters: [
      { name: "JSON Documents", extensions: ["json"] },
      { name: "Text Files", extensions: ["txt"] },
      { name: "All Files", extensions: ["*"] }
    ]
  });
});

ipcMain.handle("dialog:unified-explorer", (event, payload) => {
  return dialog.showFileExplorerDialog(payload || { type: "open" });
});

new widgetWindow({
  id: "FileExplorerDialogTestWindow",
  x: 260,
  y: 180,
  width: 580,
  height: 420,
  backgroundColor: "rgba(20, 24, 38, 0.95)",
  script: "./script.ui.js",
  show: true
}).on("close", function () {
  app.exit();
});
