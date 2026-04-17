// Case 1: Standard tooltip (should show)
ui.addText({
    id: "label1",
    x: 10, y: 10,
    text: "Hover here (Tooltip ENABLED)",
    fontColor: "white",
    fontSize: 24,
    textAlign: "center",
    tooltipText: "This tooltip SHOULD show up",
    onLeftMouseDown: function () {
        console.log("Mouse Overded");
    }
})