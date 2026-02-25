import { WidgetWindow, app } from 'novadesk';
import * as std from 'std';

console.log("OS:", JSON.stringify(std.getenviron()));
// let dir = os.opendir('.');
// if (dir) {
//   for(;;) {
//     let entry = os.readdir(dir);
//     if (!entry) break;
//     std.out.puts(entry.name + "\n");
//   }
//   os.closedir(dir);
// } else {
//   std.out.puts("Cannot open directory\n");
// }


ipcMain.on("ui-ready", (_event, payload) => {
  console.log("[main] from ui-ready:", JSON.stringify(payload ?? {}));
});

ipcMain.on("ui-ping", (_event, payload) => {
  console.log("[main] from ui-ping:", JSON.stringify(payload ?? {}));
  ipcMain.send("main-pong", { ts: Date.now(), echo: payload ?? null });
});

function createWindow() {
  const win = new WidgetWindow({
    id: "test",
    width: 400,
    height: 400,
    script: "ui.js",
    backgroundColor: "rgb(10,10,10)"
  });

  ipcMain.send("main-ready", { ts: Date.now(), note: "hello from main" });
  ipcMain.send("main-ping", { ts: Date.now(), note: "startup ping" });

  // win.on("show", () => console.log("[main] window show"));
  // win.on("hide", () => console.log("[main] window hide"));
  // win.on("focus", () => console.log("[main] window focus"));
  // win.on("unFocus", () => console.log("[main] window unFocus"));
  // win.on("move", () => console.log("[main] window move"));
  // win.on("refresh", () => console.log("[main] window refresh"));
  // win.on("mouseOver", (e) => console.log("[main] window mouseOver", e.clientX, e.clientY));
  // win.on("mouseMove", (e) => console.log("[main] window mouseMove", e.clientX, e.clientY));
  // win.on("mouseDown", (e) => console.log("[main] window mouseDown", e.clientX, e.clientY));
  // win.on("mouseUp", (e) => console.log("[main] window mouseUp", e.clientX, e.clientY));
  // win.on("mouseLeave", () => console.log("[main] window mouseLeave"));
  // win.on("close", () => console.log("[main] window close"));
  // win.on("closed", () => console.log("[main] window closed"));

  globalThis.sendPingToUi = () => {
    ipcMain.send("main-ping", { ts: Date.now() });
  };

  win.disableContextMenu(false);
  win.showDefaultContextMenuItems(true);
  win.setContextMenu([
    { text: "Refresh", action: () => win.refresh() },
    { type: "separator" },
    {
      text: "Tools",
      items: [
        { text: "Ping", checked: false, action: () => console.log("ping") }
      ]
    }
  ]);
}

createWindow();

app.setTrayMenu([
  { text: "Reload", action: () => app.reload() },
  { type: "separator" },
  { text: "Toggle Tray Icon", action: () => app.hideTrayIcon(false) },
  { text: "Exit", action: () => app.exit() }
]);
app.showDefaultTrayItems(true);

console.log("Novadesk version:", app.getNovadeskVersion());
console.log("Window created");
