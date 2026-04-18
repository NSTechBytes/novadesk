// Case 1: Standard tooltip (should show)
ui.addText({
    id: "label1",
    x: 10, y: 10,
    text: "Hover here (Tooltip ENABLED)",
    fontColor: "white",
    fontSize: 24,
    tooltipText: "This tooltip SHOULD show up",
    tooltipTitle: "Tooltip Title",
    tooltipIcon: "error",
    // tooltipBalloon: true,
    onMouseOver: () => {
        console.log("Mouse Overded");
    },
    onLeftMouseDown: function () {
        console.log("Mouse Overded");
    }
})