ui.addText({
  id: "title",
  x: 20,
  y: 20,
  width: 380,
  height: 32,
  text: "Resize Events Test",
  fontSize: 22,
  fontWeight: "bold",
  fontColor: "rgb(240,244,255)"
});

ui.addText({
  id: "info",
  x: 20,
  y: 60,
  width: 380,
  height: 80,
  text: "Window is resizable.\nDrag window borders to trigger:\n- resizeStart\n- resize\n- resizeEnd",
  fontSize: 14,
  fontColor: "rgb(180,195,230)"
});

ui.addText({
  id: "status",
  x: 20,
  y: 150,
  width: 380,
  height: 40,
  text: "Status: Idle",
  fontSize: 16,
  fontColor: "rgb(120,220,150)"
});
