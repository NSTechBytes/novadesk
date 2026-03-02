var bandCount = 20;
var barWidth = 10;
var barGap = 2;
var totalBarWidth = barWidth + barGap;
var centerX = 300;
var centerY = 200;
var maxBarHeight = 150;

var barsLU = [];
var barsRU = [];

for (var i = 0; i < bandCount; i++) {
    var xOffset = barGap + i * totalBarWidth;
    var xRight = centerX + xOffset;
    var xLeft = centerX - xOffset - barWidth;

    ui.addBar({
        id: "ru_" + i,
        x: xRight,
        y: centerY,
        width: barWidth,
        height: 1,
        barColor: "rgba(0, 255, 255, 0.8)",
        backgroundColor: "transparent",
        value: 1
    });
    barsRU.push("ru_" + i);

    ui.addBar({
        id: "lu_" + i,
        x: xLeft,
        y: centerY,
        width: barWidth,
        height: 1,
        barColor: "rgba(0, 255, 255, 0.8)",
        backgroundColor: "transparent",
        value: 1
    });
    barsLU.push("lu_" + i);
}

ui.addText({
    id: "rms_l",
    x: 20,
    y: 20,
    text: "L: 0.0%",
    fontColor: "rgba(0, 255, 255, 0.8)",
    fontSize: 14
});

ui.addText({
    id: "rms_r",
    x: 520,
    y: 20,
    text: "R: 0.0%",
    fontColor: "rgba(0, 255, 255, 0.8)",
    fontSize: 14
});

ipcRenderer.on("audio-data", function (data) {
    if (data.rms) {
        var l = ((data.rms[0] || 0) * 100).toFixed(1);
        var r = ((data.rms[1] || 0) * 100).toFixed(1);
        ui.setElementProperties("rms_l", { text: "L: " + l + "%" });
        ui.setElementProperties("rms_r", { text: "R: " + r + "%" });
    }

    if (data.bands) {
        if (ui.beginUpdate) ui.beginUpdate();

        for (var i = 0; i < bandCount; i++) {
            if (i < data.bands.length) {
                var val = data.bands[i];
                var h = Math.max(1, val * maxBarHeight);
                ui.setElementProperties(barsRU[i], { height: h, y: centerY - h });
                ui.setElementProperties(barsLU[i], { height: h, y: centerY - h });
            }
        }

        if (ui.endUpdate) ui.endUpdate();
    }
});
