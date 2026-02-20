// App Volume API Test
// Tests listAppVolumes/getAppVolume/setAppVolume/getAppMute/setAppMute

const appVolume = require("app-volume");
console.log("=== App Volume API Test Started ===");

var sessions = appVolume.listAppVolumes();
console.log("Sessions count: " + (sessions ? sessions.length : 0));

if (!sessions || !sessions.length) {
    console.log("No active sessions found, skipping mutation tests.");
    console.log("=== App Volume API Test Completed ===");
} else {
    var first = sessions[0];
    console.log("Using first session => pid: " + first.pid + ", process: " + first.process + ", fileName: " + first.fileName + ", filePath: " + first.filePath + ", volume: " + first.volume + ", peak: " + first.peak + ", muted: " + first.muted);

    var selector = { pid: first.pid };

    var volBefore = appVolume.getAppVolume(selector);
    var peakNow = appVolume.getAppPeak(selector);
    var iconPath = first.iconPath;
    var muteBefore = appVolume.getAppMute(selector);
    console.log("Before => volume: " + volBefore + ", peak: " + peakNow + ", icon: " + iconPath + ", mute: " + muteBefore);

    var targetVol = (volBefore >= 90) ? 70 : 90;
    var setVolOk = appVolume.setAppVolume(selector, targetVol);
    var volAfterSet = appVolume.getAppVolume(selector);
    console.log("Set volume to " + targetVol + " => " + setVolOk + ", now: " + volAfterSet);

    // Keep mute unchanged during volume testing.
    // var setMuteOk = appVolume.setAppMute(selector, !muteBefore);
    // var muteAfterSet = appVolume.getAppMute(selector);
    // console.log("Toggle mute => " + setMuteOk + ", now: " + muteAfterSet);

    // Keep changed state so result is visible after script exits.
    // Uncomment to restore automatically:
    // appVolume.setAppVolume(selector, volBefore);
    // appVolume.setAppMute(selector, muteBefore);
    // console.log("State restored.");

    console.log("=== App Volume API Test Completed ===");
}
