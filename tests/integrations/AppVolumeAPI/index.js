import { app } from "novadesk";
import * as system from "system";

function logPass(name, value) {
  console.log("[PASS] " + name + ": " + JSON.stringify(value));
}

function logSkip(name, reason) {
  console.log("[SKIP] " + name + ": " + reason);
}

function logFail(name, err) {
  console.error("[FAIL] " + name + ": " + String(err));
}

function call(name, fn) {
  try {
    const out = fn();
    logPass(name, out);
    return { ok: true, value: out };
  } catch (err) {
    logFail(name, err);
    return { ok: false, error: err };
  }
}

function toNum(v) {
  return typeof v === "number" && isFinite(v) ? v : null;
}

function pct(v01) {
  if (typeof v01 !== "number" || !isFinite(v01)) return "n/a";
  return (v01 * 100).toFixed(1) + "%";
}

function firstUsableSession(sessions) {
  if (!Array.isArray(sessions)) return null;
  for (let i = 0; i < sessions.length; i++) {
    const s = sessions[i];
    if (!s || typeof s !== "object") continue;
    if (typeof s.pid === "number" && s.pid > 0) return s;
  }
  return null;
}

function pickVolume01(session, byPid) {
  const candidates = [];
  if (session && typeof session === "object") {
    candidates.push(session.volume01, session.volume, session.level);
  }
  if (byPid && typeof byPid === "object") {
    candidates.push(byPid.volume01, byPid.volume, byPid.level);
  }

  for (let i = 0; i < candidates.length; i++) {
    const v = candidates[i];
    if (typeof v === "number" && isFinite(v) && v >= 0 && v <= 1) {
      return v;
    }
  }
  return null;
}

function pickMute(session, byPid) {
  const candidates = [];
  if (session && typeof session === "object") {
    candidates.push(session.muted, session.mute, session.isMuted);
  }
  if (byPid && typeof byPid === "object") {
    candidates.push(byPid.muted, byPid.mute, byPid.isMuted);
  }
  for (let i = 0; i < candidates.length; i++) {
    if (typeof candidates[i] === "boolean") return candidates[i];
  }
  return null;
}

function clamp01(v) {
  if (v < 0) return 0;
  if (v > 1) return 1;
  return v;
}

console.log("=========================================");
console.log("   Novadesk AppVolume API Integration");
console.log("=========================================");
console.log("[INFO] Tip: play audio in target app during 10s monitor to see peak changes.");

const list = call("appVolume.listSessions()", function () {
  return system.appVolume.listSessions();
});

const sessions = list.ok ? list.value : [];
const target = firstUsableSession(sessions);

if (!target) {
  logSkip("session-dependent calls", "No active audio session with valid pid");
  setTimeout(function () {
    app.exit();
  }, 10000);
} else {
  const pid = target.pid;
  const name = typeof target.processName === "string" ? target.processName : "";

  const byPid = call("appVolume.getByPid(pid)", function () {
    return system.appVolume.getByPid(pid);
  });

  if (name) {
    call("appVolume.getByProcessName(name)", function () {
      return system.appVolume.getByProcessName(name);
    });
  } else {
    logSkip("appVolume.getByProcessName(name)", "Session missing processName");
  }

  const currentVolume = pickVolume01(target, byPid.ok ? byPid.value : null);
  if (currentVolume !== null) {
    const testVolume = clamp01(currentVolume >= 0.95 ? currentVolume - 0.05 : currentVolume + 0.05);
    console.log("[INFO] Volume test => current=" + pct(currentVolume) + ", target=" + pct(testVolume));

    call("appVolume.setVolumeByPid(pid, testVolume)", function () {
      return system.appVolume.setVolumeByPid(pid, testVolume);
    });
    call("appVolume.getByPid(pid) after setVolumeByPid", function () {
      return system.appVolume.getByPid(pid);
    });

    if (name) {
      call("appVolume.setVolumeByProcessName(name, testVolume)", function () {
        return system.appVolume.setVolumeByProcessName(name, testVolume);
      });
      call("appVolume.getByProcessName(name) after setVolumeByProcessName", function () {
        return system.appVolume.getByProcessName(name);
      });
    } else {
      logSkip("appVolume.setVolumeByProcessName(name, volume)", "Session missing processName");
    }

    // Restore original volume for safety.
    call("appVolume.setVolumeByPid(pid, restoreCurrentVolume)", function () {
      return system.appVolume.setVolumeByPid(pid, currentVolume);
    });
    call("appVolume.getByPid(pid) after restore", function () {
      return system.appVolume.getByPid(pid);
    });
  } else {
    logSkip("setVolume*", "Could not determine current 0..1 volume from session data");
  }

  const currentMute = pickMute(target, byPid.ok ? byPid.value : null);
  if (currentMute !== null) {
    call("appVolume.setMuteByPid(pid, currentMute)", function () {
      return system.appVolume.setMuteByPid(pid, currentMute);
    });
    if (name) {
      call("appVolume.setMuteByProcessName(name, currentMute)", function () {
        return system.appVolume.setMuteByProcessName(name, currentMute);
      });
    } else {
      logSkip("appVolume.setMuteByProcessName(name, mute)", "Session missing processName");
    }
  } else {
    logSkip("setMute*", "Could not determine current mute state from session data");
  }

  const monitorSeconds = 20;
  const base = byPid.ok ? byPid.value : null;
  const baseVol = base ? toNum(base.volume) : null;
  const basePeak = base ? toNum(base.peak) : null;
  const baseMute = base && typeof base.muted === "boolean" ? base.muted : (base && typeof base.mute === "boolean" ? base.mute : null);
  let maxPeak = basePeak !== null ? basePeak : 0;

  console.log("[INFO] Monitoring PID " + pid + (name ? " (" + name + ")" : "") + " for " + monitorSeconds + "s");
  console.log("[INFO] Baseline => volume=" + pct(baseVol) + ", muted=" + String(baseMute) + ", peak=" + String(basePeak));

  let tick = 0;
  const monitorTimer = setInterval(function () {
    tick += 1;
    const snap = call("monitor.getByPid(" + pid + ") t=" + tick + "s", function () {
      return system.appVolume.getByPid(pid);
    });

    if (!snap.ok) {
      clearInterval(monitorTimer);
      app.exit();
      return;
    }

    const s = snap.value || {};
    const v = toNum(s.volume);
    const p = toNum(s.peak);
    const m = typeof s.muted === "boolean" ? s.muted : (typeof s.mute === "boolean" ? s.mute : null);
    if (p !== null && p > maxPeak) maxPeak = p;

    let deltaTxt = "";
    if (baseVol !== null && v !== null) {
      deltaTxt += " dVol=" + ((v - baseVol) * 100).toFixed(2) + "pp";
    }
    if (basePeak !== null && p !== null) {
      deltaTxt += " dPeak=" + (p - basePeak).toFixed(4);
    }
    if (baseMute !== null && m !== null) {
      deltaTxt += " muteChanged=" + String(m !== baseMute);
    }

    console.log(
      "[MONITOR] t=" + tick + "s"
      + " volume=" + pct(v)
      + " muted=" + String(m)
      + " peak=" + String(p)
      + deltaTxt
    );

    if (tick >= monitorSeconds) {
      clearInterval(monitorTimer);
      console.log("[SUMMARY] maxPeak=" + maxPeak + ", baselinePeak=" + String(basePeak));
      if (maxPeak === 0) {
        console.log("[SUMMARY] Peak stayed 0. If expected, play audio in target app during monitor window.");
      }
      app.exit();
    }
  }, 1000);
}
