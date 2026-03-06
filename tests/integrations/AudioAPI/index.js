import { app } from "novadesk";
import * as system from "system";

console.log("=== Audio API Integration (Latest) ===");

function call(name, fn) {
  try {
    var out = fn();
    console.log("[PASS] " + name + ": " + JSON.stringify(out));
    return out;
  } catch (err) {
    console.error("[FAIL] " + name + ": " + String(err));
    return null;
  }
}

var soundPath = __dirname + "\\..\\assets\\sound.wav";
console.log("[INFO] soundPath=" + soundPath);

var currentVol = call("audio.getVolume()", function () {
  return system.audio.getVolume();
});

var testVol = 50;
if (typeof currentVol === "number") {
  testVol = currentVol >= 95 ? currentVol - 5 : currentVol + 5;
}

call("audio.setVolume(" + testVol + ")", function () {
  return system.audio.setVolume(testVol);
});

call("audio.getVolume() after set", function () {
  return system.audio.getVolume();
});

call("audio.playSound(soundPath, true)", function () {
  return system.audio.playSound(soundPath, true);
});

setTimeout(function () {
  call("audio.stopSound()", function () {
    return system.audio.stopSound();
  });

  if (typeof currentVol === "number") {
    call("audio.setVolume(restore)", function () {
      return system.audio.setVolume(currentVol);
    });
  }

  call("audio.getVolume() final", function () {
    return system.audio.getVolume();
  });

  console.log("=== Audio API Integration Complete ===");
  app.exit();
}, 3000);
