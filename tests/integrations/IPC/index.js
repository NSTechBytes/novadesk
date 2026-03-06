import { widgetWindow } from 'novadesk';

ipcMain.on("ui-ready", (event, payload) => {
  console.log("[main] UI ready:", JSON.stringify(payload));
});

ipcMain.on("ui-ping", (event, payload) => {
  console.log("[main] ping from UI");
  ipcMain.send("main-pong", { ts: Date.now(), echo: payload });
});

ipcMain.handle("get-config", () => {
  return { theme: "dark", version: 1 };
});

const win = new widgetWindow({
  id: "demo",
  width: 400,
  height: 400,
  script: "ui/script.ui.js",
  backgroundColor: "rgb(10,10,10)"
});

ipcMain.send("main-ready", { ts: Date.now() });