// Integration test for the slider element.
//
// Note: drag behavior (BeginDrag/UpdateDrag/EndDrag, onInput throttling,
// track-click jump) needs real mouse input and is verified manually; this
// suite covers creation, the value model, property round-trips,
// programmatic methods and events.

function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

function fail(name, details) {
  console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}

function expectEq(name, actual, expected) {
  if (actual === expected) {
    pass(
      name,
      "expected=" +
        JSON.stringify(expected) +
        " actual=" +
        JSON.stringify(actual),
    );
  } else {
    fail(
      name,
      "expected=" +
        JSON.stringify(expected) +
        " actual=" +
        JSON.stringify(actual),
    );
  }
}

function expectTrue(name, actual) {
  if (actual === true || actual === 1) {
    pass(name, "value=true");
  } else {
    fail(name, "expected true, actual=" + actual);
  }
}

function expectFalse(name, actual) {
  if (actual === false || actual === 0) {
    pass(name, "value=false");
  } else {
    fail(name, "expected false, actual=" + actual);
  }
}

// Color getters return rgba(r,g,b,a) strings; compare against that form.
function rgba(hex, a) {
  const r = parseInt(hex.slice(1, 3), 16);
  const g = parseInt(hex.slice(3, 5), 16);
  const b = parseInt(hex.slice(5, 7), 16);
  return (
    "rgba(" +
    r +
    "," +
    g +
    "," +
    b +
    "," +
    (a === undefined ? "1.00" : a.toFixed(2)) +
    ")"
  );
}

// onChange/onInput receive the event object; the value string is on e.data.
function firedWith(log, index, expected) {
  const e = log[index];
  return (
    log.length === index + 1 && !!e && (e.data === expected || e.value === expected)
  );
}

ui.beginUpdate();

// Title
ui.addText({
  id: "title",
  x: 20,
  y: 15,
  text: "Slider Integration Test",
  fontSize: 16,
  fontFace: "Segoe UI",
  fontWeight: 700,
  fontColor: "#a6e3a1",
});

let changeLog = [];
let inputLog = [];

// Main slider under test
ui.addSlider({
  id: "testSlider",
  x: 20,
  y: 55,
  width: 180,
  height: 24,
  value: 50,
  minValue: 0,
  maxValue: 100,
  step: 1,
  onChange: (e) => {
    changeLog.push(e);
    console.log("testSlider onChange fired with: " + e.data);
  },
  onInput: (e) => {
    inputLog.push(e);
  },
});

// Fully styled slider
ui.addSlider({
  id: "styledSlider",
  x: 20,
  y: 90,
  width: 180,
  height: 26,
  value: 30,
  trackThickness: 6,
  trackBorderRadius: 3,
  trackColor: "#27272A",
  fillColor: "#22C55E",
  sliderOpacity: 0.9,
  thumbSize: 18,
  thumbColor: "#FAFAFA",
  thumbBorderWidth: 2,
  thumbBorderColor: "#3B82F6",
  hoverThumbColor: "#DBEAFE",
  pressedThumbColor: "#93C5FD",
  disabledTrackColor: "#3A3A40",
  disabledFillColor: "#3A3A40",
  disabledThumbColor: "#71717A",
});

// Vertical slider
ui.addSlider({
  id: "vertSlider",
  x: 230,
  y: 130,
  width: 24,
  height: 150,
  direction: "column",
  value: 25,
});

// Orientation alias variant
ui.addSlider({
  id: "aliasSlider",
  x: 20,
  y: 130,
  orientation: "vertical",
  height: 120,
});

// Stepped slider: snaps to 5, 15, 25, ...
ui.addSlider({
  id: "stepSlider",
  x: 20,
  y: 260,
  width: 180,
  minValue: 5,
  maxValue: 95,
  step: 10,
  value: 5,
});

// Continuous slider (no snapping)
ui.addSlider({
  id: "contSlider",
  x: 20,
  y: 295,
  width: 180,
  step: 0,
  value: 12.5,
});

// Disabled slider
ui.addSlider({
  id: "disabledSlider",
  x: 20,
  y: 330,
  width: 180,
  disabled: true,
  value: 40,
});

// Callback-less slider
ui.addSlider({
  id: "silentSlider",
  x: 20,
  y: 365,
  width: 180,
});

// Slider inside a layout box via elementType
ui.addLayoutBox({
  id: "boxSliders",
  x: 20,
  y: 400,
  width: 220,
  height: 40,
  direction: "row",
  gap: 10,
  children: [
    { elementType: "slider", id: "childSlider", width: 120, value: 70 },
  ],
});

ui.endUpdate();

