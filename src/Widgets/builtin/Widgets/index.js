import { widgetWindow, app } from 'novadesk';
import { clipboard, wallpaper, power, audio, brightness, fileIcon, displayMetrics, hotkey } from 'system';
import * as std from 'std';

// console.log("OS:", JSON.stringify(std.getenviron()));
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
  const win = new widgetWindow({
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
console.log("Clipboard before:", clipboard.getText());
console.log("Power status:", JSON.stringify(power.getStatus?.() ?? {}));
console.log("Display metrics count:", displayMetrics.getMetrics().count);
console.log("System modules:", !!wallpaper, !!audio, !!brightness, !!fileIcon);
console.log("Path module type:", typeof path);
console.log("__dirname:", __dirname);
console.log("__filename:", __filename);
try {
  const p = path;
  if (!p) {
    console.warn("Path module is not available");
  } else {
    console.log("Path join:", p.join(__dirname, "assets", "icon.png"));
    console.log("Path dirname:", p.dirname(__filename));
    console.log("Path basename:", p.basename(__filename));
    console.log("Path extname:", p.extname(__filename));
    console.log("Path normalize:", p.normalize(__dirname + "/../Widgets/./ui.js"));
    console.log("Path isAbsolute __filename:", p.isAbsolute(__filename));
    console.log("Path relative:", p.relative(__dirname, p.join(__dirname, "ui.js")));
    const parsedPath = p.parse(__filename);
    console.log("Path parse:", JSON.stringify(parsedPath));
    console.log("Path format:", p.format(parsedPath));
  }
} catch (err) {
  console.error("Path diagnostics failed:", String(err));
}

const timeoutId = setTimeout(() => {
  console.log("[timer] setTimeout fired after 1500ms");
}, 1500);
console.log("Timeout id:", timeoutId);

let intervalTick = 0;
const intervalId = setInterval(() => {
  intervalTick += 1;
  console.log(`[timer] setInterval tick ${intervalTick}`);
  if (intervalTick >= 5) {
    clearInterval(intervalId);
    console.log("[timer] interval cleared after 5 ticks");
  }
}, 1000);
console.log("Interval id:", intervalId);

const testHotkeyId = hotkey.register("CTRL+ALT+K", {
  onKeyDown: () => console.log("[hotkey] down CTRL+ALT+K"),
  onKeyUp: () => console.log("[hotkey] up CTRL+ALT+K")
});
console.log("Hotkey registered id:", testHotkeyId);

globalThis.unregisterTestHotkey = () => {
  const ok = hotkey.unregister(testHotkeyId);
  console.log("Hotkey unregistered:", ok);
};

globalThis.stopTimerTests = () => {
  clearTimeout(timeoutId);
  clearInterval(intervalId);
  console.log("[timer] test timers stopped");
};

console.log("Window created");
