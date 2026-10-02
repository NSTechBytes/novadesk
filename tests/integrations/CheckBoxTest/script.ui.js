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

// onChange receives the event object; the state string is on e.data/e.target.
function firedWith(log, index, expected) {
  const e = log[index];
  return (
    log.length === index + 1 &&
    !!e &&
    (e.data === expected || e.target === expected)
  );
}

ui.beginUpdate();

// Title
ui.addText({
  id: "title",
  x: 20,
  y: 15,
  text: "CheckBox Integration Test",
  fontSize: 16,
  fontFace: "Segoe UI",
  fontWeight: 700,
  fontColor: "#a6e3a1",
});

// Main check box under test
let changeLog = [];
ui.addCheckBox({
  id: "testBox",
  x: 20,
  y: 55,
  width: 18,
  height: 18,
  checked: false,
  onChange: (state) => {
    changeLog.push(state);
    console.log("testBox onChange fired with state: " + state.data);
  },
});

// Fully styled check box (label, colors, animation)
ui.addCheckBox({
  id: "styledBox",
  x: 90,
  y: 55,
  width: 20,
  height: 20,
  checked: true,
  text: "Styled",
  boxSize: 20,
  borderRadius: 5,
  uncheckedBorderColor: "#A1A1AA",
  uncheckedBorderWidth: 2,
  checkedColor: "#22C55E",
  checkColor: "#FAFAFA",
  checkThickness: 2.5,
  boxOpacity: 0.9,
  fontFace: "Arial",
  fontSize: 14,
  fontWeight: 600,
  fontColor: "#FFFFFF",
  labelGap: 10,
  hoverBorderColor: "#60A5FA",
  disabledBoxColor: "#27272A",
  disabledCheckColor: "#52525B",
  disabledTextColor: "#3F3F46",
});

// Indeterminate start (two-state box, set programmatically)
ui.addCheckBox({
  id: "indetBox",
  x: 170,
  y: 55,
  checked: "indeterminate",
});

// Tri-state box: click cycle includes indeterminate
ui.addCheckBox({
  id: "triBox",
  x: 360,
  y: 55,
  triState: true,
  onChange: (state) => {
    changeLog.push(state);
  },
});

// Disabled check box for interaction checks
ui.addCheckBox({
  id: "disabledBox",
  x: 240,
  y: 55,
  disabled: true,
});

// Callback-less box (built-in toggle still works)
ui.addCheckBox({
  id: "silentBox",
  x: 300,
  y: 55,
});

// Check box inside a layout box via elementType
ui.addLayoutBox({
  id: "boxChecks",
  x: 20,
  y: 100,
  width: 220,
  height: 40,
  direction: "row",
  gap: 10,
  children: [
    { elementType: "checkbox", id: "childBox", checked: true, text: "Child" },
  ],
});

ui.endUpdate();

console.log("----------------------------------------");
console.log("Starting CheckBox Creation Tests...");
console.log("----------------------------------------");

expectFalse("initial checked", ui.getCheckBoxChecked("testBox"));
expectEq("initial state", ui.getCheckBoxState("testBox"), "unchecked");
expectFalse(
  "getElementProperty(checked)",
  ui.getElementProperty("testBox", "checked"),
);
expectEq(
  "getElementProperty(state)",
  ui.getElementProperty("testBox", "state"),
  "unchecked",
);
expectTrue("styled initial checked", ui.getCheckBoxChecked("styledBox"));
expectEq("styled initial state", ui.getCheckBoxState("styledBox"), "checked");
expectTrue(
  "indeterminate box reports checked",
  ui.getCheckBoxChecked("indetBox"),
);
expectEq(
  "indeterminate state",
  ui.getCheckBoxState("indetBox"),
  "indeterminate",
);
expectTrue(
  "layout child created",
  ui.getElementProperty("childBox", "checked"),
);
expectEq("default size width", ui.getElementProperty("testBox", "width"), 18);
expectEq("default size height", ui.getElementProperty("testBox", "height"), 18);

console.log("----------------------------------------");
console.log("Starting CheckBox Property Tests...");
console.log("----------------------------------------");

// Colors round-trip
expectEq(
  "uncheckedBorderColor default",
  ui.getElementProperty("testBox", "uncheckedBorderColor"),
  rgba("#71717A"),
);
expectEq(
  "checkedColor default",
  ui.getElementProperty("testBox", "checkedColor"),
  rgba("#3B82F6"),
);
expectEq(
  "checkColor default",
  ui.getElementProperty("testBox", "checkColor"),
  rgba("#FFFFFF"),
);

ui.setElementProperty("testBox", "checkedColor", "#FF5500");
expectEq(
  "checkedColor after update",
  ui.getElementProperty("testBox", "checkedColor"),
  rgba("#FF5500"),
);

