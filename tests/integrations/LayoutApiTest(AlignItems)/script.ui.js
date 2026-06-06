
// LayoutBox container (row) using nested children
ui.addLayoutBox({
  id: "rowLayout",
  x: 20,
  y: 20,
  width: 220,
  height: 120,
  // direction: "ltr", // Text direction: left-to-right (default)
  gap: 5,
  padding: 10,
  alignItems: "normal",
  dispaly:"flex",
  // justifyContent: "start",
  // backgroundColor: "rgba(0, 0, 0, 1)",
  borderRadius: 8,
  borderWidth: 3,
  borderColor: "rgba(0, 191, 255, 1)",
  children: [
    ui.shape({
      id: "childA",
      type: "rectangle",
      width: 40,
      height: 20,
      fillColor: "rgba(0,160,255,0.8)"
    }),
    ui.shape({
      id: "childB",
      type: "rectangle",
      width: 30,
      height: 30,
      fillColor: "rgba(0,220,120,0.8)"
    })
  ]
});
