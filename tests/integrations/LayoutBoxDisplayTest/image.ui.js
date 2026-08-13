ui.beginUpdate();
ui.addImage({
    id: "list-item1",
    path: "C:\\Users\\nasirshahbaz\\OneDrive\\Desktop\\Screenshot 2026-05-14 181227.png",
    width: 400,
    height: 400
})

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
    backdropBlur: 14,
    borderRadius: 20,
    borderWidth:2,
    borderColor:"black",
    backgroundColor: "rgba(44, 25, 255, 0.5)",
    backgroundColorRadius: 20,
})

ui.endUpdate();