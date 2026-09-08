ui.addShape({
    id: "headerBar",
    type: "rectangle",
    x: 15,
    y: 15,
    width: 370,
    height: 38,
    radius: 8,
    fillColor: "rgba(255, 159, 10, 0.45)"
});

ui.addText({
    id: "title",
    x: 25,
    y: 24,
    text: "Window 4: Composition Effects",
    fontColor: "#ffffff",
    fontSize: 16,
    fontWeight: "bold"
});

ui.addText({
    id: "desc",
    x: 25,
    y: 70,
    text: "Testing:\n- apply(hwnd, 'blur', 'roundsmall')\n- setEffect(hwnd, 'luminance')\n- setEffect(hwnd, 'fullscreen')\n- setEffect(hwnd, 'both')",
    fontColor: "#ffe7c6",
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
    text: "Status: Applying Effects...",
    fontColor: "#ff9f0a",
    fontSize: 13
});
