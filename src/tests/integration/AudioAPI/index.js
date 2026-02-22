// Audio API Test
// Tests system.getVolume, system.setVolume, system.playSound, system.stopSound

console.log("=== Audio API Test Started ===");

// Test 1: Get current volume
console.log("Test 1: Get current volume");
var currentVol = system.getVolume();
console.log("Current volume: " + currentVol);

// Test 2: Set volume
console.log("Test 2: Set volume to 50%");
var setSuccess = system.setVolume(50);
console.log("Set volume result: " + setSuccess);

// Test 3: Set volume to different level
console.log("Test 3: Set volume to 75%");
system.setVolume(75);

// Test 4: Play sound file
console.log("Test 4: Play sound");
var playResult = system.playSound("test_sound.wav");
console.log("Play sound result: " + playResult);

// Test 5: Play sound with loop
console.log("Test 5: Play sound with loop");
var loopResult = system.playSound("loop_sound.wav", true);
console.log("Loop play result: " + loopResult);

// Test 6: Stop sound
console.log("Test 6: Stop sound");
setTimeout(function() {
    var stopResult = system.stopSound();
    console.log("Stop sound result: " + stopResult);
}, 2000);

// Test 7: Volume bounds testing
console.log("Test 7: Volume bounds");
system.setVolume(-10); // Should clamp to 0
system.setVolume(150); // Should clamp to 100

console.log("=== Audio API Test Completed ===");