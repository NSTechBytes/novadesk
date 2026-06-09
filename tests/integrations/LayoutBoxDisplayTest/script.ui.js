ui.beginUpdate();

// Test 1: display:flex with explicit width/height
ui.addLayoutBox({
    id: "flex-type",
    display: "flex",
    x:"10",
    y:"10",
    padding:10,
    gap:10,
    width:500,
    height:200,
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
            height: 40,
            fillColor: "rgba(100, 200, 255, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "flexChild4",
            type: "rectangle",
            width: 60,
            height: 70,
            fillColor: "rgba(255, 200, 100, 1)",
            borderRadius: 4
        }
    ]
})

// Test 2: display:none (should be hidden)
ui.addLayoutBox({
    id: "none-type",
    display: "none",
    x:"10",
    y:"230",
    padding:10,
    gap:10,
    width:500,
    height:200,
    backgroundColor:"rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "shape",
            id: "noneChild1",
            type: "rectangle",
            width: 60,
            height: 50,
            fillColor: "rgba(255, 100, 100, 1)",
            borderRadius: 4
        },
        {
            elementType: "shape",
            id: "noneChild2",
            type: "rectangle",
            width: 60,
            height: 60,
            fillColor: "rgba(100, 255, 100, 1)",
            borderRadius: 4
        }
    ]
})

// Test 3: display:listitem with listStyleType:disc (default filled circle marker)
ui.addLayoutBox({
    id: "listitem-disc",
    display: "listitem",
    listStyleType: "disc",
    x:"20",
    y:"230",
    flexDirection: "column",
    padding:10,
    gap:10,
    width:500,
    height:80,
    backgroundColor:"rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "listitemText1",
            text: "List item with disc marker (filled circle) \n New line goes here",
            x: 20,
            y: 10,
            fontSize: 56,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 4: display:listitem with listStyleType:circle (hollow circle marker)
ui.addLayoutBox({
    id: "listitem-circle",
    display: "listitem",
    listStyleType: "circle",
    x:"20",
    y:"330",
    flexDirection: "column",
    padding:10,
    gap:10,
    width:500,
    height:80,
    backgroundColor:"rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "listitemText2",
            text: "List item with circle marker (hollow circle)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 5: display:listitem with listStyleType:square (filled square marker)
ui.addLayoutBox({
    id: "listitem-square",
    display: "listitem",
    listStyleType: "square",
    x:"20",
    y:"430",
    flexDirection: "column",
    padding:10,
    gap:10,
    width:500,
    height:80,
    backgroundColor:"rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "listitemText3",
            text: "List item with square marker (filled square)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

// Test 6: display:listitem with listStyleType:none (no marker)
ui.addLayoutBox({
    id: "listitem-none",
    display: "listitem",
    listStyleType: "none",
    x:"20",
    y:"530",
    flexDirection: "column",
    padding:10,
    gap:10,
    width:500,
    height:80,
    backgroundColor:"rgba(10,10,10,0.5)",
    children: [
        {
            elementType: "text",
            id: "listitemText4",
            text: "List item with no marker (listStyleType: none)",
            x: 20,
            y: 10,
            fontSize: 16,
            fontColor: "rgba(255, 255, 255, 1)"
        }
    ]
})

ui.endUpdate();
