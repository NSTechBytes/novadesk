// Integration test for the drop-down element.
//
// Note: opening the menu needs a real click, so everything about the popup
// window itself (hover highlight, wheel/thumb scrolling, click-to-select,
// Escape cancel, tracking a moved/resized widget, flipping near the screen
// bottom, onOpen/onClose/onCancel) is verified manually. This suite covers
// creation, the option/value model, selection clamping, property round-trips,
// programmatic methods and onChange.

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

// onChange/onOpen receive the event object; the value string is on e.data.
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
  text: "Drop-Down Integration Test",
  fontSize: 16,
  fontFace: "Segoe UI",
  fontWeight: 700,
  fontColor: "#a6e3a1",
});

let changeLog = [];
let openLog = [];
let closeLog = [];
let cancelLog = [];

// Default drop-down: no options, nothing selected.
ui.addDropDown({
  id: "bareDropDown",
  x: 20,
  y: 55,
});

// Plain-string options.
ui.addDropDown({
  id: "stringOptions",
  x: 20,
  y: 95,
  width: 180,
  options: ["Small", "Medium", "Large"],
  selectedIndex: 1,
});

// { label, value } options: onChange reports the value, not the label.
ui.addDropDown({
  id: "valueOptions",
  x: 20,
  y: 130,
  width: 180,
  options: [
    { label: "Low", value: "1" },
    { label: "Balanced", value: "1.5" },
    { label: "High", value: "2" },
  ],
  onChange: (e) => {
    changeLog.push(e);
    console.log("valueOptions onChange fired with: " + e.data);
  },
  onOpen: (e) => {
    openLog.push(e);
  },
  onClose: (e) => {
    closeLog.push(e);
  },
  onCancel: (e) => {
    cancelLog.push(e);
  },
});

// Fully styled drop-down.
ui.addDropDown({
  id: "styledDropDown",
  x: 20,
  y: 165,
  width: 200,
  height: 32,
  options: ["One", "Two", "Three", "Four"],
  selectedValue: "Three",
  placeholder: "Pick one",
  backgroundColor: "#2D2D36",
  borderWidth: 2,
  borderColor: "#3F3F46",
  borderRadius: 8,
  paddingLeft: 12,
  paddingRight: 12,
  chevronColor: "#C8C8D0",
  chevronSize: 7,
  chevronGap: 8,
  fontFace: "Segoe UI",
  fontSize: 13,
  fontWeight: 600,
  fontColor: "#E4E4E7",
  placeholderColor: "#8C8C96",
  maxDisplayLength: 20,
  hoverBorderColor: "#5A5A68",
  openBorderColor: "#3B82F6",
  disabledBackgroundColor: "#282830",
  disabledBorderColor: "#3C3C44",
  disabledTextColor: "#73737D",
  popupBackground: "#24242C",
  popupBorderColor: "#3F3F46",
  popupHoverColor: "#343440",
  popupSelectedColor: "#3B82F6",
  popupTextColor: "#E4E4E7",
  popupCheckColor: "#3B82F6",
  popupItemHeight: 30,
  popupMaxVisibleItems: 5,
  popupPadding: 6,
});

// Placeholder-only drop-down: unselected by design.
ui.addDropDown({
  id: "placeholderDropDown",
  x: 20,
  y: 205,
  width: 180,
  options: ["Alpha", "Beta"],
  placeholder: "Nothing chosen",
});

// Disabled drop-down.
ui.addDropDown({
  id: "disabledDropDown",
  x: 20,
  y: 240,
  width: 180,
  options: ["Yes", "No"],
  selectedIndex: 0,
  disabled: true,
});

// Callback-less drop-down.
ui.addDropDown({
  id: "silentDropDown",
  x: 20,
  y: 275,
  width: 180,
  options: ["A", "B", "C"],
});

// Duplicate values: resolution must take the first match.
ui.addDropDown({
  id: "dupeDropDown",
  x: 20,
  y: 310,
  width: 180,
  options: [
    { label: "First", value: "same" },
    { label: "Second", value: "same" },
  ],
});

// Long list, for the manual scroll checks.
ui.addDropDown({
  id: "longDropDown",
  x: 20,
  y: 345,
  width: 180,
  options: [
    "Item 1", "Item 2", "Item 3", "Item 4", "Item 5", "Item 6", "Item 7",
    "Item 8", "Item 9", "Item 10", "Item 11", "Item 12",
  ],
  popupMaxVisibleItems: 6,
});

