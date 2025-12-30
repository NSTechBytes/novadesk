// Test controller script for Hand Cursor
novadesk.log("Starting Hand Cursor Test...");

testWidget = new widgetWindow({
    id: "TestHandCursor",
    x: 100, y: 100,
    width: 400, height: 300,
    backgroundcolor: "rgba(50, 50, 50, 200)",
    script: "test_ui.js"
});

pathTestWidget = new widgetWindow({
    id: "PathTest",
    x: 550, y: 100,
    width: 300, height: 300,
    backgroundcolor: "rgba(50, 0, 50, 200)",
    script: "test/../assets/test_path.js" // Test canonicalization
});

novadesk.log("Test widgets created. Hover over elements and check path resolution.");
