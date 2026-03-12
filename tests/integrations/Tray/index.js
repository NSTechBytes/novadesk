import { tray, app } from "novadesk";

const tray = new tray();
tray.setToolTip("Novadesk Tray Test");
tray.setImage("../assets/icon.ico")
function logEvent(name, event) {
    if (event) {
        console.log("[TrayTest] " + name + " " + JSON.stringify(event));
        return;
    }
    console.log("[TrayTest] " + name);
}

tray.on("click", (event) => logEvent("click", event));
tray.on("right-click", (event) => logEvent("right-click", event));
tray.setContextMenu([
    {
        text: "Exit",
        action: function () {
            tray.destroy();
            app.exit();
        }
    }
]);
