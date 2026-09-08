ui.addShape({
    id: "headerBar",
    type: "rectangle",
    x: 15,
    y: 15,
    width: 370,
    height: 38,
    radius: 8,
    fillColor: "rgba(0, 122, 255, 0.45)"
});

ui.addText({
    id: "title",
    x: 25,
    y: 24,
    text: "Window 1: Classic Blur & Toggle",
    fontColor: "#ffffff",
    fontSize: 16,
    fontWeight: "bold"
});

ui.addText({
    id: "desc",
    x: 25,
    y: 70,
    text: "Testing:\n- BlurBehind.apply(hwnd, 'blur', 'round')\n- BlurBehind.setCorner(hwnd, 'roundsmall')\n- BlurBehind.toggle(hwnd) [auto-toggle test]\n- BlurBehind.disable(hwnd)",
    fontColor: "#d0d8e8",
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
    text: "Status: Initializing Blur...",
    fontColor: "#64d2ff",
    fontSize: 13
});