console.log("----------------------------------------");
console.log("Starting Slider Creation Tests...");
console.log("----------------------------------------");

expectEq("initial value", ui.getSliderValue("testSlider"), 50);
expectEq("getElementProperty(value)", ui.getElementProperty("testSlider", "value"), 50);
expectEq("minValue", ui.getElementProperty("testSlider", "minValue"), 0);
expectEq("maxValue", ui.getElementProperty("testSlider", "maxValue"), 100);
expectEq("step", ui.getElementProperty("testSlider", "step"), 1);
expectEq("default direction row", ui.getElementProperty("testSlider", "direction"), "row");
expectEq("column direction", ui.getElementProperty("vertSlider", "direction"), "column");
expectEq("orientation alias maps to column", ui.getElementProperty("aliasSlider", "orientation"), "column");
expectEq("styled initial value", ui.getSliderValue("styledSlider"), 30);
expectEq("layout child created", ui.getElementProperty("childSlider", "value"), 70);
expectEq("default width", ui.getElementProperty("testSlider", "width"), 180);
expectEq("default height", ui.getElementProperty("testSlider", "height"), 24);

console.log("----------------------------------------");
console.log("Starting Slider Property Tests...");
console.log("----------------------------------------");

// Defaults
expectEq("default trackThickness", ui.getElementProperty("testSlider", "trackThickness"), 4);
expectEq("default trackColor", ui.getElementProperty("testSlider", "trackColor"), rgba("#3F3F46"));
expectEq("default fillColor", ui.getElementProperty("testSlider", "fillColor"), rgba("#3B82F6"));
expectEq("default thumbColor", ui.getElementProperty("testSlider", "thumbColor"), rgba("#FFFFFF"));
expectEq("default thumbSize auto (0)", ui.getElementProperty("testSlider", "thumbSize"), 0);
expectEq("default thumbBorderWidth", ui.getElementProperty("testSlider", "thumbBorderWidth"), 0);
expectEq("default sliderOpacity", Math.round(ui.getElementProperty("testSlider", "sliderOpacity") * 100) / 100, 1);
expectEq("unset hoverThumbColor", ui.getElementProperty("testSlider", "hoverThumbColor"), undefined);
expectEq("unset pressedThumbColor", ui.getElementProperty("testSlider", "pressedThumbColor"), undefined);

// Styled slider round-trip
expectEq("trackThickness styled", ui.getElementProperty("styledSlider", "trackThickness"), 6);
expectEq("trackBorderRadius styled", ui.getElementProperty("styledSlider", "trackBorderRadius"), 3);
expectEq("trackColor styled", ui.getElementProperty("styledSlider", "trackColor"), rgba("#27272A"));
expectEq("fillColor styled", ui.getElementProperty("styledSlider", "fillColor"), rgba("#22C55E"));
expectEq("sliderOpacity styled", Math.round(ui.getElementProperty("styledSlider", "sliderOpacity") * 100) / 100, 0.9);
expectEq("thumbSize styled", ui.getElementProperty("styledSlider", "thumbSize"), 18);
expectEq("thumbColor styled", ui.getElementProperty("styledSlider", "thumbColor"), rgba("#FAFAFA"));
expectEq("thumbBorderWidth styled", ui.getElementProperty("styledSlider", "thumbBorderWidth"), 2);
expectEq("thumbBorderColor styled", ui.getElementProperty("styledSlider", "thumbBorderColor"), rgba("#3B82F6"));
expectEq("hoverThumbColor styled", ui.getElementProperty("styledSlider", "hoverThumbColor"), rgba("#DBEAFE"));
expectEq("pressedThumbColor styled", ui.getElementProperty("styledSlider", "pressedThumbColor"), rgba("#93C5FD"));
expectEq("disabledTrackColor styled", ui.getElementProperty("styledSlider", "disabledTrackColor"), rgba("#3A3A40"));
expectEq("disabledFillColor styled", ui.getElementProperty("styledSlider", "disabledFillColor"), rgba("#3A3A40"));
expectEq("disabledThumbColor styled", ui.getElementProperty("styledSlider", "disabledThumbColor"), rgba("#71717A"));

// Single-key updates
ui.setElementProperty("testSlider", "fillColor", "#FF5500");
expectEq("fillColor after update", ui.getElementProperty("testSlider", "fillColor"), rgba("#FF5500"));

ui.setElementProperties("testSlider", {
  trackThickness: 8,
  thumbSize: 20,
});
expectEq("trackThickness after update", ui.getElementProperty("testSlider", "trackThickness"), 8);
expectEq("thumbSize after update", ui.getElementProperty("testSlider", "thumbSize"), 20);

