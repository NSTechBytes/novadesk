// Clipboard API Test
// Tests clipper.getClipboardText and clipper.setClipboardText

console.log("=== Clipboard API Test Started ===");
var clipper = require("clipper");

// Test 1: Get current clipboard content
console.log("Test 1: Get current clipboard");
var currentClipboard = clipper.getClipboardText();
console.log("Current clipboard: " + (currentClipboard || "(empty)"));

// Test 2: Set clipboard text
console.log("Test 2: Set clipboard text");
var setResult = clipper.setClipboardText("Hello from Novadesk!");
console.log("Set clipboard result: " + setResult);

// Test 3: Get clipboard after setting
console.log("Test 3: Get clipboard after setting");
var newClipboard = clipper.getClipboardText();
console.log("New clipboard content: " + newClipboard);

// Test 4: Set clipboard with special characters
console.log("Test 4: Set clipboard with special characters");
var specialText = "Special chars: @#$%^&*()_+-=[]{}|;':\",./<>?";
clipper.setClipboardText(specialText);
var verifySpecial = clipper.getClipboardText();
console.log("Special chars verification: " + (verifySpecial === specialText ? "✓ Match" : "✗ Mismatch"));

// Test 5: Set clipboard with unicode
console.log("Test 5: Set clipboard with unicode");
var unicodeText = "Unicode: αβγδε 中文 русский العربية";
clipper.setClipboardText(unicodeText);
var verifyUnicode = clipper.getClipboardText();
console.log("Unicode verification: " + (verifyUnicode === unicodeText ? "✓ Match" : "✗ Mismatch"));

// Test 6: Set empty clipboard
console.log("Test 6: Set empty clipboard");
clipper.setClipboardText("");
var emptyResult = clipper.getClipboardText();
console.log("Empty clipboard result: '" + emptyResult + "'");

console.log("=== Clipboard API Test Completed ===");