// Drop-down inside a layout box via elementType.
ui.addLayoutBox({
  id: "boxDropDowns",
  x: 20,
  y: 385,
  width: 240,
  height: 40,
  direction: "row",
  gap: 10,
  children: [
    {
      elementType: "dropdown",
      id: "childDropDown",
      width: 140,
      options: ["Child A", "Child B"],
      selectedIndex: 1,
    },
  ],
});

ui.endUpdate();

console.log("----------------------------------------");
console.log("Starting Drop-Down Creation Tests...");
console.log("----------------------------------------");

expectEq("default width", ui.getElementProperty("bareDropDown", "width"), 160);
expectEq("default height", ui.getElementProperty("bareDropDown", "height"), 28);
expectEq("no options", ui.getElementProperty("bareDropDown", "optionCount"), 0);
expectEq("nothing selected", ui.getDropDownIndex("bareDropDown"), -1);
expectEq("empty value when unselected", ui.getDropDownValue("bareDropDown"), "");
expectEq("empty label when unselected", ui.getDropDownLabel("bareDropDown"), "");
expectEq("string option count", ui.getElementProperty("stringOptions", "optionCount"), 3);
expectEq("selectedIndex round-trip", ui.getElementProperty("stringOptions", "selectedIndex"), 1);
expectEq("selectedValue round-trip", ui.getElementProperty("stringOptions", "selectedValue"), "Medium");
expectEq("selectedLabel round-trip", ui.getElementProperty("stringOptions", "selectedLabel"), "Medium");
expectEq("plain string makes value equal label", ui.getDropDownValue("stringOptions"), "Medium");
expectEq("object options start unselected", ui.getDropDownIndex("valueOptions"), -1);
expectEq("styled selectedValue resolves to index", ui.getElementProperty("styledDropDown", "selectedIndex"), 2);
expectEq("styled selectedLabel", ui.getElementProperty("styledDropDown", "selectedLabel"), "Three");
expectEq("layout child created", ui.getElementProperty("childDropDown", "selectedIndex"), 1);
expectEq("popup not open", ui.isDropDownOpen(), false);
// A numeric option value is coerced to the text the script wrote, so a
// selectedValue lookup finds it.
expectTrue("numeric option value resolves by string", ui.setDropDownValue("valueOptions", "1.5"));
expectEq("numeric value index", ui.getDropDownIndex("valueOptions"), 1);
expectEq("numeric value label", ui.getDropDownLabel("valueOptions"), "Balanced");

console.log("----------------------------------------");
console.log("Starting Drop-Down Property Tests...");
console.log("----------------------------------------");

