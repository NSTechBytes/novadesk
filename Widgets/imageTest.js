
// Create a widget window
var w = new widgetWindow({
    id: "ImageTest",
    x: 100,
    y: 100,
    width: 800,
    height: 600,
    backgroundcolor: "rgba(30,30,30,255)",
    clickthrough: false,
    draggable: true
});

// Assuming we have a test image. Using a placeholder path, user might need to adjust.
// Or I can try to use a solid color standard image if I had one, but I don't.
// I'll try to use a known file if possible, or just expect the user to have 'test.png'
var imgPath = "assets/Background.png";

w.addText({
    id: "t1",
    text: "Original (0: Stretch, 1: Fit, 2: Crop)",
    x: 10, y: 10,
    fontsize: 20,
    color: "white"
});

// 1. Stretch (Default)
w.addText({ id: "l1", text: "Stretch (0)", x: 10, y: 50, color: "white" });
w.addImage({
    id: "img_stretch",
    path: imgPath,
    x: 10, y: 80,
    width: 200, height: 100,
    solidcolor: "rgba(255,255,255,50)", // Semi-transparent bg to see the box
    preserveaspectratio: 0
});

// 2. Preserve (Fit)
w.addText({ id: "l2", text: "Preserve (1)", x: 250, y: 50, color: "white" });
w.addImage({
    id: "img_preserve",
    path: imgPath,
    x: 250, y: 80,
    width: 200, height: 100,
    solidcolor: "rgba(255,255,255,50)",
    preserveaspectratio: 1
});

// 3. Crop (Fill)
w.addText({ id: "l3", text: "Crop (2)", x: 500, y: 50, color: "white" });
w.addImage({
    id: "img_crop",
    path: imgPath,
    x: 500, y: 80,
    width: 200, height: 100,
    solidcolor: "rgba(255,255,255,50)",
    preserveaspectratio: 2
});

// 4. Tint
w.addText({ id: "l4", text: "Tint (Red)", x: 10, y: 250, color: "white" });
w.addImage({
    id: "img_tint",
    path: imgPath,
    x: 10, y: 280,
    width: 200, height: 100,
    preserveaspectratio: 1,
    imagetint: "rgb(255,0,0)"
});

// 5. Tint + Alpha
w.addText({ id: "l5", text: "Tint (Blue + 50% Alpha)", x: 250, y: 250, color: "white" });
w.addImage({
    id: "img_tint_alpha",
    path: imgPath,
    x: 250, y: 280,
    width: 200, height: 100,
    preserveaspectratio: 1,
    imagetint: "rgb(0,0,255)"
});

novadesk.log("Image Test Loaded");
