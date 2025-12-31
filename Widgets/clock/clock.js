var background_Width = 210;
var background_Height = 70;
var font_Color = "rgb(255,255,255)";

win.addImage({
    id: "background_Image",
    width: background_Width,
    height: background_Height,
    path: "../assets/background.png"
});


win.addText({
    id: "time_Title",
    text: "Time",
    x: (background_Width / 2),
    y: 10,
    fontsize: 20,
    textalign: "center",
    fontcolor: font_Color
});

win.addText({
    id: "time",
    text: "00:00:00",
    x: (background_Width / 2),
    y: 40,
    fontsize: 25,
    textalign: "center",
    fontcolor: font_Color
});

function pad(n) {
    return n < 10 ? '0' + n : n.toString();
}

function updateTime() {
    var now = new Date();
    var timeStr = pad(now.getHours()) + ":" +
        pad(now.getMinutes()) + ":" +
        pad(now.getSeconds());

    win.setElementProperties("time", {
        text: timeStr
    });
}

// Update every second
setInterval(updateTime, 1000);
updateTime(); // Initial update

novadesk.log("Clock widget initialized");