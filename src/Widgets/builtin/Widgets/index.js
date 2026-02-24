import { WidgetWindow } from 'novadesk';

function createWindow() {
  const win = new WidgetWindow({
    id:"test",
    width: 400,
    height: 400
  });
}

createWindow();

console.log("Window created");
