// Execute Command API Test
// Tests system.execute()

console.log("=== Execute Command API Test Started ===");

// Test 1: Basic command execution
console.log("Test 1: Execute notepad");
var result1 = system.execute("notepad.exe");
console.log("Notepad execution result: " + result1);

// Test 2: Execute with parameters
console.log("Test 2: Execute cmd with dir command");
var result2 = system.execute("cmd.exe", "/c dir", "C:\\", "normal");
console.log("CMD execution result: " + result2);

// Test 3: Open URL
console.log("Test 3: Open URL");
var result3 = system.execute("https://www.google.com");
console.log("URL open result: " + result3);

// Test 4: Execute with different show options
console.log("Test 4: Execute hidden");
var result4 = system.execute("cmd.exe", "/c echo Hidden Window", "", "hide");
console.log("Hidden execution result: " + result4);

console.log("Test 5: Execute minimized");
var result5 = system.execute("notepad.exe", "", "", "minimized");
console.log("Minimized execution result: " + result5);

// Test 6: Invalid command
console.log("Test 6: Invalid command");
var result6 = system.execute("nonexistent.exe");
console.log("Invalid command result: " + result6);

console.log("=== Execute Command API Test Completed ===");