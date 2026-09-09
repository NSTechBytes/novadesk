ui.beginUpdate();

ui.addText({
  id: "title",
  x: 20,
  y: 20,
  text: "Window Animation Integration Test",
  fontSize: 16,
  fontFace: "Segoe UI",
  fontWeight: 700,
  fontColor: "#a6e3a1"
});

ui.addText({
  id: "desc",
  x: 20,
  y: 50,
  text: "Testing win.animate() and ui.animate()...",
  fontSize: 12,
  fontFace: "Segoe UI",
  fontWeight: 400,
  fontColor: "#cdd6f4"
});

ui.addShape({
  id: "box",
  x: 20,
  y: 80,
  width: 50,
  height: 50,
  fillColor: "#f38ba8",
  shapeType: "rectangle"
});

ui.endUpdate();

// Test ui.animate with widget-relative strings:
// Animate box to the bottom-right of the widget canvas
ui.animate({
  id: "box",
  duration: 800,
  easing: "ease-in-out",
  from: { x: "left + 20", y: "top + 80" },
  to: { x: "right - 20", y: "bottom - 20" }
});

