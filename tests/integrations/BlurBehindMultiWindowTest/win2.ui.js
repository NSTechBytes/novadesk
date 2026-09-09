ui.addShape({
    id: "headerBar",
    type: "rectangle",
    x: 15,
    y: 15,
    width: 370,
    height: 38,
    radius: 8,
    fillColor: "rgba(175, 82, 222, 0.45)"
});

ui.addText({
    id: "title",
    x: 25,
    y: 24,
    text: "Window 2: Acrylic & Config Object",
    fontColor: "#ffffff",
    fontSize: 16,
    fontWeight: "bold"
});

ui.addText({
    id: "desc",
    x: 25,
    y: 70,
    text: "Testing:\n- apply(hwnd, { type: 'acrylic', stroke: 'hidden' })\n- setStroke(hwnd, '#00FF7F')\n- setStroke(hwnd, 'visible')\n- Win11 acrylic fallback on Win10",
    fontColor: "#ecd5fa",
    fontSize: 13
});

ui.addShape({
    id: "statusBadge",
    type: "rectangle",
    x: 25,
    y: 170,
    width: 350,
    height: 40,
    radius: 6,
    fillColor: "rgba(255, 255, 255, 0.1)"
});

ui.addText({
    id: "statusText",
    x: 35,
    y: 182,
    text: "Status: Applying Acrylic Config...",
    fontColor: "#bf5af2",
    fontSize: 13
});
