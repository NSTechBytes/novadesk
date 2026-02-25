import { WidgetWindow } from 'novadesk';

ipcMain.on("ui-ready", (_event, payload) => {
  console.log("[main] from ui-ready:", JSON.stringify(payload ?? {}));
});

ipcMain.on("ui-ping", (_event, payload) => {
  console.log("[main] from ui-ping:", JSON.stringify(payload ?? {}));
  ipcMain.send("main-pong", { ts: Date.now(), echo: payload ?? null });
});

function createWindow() {
  const win = new WidgetWindow({
    id:"test",
    width: 400,
    height: 400,
    script: "ui.js",
    backgroundColor: "rgb(10,10,10)"
  });

  ipcMain.send("main-ready", { ts: Date.now(), note: "hello from main" });

  globalThis.sendPingToUi = () => {
    ipcMain.send("main-ping", { ts: Date.now() });
  };
}

createWindow();

console.log("Window created");