// Styled box properties
expectEq("boxSize", ui.getElementProperty("styledBox", "boxSize"), 20);
expectEq(
  "uncheckedBorderWidth",
  ui.getElementProperty("styledBox", "uncheckedBorderWidth"),
  2,
);
expectEq(
  "uncheckedBorderColor styled",
  ui.getElementProperty("styledBox", "uncheckedBorderColor"),
  rgba("#A1A1AA"),
);
expectEq(
  "checkedColor styled",
  ui.getElementProperty("styledBox", "checkedColor"),
  rgba("#22C55E"),
);
expectEq(
  "checkColor styled",
  ui.getElementProperty("styledBox", "checkColor"),
  rgba("#FAFAFA"),
);
expectEq(
  "checkThickness",
  ui.getElementProperty("styledBox", "checkThickness"),
  2.5,
);
expectEq(
  "boxOpacity",
  Math.round(ui.getElementProperty("styledBox", "boxOpacity") * 100) / 100,
  0.9,
);
expectEq("text", ui.getElementProperty("styledBox", "text"), "Styled");
expectEq("fontFace", ui.getElementProperty("styledBox", "fontFace"), "Arial");
expectEq("fontSize", ui.getElementProperty("styledBox", "fontSize"), 14);
expectEq("fontWeight", ui.getElementProperty("styledBox", "fontWeight"), 600);
expectEq(
  "fontColor",
  ui.getElementProperty("styledBox", "fontColor"),
  rgba("#FFFFFF"),
);
expectEq("labelGap", ui.getElementProperty("styledBox", "labelGap"), 10);
expectEq(
  "hoverBorderColor",
  ui.getElementProperty("styledBox", "hoverBorderColor"),
  rgba("#60A5FA"),
);
expectEq(
  "disabledBoxColor",
  ui.getElementProperty("styledBox", "disabledBoxColor"),
  rgba("#27272A"),
);
expectEq(
  "disabledCheckColor",
  ui.getElementProperty("styledBox", "disabledCheckColor"),
  rgba("#52525B"),
);
expectEq(
  "disabledTextColor",
  ui.getElementProperty("styledBox", "disabledTextColor"),
  rgba("#3F3F46"),
);

// Defaults
expectEq(
  "default triState is false",
  ui.getElementProperty("testBox", "triState"),
  false,
);
expectEq(
  "default boxSize is auto (0)",
  ui.getElementProperty("testBox", "boxSize"),
  0,
);
expectEq(
  "default uncheckedBorderWidth",
  ui.getElementProperty("testBox", "uncheckedBorderWidth"),
  1.5,
);
expectEq(
  "default checkThickness",
  ui.getElementProperty("testBox", "checkThickness"),
  2,
);
expectEq(
  "default fontFace",
  ui.getElementProperty("testBox", "fontFace"),
  "Segoe UI",
);
expectEq("default fontSize", ui.getElementProperty("testBox", "fontSize"), 12);
expectEq(
  "default fontColor",
  ui.getElementProperty("testBox", "fontColor"),
  rgba("#E4E4E7"),
);
expectEq(
  "unset hoverBorderColor",
  ui.getElementProperty("testBox", "hoverBorderColor"),
  undefined,
);

// Disabled state
expectTrue(
  "disabledBox disabled",
  ui.getElementProperty("disabledBox", "disabled"),
);
expectFalse(
  "testBox not disabled",
  ui.getElementProperty("testBox", "disabled"),
);
ui.setElementProperty("testBox", "disabled", true);
expectTrue(
  "disabled after setElementProperty",
  ui.getElementProperty("testBox", "disabled"),
);
ui.setElementProperty("testBox", "disabled", false);
expectFalse("re-enabled", ui.getElementProperty("testBox", "disabled"));

// Runtime styling updates
ui.setElementProperties("testBox", {
  checkThickness: 3,
});
expectEq(
  "checkThickness after update",
  ui.getElementProperty("testBox", "checkThickness"),
  3,
);

console.log("----------------------------------------");
console.log("Starting CheckBox Methods Tests...");
console.log("----------------------------------------");

// setCheckBoxChecked / getCheckBoxChecked
changeLog = [];
expectTrue(
  "setCheckBoxChecked returns true",
  ui.setCheckBoxChecked("testBox", true),
);
expectTrue("checked after set true", ui.getCheckBoxChecked("testBox"));
expectEq("state after set true", ui.getCheckBoxState("testBox"), "checked");
expectTrue("onChange fired with 'true'", firedWith(changeLog, 0, "true"));

ui.setCheckBoxChecked("testBox", false);
expectFalse("checked after set false", ui.getCheckBoxChecked("testBox"));
expectTrue("onChange fired with 'false'", firedWith(changeLog, 1, "false"));

// Setting the same value must not fire onChange again
ui.setCheckBoxChecked("testBox", false);
expectEq("no duplicate onChange for same value", changeLog.length, 2);

// String "indeterminate" sets the third state
ui.setCheckBoxChecked("testBox", "indeterminate");
expectTrue("indeterminate reports checked", ui.getCheckBoxChecked("testBox"));
expectEq(
  "state after indeterminate",
  ui.getCheckBoxState("testBox"),
  "indeterminate",
);
expectTrue(
  "onChange fired with 'indeterminate'",
  firedWith(changeLog, 2, "indeterminate"),
);