// Range edits re-clamp the value
ui.setElementProperties("testSlider", { maxValue: 40 });
expectEq("value re-clamped into new range", ui.getSliderValue("testSlider"), 40);
ui.setElementProperties("testSlider", { maxValue: 100, value: 50 });
expectEq("value restored", ui.getSliderValue("testSlider"), 50);

// Disabled state round-trip
expectTrue("disabledSlider disabled", ui.getElementProperty("disabledSlider", "disabled"));
expectFalse("testSlider not disabled", ui.getElementProperty("testSlider", "disabled"));
ui.setElementProperty("testSlider", "disabled", true);
expectTrue("disabled after setElementProperty", ui.getElementProperty("testSlider", "disabled"));
ui.setElementProperty("testSlider", "disabled", false);
expectFalse("re-enabled", ui.getElementProperty("testSlider", "disabled"));

console.log("----------------------------------------");
console.log("Starting Slider Methods Tests...");
console.log("----------------------------------------");

// setSliderValue / getSliderValue
changeLog = [];
inputLog = [];
expectTrue("setSliderValue returns true", ui.setSliderValue("testSlider", 75));
expectEq("value after set", ui.getSliderValue("testSlider"), 75);
expectTrue("onChange fired with '75'", firedWith(changeLog, 0, "75"));
expectEq("onInput not fired programmatically", inputLog.length, 0);

// Clamping
ui.setSliderValue("testSlider", 500);
expectEq("clamped to max", ui.getSliderValue("testSlider"), 100);
expectTrue("onChange fired with '100'", firedWith(changeLog, 1, "100"));

ui.setSliderValue("testSlider", -10);
expectEq("clamped to min", ui.getSliderValue("testSlider"), 0);
expectTrue("onChange fired with '0'", firedWith(changeLog, 2, "0"));

// Snapping: step 10 counted from minValue 5 -> 5,15,25,...,35
ui.setSliderValue("stepSlider", 33);
expectEq("snapped to step grid", ui.getSliderValue("stepSlider"), 35);
expectEq("value prop reflects snap", ui.getElementProperty("stepSlider", "value"), 35);

// Continuous slider keeps fractions
expectEq("continuous value kept", ui.getSliderValue("contSlider"), 12.5);
ui.setSliderValue("contSlider", 33.25);
expectEq("continuous set kept", ui.getSliderValue("contSlider"), 33.25);

// Same-value set stays silent
const silentCount = changeLog.length;
ui.setSliderValue("testSlider", 0);
expectEq("no duplicate onChange for same value", changeLog.length, silentCount);

// Programmatic sets still work on a disabled slider
ui.setSliderValue("disabledSlider", 60);
expectEq("disabled slider accepts programmatic set", ui.getSliderValue("disabledSlider"), 60);

// Callback-less slider changes silently
ui.setSliderValue("silentSlider", 42);
expectEq("silent slider moved", ui.getSliderValue("silentSlider"), 42);
expectEq("silent set fired no events", changeLog.length, silentCount);

// Unknown ids return false / undefined and do not crash
expectFalse("setSliderValue unknown id", ui.setSliderValue("nope", 10));
expectEq("getSliderValue unknown id", ui.getSliderValue("nope"), undefined);
expectEq("getElementProperty unknown id", ui.getElementProperty("nope", "value"), undefined);

// setElementProperties imperative value
changeLog = [];
ui.setElementProperties("testSlider", { value: 20 });
expectEq("value via setElementProperties", ui.getSliderValue("testSlider"), 20);
expectTrue("imperative value fires onChange", firedWith(changeLog, 0, "20"));

// Setting the current value via properties stays silent
ui.setElementProperties("testSlider", { value: 20 });
expectEq("same value via properties silent", changeLog.length, 1);

// Fractional formatting through the event payload
ui.setSliderValue("contSlider", 7.5);
changeLog = [];
ui.setSliderValue("contSlider", 8.25);
expectTrue("fraction formatted in payload", firedWith(changeLog, 0, "8.25"));

// Re-adding with the same id replaces cleanly and keeps new options
ui.addSlider({
  id: "testSlider",
  x: 20,
  y: 55,
  width: 180,
  height: 24,
  value: 88,
  fillColor: "#FF0000",
  onChange: (e) => {
    changeLog.push(e);
  },
});
expectEq("re-added slider value", ui.getSliderValue("testSlider"), 88);
expectEq("re-added slider color", ui.getElementProperty("testSlider", "fillColor"), rgba("#FF0000"));

console.log("----------------------------------------");
console.log("=== All Tests Completed ===");
console.log("----------------------------------------");
