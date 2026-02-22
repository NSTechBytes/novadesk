var widgetWindow = require("widget-window");
var AudioLevelMonitor = require("audio-level-monitor");
var app = require("app");
// Audio Test Widget
var widget = new widgetWindow({
    id: "audioTest",
    width: 600,
    height: 400,
    backgroundColor: "rgba(30, 30, 40, 0.9)",
    script: "ui.js"
});

var audio = new AudioLevelMonitor({
    port: "output",
    fftSize: 1024,
    fftOverlap: 512,
    bands: 20,
    fftAttack: 50,
    fftDecay: 200,
    sensitivity: 60
});

// Update loop
setInterval(function() {
    var data = audio.stats();
    // Send data to UI
    ipc.send("audio-data", data);
}, 33); // ~30 fps

widget.on("close", function() {
    audio.destroy();
});

app.useHardwareAcceleration(true);