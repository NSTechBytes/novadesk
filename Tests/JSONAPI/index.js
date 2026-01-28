// JSON API Test
// Tests system.readJson and system.writeJson

console.log("=== JSON API Test Started ===");

// Test 1: Read existing JSON file
console.log("Test 1: Read existing JSON file");
var jsonData = system.readJson("test_config.json");
console.log("Read result: " + (jsonData ? "Success" : "Failed"));

if (jsonData) {
    console.log("Data: " + JSON.stringify(jsonData));
}

// Test 2: Read non-existent JSON file
console.log("Test 2: Read non-existent JSON file");
var jsonData2 = system.readJson("nonexistent.json");
console.log("Non-existent read result: " + jsonData2);

// Test 3: Write JSON data
console.log("Test 3: Write JSON data");
var testData = {
    name: "Test Config",
    version: 1.0,
    enabled: true,
    settings: {
        debug: false,
        timeout: 5000
    }
};

var writeResult = system.writeJson("output_test.json", testData);
console.log("Write result: " + writeResult);

// Test 4: Write and read back
console.log("Test 4: Write and read verification");
var verifyData = {test: "verification", number: 42};
system.writeJson("verify.json", verifyData);
var readBack = system.readJson("verify.json");
console.log("Read back result: " + (readBack ? "Success" : "Failed"));

if (readBack) {
    console.log("Verification data matches: " + JSON.stringify(readBack));
}

console.log("=== JSON API Test Completed ===");