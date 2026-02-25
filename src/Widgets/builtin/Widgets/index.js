import { WidgetWindow } from 'novadesk';

function createWindow() {
  const win = new WidgetWindow({
    id:"test",
    width: 400,
    height: 400,
    script: "ui.js",
    backgroundColor: "rgb(10,10,10)"
  });
}

createWindow();

console.log("Window created");
