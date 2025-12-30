// Test controller script for Hand Cursor
novadesk.log("Starting Hand Cursor Test...");

testWidget = new widgetWindow({
    id: "TestHandCursor",
    x: 100, y: 100,
    width: 400, height: 300,
    backgroundcolor: "rgba(50, 50, 50, 200)",
    script: "test_ui.js"
});

novadesk.log("Test widget created. Hover over elements to test cursor changes.");