// Defaults
expectEq("default backgroundColor", ui.getElementProperty("bareDropDown", "backgroundColor"), rgba("#2D2D36"));
expectEq("default borderColor", ui.getElementProperty("bareDropDown", "borderColor"), rgba("#3F3F46"));
expectEq("default borderWidth", ui.getElementProperty("bareDropDown", "borderWidth"), 1);
expectEq("default borderRadius", ui.getElementProperty("bareDropDown", "borderRadius"), 6);
expectEq("default opacity", Math.round(ui.getElementProperty("bareDropDown", "opacity") * 100) / 100, 1);
expectEq("default paddingLeft", ui.getElementProperty("bareDropDown", "paddingLeft"), 10);
expectEq("default paddingRight", ui.getElementProperty("bareDropDown", "paddingRight"), 10);
expectEq("default chevronColor", ui.getElementProperty("bareDropDown", "chevronColor"), rgba("#C8C8D0"));
expectEq("default chevronSize", ui.getElementProperty("bareDropDown", "chevronSize"), 6);
expectEq("default chevronGap", ui.getElementProperty("bareDropDown", "chevronGap"), 10);
expectEq("default fontFace", ui.getElementProperty("bareDropDown", "fontFace"), "Segoe UI");
expectEq("default fontSize", ui.getElementProperty("bareDropDown", "fontSize"), 12);
expectEq("default fontWeight", ui.getElementProperty("bareDropDown", "fontWeight"), 400);
expectEq("default fontColor", ui.getElementProperty("bareDropDown", "fontColor"), rgba("#E4E4E7"));
expectEq("default placeholderColor", ui.getElementProperty("bareDropDown", "placeholderColor"), rgba("#8C8C96"));
expectEq("default maxDisplayLength", ui.getElementProperty("bareDropDown", "maxDisplayLength"), 0);
expectEq("default placeholder empty", ui.getElementProperty("bareDropDown", "placeholder"), "");
expectEq("default hoverBorderColor", ui.getElementProperty("bareDropDown", "hoverBorderColor"), rgba("#5A5A68"));
expectEq("default openBorderColor", ui.getElementProperty("bareDropDown", "openBorderColor"), rgba("#3B82F6"));
expectEq("default disabledBackgroundColor", ui.getElementProperty("bareDropDown", "disabledBackgroundColor"), rgba("#282830"));
expectEq("default disabledBorderColor", ui.getElementProperty("bareDropDown", "disabledBorderColor"), rgba("#3C3C44"));
expectEq("default disabledTextColor", ui.getElementProperty("bareDropDown", "disabledTextColor"), rgba("#73737D"));
expectEq("default popupBackground", ui.getElementProperty("bareDropDown", "popupBackground"), rgba("#24242C"));
expectEq("default popupBorderColor", ui.getElementProperty("bareDropDown", "popupBorderColor"), rgba("#3F3F46"));
expectEq("default popupHoverColor", ui.getElementProperty("bareDropDown", "popupHoverColor"), rgba("#343440"));
expectEq("default popupSelectedColor", ui.getElementProperty("bareDropDown", "popupSelectedColor"), rgba("#3B82F6"));
expectEq("default popupTextColor", ui.getElementProperty("bareDropDown", "popupTextColor"), rgba("#E4E4E7"));
expectEq("default popupCheckColor", ui.getElementProperty("bareDropDown", "popupCheckColor"), rgba("#3B82F6"));
expectEq("default popupItemHeight", ui.getElementProperty("bareDropDown", "popupItemHeight"), 28);
expectEq("default popupMaxVisibleItems", ui.getElementProperty("bareDropDown", "popupMaxVisibleItems"), 8);
expectEq("default popupPadding", ui.getElementProperty("bareDropDown", "popupPadding"), 4);

// Styled round-trip
expectEq("styled height", ui.getElementProperty("styledDropDown", "height"), 32);
expectEq("styled borderWidth", ui.getElementProperty("styledDropDown", "borderWidth"), 2);
expectEq("styled borderRadius", ui.getElementProperty("styledDropDown", "borderRadius"), 8);
expectEq("styled paddingLeft", ui.getElementProperty("styledDropDown", "paddingLeft"), 12);
expectEq("styled chevronSize", ui.getElementProperty("styledDropDown", "chevronSize"), 7);
expectEq("styled chevronGap", ui.getElementProperty("styledDropDown", "chevronGap"), 8);
expectEq("styled fontSize", ui.getElementProperty("styledDropDown", "fontSize"), 13);
expectEq("styled fontWeight", ui.getElementProperty("styledDropDown", "fontWeight"), 600);
expectEq("styled placeholder", ui.getElementProperty("styledDropDown", "placeholder"), "Pick one");
expectEq("styled maxDisplayLength", ui.getElementProperty("styledDropDown", "maxDisplayLength"), 20);
expectEq("styled popupItemHeight", ui.getElementProperty("styledDropDown", "popupItemHeight"), 30);
expectEq("styled popupMaxVisibleItems", ui.getElementProperty("styledDropDown", "popupMaxVisibleItems"), 5);
expectEq("styled popupPadding", ui.getElementProperty("styledDropDown", "popupPadding"), 6);
expectEq("long list visible cap", ui.getElementProperty("longDropDown", "popupMaxVisibleItems"), 6);
expectEq("long list option count", ui.getElementProperty("longDropDown", "optionCount"), 12);

// options array shape
const opts = ui.getElementProperty("valueOptions", "options");
expectEq("options is an array of 3", opts && opts.length, 3);
expectEq("options[0].label", opts && opts[0].label, "Low");
expectEq("options[0].value", opts && opts[0].value, "1");
expectEq("options[1].value", opts && opts[1].value, "1.5");
const strOpts = ui.getElementProperty("stringOptions", "options");
expectEq("string options keep label", strOpts && strOpts[0].label, "Small");
expectEq("string options copy into value", strOpts && strOpts[0].value, "Small");
const bareOpts = ui.getElementProperty("bareDropDown", "options");
expectEq("no options returns empty array", bareOpts && bareOpts.length, 0);