// toggleCheckBox on a two-state box flips checked/unchecked only
ui.setCheckBoxChecked("testBox", false);
changeLog = [];
const toggled1 = ui.toggleCheckBox("testBox");
expectTrue("toggleCheckBox returns true", toggled1);
expectEq(
  "two-state toggle to checked",
  ui.getCheckBoxState("testBox"),
  "checked",
);
ui.toggleCheckBox("testBox");
expectEq(
  "two-state toggle skips indeterminate",
  ui.getCheckBoxState("testBox"),
  "unchecked",
);
expectTrue(
  "two-state cycle fired true then false",
  changeLog.length === 2 &&
    !!changeLog[0] &&
    changeLog[0].data === "true" &&
    !!changeLog[1] &&
    changeLog[1].data === "false",
);

// Tri-state cycling via toggleCheckBox: unchecked -> checked -> indeterminate
changeLog = [];
ui.toggleCheckBox("triBox");
expectEq("tri toggle to checked", ui.getCheckBoxState("triBox"), "checked");
ui.toggleCheckBox("triBox");
expectEq(
  "tri toggle to indeterminate",
  ui.getCheckBoxState("triBox"),
  "indeterminate",
);
ui.toggleCheckBox("triBox");
expectEq("tri toggle clears", ui.getCheckBoxState("triBox"), "unchecked");
expectTrue(
  "tri cycle fired three events",
  changeLog.length === 3 &&
    !!changeLog[0] &&
    changeLog[0].data === "true" &&
    !!changeLog[1] &&
    changeLog[1].data === "indeterminate" &&
    !!changeLog[2] &&
    changeLog[2].data === "false",
);

// Programmatic indeterminate still works on a two-state box, and the next
// click from it lands on unchecked.
ui.setCheckBoxChecked("testBox", "indeterminate");
expectTrue(
  "two-state box accepts programmatic indeterminate",
  ui.getCheckBoxChecked("testBox"),
);
expectEq(
  "state after programmatic indeterminate",
  ui.getCheckBoxState("testBox"),
  "indeterminate",
);
ui.toggleCheckBox("testBox");
expectEq(
  "toggle from indeterminate clears",
  ui.getCheckBoxState("testBox"),
  "unchecked",
);

// Callback-less box still changes state silently
const silentBefore = changeLog.length;
ui.toggleCheckBox("silentBox");
expectTrue(
  "silent box flipped without callback",
  ui.getCheckBoxChecked("silentBox"),
);
expectEq("silent toggle fired no events", changeLog.length, silentBefore);

// Disabled boxes ignore toggle
ui.toggleCheckBox("disabledBox");
expectFalse(
  "disabled box ignores toggle",
  ui.getCheckBoxChecked("disabledBox"),
);

// Unknown ids return false / null and do not crash
expectFalse(
  "setCheckBoxChecked unknown id",
  ui.setCheckBoxChecked("nope", true),
);
expectFalse("getCheckBoxChecked unknown id", ui.getCheckBoxChecked("nope"));
expectEq("getCheckBoxState unknown id", ui.getCheckBoxState("nope"), null);
expectFalse("toggleCheckBox unknown id", ui.toggleCheckBox("nope"));

// triState round-trip through setElementProperty
ui.setElementProperty("testBox", "triState", true);
expectTrue(
  "triState after set true",
  ui.getElementProperty("testBox", "triState"),
);
ui.setElementProperty("testBox", "triState", false);
expectFalse(
  "triState after set false",
  ui.getElementProperty("testBox", "triState"),
);

// setElementProperties imperative checked
ui.setElementProperties("testBox", { checked: true });
expectTrue(
  "checked via setElementProperties",
  ui.getElementProperty("testBox", "checked"),
);
expectEq(
  "imperative checked fires onChange",
  changeLog[changeLog.length - 1].data,
  "true",
);

ui.setElementProperties("testBox", { checked: "indeterminate" });
expectEq(
  "string checked via setElementProperties",
  ui.getCheckBoxState("testBox"),
  "indeterminate",
);

ui.setElementProperties("testBox", { checked: false });
expectFalse(
  "unchecked via setElementProperties",
  ui.getElementProperty("testBox", "checked"),
);

// Re-adding with the same id replaces cleanly and keeps new options
ui.addCheckBox({
  id: "testBox",
  x: 20,
  y: 55,
  checked: true,
  checkedColor: "#FF0000",
  onChange: (state) => {
    changeLog.push(state);
    console.log("testBox onChange fired with state: " + state.data);
  },
});
expectTrue("re-added box checked", ui.getCheckBoxChecked("testBox"));
expectEq(
  "re-added box color",
  ui.getElementProperty("testBox", "checkedColor"),
  rgba("#FF0000"),
);

console.log("----------------------------------------");
console.log("Starting CheckBox Update Tests...");
console.log("----------------------------------------");

ui.beginUpdate();
ui.setElementProperties("testBox", { checked: false });
ui.setElementProperties("testBox", { checked: true });
ui.endUpdate();
expectTrue(
  "checked after batched update",
  ui.getElementProperty("testBox", "checked"),
);
expectEq("state settled", ui.getElementProperty("testBox", "state"), "checked");

console.log("----------------------------------------");
console.log("=== All Tests Completed ===");
console.log("----------------------------------------");
