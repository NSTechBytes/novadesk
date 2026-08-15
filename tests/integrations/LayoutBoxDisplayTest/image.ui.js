ui.beginUpdate();
// ui.addImage({
//     id: "list-item1",
//     path: "C:\\Users\\nasirshahbaz\\OneDrive\\Desktop\\Screenshot 2026-05-14 181227.png",
//     width: 400,
//     height: 400
// })

// Test 3: display:list-item with listStyleType:disc (default filled circle marker)
ui.addLayoutBox({
    id: "list-item-disc",
    display: "list-item",
    listStyleType: "disc",
    flexDirection: "column",
    x: 50,
    y: 50,
    width: 200,
    height: 200,
    backdropFilter: {
        blur: 10,
        // brightness: 0.2,
        // contrast: 0.2,
        // greyScale: 0.2,
        // saturate: 0.2,
        // sepia: 0.2,
        // hueRotate: 90,
        // invert: 1,
        // opacity: 0.5
    },
    borderRadius: 20,
    borderWidth: 2,
    borderColor: "black",
    backgroundColor: "rgba(255, 255, 255, 0.2)",
    backgroundColorRadius: 20,
})

ui.addColorPicker({
  id: "accent",
  x: 20,
  y: 20,
  width: 32,
  height: 32,
  color: "#1b1b1b",
  onChange: (hexColor) => {}
});

ui.endUpdate();