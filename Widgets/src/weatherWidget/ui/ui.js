// Background Shape
win.addShape({
    id: "backgroundShape",
    type: "rectangle",
    x: 1,
    y: 1,
    width: 210,
    height: 120,
    fillColor: "rgba(27 ,27 ,31,0.8)",
    strokeColor: "rgba(200,200,200,0.5)",
    radius: 10,
    strokeWidth: 2
})

// Title Text
win.addText({
    id: "title_Text",
    x: ((win.getElementProperty("backgroundShape", "x") + win.getElementProperty("backgroundShape", "width")) / 2),
    y: 15,
    text: "Weather",
    fontSize: 20,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "linearGradient(120deg, #51BCFE, #BD34FE)",
})

// Weather Icon (Large, centered)
win.addText({
    id: "icon_Text",
    x: 15,
    y: 45,
    text: "?",
    fontSize: 40,
    fontFace: "Consolas",
    textAlign: "left",
    fontColor: "rgb(255,255,255)",
})

// Temperature Text (Large, right side)
win.addText({
    id: "temperature_Text",
    x: 175,
    y: 55,
    text: "--C",
    fontSize: 35,
    fontFace: "Consolas",
    textAlign: "right",
    fontColor: "rgb(255,255,255)",
})

// Weather Description (Bottom center)
win.addText({
    id: "description_Text",
    x: ((win.getElementProperty("backgroundShape", "x") + win.getElementProperty("backgroundShape", "width")) / 2),
    y: 100,
    text: "Loading...",
    fontSize: 12,
    fontFace: "Consolas",
    textAlign: "center",
    fontColor: "rgb(200,200,200)",
})

// IPC listener for weather updates
ipc.on("weatherUpdate", function(data) {
    // Update temperature
    win.setElementProperties("temperature_Text", { 
        "text": data.temperature + data.unit 
    });
    
    // Update description
    win.setElementProperties("description_Text", { 
        "text": data.description 
    });
    
    // Update weather icon
    win.setElementProperties("icon_Text", { 
        "text": data.icon 
    });
});
