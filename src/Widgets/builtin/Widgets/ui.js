ui.beginUpdate();

ui.addText({
  id: "title",
  text: "Mouse callback demo",
  x: 16,
  y: 14,
  width: 260,
  height: 28,
  fontSize: 16,
  fontColor: "rgb(230,230,230)"
});

ui.addShape({
  id: "clickBox",
  shapeType: "rectangle",
  x: 16,
  y: 52,
  width: 260,
  height: 90,
  fillColor: "rgba(35,35,35,220)",
  strokeColor: "rgba(255,255,255,40)",
  strokeWidth: 1,
  backgroundColorRadius: 10,
  tooltipText: "Click or hover this area",
  onMouseOver: (e) => {
    console.log("onMouseOver", e.clientX, e.clientY);
    ui.setElementProperties("clickBox", { fillColor: "rgba(55,90,140,230)" });
    ui.setElementProperties("status", { text: "hover: " + e.clientX + "," + e.clientY });
  },
  onMouseLeave: () => {
    console.log("onMouseLeave");
    ui.setElementProperties("clickBox", { fillColor: "rgba(35,35,35,220)" });
    ui.setElementProperties("status", { text: "waiting for input..." });
  },
  onLeftMouseDown: () => {
    console.log("onLeftMouseDown");
    ui.setElementProperties("status", { text: "left mouse down" });
  },
  onLeftMouseUp: () => {
    console.log("onLeftMouseUp");
    ui.setElementProperties("status", { text: "left mouse up" });
  },
  onRightMouseUp: () => {
    console.log("onRightMouseUp");
    ui.setElementProperties("status", { text: "right click" });
  }
});

ui.addText({
  id: "status",
  text: "waiting for input...",
  x: 16,
  y: 154,
  width: 320,
  height: 24,
  fontSize: 13,
  fontColor: "rgb(180,180,180)"
});

ui.endUpdate();
