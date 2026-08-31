function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

function fail(name, details) {
  console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}

function expect(name, condition, details) {
  if (condition) {
    pass(name, details);
  } else {
    fail(name, details);
  }
}

// Background panel behind container
ui.addShape({
  id: "bgPanel",
  type: "rectangle",
  x: 30,
  y: 30,
  width: 320,
  height: 220,
  // radius: 12,
  fillColor: "rgba(35, 42, 68, 0.95)",
  strokeColor: "rgba(255, 255, 255, 0.15)",
  strokeWidth: 1
});

// 1. Create a rectangle container with native scroll enabled
ui.addShape({
  id: "scrollBox",
  type: "rectangle",
  x: 30,
  y: 30,
  width: 320,
  height: 220,
  // radius: 12,
  fillColor: "rgba(35, 42, 68, 0.95)",
  strokeColor: "rgba(255, 255, 255, 0.15)",
  strokeWidth: 1,
  overflow: "auto",
  scrollX:50,
  scrollY:50,
  scrollStep: 30,
  showScrollbar: true,
  scrollbarWidth: 6,
  scrollbarColor: "rgba(100, 180, 255, 0.7)",
  scrollbarRadius: 3
});

// 2. Add overflowing children attached via `container: "scrollBox"`
for (let i = 0; i < 10; i++) {
  ui.addShape({
    id: "item_card_" + i,
    container: "scrollBox",
    type: "rectangle",
    x: 12,
    y: 12 + (i * 55),
    width: 480,
    height: 45,
    radius: 8,
    fillColor: "rgba(255, 255, 255, 0.08)",
    strokeColor: "rgba(255, 255, 255, 0.08)",
    strokeWidth: 1
  });

  ui.addText({
    id: "item_txt_" + i,
    container: "scrollBox",
    x: 24,
    y: 24 + (i * 55),
    width: 250,
    height: 20,
    text: "Card #" + (i + 1),
    fontSize: 14,
    fontColor: "#ffffff"
  });
}

// // Verification checks
// const scrollInfo = ui.getScroll("scrollBox");
// expect("getScroll() returned object", !!scrollInfo, JSON.stringify(scrollInfo));
// expect("scrollX initially 0", scrollInfo?.scrollX === 0, String(scrollInfo?.scrollX));
// expect("scrollY initially 0", scrollInfo?.scrollY === 0, String(scrollInfo?.scrollY));
// expect("contentHeight > 220", scrollInfo?.contentHeight > 220, String(scrollInfo?.contentHeight));
// expect("maxScrollY > 0", scrollInfo?.maxScrollY > 0, String(scrollInfo?.maxScrollY));

// const overflowY = ui.getElementProperty("scrollBox", "overflowY");
// expect("getElementProperty('overflowY') === 'auto'", overflowY === "auto", String(overflowY));

// // Test scrollTo
// ui.scrollTo("scrollBox", { y: 80 });
// const afterScroll = ui.getScroll("scrollBox");
// expect("ui.scrollTo({ y: 80 }) -> scrollY === 80", afterScroll?.scrollY === 80, String(afterScroll?.scrollY));

// // Test setElementProperties with scrollY
// ui.setElementProperties("scrollBox", { scrollY: 150 });
// const afterSetProp = ui.getScroll("scrollBox");
// expect("setElementProperties({ scrollY: 150 })", afterSetProp?.scrollY === 150, String(afterSetProp?.scrollY));

// // Reset back to top for visual interactive testing
// ui.scrollTo("scrollBox", { y: 0 });
