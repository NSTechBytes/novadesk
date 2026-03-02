import { app, widgetWindow } from "novadesk";
import * as system from "system";

console.log("=== AudioLevelTest (Latest API) ===");

var widget = new widgetWindow({
    id: "audioTest",
    width: 600,
    height: 400,
    backgroundColor: "rgba(30, 30, 40, 0.9)",
    script: "./script.ui.js"
});

console.log("[INFO] Widget created: audioTest (600x400), ui=script.ui.js");

var tick = 0;
var timer = setInterval(function () {
    tick += 1;
    var data = system.audioLevel.stats({
        port: "output",
        fftSize: 1024,
        fftOverlap: 512,
        bands: 20,
        rmsGain: 120,
        fftAttack: 50,
        fftDecay: 200,
        sensitivity: 60
    });

    if (data) {
        ipcMain.send("audio-data", data);
        if (tick === 1 || tick % 30 === 0) {
            var l = (data.rms && data.rms.length > 0) ? data.rms[0] : 0;
            var r = (data.rms && data.rms.length > 1) ? data.rms[1] : 0;
            var b = (data.bands && data.bands.length) ? data.bands.length : 0;
            console.log("[INFO] tick=" + tick + " rmsL=" + l + " rmsR=" + r + " bands=" + b);
        }
    } else if (tick === 1 || tick % 60 === 0) {
        console.log("[WARN] audioLevel.stats returned null at tick=" + tick);
    }
}, 33);

widget.on("close", function () {
    console.log("[INFO] widget close event received");
    if (timer) {
        clearInterval(timer);
        timer = null;
        console.log("[INFO] monitor timer stopped");
    }
});

app.useHardwareAcceleration(true);
console.log("[INFO] Hardware acceleration enabled");
