ui.beginUpdate();

// Background shape (renders first = bottom layer, like CSS)
ui.addShape({
    id: "bgShape",
    type: "rectangle",
    x: 0,
    y: 0,
    width: 320,
    height: 200,
    fillColor: "rgba(30, 30, 34, 1)",
    radius: 12,
});

// Title text (renders second = middle layer)
ui.addText({
    id: "titleText",
    x: 160,
    y: 20,
    text: "Search",
    fontSize: 18,
    fontFace: "Segoe UI",
    fontColor: "#e0e0e0",
    textAlign: "center",
    fontWeight: "bold",
});

// Input box (renders third = top layer, composites over the background shape)
// This is the new Direct2D-rendered input element — it renders inline between
// the background shape and subsequent elements, just like CSS document flow.
ui.addInputBox({
    id: "searchInput",
    x: 20,
    y: 55,
    width: 280,
    height: 36,
    text: "",
    placeholder: "Type to search...",
    fontFace: "Segoe UI",
    fontSize: 14,
    fontColor: "#ffffff",
    placeholderColor: "rgba(150, 150, 150, 1)",
    caretColor: "#51BCFE",
    selectionColor: "rgba(81, 188, 254, 0.4)",
    backgroundColor: "rgba(45, 45, 50, 1)",
    radius: 8,
    padding: 10,
    align: "left",
    onTextChange: function (e) {
        // e.data contains current input text
        ui.setElementProperties("resultText", {
            text: "You typed: " + e.data,
        });
    },
    onEnter: function (e) {
        ui.setElementProperties("resultText", {
            text: "Submitted: " + e.data,
        });
    },
});

// Another input box with different styling (renders after the first)
ui.addInputBox({
    id: "nameInput",
    x: 20,
    y: 105,
    width: 280,
    height: 36,
    text: "",
    placeholder: "Enter your name...",
    fontFace: "Consolas",
    fontSize: 14,
    fontColor: "#ffffff",
    placeholderColor: "rgba(120, 120, 120, 1)",
    caretColor: "#BD34FE",
    selectionColor: "rgba(189, 52, 254, 0.4)",
    backgroundColor: "rgba(45, 45, 50, 1)",
    radius: 8,
    padding: 10,
    align: "left",
});

// Result text below the inputs (renders last = on top of everything)
ui.addText({
    id: "resultText",
    x: 160,
    y: 155,
    text: "Start typing above...",
    fontSize: 12,
    fontFace: "Segoe UI",
    fontColor: "#888888",
    textAlign: "center",
});

ui.endUpdate();
