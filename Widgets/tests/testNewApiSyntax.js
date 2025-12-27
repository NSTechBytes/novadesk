/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

novadesk.log(" --- Starting API Syntax Verification --- ");

// 1. Test system.getEnv
try {
    var path = system.getEnv("PATH");
    novadesk.log("system.getEnv('PATH') success. Length:", path ? path.length : "null");

    var allEnv = system.getEnv();
    novadesk.log("system.getEnv() (all) success. Count:", Object.keys(allEnv).length);
} catch (e) {
    novadesk.error("system.getEnv test FAILED:", e.message);
}

// 2. Test system.getDisplayMetrics
try {
    var metrics = system.getDisplayMetrics();
    novadesk.log("system.getDisplayMetrics() success. Primary:", metrics.primary ? "found" : "not found");
} catch (e) {
    novadesk.error("system.getDisplayMetrics test FAILED:", e.message);
}

// 3. Test Monitor Classes (lowercase)
try {
    var cpu = new system.cpu();
    novadesk.log("system.cpu() success. Usage:", cpu.usage());
    cpu.destroy();

    var mem = new system.memory();
    novadesk.log("system.memory() success. Percent:", mem.stats().percent);
    mem.destroy();

    var mouse = new system.mouse();
    var pos = mouse.position();
    novadesk.log("system.mouse() success. Position:", pos.x, pos.y);
    mouse.destroy();
} catch (e) {
    novadesk.error("Monitor Classes test FAILED:", e.message);
}

// 4. Test system.registerHotkey
try {
    // This might fail if another app has the hotkey, but we just check if it's a function
    if (typeof system.registerHotkey === 'function') {
        novadesk.log("system.registerHotkey is available.");
    } else {
        novadesk.error("system.registerHotkey is NOT a function");
    }
} catch (e) {
    novadesk.error("system.registerHotkey test FAILED:", e.message);
}

novadesk.log(" --- API Syntax Verification Finished --- ");
