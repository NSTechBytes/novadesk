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

ui.beginUpdate();

// Title
ui.addText({
  id: "title",
  x: 20,
  y: 15,
  text: "ToggleSwitch Integration Test",
  fontSize: 16,
  fontFace: "Segoe UI",
  fontWeight: 700,
  fontColor: "#a6e3a1",
});

// Main switch under test
let changeLog = [];
ui.addToggleSwitch({
  id: "testSwitch",
  x: 20,
  y: 55,
  width: 48,
  height: 26,
  checked: false,
  onColor: "#3B82F6",
  offColor: "#3F3F46",
  knobColor: "#FFFFFF",
  onChange: (checked) => {
    changeLog.push(checked);
  },
});

// Fully styled switch (labels, border, animation)
ui.addToggleSwitch({
  id: "styledSwitch",
  x: 90,
  y: 55,
  width: 64,
  height: 26,
  checked: true,
  onText: "ON",
  offText: "OFF",
  labelFontSize: 9,
  labelFontWeight: 700,
  labelFontColor: "#FFFFFF",
  borderWidth: 1,
  borderColor: "#555555",
  borderRadius: 6,
  knobSize: 20,
  knobPadding: 3,
  knobBorderWidth: 1,
  knobBorderColor: "#000000",
  opacity: 0.9,
  hoverTrackColor: "#60A5FA",
  disabledTrackColor: "#27272A",
  disabledKnobColor: "#52525B",
});

// Disabled switch for interaction checks
ui.addToggleSwitch({
  id: "disabledSwitch",
  x: 170,
  y: 55,
  disabled: true,
});

// Callback-less switch (built-in toggle still works)
ui.addToggleSwitch({
  id: "silentSwitch",
  x: 240,
  y: 55,
});

// Switch inside a layout box via elementType
ui.addLayoutBox({
  id: "boxSwitches",
  x: 20,
  y: 100,
  width: 200,
  height: 40,
  direction: "row",
  gap: 10,
  children: [{ elementType: "toggleSwitch", id: "childSwitch", checked: true }],
});

ui.endUpdate();

console.log("----------------------------------------");
console.log("Starting ToggleSwitch Creation Tests...");
console.log("----------------------------------------");

expectFalse("initial checked", ui.getToggleSwitchChecked("testSwitch"));
expectFalse(
  "getElementProperty(checked)",
  ui.getElementProperty("testSwitch", "checked"),
);
expectFalse(
  "getElementProperty(value) alias",
  ui.getElementProperty("testSwitch", "value"),
);
expectFalse(
  "getElementProperty(isOn) alias",
  ui.getElementProperty("testSwitch", "isOn"),
);
expectTrue("styled initial checked", ui.getToggleSwitchChecked("styledSwitch"));
expectTrue(
  "layout child created",
  ui.getElementProperty("childSwitch", "checked"),
);
expectEq(
  "default size width",
  ui.getElementProperty("testSwitch", "width"),
  48,
);
expectEq(
  "default size height",
  ui.getElementProperty("testSwitch", "height"),
  26,
);

console.log("----------------------------------------");
console.log("Starting ToggleSwitch Property Tests...");
console.log("----------------------------------------");

// Colors round-trip
expectEq(
  "onColor",
  ui.getElementProperty("testSwitch", "onColor"),
  rgba("#3B82F6"),
);
expectEq(
  "offColor",
  ui.getElementProperty("testSwitch", "offColor"),
  rgba("#3F3F46"),
);
expectEq(
  "knobColor",
  ui.getElementProperty("testSwitch", "knobColor"),
  rgba("#FFFFFF"),
);

ui.setElementProperty("testSwitch", "onColor", "#22C55E");
expectEq(
  "onColor after update",
  ui.getElementProperty("testSwitch", "onColor"),
  rgba("#22C55E"),
);

// Styled switch properties
expectEq(
  "borderWidth",
  ui.getElementProperty("styledSwitch", "borderWidth"),
  1,
);
expectEq(
  "borderColor",
  ui.getElementProperty("styledSwitch", "borderColor"),
  rgba("#555555"),
);
expectEq(
  "borderRadius",
  ui.getElementProperty("styledSwitch", "borderRadius"),
  6,
);
expectEq("knobSize", ui.getElementProperty("styledSwitch", "knobSize"), 20);
expectEq(
  "knobPadding",
  ui.getElementProperty("styledSwitch", "knobPadding"),
  3,
);
expectEq(
  "knobBorderWidth",
  ui.getElementProperty("styledSwitch", "knobBorderWidth"),
  1,
);
expectEq(
  "opacity",
  Math.round(ui.getElementProperty("styledSwitch", "opacity") * 100) / 100,
  0.9,
);
expectEq("onText", ui.getElementProperty("styledSwitch", "onText"), "ON");
expectEq("offText", ui.getElementProperty("styledSwitch", "offText"), "OFF");
expectEq(
  "labelFontFace",
  ui.getElementProperty("styledSwitch", "labelFontFace"),
  "Segoe UI",
);
expectEq(
  "labelFontSize",
  ui.getElementProperty("styledSwitch", "labelFontSize"),
  9,
);
expectEq(
  "labelFontWeight",
  ui.getElementProperty("styledSwitch", "labelFontWeight"),
  700,
);
expectEq(
  "labelFontColor",
  ui.getElementProperty("styledSwitch", "labelFontColor"),
  rgba("#FFFFFF"),
);
expectEq(
  "hoverTrackColor",
  ui.getElementProperty("styledSwitch", "hoverTrackColor"),
  rgba("#60A5FA"),
);
expectEq(
  "disabledTrackColor",
  ui.getElementProperty("styledSwitch", "disabledTrackColor"),
  rgba("#27272A"),
);
expectEq(
  "disabledKnobColor",
  ui.getElementProperty("styledSwitch", "disabledKnobColor"),
  rgba("#52525B"),
);

