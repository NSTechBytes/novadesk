// Main controller script
novadesk.log("Starting Independent Refresh Test...");

// // Create Widget A (Static)
// widgetA = new widgetWindow({
//     id: "WidgetA",
//     x: 100, y: 100,
//     width: 200, height: 100,
//     backgroundcolor: "rgba(255, 0, 0, 100)" // Red
// });

// widgetA.addText({
//     id: "labelA",
//     text: "I am Widget A (Static)",
//     x: 10, y: 10,
//     fontSize: 12,
//     fontcolor: "rgba(255, 255, 255, 255)"
// });

// Create Widget B (Dynamic/Refreshable)
widgetB = new widgetWindow({
    id: "WidgetB",
    x: 400, y: 100,
    width: 300, height: 150,
    backgroundcolor: "rgba(0, 0, 255, 100)", // Blue
    script: "widgetB_ui.js"
});

novadesk.log("Test widgets created. Right-click Widget B to refresh it.");
