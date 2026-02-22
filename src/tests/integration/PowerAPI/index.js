// Power API Test
// Tests system.getPowerStatus

console.log("=== Power API Test Started ===");

// Test 1: Get power status
console.log("Test 1: Get power status");
var powerStatus = system.getPowerStatus();

if (powerStatus) {
    console.log("✓ Power status retrieved successfully");
    
    // Test 2: AC line status
    console.log("AC Line Status: " + (powerStatus.acline ? "Plugged In" : "Battery"));
    
    // Test 3: Battery status
    var statusMap = {
        0: "No Battery",
        1: "Charging", 
        2: "Critical",
        3: "Low",
        4: "High/Full"
    };
    console.log("Battery Status: " + (statusMap[powerStatus.status] || "Unknown"));
    
    // Test 4: Battery percentage
    console.log("Battery Percentage: " + powerStatus.percent + "%");
    
    // Test 5: Battery lifetime
    if (powerStatus.lifetime !== -1) {
        console.log("Battery Lifetime: " + powerStatus.lifetime + " seconds");
    } else {
        console.log("Battery Lifetime: Unknown");
    }
    
    // Test 6: CPU frequency
    console.log("CPU Frequency: " + powerStatus.mhz + " MHz (" + powerStatus.hz + " Hz)");
    
    // Test 7: Raw battery flag
    console.log("Raw Battery Flag: " + powerStatus.status2);
    
} else {
    console.log("✗ Failed to get power status");
}

// Test 8: Performance test
console.log("Test 8: Performance measurement");
var startTime = Date.now();
var testStatus = system.getPowerStatus();
var endTime = Date.now();
console.log("Execution time: " + (endTime - startTime) + "ms");

console.log("=== Power API Test Completed ===");