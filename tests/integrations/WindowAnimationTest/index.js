import { app, widgetWindow } from "novadesk";

console.log("==================================================");
console.log("=== Starting WindowAnimationTest Integration ===");
console.log("==================================================");

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

const win = new widgetWindow({
  id: "WindowAnimationTestWidget",
  x: 100,
  y: 100,
  width: 400,
  height: 250,
  backgroundColor: "rgba(30, 30, 46, 0.95)",
  script: "./script.ui.js",
  show: true
});

// Step 1: Verify initial state & animate method existence
setTimeout(() => {
  expect("win.animate is a function", typeof win.animate === "function");
  expect("win.stopAnimation is a function", typeof win.stopAnimation === "function");

  const initialBounds = win.getBounds();
  expect("Initial bounds", !!initialBounds && initialBounds.x === 100 && initialBounds.y === 100, JSON.stringify(initialBounds));
}, 200);

// Step 2: Test basic tween animation (from -> to)
setTimeout(() => {
  console.log("Starting Tween Animation...");
  const ret = win.animate({
    duration: 300,
    easing: "ease-out",
    from: { x: 100, y: 100, opacity: 0.5 },
    to: { x: 250, y: 200, width: 450, height: 280, opacity: 1.0 }
  });
  expect("win.animate() return value allows chaining", ret === win);
}, 400);

// Step 3: Verify intermediate or completed state of tween
setTimeout(() => {
  const b = win.getBounds();
  expect("Bounds updated after tween", b && b.x >= 150 && b.y >= 150, JSON.stringify(b));
}, 800);

// Step 4: Test array keyframe animation
setTimeout(() => {
  console.log("Starting Array Keyframes Animation...");
  win.animate({
    duration: 400,
    easing: "ease",
    keyframes: [
      { offset: 0.0, x: 250, y: 200, opacity: 1.0 },
      { offset: 0.5, x: 350, y: 250, opacity: 0.7, easing: "ease-in" },
      { offset: 1.0, x: 450, y: 300, opacity: 1.0 }
    ]
  });
}, 1000);

// Step 5: Test percentage object keyframe animation
setTimeout(() => {
  console.log("Starting Percentage Object Keyframes Animation...");
  win.animate({
    duration: 400,
    keyframes: {
      "0%":   { x: 450, y: 300 },
      "50%":  { x: 300, y: 180 },
      "100%": { x: 150, y: 120 }
    }
  });
}, 1600);

// Step 6: Test stopAnimation()
setTimeout(() => {
  console.log("Starting infinite animation then calling stopAnimation()...");
  win.animate({
    duration: 1000,
    iterationCount: "infinite",
    to: { x: 600, y: 400 }
  });

  setTimeout(() => {
    win.stopAnimation();
    const stoppedBounds = win.getBounds();
    pass("win.stopAnimation() executed", JSON.stringify(stoppedBounds));
  }, 100);
}, 2200);

// Step 7: Error handling tests
setTimeout(() => {
  console.log("Testing error handling...");
  try {
    win.animate({ duration: 300 }); // Missing 'to' or 'keyframes'
    fail("Should throw on missing animation targets");
  } catch (e) {
    pass("Throws when neither to nor keyframes provided", e.message);
  }

  try {
    win.animate({ to: { x: 100 }, keyframes: [{ offset: 0, x: 50 }, { offset: 1, x: 100 }] });
    fail("Should throw on both to and keyframes");
  } catch (e) {
    pass("Throws when both to and keyframes provided", e.message);
  }

  try {
    win.animate({ to: { x: 100 }, iterationCount: 0 });
    fail("Should throw on invalid iterationCount");
  } catch (e) {
    pass("Throws on invalid iterationCount", e.message);
  }
}, 2600);

// Step 8: Screen anchor keyword tests
setTimeout(() => {
  console.log("=== Testing Screen Anchor Keywords ===");

  // Slide to bottom-right corner with offset
  win.animate({
    duration: 500,
    easing: "ease-in-out",
    to: { position: "bottom-right", offsetX: -20, offsetY: -20 }
  });
  pass("position: bottom-right with offset accepted");
}, 3000);

setTimeout(() => {
  // Animate from offscreen-bottom to bottom
  win.animate({
    duration: 400,
    easing: "ease-out",
    from: { y: "offscreen-bottom" },
    to: { y: "bottom - 20" }
  });
  pass("y: offscreen-bottom -> bottom-20 accepted");
}, 3700);

setTimeout(() => {
  // Animate to screen center
  win.animate({
    duration: 300,
    easing: "ease",
    to: { x: "center", y: "center" }
  });
  pass("x: center, y: center accepted");
}, 4300);

setTimeout(() => {
  // Slide in from left offscreen to left edge
  win.animate({
    duration: 350,
    easing: "ease-out",
    from: { x: "offscreen-left" },
    to: { x: "left + 20" }
  });
  pass("x: offscreen-left -> left+20 accepted");
}, 4800);

setTimeout(() => {
  // Keyframe with screen anchors
  win.animate({
    duration: 800,
    easing: "linear",
    keyframes: [
      { offset: 0.0, x: "left + 20", y: "top + 20" },
      { offset: 0.5, x: "right - 20", y: "top + 20" },
      { offset: 1.0, x: "right - 20", y: "bottom - 20" }
    ]
  });
  pass("Keyframes with screen anchor keywords accepted");
}, 5300);

// Step 9: Clean up & exit
setTimeout(() => {
  console.log("==================================================");
  console.log("=== All WindowAnimation Tests Completed ===");
  console.log("==================================================");
  win.destroy();
  app.exit();
}, 6500);
