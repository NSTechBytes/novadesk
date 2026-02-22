// Web Fetch API Test
// Tests system.fetch

console.log("=== Web Fetch API Test Started ===");

// Test 1: Fetch local file
console.log("Test 1: Fetch local file");
system.fetch("test_data.txt", function(data) {
    console.log("Local file fetch result: " + (data ? "Success" : "Failed"));
    if (data) console.log("Data length: " + data.length);
});

// Test 2: Fetch web URL
console.log("Test 2: Fetch web URL");
system.fetch("https://httpbin.org/get", function(data) {
    console.log("Web fetch result: " + (data ? "Success" : "Failed"));
    if (data) console.log("Web data received");
});

// Test 3: Fetch with error handling
console.log("Test 3: Fetch with error handling");
system.fetch("https://nonexistent.invalid", function(data) {
    console.log("Error fetch result: " + (data ? "Unexpected Success" : "Expected Failure"));
});

// Test 4: Fetch JSON data
console.log("Test 4: Fetch and parse JSON");
system.fetch("https://jsonplaceholder.typicode.com/posts/1", function(data) {
    if (data) {
        try {
            var json = JSON.parse(data);
            console.log("JSON parse success: " + json.title);
        } catch (e) {
            console.log("JSON parse failed");
        }
    }
});

console.log("=== Web Fetch API Test Completed ===");