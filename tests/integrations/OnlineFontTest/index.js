import { app, widgetWindow } from "novadesk";

console.log("=== OnlineFontTest Integration ===");

// Keep a reference on globalThis to prevent garbage collection of the window.
var window = new widgetWindow({
  id: "OnlineFontTestWindow",
  x: 200,
  y: 150,
  width: 620,
  height: 480,
  backgroundColor: "rgba(18,22,32,0.97)",
  script: "./script.ui.js",
  show: true,
});

  window.setSettings({

    title: "My Widget",        // optional: overrides "Settings - <id>" in header
    showWindowTab: true,       // optional: show/hide built-in Window tab (default true)

    about: {                   // optional: controls the About tab
      name:        "My Widget",
      version:     "1.2.0",
      description: "Does awesome things",
    },

    tabs: [
      {
        label: "General",       // tab button text
        icon:  "\uE713",        // optional: Segoe MDL2 glyph before label
        settings: [
          { id: "titleColor", type: "color",  label: "Title color",  default: "#a6e3a1",
            bind: { element: "myTitle", property: "fontColor" } },
          { id: "fontSize",   type: "number", label: "Font size",    default: 16,
            bind: { element: "myTitle", property: "fontSize" } },
        ],
      },
      {
        label: "Behaviour",
        settings: [
          { id: "compact", type: "toggle", label: "Compact mode", default: false },
          { id: "style",   type: "select", label: "Style",        default: "rounded",
            options: ["rounded", "sharp", "pill"] },
        ],
      },
    ],

  });

globalThis.window.on("close", function () {
  app.exit();
});

console.log("[PASS] widgetWindow created: OnlineFontTestWindow");

// Give the async downloads up to 10 s to complete, then signal the UI to
// perform its post-download assertions.
setTimeout(() => {
  console.log("[INFO] Triggering font assertions check via IPC...");
  ipcMain.send("font:test:check");
}, 10000);

// Close app 2 s after the check triggers
setTimeout(() => {
  console.log("[INFO] OnlineFontTest exiting...");
  // app.exit();
}, 12000);
