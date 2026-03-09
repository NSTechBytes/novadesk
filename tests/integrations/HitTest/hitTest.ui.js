ui.addImage({
    id: "O_Image",
    path: "../assets/O.png",
    width: 100,
    height: 100,
    onLeftMouseDown: function () {
        console.log("Left Mouse Down");
    }
})

ui.addText({
    id: "O_Text",
    text: "O",
    fontSize: 100,
    fontColor:"rgb(255,255,255)",
    x: 150,
    y: 150,
    align: "center",
    onLeftMouseDown: function () {
        console.log("Left Mouse Down");
    }
})