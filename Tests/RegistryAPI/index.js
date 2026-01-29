// Registry API Test
// Tests system.readRegistry and system.writeRegistry

console.log("=== Registry API Test Started ===");

// Test 1: Read existing registry value
console.log("Test 1: Read existing registry value");
var regValue = system.readRegistry("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Explorer", "SmartScreenEnabled");
console.log("Registry read result: " + regValue);

// Test 2: Read non-existent registry value
console.log("Test 2: Read non-existent registry value");
var regValue2 = system.readRegistry("HKCU\\Software\\Novadesk", "NonExistent");
console.log("Non-existent read result: " + regValue2);

// // Test 3: Write registry value
// console.log("Test 3: Write registry value");
// var writeResult = system.writeRegistry("HKCU\\Software\\Novadesk", "TestValue", "TestData");
// console.log("Registry write result: " + writeResult);

// // Test 4: Write numeric value
// console.log("Test 4: Write numeric registry value");
// var writeResult2 = system.writeRegistry("HKCU\\Software\\Novadesk", "TestNumber", 12345);
// console.log("Numeric write result: " + writeResult2);

// // Test 5: Read back written value
// console.log("Test 5: Read back written value");
// var readBack = system.readRegistry("HKCU\\Software\\Novadesk", "TestValue");
// console.log("Read back result: " + readBack);

// // Test 6: Write to different registry hive
// console.log("Test 6: Write to HKLM (may fail without admin)");
// var writeResult3 = system.writeRegistry("HKLM\\Software\\Novadesk", "SystemValue", "SystemData");
// console.log("HKLM write result: " + writeResult3);

console.log("=== Registry API Test Completed ===");