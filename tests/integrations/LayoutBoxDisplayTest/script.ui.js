ui.beginUpdate();
ui.addLayoutBox({
    id: "flex-type",
    display: "flex",
    x:"10",
    y:"10",
    // width:"500",
    // height:"500",
    backgroundColor:"rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "shape",
            id: "flexChild1",
            type: "rectangle",
            width: 60,
            height: 50,
            fillColor: "rgba(255, 100, 100, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "flexChild2",
            type: "rectangle",
            width: 60,
            height: 60,
            fillColor: "rgba(100, 255, 100, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "flexChild3",
            type: "rectangle",
            width: 60,
            height: 40, // Will be overridden → 80px
            fillColor: "rgba(100, 200, 255, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "flexChild4",
            type: "rectangle",
            width: 60,
            height: 70, // Will be overridden → 80px
            fillColor: "rgba(255, 200, 100, 1)",
            borderRadius: 4
        }
    ]

})
ui.endUpdate();