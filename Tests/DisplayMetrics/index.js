// Display Metrics API Test
// Tests system.getDisplayMetrics()

console.log("=== Display Metrics API Test Started ===");

// Test 1: Basic getDisplayMetrics call
console.log("Test 1: Get display metrics");
var metrics = system.getDisplayMetrics();

if (metrics) {
    console.log("✓ getDisplayMetrics() successful");
    
    // Test 2: Primary monitor information
    console.log("\n--- Primary Monitor ---");
    if (metrics.primary) {
        console.log("✓ Primary monitor data available");
        
        if (metrics.primary.screenArea) {
            console.log("Screen Area: " + metrics.primary.screenArea.width + "x" + metrics.primary.screenArea.height);
            console.log("Screen Position: (" + metrics.primary.screenArea.x + ", " + metrics.primary.screenArea.y + ")");
        }
        
        if (metrics.primary.workArea) {
            console.log("Work Area: " + metrics.primary.workArea.width + "x" + metrics.primary.workArea.height);
            console.log("Work Position: (" + metrics.primary.workArea.x + ", " + metrics.primary.workArea.y + ")");
        }
    } else {
        console.log("✗ Primary monitor data missing");
    }
    
    // Test 3: Virtual screen information
    console.log("\n--- Virtual Screen ---");
    if (metrics.virtualScreen) {
        console.log("✓ Virtual screen data available");
        console.log("Virtual Screen: " + metrics.virtualScreen.width + "x" + metrics.virtualScreen.height);
        console.log("Virtual Position: (" + metrics.virtualScreen.x + ", " + metrics.virtualScreen.y + ")");
    } else {
        console.log("✗ Virtual screen data missing");
    }
    
    // Test 4: Monitor enumeration
    console.log("\n--- Connected Monitors ---");
    if (metrics.monitors && Array.isArray(metrics.monitors)) {
        console.log("✓ Monitor array available");
        console.log("Number of monitors: " + metrics.monitors.length);
        
        metrics.monitors.forEach(function(monitor, index) {
            console.log("\nMonitor #" + (index + 1) + ":");
            console.log("  ID: " + (monitor.id || "N/A"));
            
            if (monitor.screenArea) {
                console.log("  Screen: " + monitor.screenArea.width + "x" + monitor.screenArea.height);
                console.log("  Screen Pos: (" + monitor.screenArea.x + ", " + monitor.screenArea.y + ")");
            }
            
            if (monitor.workArea) {
                console.log("  Work: " + monitor.workArea.width + "x" + monitor.workArea.height);
                console.log("  Work Pos: (" + monitor.workArea.x + ", " + monitor.workArea.y + ")");
            }
        });
    } else {
        console.log("✗ Monitor array missing or invalid");
    }
    
    // Test 5: Data validation
    console.log("\n--- Data Validation ---");
    var isValid = true;
    
    if (!metrics.primary || !metrics.primary.screenArea) {
        console.log("✗ Missing primary screen area");
        isValid = false;
    }
    
    if (!metrics.virtualScreen) {
        console.log("✗ Missing virtual screen data");
        isValid = false;
    }
    
    if (isValid) {
        console.log("✓ All required data present");
    }
    
} else {
    console.log("✗ getDisplayMetrics() failed - returned null");
}

// Test 6: Performance test
console.log("\n--- Performance Test ---");
var startTime = Date.now();
var testMetrics = system.getDisplayMetrics();
var endTime = Date.now();
console.log("Execution time: " + (endTime - startTime) + "ms");

console.log("\n=== Display Metrics API Test Completed ===");