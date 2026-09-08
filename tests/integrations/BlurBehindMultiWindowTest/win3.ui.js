ui.addShape({
    id: "headerBar",
    type: "rectangle",
    x: 15,
    y: 15,
    width: 370,
    height: 38,
    radius: 8,
    fillColor: "rgba(48, 209, 88, 0.45)"
});

ui.addText({
    id: "title",
    x: 25,
    y: 24,
    text: "Window 3: Config Object & Shadows",
    fontColor: "#ffffff",
    fontSize: 16,
    fontWeight: "bold"
});

ui.addText({
    id: "desc",
    x: 25,
    y: 70,
    text: "Testing:\n- apply(h3, { type: 'blur', corner: 'roundsmall' })\n- setShadow(h3, 'top|left')\n- apply(h3, { type: 'acrylic', shadow: 'all' })\n- Dynamic property reconfiguration",
    fontColor: "#d2f7dc",
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
    text: "Status: Config Object Active",
    fontColor: "#32d74b",
    fontSize: 13
});
