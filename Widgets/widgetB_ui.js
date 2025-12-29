// The global 'widgetWindow' object automatically points to the 
// specific widget instance that is being loaded or refreshed.
novadesk.log("UI Script for Widget B is running...");

widgetWindow.addText({
    id: "timeLabel",
    text: "Current Time: " + new Date().toLocaleTimeString(),
    x: 10, y: 10,
    fontSize: 14,
    fontcolor: "rgba(0, 0, 0, 0.5)"
});

widgetWindow.addText({
    id: "hint",
    text: "Right-click me to refresh ONLY this widget!",
    x: 10, y: 40,
    fontSize: 10,
    fontcolor: "rgba(200, 200, 200, 255)"
});
