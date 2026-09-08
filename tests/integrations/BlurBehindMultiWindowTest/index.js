import { app, widgetWindow, addon } from "novadesk";

console.log("========================================================");
console.log("   BlurBehind Addon Multi-Window Integration Test (v3.0.0)");
console.log("========================================================");

let passCount = 0;
let failCount = 0;

function assert(cond, testName, detail) {
    if (cond) {
        passCount++;
        console.log("[PASS] " + testName + (detail ? " (" + detail + ")" : ""));
    } else {
        failCount++;
        console.error("[FAIL] " + testName + (detail ? " (" + detail + ")" : ""));
    }
}

// ----------------------------------------------------------------------------
// 1. Locate and Load the BlurBehind Addon DLL
// ----------------------------------------------------------------------------
const dllCandidates = [
    __dirname + "\\..\\..\\..\\dist\\Addons\\BlurBehind.dll",
    __dirname + "\\..\\..\\..\\src\\addons\\dist\\x64\\Release\\BlurBehind\\BlurBehind.dll",
    "D:\\Novadesk-Project\\novadesk\\dist\\Addons\\BlurBehind.dll"
];

let BlurBehind = null;
let loadedPath = "";

for (const path of dllCandidates) {
    try {
        BlurBehind = addon.load(path);
        if (BlurBehind && typeof BlurBehind.apply === "function") {
            loadedPath = path;
            break;
        }
    } catch (e) {
        // Try next candidate
    }
}

assert(!!BlurBehind, "addon.load('BlurBehind.dll')", loadedPath);

if (!BlurBehind) {
    console.error("[FATAL] Could not load BlurBehind addon. Aborting test.");
    app.exit();
}

// ----------------------------------------------------------------------------
// 2. Validate Addon Exports and Metadata
// ----------------------------------------------------------------------------
console.log("\n--- Validating Addon Metadata and Functions ---");
assert(BlurBehind.name === "BlurBehind", "Addon name", BlurBehind.name);
assert(BlurBehind.version === "3.0.0", "Addon version", BlurBehind.version);

const expectedFunctions = [
    "apply", "disable", "setCorner",
    "setEffect", "setStroke",
    "toggle", "isSupported"
];

for (const fn of expectedFunctions) {
    assert(typeof BlurBehind[fn] === "function", "Function exported: " + fn);
}

// ----------------------------------------------------------------------------
// 3. Validate Capability Object & isSupported()
// ----------------------------------------------------------------------------
console.log("\n--- Validating Capability Flags & Feature Detection ---");
assert(typeof BlurBehind.supports === "object" && BlurBehind.supports !== null, "BlurBehind.supports object exists");

console.log("[INFO] System Capabilities Detected:");
console.log("       - blur:     " + BlurBehind.supports.blur);
console.log("       - acrylic:  " + BlurBehind.supports.acrylic);
console.log("       - corner:   " + BlurBehind.supports.corner);
console.log("       - stroke:   " + BlurBehind.supports.stroke);

assert(BlurBehind.isSupported("blur") === true, "isSupported('blur') == true");
assert(BlurBehind.isSupported("acrylic") === BlurBehind.supports.acrylic, "isSupported('acrylic') matches supports");
assert(BlurBehind.isSupported("corner") === BlurBehind.supports.corner, "isSupported('corner') matches supports");
assert(BlurBehind.isSupported("stroke") === BlurBehind.supports.stroke, "isSupported('stroke') matches supports");

// ----------------------------------------------------------------------------
// 4. Create 4 Multi-Window Instances
// ----------------------------------------------------------------------------
console.log("\n--- Creating 4 Multi-Window Instances ---");

const win1 = new widgetWindow({
    id: "blurTestWin1",
    x: 80,
    y: 80,
    width: 400,
    height: 230,
    backgroundColor: "rgba(20, 30, 45, 0.35)",
    script: "./win1.ui.js",
    show: true
});

const win2 = new widgetWindow({
    id: "blurTestWin2",
    x: 500,
    y: 80,
    width: 400,
    height: 230,
    backgroundColor: "rgba(35, 20, 45, 0.35)",
    script: "./win2.ui.js",
    show: true
});

const win3 = new widgetWindow({
    id: "blurTestWin3",
    x: 80,
    y: 340,
    width: 400,
    height: 230,
    backgroundColor: "rgba(20, 45, 30, 0.35)",
    script: "./win3.ui.js",
    show: true
});

const win4 = new widgetWindow({
    id: "blurTestWin4",
    x: 500,
    y: 340,
    width: 400,
    height: 230,
    backgroundColor: "rgba(45, 35, 20, 0.35)",
    script: "./win4.ui.js",
    show: true
});

const h1 = win1.getHandle();
const h2 = win2.getHandle();
const h3 = win3.getHandle();
const h4 = win4.getHandle();

assert(!!h1, "Window 1 handle retrieved", "HWND=" + h1);
assert(!!h2, "Window 2 handle retrieved", "HWND=" + h2);
assert(!!h3, "Window 3 handle retrieved", "HWND=" + h3);
assert(!!h4, "Window 4 handle retrieved", "HWND=" + h4);

// ----------------------------------------------------------------------------
// 5. Test Sequences Across Multiple Windows
// ----------------------------------------------------------------------------

