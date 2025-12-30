// The global 'widgetWindow' object automatically points to the 
// specific widget instance that is being loaded or refreshed.
novadesk.log("UI Script for Widget B is running...");

widgetWindow.addText({
    id: "timeLabel",
    text: "Current Time: " + new Date().toLocaleTimeString(),
    x: 10, y: 10,
    fontSize: 14,
    fontcolor: "rgba(0, 0, 0, 0.5)",
    onleftmousedown: 'novadesk.log("Clock clicked!");'
});

widgetWindow.addText({
    id: "hint",
    text: "Right-click me to refresh ONLY this widget!",
    x: 10, y: 40,
    fontSize: 10,
    fontcolor: "rgba(200, 200, 200, 255)"
});


widgetWindow.addImage({
    id: "clockImage",
    x: 10, y: 60,
    width: 200, height: 200,
    path: "../assets/background.png"
});