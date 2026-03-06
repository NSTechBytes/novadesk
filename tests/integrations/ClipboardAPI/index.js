import { app } from "novadesk";
import * as system from "system";

console.log("=========================================");
console.log("   Novadesk Clipboard API Integration");
console.log("=========================================");

function call(name, fn) {
    try {
        var out = fn();
        console.log("[PASS] " + name + ": " + JSON.stringify(out));
        return out;
    } catch (err) {
        console.error("[FAIL] " + name + ": " + String(err));
        return null;
    }
}

var original = call("clipboard.getText()", function () {
    return system.clipboard.getText();
});

call("clipboard.setText('Hello from Novadesk!')", function () {
    return system.clipboard.setText("Hello from Novadesk!");
});

call("clipboard.getText() after basic set", function () {
    return system.clipboard.getText();
});

var specialText = "Special chars: @#$%^&*()_+-=[]{}|;':\\\",./<>?";
call("clipboard.setText(special chars)", function () {
    return system.clipboard.setText(specialText);
});

var verifySpecial = call("clipboard.getText() verify special", function () {
    return system.clipboard.getText();
});
console.log("[INFO] special match: " + (verifySpecial === specialText));

var unicodeText = "Unicode: αβγδε 中文 русский العربية";
call("clipboard.setText(unicode)", function () {
    return system.clipboard.setText(unicodeText);
});

var verifyUnicode = call("clipboard.getText() verify unicode", function () {
    return system.clipboard.getText();
});
console.log("[INFO] unicode match: " + (verifyUnicode === unicodeText));

call("clipboard.setText('')", function () {
    return system.clipboard.setText("");
});

call("clipboard.getText() after empty set", function () {
    return system.clipboard.getText();
});

if (typeof original === "string") {
    call("clipboard.setText(original) [restore]", function () {
        return system.clipboard.setText(original);
    });
}

console.log("=== Clipboard API Integration Complete ===");
app.exit();
