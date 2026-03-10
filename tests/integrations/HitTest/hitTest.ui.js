function setButtonFill(id, fillColor) {
    ui.setElementProperties(id, {
        fillColor: fillColor,
    });
}

ui.addShape({
    id: "rect1",
    type: "rectangle",
    width: 160,
    height: 90,
    radius: 12,
    fillColor: "rgba(30, 30, 30, 1)",
    strokeColor: "rgba(255, 255, 255, 0.2)",
    strokeWidth: 1,
    onLeftMouseDown: function () {
        console.log("Left Mouse Down");
    },
    onMouseOver: function () {
        console.log("Mouse Over");
        setButtonFill("rect1", "rgba(30, 30, 30, 1)");
    },
    onMouseLeave: function () {
        console.log("Mouse Out");
        setButtonFill("rect1", "rgba(30, 30, 30, 0.5)");
    }
});

ui.addImage({
    id: "O_Image",
    path: "D:\\Novadesk-Project\\novadesk\\dist\\NowPlaying\\cover.jpg",
    width: 100,
    height: 100,
    preserveAspectRatio: "preserve",
})

ui.addImage({
    id: "O_Image2",
    x: 150,
    y: 50,
    path: "../assets/O.png",
    width: 100,
    height: 100,
    preserveAspectRatio: "crop",
})
ui.setElementProperties("O_Image2", {
    path:"D:\\Novadesk-Project\\novadesk\\dist\\NowPlaying\\cover.jpg",
});

ui.addText({
    id: "O_Text",
    text: "O",
    fontSize: 100,
    fontColor: "rgb(255,255,255)",
    x: 150,
    y: 150,
    align: "center",
    onLeftMouseDown: function () {
        console.log("Left Mouse Down");
    }
})