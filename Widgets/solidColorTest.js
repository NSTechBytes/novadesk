/* SolidColor and CornerRadius Test Widget */

var solidWidget;

function createSolidTest() {
    solidWidget = new widgetWindow({
        id: "solidTest",
        x: 610, y: 100,
        width: 400, height: 400,
        backgroundcolor: "rgba(20,20,30,255)",
        draggable: true
    });

    solidWidget.addText({
        id: "header",
        text: "SolidColor Test",
        x: 10, y: 10,
        fontsize: 16, color: "white", bold: true
    });

    // 1. Text with simple SolidColor
    solidWidget.addText({
        id: "simpleSolid",
        text: "Simple Solid Background",
        x: 20, y: 50, width: 220, height: 30,
        fontsize: 12, color: "white",
        solidcolor: "rgba(0, 100, 200, 150)",
        align: "centercenter"
    });

    // 2. Text with SolidColor and CornerRadius
    solidWidget.addText({
        id: "roundedSolid",
        text: "Rounded Background (r=10)",
        x: 20, y: 100, width: 220, height: 40,
        fontsize: 12, color: "black",
        solidcolor: "rgba(200, 200, 100, 255)",
        solidcolorradius: 10,
        align: "centercenter"
    });

    // 3. Image with SolidColor (Border effect)
    // Using a non-existent image to show just the background if needed, 
    // or you can put a valid path.
    solidWidget.addImage({
        id: "imageBg",
        x: 20, y: 160, width: 100, height: 100,
        path: "D:\\GITHUB\\Rainmeter\\rainmeter\\Library\\Skin.cpp", // Invalid path, will fail load but render BG
        solidcolor: "rgba(255, 100, 100, 100)",
        solidcolorradius: 20
    });

    solidWidget.addText({
        id: "imgInfo",
        text: "Image Placeholders (Red/Blue)",
        x: 140, y: 170,
        fontsize: 10, color: "silver"
    });

    // 4. Fully opaque rounded button-like text
    solidWidget.addText({
        id: "btn",
        text: "Click Me",
        x: 20, y: 280, width: 120, height: 35,
        fontsize: 12, color: "white",
        solidcolor: "#4CAF50",
        solidcolorradius: 17, // Fully rounded sides
        align: "centercenter"
    });

    novadesk.log("SolidColor test widget created.");
}

novadesk.onReady(function () {
    createSolidTest();
});
