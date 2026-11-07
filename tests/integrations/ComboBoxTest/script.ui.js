// ComboBox Element Integration Test
console.log("[ComboBoxTest] Initializing UI script...");

// Title label
ui.addText({
    id: "titleText",
    x: 30,
    y: 20,
    text: "ComboBox Element Test Suite",
    fontFace: "Segoe UI",
    fontSize: 20,
    fontWeight: 600,
    fontColor: "rgba(255, 255, 255, 1.0)"
});

// Status label
ui.addText({
    id: "statusText",
    x: 30,
    y: 55,
    text: "Status: Ready",
    fontFace: "Segoe UI",
    fontSize: 13,
    fontColor: "rgba(180, 180, 195, 1.0)"
});

// Label for ComboBox 1
ui.addText({
    id: "labelCombo1",
    x: 30,
    y: 95,
    text: "1. Editable ComboBox (Freeform + Dropdown Choices):",
    fontFace: "Segoe UI",
    fontSize: 13,
    fontWeight: 500,
    fontColor: "rgba(220, 220, 230, 1.0)"
});

// ComboBox 1: Editable with options
ui.addComboBox({
    id: "combo1",
    x: 30,
    y: 125,
    width: 250,
    height: 32,
    placeholder: "Type or pick an option...",
    options: [
        { label: "Apple", value: "apple" },
        { label: "Banana", value: "banana" },
        { label: "Cherry", value: "cherry" },
        { label: "Date", value: "date" },
        { label: "Elderberry", value: "elderberry" }
    ],
    value: "apple",
    editable: true,
    filterOnType: true,
    backgroundColor: "rgba(40, 40, 50, 1.0)",
    borderColor: "rgba(70, 70, 85, 1.0)",
    borderFocusColor: "rgba(59, 130, 246, 1.0)",
    chevronColor: "rgba(200, 200, 215, 1.0)",
    fontSize: 13,
    onChange: function (val) {
        console.log("[combo1] onChange fired with value:", val);
        ui.updateElement("statusText", { text: "Status: combo1 onChange -> " + val });
    },
    onSelect: function (val) {
        console.log("[combo1] onSelect fired with value:", val);
        ui.updateElement("statusText", { text: "Status: combo1 onSelect -> " + val });
    },
    onTextChange: function (text) {
        console.log("[combo1] onTextChange fired with text:", text);
        ui.updateElement("statusText", { text: "Status: combo1 live text -> " + text });
    },
    onOpen: function () {
        console.log("[combo1] onOpen popup opened");
    },
    onClose: function () {
        console.log("[combo1] onClose popup closed");
    }
});

// Label for ComboBox 2
ui.addText({
    id: "labelCombo2",
    x: 30,
    y: 180,
    text: "2. Strict / Non-Editable ComboBox (Select from List Only):",
    fontFace: "Segoe UI",
    fontSize: 13,
    fontWeight: 500,
    fontColor: "rgba(220, 220, 230, 1.0)"
});

// ComboBox 2: Non-editable
ui.addComboBox({
    id: "combo2",
    x: 30,
    y: 210,
    width: 250,
    height: 32,
    placeholder: "Select an option...",
    options: [
        "Light Theme",
        "Dark Theme",
        "High Contrast",
        "System Default"
    ],
    editable: false,
    backgroundColor: "rgba(40, 40, 50, 1.0)",
    borderColor: "rgba(70, 70, 85, 1.0)",
    chevronColor: "rgba(200, 200, 215, 1.0)",
    fontSize: 13,
    onChange: function (val) {
        console.log("[combo2] onChange fired with value:", val);
        ui.updateElement("statusText", { text: "Status: combo2 selected -> " + val });
    }
});

// Label for ComboBox 3 (Right-to-Left / Custom Styling)
ui.addText({
    id: "labelCombo3",
    x: 30,
    y: 265,
    text: "3. RTL & Styled ComboBox:",
    fontFace: "Segoe UI",
    fontSize: 13,
    fontWeight: 500,
    fontColor: "rgba(220, 220, 230, 1.0)"
});

ui.addComboBox({
    id: "combo3",
    x: 30,
    y: 295,
    width: 250,
    height: 32,
    placeholder: "اختر خيارا...",
    rightToLeft: true,
    options: [
        { label: "خيار أول", value: "opt1" },
        { label: "خيار ثان", value: "opt2" },
        { label: "خيار ثالث", value: "opt3" }
    ],
    backgroundColor: "rgba(35, 35, 45, 1.0)",
    borderColor: "rgba(80, 80, 100, 1.0)",
    fontSize: 13,
    onChange: function (val) {
        console.log("[combo3] onChange:", val);
    }
});



// Label for ComboBox 4: City Search (No Chevron + Auto-Open on Type)
ui.addText({
    id: "labelCombo4",
    x: 30,
    y: 350,
    text: "4. City Search (No Chevron + Auto-Open on Type):",
    fontFace: "Segoe UI",
    fontSize: 13,
    fontWeight: 500,
    fontColor: "rgba(220, 220, 230, 1.0)"
});

ui.addComboBox({
    id: "cityCombo",
    x: 30,
    y: 380,
    width: 250,
    height: 32,
    placeholder: "Type a city (e.g. Lahore, Tokyo)...",
    editable: true,
    showChevron: false,
    openOnType: true,
    filterOnType: true,
    options: [
        { label: "Amsterdam", value: "AMS" },
        { label: "Berlin", value: "BER" },
        { label: "Dubai", value: "DXB" },
        { label: "Karachi", value: "KHI" },
        { label: "Lahore", value: "LHE" },
        { label: "London", value: "LHR" },
        { label: "New York", value: "NYC" },
        { label: "Paris", value: "PAR" },
        { label: "Tokyo", value: "TYO" }
    ],
    backgroundColor: "rgba(35, 40, 50, 1.0)",
    borderColor: "rgba(80, 100, 130, 1.0)",
    fontSize: 13,
    onChange: function (val) {
        console.log("[cityCombo] Selected city code:", val);
        ui.updateElement("statusText", { text: "Status: City selected -> " + val });
    }
});

ui.addComboBox({
    id: "cityCombo",
    x: 30,
    y: 400,
    width: 260,
    height: 34,
    placeholder: "Type a city (e.g. Lahore, Tokyo)...",
    
    // Key flags:
    editable: true,         // Allows typing in the box
    showChevron: false,     // Clean search-box look without chevron
    openOnType: true,       // Popup appears automatically as soon as you type
    filterOnType: true,     // Popup options filter in real time to match query

    options: [
        { label: "Amsterdam", value: "AMS" },
        { label: "Berlin",    value: "BER" },
        { label: "Dubai",     value: "DXB" },
        { label: "Karachi",   value: "KHI" },
        { label: "Lahore",    value: "LHE" },
        { label: "London",    value: "LHR" },
        { label: "New York",  value: "NYC" },
        { label: "Paris",     value: "PAR" },
        { label: "Tokyo",     value: "TYO" }
    ],

    onChange: function (val) {
        console.log("Selected city value:", val);
    }
});