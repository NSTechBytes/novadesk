// Addon API Test
// Tests system.loadAddon and system.unloadAddon

console.log("=== Addon API Test Started ===");

// Test 1: Try to load a non-existent addon
console.log("Test 1: Load non-existent addon");
var addon1 = system.loadAddon("./nonexistent.dll");
console.log("Non-existent addon result: " + addon1);

// Test 2: Load addon with relative path
console.log("Test 2: Load addon with relative path");
var addon2 = system.loadAddon("../test_addon.dll");
console.log("Relative path addon result: " + addon2);

// Test 3: Load addon with absolute path
console.log("Test 3: Load addon with absolute path");
var addon3 = system.loadAddon("C:\\temp\\test.dll");
console.log("Absolute path addon result: " + addon3);

// Test 4: Unload addon
console.log("Test 4: Unload addon");
var unloadResult = system.unloadAddon("./test.dll");
console.log("Unload result: " + unloadResult);

// Test 5: Unload non-loaded addon
console.log("Test 5: Unload non-loaded addon");
var unloadResult2 = system.unloadAddon("./never_loaded.dll");
console.log("Unload non-loaded result: " + unloadResult2);

console.log("=== Addon API Test Completed ===");