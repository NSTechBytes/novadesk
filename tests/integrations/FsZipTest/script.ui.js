// Header background
ui.addShape({
  id: "headerBg",
  type: "rectangle",
  x: 0,
  y: 0,
  width: 580,
  height: 50,
  fillColor: "rgba(30, 38, 60, 0.95)"
});

ui.addText({
  id: "titleText",
  x: 20,
  y: 15,
  width: 540,
  height: 25,
  text: "fs Module - Zip Methods Test Suite",
  fontSize: 16,
  fontColor: "#ffffff"
});

// Fetch results via IPC
const results = ipcRenderer.invoke("get-results") || [];

let yOffset = 70;
results.forEach(function (res, idx) {
  const isPassed = res.passed;
  const bg = isPassed ? "rgba(35, 120, 70, 0.85)" : "rgba(180, 50, 50, 0.85)";

  ui.addShape({
    id: "card_" + idx,
    type: "rectangle",
    x: 30,
    y: yOffset,
    width: 520,
    height: 48,
    radius: 6,
    fillColor: "rgba(25, 30, 45, 0.9)",
    strokeColor: isPassed ? "rgba(40, 180, 90, 0.4)" : "rgba(240, 60, 60, 0.4)",
    strokeWidth: 1
  });

  ui.addShape({
    id: "badge_" + idx,
    type: "rectangle",
    x: 45,
    y: yOffset + 12,
    width: 70,
    height: 24,
    radius: 4,
    fillColor: bg
  });

  ui.addText({
    id: "badge_txt_" + idx,
    x: 52,
    y: yOffset + 16,
    width: 56,
    height: 18,
    text: isPassed ? "PASS" : "FAIL",
    fontSize: 12,
    fontColor: "#ffffff"
  });

  ui.addText({
    id: "name_txt_" + idx,
    x: 130,
    y: yOffset + 15,
    width: 400,
    height: 20,
    text: (idx + 1) + ". " + res.name,
    fontSize: 13,
    fontColor: "#e0e6f0"
  });

  yOffset += 58;
});