// An empty options list cannot hold a selection: the index stays -1.
ui.setElementProperties("bareDropDown", { selectedIndex: 3 });
expectEq("index ignored without options", ui.getDropDownIndex("bareDropDown"), -1);

// Single-key updates
ui.setElementProperty("bareDropDown", "placeholder", "Choose");
expectEq("placeholder after update", ui.getElementProperty("bareDropDown", "placeholder"), "Choose");

ui.setElementProperties("bareDropDown", {
  borderColor: "#FF5500",
  borderRadius: 10,
  fontSize: 14,
});
expectEq("borderColor after update", ui.getElementProperty("bareDropDown", "borderColor"), rgba("#FF5500"));
expectEq("borderRadius after update", ui.getElementProperty("bareDropDown", "borderRadius"), 10);
expectEq("fontSize after update", ui.getElementProperty("bareDropDown", "fontSize"), 14);

// Options can be replaced and cleared
ui.setElementProperties("stringOptions", { options: ["Only"] });
expectEq("options replaced", ui.getElementProperty("stringOptions", "optionCount"), 1);
expectEq("index re-clamped after shrink", ui.getElementProperty("stringOptions", "selectedIndex"), 0);
ui.setElementProperties("stringOptions", { options: [] });
expectEq("options cleared", ui.getElementProperty("stringOptions", "optionCount"), 0);
expectEq("index cleared with options", ui.getElementProperty("stringOptions", "selectedIndex"), -1);
ui.setElementProperties("stringOptions", {
  options: ["Small", "Medium", "Large"],
  selectedIndex: 1,
});

// Disabled state round-trip
expectTrue("disabledDropDown disabled", ui.getElementProperty("disabledDropDown", "disabled"));
expectFalse("styledDropDown not disabled", ui.getElementProperty("styledDropDown", "disabled"));
ui.setElementProperty("styledDropDown", "disabled", true);
expectTrue("disabled after setElementProperty", ui.getElementProperty("styledDropDown", "disabled"));
ui.setElementProperty("styledDropDown", "disabled", false);
expectFalse("re-enabled", ui.getElementProperty("styledDropDown", "disabled"));

// The hand cursor is the default for an interactive drop-down
expectTrue("default mouse cursor", ui.getElementProperty("bareDropDown", "mouseEventCursor"));
expectEq("default cursor name", ui.getElementProperty("bareDropDown", "mouseEventCursorName"), "hand");

console.log("----------------------------------------");
console.log("Starting Drop-Down Methods Tests...");
console.log("----------------------------------------");

changeLog = [];
openLog = [];
closeLog = [];
cancelLog = [];

// setDropDownIndex / getDropDownIndex
expectTrue("setDropDownIndex returns true", ui.setDropDownIndex("valueOptions", 1));
expectEq("index after set", ui.getDropDownIndex("valueOptions"), 1);
expectEq("value after set", ui.getDropDownValue("valueOptions"), "1.5");
expectEq("label after set", ui.getDropDownLabel("valueOptions"), "Balanced");
expectTrue("onChange fired with the value string", firedWith(changeLog, 0, "1.5"));

// Clamping: out-of-range indexes clamp rather than failing
ui.setDropDownIndex("valueOptions", 99);
expectEq("clamped to last", ui.getDropDownIndex("valueOptions"), 2);
expectTrue("onChange fired with '2'", firedWith(changeLog, 1, "2"));

ui.setDropDownIndex("valueOptions", -5);
expectEq("clamped to -1", ui.getDropDownIndex("valueOptions"), -1);
expectEq("value cleared", ui.getDropDownValue("valueOptions"), "");
expectTrue("onChange fired with ''", firedWith(changeLog, 2, ""));

// Same-index set stays silent
const silentCount = changeLog.length;
ui.setDropDownIndex("valueOptions", -1);
expectEq("no duplicate onChange for same index", changeLog.length, silentCount);

// setDropDownValue resolves by value string
changeLog = [];
expectTrue("setDropDownValue by string", ui.setDropDownValue("valueOptions", "1"));
expectEq("value resolved", ui.getDropDownValue("valueOptions"), "1");
expectEq("index resolved", ui.getDropDownIndex("valueOptions"), 0);
expectTrue("onChange fired with '1'", firedWith(changeLog, 0, "1"));

