import { app, widgetWindow } from "novadesk";

console.log("=== RoundLineElement Integration (Latest API) ===");

const widget = new widgetWindow({
  id: "roundLineTest",
  width: 600,
  height: 400,
  backgroundColor: "rgba(30, 30, 40, 0.9)",
  script: "./script.ui.js"
});

let val = 0.0;
const id = setInterval(function () {
  val += 0.01;
  if (val > 1.0) val = 0;
  ipcMain.send("update-ring", val);
}, 50);

widget.on("close", function () {
  clearInterval(id);
  app.exit();
});
