// Timers API Test
// Tests setTimeout, setInterval, clearTimeout, clearInterval, setImmediate

console.log("=== Timer API Tests Started ===");

// Test 1: setTimeout basic usage
console.log("Test 1: setTimeout basic");
var timeoutId1 = setTimeout(function() {
    console.log("✓ setTimeout executed after 1 second");
}, 1000);

// Test 2: setTimeout with cancellation
console.log("Test 2: setTimeout with clearTimeout");
var timeoutId2 = setTimeout(function() {
    console.log("✗ This should not execute (cancelled)");
}, 2000);
clearTimeout(timeoutId2);
console.log("✓ clearTimeout called successfully");

// Test 3: setInterval basic usage
console.log("Test 3: setInterval basic");
var intervalCount = 0;
var intervalId1 = setInterval(function() {
    intervalCount++;
    console.log("✓ setInterval tick #" + intervalCount);
    if (intervalCount >= 3) {
        clearInterval(intervalId1);
        console.log("✓ clearInterval called after 3 ticks");
    }
}, 500);

// Test 4: setImmediate
console.log("Test 4: setImmediate");
console.log("Before setImmediate");
setImmediate(function() {
    console.log("✓ setImmediate executed (should run after current execution)");
});
console.log("After setImmediate (this runs first)");

// Test 5: Multiple timers with different delays
console.log("Test 5: Multiple concurrent timers");
setTimeout(function() {
    console.log("✓ 500ms timer executed");
}, 500);

setTimeout(function() {
    console.log("✓ 1000ms timer executed");
}, 1000);

setTimeout(function() {
    console.log("✓ 1500ms timer executed");
}, 1500);

// Test 6: Timer ID uniqueness
console.log("Test 6: Timer ID verification");
var id1 = setTimeout(function() {}, 1000);
var id2 = setTimeout(function() {}, 1000);
var id3 = setInterval(function() {}, 1000);

console.log("Timer IDs - Timeout1: " + id1 + ", Timeout2: " + id2 + ", Interval: " + id3);
if (id1 !== id2 && id2 !== id3 && id1 !== id3) {
    console.log("✓ Timer IDs are unique");
} else {
    console.log("✗ Timer IDs conflict detected");
}

// Test 7: Nested timers
console.log("Test 7: Nested timers");
setTimeout(function() {
    console.log("✓ Outer timer executed");
    setTimeout(function() {
        console.log("✓ Inner timer executed");
    }, 300);
}, 300);

// Test 8: Timer with parameters
console.log("Test 8: Timer with closure variables");
var testData = "Hello from timer!";
setTimeout(function() {
    console.log("✓ Timer with closure data: " + testData);
}, 400);

console.log("=== Timer API Tests Completed ===");