// Unknown value is a silent no-op
changeLog = [];
expectFalse("unknown value rejected", ui.setDropDownValue("valueOptions", "no-such-value"));
expectEq("index untouched by unknown value", ui.getDropDownIndex("valueOptions"), 0);
expectEq("unknown value fired no event", changeLog.length, 0);

// Duplicate values resolve to the first match
expectTrue("duplicate value selects first", ui.setDropDownValue("dupeDropDown", "same"));
expectEq("first match wins", ui.getDropDownIndex("dupeDropDown"), 0);
expectEq("first label shown", ui.getDropDownLabel("dupeDropDown"), "First");

// A number argument is treated as an index
expectTrue("setDropDownValue with a number", ui.setDropDownValue("valueOptions", 2));
expectEq("number treated as index", ui.getDropDownIndex("valueOptions"), 2);

// Programmatic sets still work on a disabled drop-down
ui.setDropDownIndex("disabledDropDown", 1);
expectEq("disabled drop-down accepts programmatic set", ui.getDropDownIndex("disabledDropDown"), 1);

// Callback-less drop-down changes silently
changeLog = [];
ui.setDropDownIndex("silentDropDown", 2);
expectEq("silent drop-down moved", ui.getDropDownIndex("silentDropDown"), 2);
expectEq("silent set fired no events", changeLog.length, 0);

// An unselected drop-down reports an empty string, not null
expectEq("unselected value is empty string", ui.getDropDownValue("placeholderDropDown"), "");
expectEq("unselected index is -1", ui.getDropDownIndex("placeholderDropDown"), -1);

// Open/close helpers report without a click
expectFalse("isDropDownOpen(id) false before any click", ui.isDropDownOpen("valueOptions"));
expectFalse("closeDropDown with nothing open still succeeds", ui.closeDropDown());
expectFalse("openDropDown on a disabled drop-down", ui.openDropDown("disabledDropDown"));
expectFalse("openDropDown on an unknown id", ui.openDropDown("nope"));

// Unknown ids return false / undefined and do not crash
expectFalse("setDropDownIndex unknown id", ui.setDropDownIndex("nope", 0));
expectFalse("setDropDownValue unknown id", ui.setDropDownValue("nope", "x"));
expectEq("getDropDownValue unknown id", ui.getDropDownValue("nope"), undefined);
expectEq("getDropDownIndex unknown id", ui.getDropDownIndex("nope"), undefined);
expectEq("getDropDownLabel unknown id", ui.getDropDownLabel("nope"), undefined);
expectEq("getDropDownOptionCount unknown id", ui.getDropDownOptionCount("nope"), undefined);
expectEq("getElementProperty unknown id", ui.getElementProperty("nope", "selectedIndex"), undefined);

// setElementProperties imperative selection
changeLog = [];
ui.setElementProperties("valueOptions", { selectedIndex: 0 });
expectEq("index via setElementProperties", ui.getDropDownIndex("valueOptions"), 0);
expectTrue("imperative index fires onChange", firedWith(changeLog, 0, "1"));

changeLog = [];
ui.setElementProperties("valueOptions", { selectedValue: "2" });
expectEq("value via setElementProperties", ui.getDropDownValue("valueOptions"), "2");
expectTrue("imperative value fires onChange", firedWith(changeLog, 0, "2"));

// Setting the current selection via properties stays silent
changeLog = [];
ui.setElementProperties("valueOptions", { selectedIndex: 1 });
ui.setElementProperties("valueOptions", { selectedIndex: 1 });
expectEq("same index via properties silent", changeLog.length, 1);

// Re-adding with the same id replaces cleanly and keeps new options
ui.addDropDown({
  id: "valueOptions",
  x: 20,
  y: 130,
  width: 180,
  options: ["Readded A", "Readded B"],
  selectedIndex: 1,
  borderColor: "#FF0000",
  onChange: (e) => {
    changeLog.push(e);
  },
});
expectEq("re-added index", ui.getDropDownIndex("valueOptions"), 1);
expectEq("re-added value", ui.getDropDownValue("valueOptions"), "Readded B");
expectEq("re-added option count", ui.getElementProperty("valueOptions", "optionCount"), 2);
expectEq("re-added color", ui.getElementProperty("valueOptions", "borderColor"), rgba("#FF0000"));

console.log("----------------------------------------");
console.log("=== All Tests Completed ===");
console.log("----------------------------------------");
