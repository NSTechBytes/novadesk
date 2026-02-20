var hotkeys = require("hotkeys");

var hotkeyId = hotkeys.registerHotkey("Win+D", function() {
    console.log("Win+D pressed");
});

var hotkeyId2 = hotkeys.registerHotkey("Space", {
    onKeyDown: function() {
        console.log("SPACE DOWN (Global)");
    },
    onKeyUp: function() {
        console.log("SPACE UP (Global)");
    }
});

setTimeout(function() {
    // Later, unregister the hotkey
    hotkeys.unregisterHotkey(hotkeyId);
    console.log("Hotkey Id unregistered");
    hotkeys.unregisterHotkey(hotkeyId2);
    console.log("Hotkey Id2 unregistered");
}, 5000);
