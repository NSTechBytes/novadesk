// UI Script for Test Widget
novadesk.log("Loading Test UI...");

// 1. Non-interactive text
widgetWindow.addText({
    id: "staticText",
    text: "I have NO actions (Arrow Cursor)",
    x: 20, y: 20,
    fontSize: 14,
    fontcolor: "rgba(255, 255, 255, 255)"
});

// 2. Interactive text (Click)
widgetWindow.addText({
    id: "clickableText",
    text: "Click me! (Hand Cursor)",
    x: 20, y: 60,
    fontSize: 14,
    fontcolor: "rgba(100, 200, 255, 255)",
    onClick: 'novadesk.log("Text clicked!");'
});

// 3. Interactive text (Hover)
widgetWindow.addText({
    id: "hoverText",
    text: "Hover me! (Hand Cursor)",
    x: 20, y: 100,
    fontSize: 14,
    fontcolor: "rgba(100, 255, 100, 255)",
    onMouseOver: 'novadesk.log("Mouse over text!");',
    onMouseLeave: 'novadesk.log("Mouse leave text!");'
});

// 4. Interactive rectangle (Solid color)
widgetWindow.addText({
    id: "rectAction",
    text: "Rectangle with Action (Hand)",
    x: 20, y: 150,
    width: 200, height: 50,
    fontSize: 12,
    fontcolor: "rgba(0, 0, 0, 255)",
    backgroundcolor: "rgba(255, 255, 0, 255)",
    onLeftMouseUp: 'novadesk.log("Rectangle acted!");'
});

// 5. Non-interactive rectangle
widgetWindow.addText({
    id: "rectStatic",
    text: "Static Rectangle (Arrow)",
    x: 20, y: 210,
    width: 200, height: 50,
    fontSize: 12,
    fontcolor: "rgba(255, 255, 255, 255)",
    backgroundcolor: "rgba(100, 100, 100, 255)"
});

novadesk.log("Test UI Loaded.");
