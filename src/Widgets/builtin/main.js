import { WidgetWindow } from 'novadesk';

function createWindow() {
  const win = new WidgetWindow({
    width: 800,
    height: 600
  });
  return win;
}

createWindow();
