win.addShape({
    id: "backgroundShape",
    type: "rectangle",
    x: 1,
    y: 1,
    width: 210,
    height: 70,
    fillColor: "rgba(27 ,27 ,31,0.8)",
    strokeColor: "rgba(200,200,200,0.5)",
    radius: 10,
    strokeWidth: 2
})

win.addText({
    id: "time_Text",
    x: ((win.getElementProperty("backgroundShape", "x") + win.getElementProperty("backgroundShape", "width")) / 2),
    y: 10,
    text: "00:00",
    fontSize: 25,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
})

// var now = Date.now();

// console.log('Current Time: ' + now.toTimeString());