// Phase 1 (100ms): Apply initial composition to all 4 windows
setTimeout(() => {
    console.log("\n>>> Phase 1 (100ms): Initial Effects Application");

    // Win 1: Classic Blur + Rounded Corner (backward-compatible API)
    const r1 = BlurBehind.apply(h1, "blur", "round");
    assert(r1 === true, "Win1 apply(h1, 'blur', 'round')");

    // Win 2: Acrylic via Config Object with hidden stroke
    const r2 = BlurBehind.apply(h2, {
        type: "acrylic",
        corner: "round",
        stroke: "hidden"
    });
    assert(r2 === true, "Win2 apply(h2, { type: 'acrylic', stroke: 'hidden' })");

    // Win 3: Blur via config object with small rounded corners
    const r3 = BlurBehind.apply(h3, {
        type: "blur",
        corner: "roundsmall"
    });
    assert(r3 === true, "Win3 apply(h3, { type: 'blur', corner: 'roundsmall' })");

    // Win 4: Apply blur with small rounded corner
    const r4 = BlurBehind.apply(h4, "blur", "roundsmall");
    assert(r4 === true, "Win4 apply(h4, 'blur', 'roundsmall')");
}, 100);

// Phase 2 (1000ms): Fine-tune window properties
setTimeout(() => {
    console.log("\n>>> Phase 2 (1000ms): Corner & Border Adjustments");

    // Win 1: Change corner to small round
    const c1 = BlurBehind.setCorner(h1, "roundsmall");
    assert(c1 === true, "Win1 setCorner(h1, 'roundsmall')");

    // Win 2: Apply custom border stroke color (bright green tint #00FF7F)
    const s2 = BlurBehind.setStroke(h2, "#00FF7F");
    assert(s2 === true, "Win2 setStroke(h2, '#00FF7F')");

    // Win 3: Apply a custom stroke color
    const s3 = BlurBehind.setStroke(h3, "#FF4500");
    assert(s3 === true, "Win3 setStroke(h3, '#FF4500')");

    // Win 4: Set Luminance boost effect
    const e4 = BlurBehind.setEffect(h4, "luminance");
    assert(e4 === true, "Win4 setEffect(h4, 'luminance')");
}, 1000);

// Phase 3 (2000ms): Test Runtime Toggle & Combined Config
setTimeout(() => {
    console.log("\n>>> Phase 3 (2000ms): Toggle & Combined Config");

    // Win 1: Toggle blur off (should return false = OFF)
    const t1Off = BlurBehind.toggle(h1);
    assert(t1Off === false, "Win1 toggle(h1) -> Blur OFF", "result=" + t1Off);

    // Win 2: Reset stroke to visible
    const s2Vis = BlurBehind.setStroke(h2, "visible");
    assert(s2Vis === true, "Win2 setStroke(h2, 'visible')");

    // Win 3: Test config object with type: 'acrylic' and custom corner
    let r3Config = false;
    try {
        r3Config = BlurBehind.apply({
            hwnd: h3,
            type: "acrylic",
            corner: "round"
        });
    } catch (e) {
        console.error("[ERROR] Win3 apply config error: " + e);
    }
    assert(r3Config === true, "Win3 apply({ hwnd, type: 'acrylic', corner: 'round' })");

    // Win 4: Switch effect to 'fullscreen'
    const e4Full = BlurBehind.setEffect(h4, "fullscreen");
    assert(e4Full === true, "Win4 setEffect(h4, 'fullscreen')");
}, 2000);

// Phase 4 (3000ms): Restore Toggle & Combined Effect
setTimeout(() => {
    console.log("\n>>> Phase 4 (3000ms): Toggle Restore & Combined Effect");

    // Win 1: Toggle blur back on (should return true = ON and restore previous state)
    const t1On = BlurBehind.toggle(h1);
    assert(t1On === true, "Win1 toggle(h1) -> Blur ON (Restored)", "result=" + t1On);

    // Win 4: Set combined effect (both luminance + fullscreen)
    const e4Both = BlurBehind.setEffect(h4, "both");
    assert(e4Both === true, "Win4 setEffect(h4, 'both')");
}, 3000);

// Phase 5 (4000ms): Teardown individual window
setTimeout(() => {
    console.log("\n>>> Phase 5 (4000ms): Individual Window Teardown");

    // Disable Win 1 completely
    const dis1 = BlurBehind.disable(h1);
    assert(dis1 === true, "Win1 disable(h1)");
}, 4000);

// Phase 6 (5000ms): Final Validation, Cleanup, and Exit
setTimeout(() => {
    console.log("\n========================================================");
    console.log("   BlurBehind Multi-Window Test Results Summary");
    console.log("========================================================");
    console.log("Passed: " + passCount);
    console.log("Failed: " + failCount);

    if (failCount === 0) {
        console.log("[ALL TESTS PASSED] BlurBehind v3.0.0 multi-window integration verified successfully!");
    } else {
        console.error("[TESTS FAILED] " + failCount + " test(s) failed.");
    }

    console.log("[INFO] Closing test windows and exiting...");
    try { win1.close(); } catch (e) {}
    try { win2.close(); } catch (e) {}
    try { win3.close(); } catch (e) {}
    try { win4.close(); } catch (e) {}
    try { BlurBehind.unload(); } catch (e) {}

    app.exit();
}, 5000);
