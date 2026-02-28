ui.beginUpdate();

ui.addText({
  id: "status",
  text: "waiting...",
  x: 16, y: 14,
  width: 260, height: 28,
  fontSize: 16,
  fontColor: "rgb(230,230,230)"
});

ui.endUpdate();

ipcRenderer.on("main-ready", (event, payload) => {
  ui.setElementProperties("status", { text: "main-ready" });
});

ipcRenderer.on("main-pong", (event, payload) => {
  console.log("[ui] pong:", JSON.stringify(payload));
});

const config = ipcRenderer.invoke("get-config");
console.log("[ui] config:", JSON.stringify(config));

ipcRenderer.send("ui-ready", { ts: Date.now() });