// Defaults
expectEq(
  "default borderRadius is auto (-1)",
  ui.getElementProperty("testSwitch", "borderRadius"),
  -1,
);
expectEq(
  "default knobSize is auto (0)",
  ui.getElementProperty("testSwitch", "knobSize"),
  0,
);
expectEq(
  "default knobPadding",
  ui.getElementProperty("testSwitch", "knobPadding"),
  2,
);
expectEq(
  "unset hoverTrackColor",
  ui.getElementProperty("testSwitch", "hoverTrackColor"),
  undefined,
);

// Disabled state
expectTrue(
  "disabledSwitch disabled",
  ui.getElementProperty("disabledSwitch", "disabled"),
);
expectFalse(
  "testSwitch not disabled",
  ui.getElementProperty("testSwitch", "disabled"),
);
ui.setElementProperty("testSwitch", "disabled", true);
expectTrue(
  "disabled after setElementProperty",
  ui.getElementProperty("testSwitch", "disabled"),
);
ui.setElementProperty("testSwitch", "disabled", false);
expectFalse("re-enabled", ui.getElementProperty("testSwitch", "disabled"));

// Runtime styling updates
ui.setElementProperties("testSwitch", {
  knobPadding: 4,
});
expectEq(
  "knobPadding after update",
  ui.getElementProperty("testSwitch", "knobPadding"),
  4,
);

console.log("----------------------------------------");
console.log("Starting ToggleSwitch Methods Tests...");
console.log("----------------------------------------");

// setToggleSwitchChecked / getToggleSwitchChecked
changeLog = [];
expectTrue(
  "setToggleSwitchChecked returns true",
  ui.setToggleSwitchChecked("testSwitch", true),
);
expectTrue("checked after set true", ui.getToggleSwitchChecked("testSwitch"));
expectTrue(
  "onChange fired with 'true'",
  changeLog.length === 1 && changeLog[0] === "true",
);

ui.setToggleSwitchChecked("testSwitch", false);
expectFalse("checked after set false", ui.getToggleSwitchChecked("testSwitch"));
expectTrue(
  "onChange fired with 'false'",
  changeLog.length === 2 && changeLog[1] === "false",
);

// Setting the same value must not fire onChange again
ui.setToggleSwitchChecked("testSwitch", false);
expectEq("no duplicate onChange for same value", changeLog.length, 2);

// toggleToggleSwitch flips state
const toggled = ui.toggleToggleSwitch("testSwitch");
expectTrue("toggleToggleSwitch returns true", toggled);
expectTrue("checked after toggle", ui.getToggleSwitchChecked("testSwitch"));
ui.toggleToggleSwitch("testSwitch");
expectFalse(
  "unchecked after second toggle",
  ui.getToggleSwitchChecked("testSwitch"),
);

// Callback-less switch still changes state silently
ui.toggleToggleSwitch("silentSwitch");
expectTrue(
  "silent switch flipped without callback",
  ui.getToggleSwitchChecked("silentSwitch"),
);

// Disabled switches ignore programmatic user-toggle too? (toggle method respects m_Disabled)
const beforeDisabled = ui.getToggleSwitchChecked("disabledSwitch");
ui.toggleToggleSwitch("disabledSwitch");
expectEq(
  "disabled switch ignores toggle",
  ui.getToggleSwitchChecked("disabledSwitch"),
  beforeDisabled,
);

// Unknown ids return false / do not crash
expectFalse(
  "setToggleSwitchChecked unknown id",
  ui.setToggleSwitchChecked("nope", true),
);
expectFalse(
  "getToggleSwitchChecked unknown id",
  ui.getToggleSwitchChecked("nope"),
);
expectFalse("toggleToggleSwitch unknown id", ui.toggleToggleSwitch("nope"));

// setElementProperties imperative checked
ui.setElementProperties("testSwitch", { checked: true });
expectTrue(
  "checked via setElementProperties",
  ui.getElementProperty("testSwitch", "checked"),
);
ui.setElementProperties("testSwitch", { checked: false });
expectFalse(
  "unchecked via setElementProperties",
  ui.getElementProperty("testSwitch", "checked"),
);

// Re-adding with the same id replaces cleanly and keeps new options
ui.addToggleSwitch({
  id: "testSwitch",
  x: 20,
  y: 55,
  checked: true,
  onColor: "#FF0000",
});
expectTrue("re-added switch checked", ui.getToggleSwitchChecked("testSwitch"));
expectEq(
  "re-added switch color",
  ui.getElementProperty("testSwitch", "onColor"),
  rgba("#FF0000"),
);

console.log("----------------------------------------");
console.log("Starting ToggleSwitch Update Tests...");
console.log("----------------------------------------");

ui.beginUpdate();
ui.setElementProperties("testSwitch", { checked: false });
ui.setElementProperties("testSwitch", { checked: true });
ui.endUpdate();
expectTrue(
  "checked after batched update",
  ui.getElementProperty("testSwitch", "checked"),
);

console.log("----------------------------------------");
console.log("=== All Tests Completed ===");
console.log("----------------------------------------");
