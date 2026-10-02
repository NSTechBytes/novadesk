ui.beginUpdate();

// Title
ui.addText({
  id: "title",
  x: 20,
  y: 15,
  text: "Drop-Down Integration Test",
  fontSize: 16,
  fontFace: "Segoe UI",
  fontWeight: 700,
  fontColor: "#a6e3a1",
  onLeftMouseUp: function () {
    console.log("Title clicked");
  },
});

// Default drop-down: no options, nothing selected.
ui.addDropDown({
  id: "bareDropDown",
  x: 20,
  y: 55,
});

// Plain-string options.
ui.addDropDown({
  id: "stringOptions",
  x: 20,
  y: 95,
  width: 180,
  options: ["Small", "Medium", "Large"],
  selectedIndex: 1,
  popupHoverColor: "#a6e3a1",
  chevronSize: 10,
});

ui.addLayoutBox({
  id: "boxDropDowns",
  x: 20,
  y: 150,
  width: 240,
  height: 40,
  direction: "row",
  gap: 10,
  children: [
    {
      elementType: "dropdown",
      id: "childDropDown",
      width: 140,
      options: ["Child A", "Child B"],
      selectedIndex: 1,
      onChange: (e) => {
        console.log("valueOptions onChange fired with: " + e.data);
      },
      onOpen: (e) => {
        console.log("valueOptions onOpen fired with: " + e.data);
      },
      onClose: (e) => {
        console.log("valueOptions onClose fired with: " + e.data);
      },
      onCancel: (e) => {
        console.log("valueOptions onCancel fired with: " + e.data);
      },
    },
  ],
});

ui.endUpdate();
