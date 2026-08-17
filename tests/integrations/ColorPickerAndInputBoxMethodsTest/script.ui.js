function pass(name, details) {
    console.log("[PASS] " + name + (details ? " -> " + details : ""));
}

function fail(name, details) {
    console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}

function expectEq(name, actual, expected) {
    if (actual === expected) {
        pass(name, "expected=" + JSON.stringify(expected) + " actual=" + JSON.stringify(actual));
    } else {
        fail(name, "expected=" + JSON.stringify(expected) + " actual=" + JSON.stringify(actual));
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

ui.beginUpdate();

// Title
ui.addText({
    id: "title",
    x: 20, y: 15,
    text: "ColorPicker & InputBox Methods Integration Test",
    fontSize: 16,
    fontFace: "Segoe UI",
    fontWeight: 700,
    fontColor: "#a6e3a1",
});

// Input Box for testing
ui.addInputBox({
    id: "testInput",
    x: 20, y: 55,
    width: 260, height: 36,
    placeholder: "Type here...",
    text: "Hello World",
    fontSize: 14,
    fillColor: "#313244",
    fontColor: "#cdd6f4",
    borderColor: "#45475a",
    borderFocusColor: "#89b4fa",
    borderWidth: 2,
    borderRadius: 6,
    padding: 8,
    onChange: (text) => { console.log("[InputBox onChange]:", text); },
    onFocus: () => { console.log("[InputBox onFocus]"); },
    onBlur: () => { console.log("[InputBox onBlur]"); },
    onEnter: (text) => { console.log("[InputBox onEnter]:", text); },
});

// Color Picker for testing
ui.addColorPicker({
    id: "testPicker",
    x: 300, y: 55,
    width: 36, height: 36,
    color: "#ff5500",
    borderRadius: 6,
    borderWidth: 2,
    borderColor: "#45475a",
    onChange: (color) => { console.log("[ColorPicker onChange]:", color); },
    onOpen: (color) => { console.log("[ColorPicker onOpen]:", color); },
    onClose: (color) => { console.log("[ColorPicker onClose]:", color); },
    onCancel: (origColor) => { console.log("[ColorPicker onCancel]:", origColor); },
    onEyedropperOpen: () => { console.log("[ColorPicker onEyedropperOpen]"); },
    onEyedropperPick: (color) => { console.log("[ColorPicker onEyedropperPick]:", color); },
});

ui.endUpdate();

console.log("----------------------------------------");
console.log("Starting InputBox Tests...");
console.log("----------------------------------------");

// // 1. Test InputBox Text Methods
// expectEq("InputBox initial text", ui.getInputBoxText("testInput"), "Hello World");

// ui.setInputBoxText("testInput", "Testing 123");
// expectEq("InputBox after setInputBoxText", ui.getInputBoxText("testInput"), "Testing 123");

// // 2. Test InputBox Selection Methods
// ui.selectInputBoxText("testInput");
// expectTrue("InputBox hasSelection after selectInputBoxText", ui.getElementProperty("testInput", "hasSelection"));
// expectEq("InputBox getSelectedText", ui.getInputBoxSelectedText("testInput"), "Testing 123");

// ui.replaceInputBoxSelection("testInput", "Replaced");
// expectEq("InputBox after replaceInputBoxSelection", ui.getInputBoxText("testInput"), "Replaced");

// ui.clearInputBoxSelection("testInput");
// expectFalse("InputBox hasSelection after clearInputBoxSelection", ui.getElementProperty("testInput", "hasSelection"));

// // 3. Test InputBox Focus / Blur Methods
// ui.focusInputBox("testInput");
// expectTrue("InputBox isInputBoxFocused after focusInputBox", ui.isInputBoxFocused("testInput"));
// expectTrue("InputBox focused property", ui.getElementProperty("testInput", "focused"));

// ui.blurInputBox("testInput");
// expectFalse("InputBox isInputBoxFocused after blurInputBox", ui.isInputBoxFocused("testInput"));
// expectFalse("InputBox focused property after blur", ui.getElementProperty("testInput", "focused"));

// // 4. Test InputBox Clear Method
// ui.clearInputBox("testInput");
// expectEq("InputBox text after clearInputBox", ui.getInputBoxText("testInput"), "");

// // 5. Test InputBox via setElementProperty / getElementProperty
// ui.setElementProperty("testInput", "text", "Dynamic Text");
// expectEq("InputBox getElementProperty(text)", ui.getElementProperty("testInput", "text"), "Dynamic Text");

// ui.setElementProperty("testInput", "focused", true);
// expectTrue("InputBox getElementProperty(focused) after setElementProperty", ui.getElementProperty("testInput", "focused"));

// ui.setElementProperty("testInput", "focused", false);
// expectFalse("InputBox getElementProperty(focused) false", ui.getElementProperty("testInput", "focused"));

// console.log("----------------------------------------");
// console.log("Starting ColorPicker Tests...");
// console.log("----------------------------------------");

// // 1. Test ColorPicker Color Methods & Properties
// expectEq("ColorPicker initial color", ui.getColorPickerColor("testPicker"), "#FF5500");
// expectEq("ColorPicker getElementProperty(color)", ui.getElementProperty("testPicker", "color"), "#FF5500");

// ui.setColorPickerColor("testPicker", "#3498DB");
// expectEq("ColorPicker after setColorPickerColor", ui.getColorPickerColor("testPicker"), "#3498DB");

// ui.setElementProperty("testPicker", "color", "#00FF88");
// expectEq("ColorPicker after setElementProperty(color)", ui.getColorPickerColor("testPicker"), "#00FF88");

// // 2. Test ColorPicker Popup Open / Close / isOpen Methods & Properties
// expectFalse("ColorPicker isColorPickerOpen initial", ui.isColorPickerOpen("testPicker"));
// expectFalse("ColorPicker getElementProperty(isOpen) initial", ui.getElementProperty("testPicker", "isOpen"));

// // Open color picker
// const openResult = ui.openColorPicker("testPicker");
// expectTrue("ColorPicker openColorPicker return value", openResult);
// expectTrue("ColorPicker isColorPickerOpen after open", ui.isColorPickerOpen("testPicker"));
// expectTrue("ColorPicker getElementProperty(isOpen) after open", ui.getElementProperty("testPicker", "isOpen"));

// // Close color picker
// const closeResult = ui.closeColorPicker();
// expectTrue("ColorPicker closeColorPicker return value", closeResult);
// expectFalse("ColorPicker isColorPickerOpen after close", ui.isColorPickerOpen("testPicker"));
// expectFalse("ColorPicker getElementProperty(isOpen) after close", ui.getElementProperty("testPicker", "isOpen"));

// // Open color picker via setElementProperty
// ui.setElementProperty("testPicker", "isOpen", true);
// expectTrue("ColorPicker isColorPickerOpen after setElementProperty(isOpen, true)", ui.isColorPickerOpen("testPicker"));

// // Close color picker via setElementProperty
// ui.setElementProperty("testPicker", "isOpen", false);
// expectFalse("ColorPicker isColorPickerOpen after setElementProperty(isOpen, false)", ui.isColorPickerOpen("testPicker"));

// 3. Test ColorPicker Eyedropper launch method
const eyedropperResult = ui.openColorPickerEyedropper("testPicker");
expectTrue("ColorPicker openColorPickerEyedropper return value", eyedropperResult);
expectTrue("ColorPicker isOpen after openColorPickerEyedropper", ui.isColorPickerOpen("testPicker"));

// // Clean up: close popup
// ui.closeColorPicker();
// expectFalse("ColorPicker isOpen after cleanup close", ui.isColorPickerOpen("testPicker"));

// console.log("----------------------------------------");
// console.log("=== All Tests Completed Successfully ===");
// console.log("----------------------------------------");
