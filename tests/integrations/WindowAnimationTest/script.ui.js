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
  text: "Testing win.animate() tween, keyframes, and stopAnimation()...",
  fontSize: 12,
  fontFace: "Segoe UI",
  fontWeight: 400,
  fontColor: "#cdd6f4"
});

ui.endUpdate();
