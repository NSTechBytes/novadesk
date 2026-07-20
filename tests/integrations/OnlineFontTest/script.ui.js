// ============================================================
// OnlineFontTest - script.ui.js
// Exercises the fontUrl async-download feature added to Novadesk.
//
// Test cases:
//   1. Text element renders with a Google Fonts WOFF2 URL  (Orbitron)
//   2. InputBox element renders with a Google Fonts WOFF2 URL (Exo 2)
//   3. A second Text element with the *same* URL hits the in-memory cache
//      and resolves immediately without downloading again.
//   4. An invalid URL is handled gracefully (element still renders,
//      fontPath stays empty, no crash).
//
// Detection strategy:
//   After a successful async download, FontDownloader calls
//   element->SetFontPath(url) on the main thread.
//   We read that back via ui.getElementProperty(id, "fontPath").
//   A non-empty fontPath means the font was downloaded and loaded.
// ============================================================

// --------------- helpers ---------------
function pass(name, details) {
  console.log("[PASS] " + name + (details ? " -> " + details : ""));
}
function fail(name, details) {
  console.log("[FAIL] " + name + (details ? " -> " + details : ""));
}
function expectNonEmpty(name, val) {
  if (val !== null && val !== undefined && val !== "")
    pass(name, "value=" + JSON.stringify(val));
  else
    fail(name, "value is empty/null/undefined");
}
function expectEmpty(name, val) {
  if (val === null || val === undefined || val === "")
    pass(name, "(empty as expected)");
  else
    fail(name, "expected empty but got=" + JSON.stringify(val));
}

// --------------- Google Fonts WOFF2 / TTF raw URLs (stable static CDN) ---------------
var ORBITRON_URL  = "https://fonts.gstatic.com/s/orbitron/v31/yMJMMIlzdpvBhQQL_SC3X9yhF25-T1nyGy6BoWgz.woff2";
// Using the Playwrite NZ Guides TTF from the user's example
var PLAYWRITE_URL = "https://fonts.gstatic.com/s/playwritenzguides/v2/t5t8IQQPN4uFDRepJwiX4vzIikyGzv71Who.woff2";
var INVALID_URL   = "https://fonts.gstatic.com/s/this_font_does_not_exist/v1/fake.woff2";

// --------------- UI setup ---------------
ui.beginUpdate();

// Title label (system font – no fontUrl)
ui.addText({
  id: "title",
  x: 20, y: 12,
  text: "Online Font Test",
  fontSize: 16,
  fontColor: "#ffffff",
  fontWeight: 700
});

// Test 1 – Text element with Orbitron fontUrl
ui.addText({
  id: "orbitronLabel",
  x: 20, y: 55,
  text: "Orbitron (downloading…)",
  fontSize: 30,
  fontColor: "#00d4ff",
  fontFace: "Orbitron",
  fontUrl: ORBITRON_URL
});

// Test 2 – InputBox with Playwrite NZ Guides fontUrl
ui.addInputBox({
  id: "exo2Input",
  x: 20, y: 115,
  width: 400, height: 46,
  text: "Playwrite font (downloading…)",
  fontSize: 20,
  fontColor: "#f0e090",
  fontFace: "Playwrite NZ Guides",
  fontUrl: PLAYWRITE_URL,
  borderRadius: 8,
  backgroundColor: "rgba(255,255,255,0.08)"
});

// Test 3 – Same URL as Test 1 → should hit in-memory cache immediately
ui.addText({
  id: "orbitronCached",
  x: 20, y: 180,
  text: "Orbitron cached (should be fast)",
  fontSize: 22,
  fontColor: "#a0f0c0",
  fontFace: "Orbitron",
  fontUrl: ORBITRON_URL    // same URL as Test 1
});

// Test 4 – Invalid URL → must not crash; fontPath should stay empty
ui.addText({
  id: "invalidFont",
  x: 20, y: 230,
  text: "Invalid URL (graceful fallback)",
  fontSize: 18,
  fontColor: "#ff8888",
  fontFace: "NonExistentFont",
  fontUrl: INVALID_URL
});

// Status line
ui.addText({
  id: "status",
  x: 20, y: 305,
  text: "Waiting for downloads… (timeout 15 s)",
  fontSize: 13,
  fontColor: "#888888"
});

ui.endUpdate();

// --------------- final check triggered by index.js after 15 s ---------------
ipcRenderer.on("font:test:check", function () {
  console.log("--- OnlineFontTest: final assertions ---");

  // --- Test 1: Orbitron fontPath non-empty ---
  var orbitronPath = ui.getElementProperty("orbitronLabel", "fontPath");
  if (orbitronPath && orbitronPath !== "") {
    pass("Test 1 – Orbitron fontUrl: fontPath applied after async download",
         "fontPath=" + orbitronPath);
    ui.setElementProperties("orbitronLabel", { text: "Orbitron loaded ✓" });
  } else {
    fail("Test 1 – Orbitron fontUrl: fontPath still empty after 15 s");
  }

  // --- Test 2: Playwrite fontPath non-empty ---
  var playwritePath = ui.getElementProperty("exo2Input", "fontPath");
  if (playwritePath && playwritePath !== "") {
    pass("Test 2 – Playwrite fontUrl (InputBox): fontPath applied after async download",
         "fontPath=" + playwritePath);
    ui.setElementProperties("exo2Input", { text: "Playwrite loaded ✓" });
  } else {
    fail("Test 2 – Playwrite fontUrl (InputBox): fontPath still empty after 15 s");
  }

  // --- Test 3: Cached copy should have resolved immediately ---
  var cachedPath = ui.getElementProperty("orbitronCached", "fontPath");
  if (cachedPath && cachedPath !== "") {
    pass("Test 3 – Same URL cache: fontPath applied on cached element",
         "fontPath=" + cachedPath);
  } else {
    fail("Test 3 – Same URL cache: fontPath empty after 15 s");
  }

  // --- Test 4: Invalid URL must not crash; fontPath must remain empty ---
  var invalidPath = ui.getElementProperty("invalidFont", "fontPath");
  if (invalidPath === null || invalidPath === undefined || invalidPath === "") {
    pass("Test 4 – Invalid fontUrl: graceful fallback, no crash, fontPath stays empty");
    ui.setElementProperties("invalidFont", { text: "Invalid URL → graceful ✓" });
  } else {
    fail("Test 4 – Invalid fontUrl: unexpectedly got a fontPath=" + invalidPath);
  }

  // --- Sanity: fontUrl property round-trip ---
  var orbitronUrl = ui.getElementProperty("orbitronLabel", "fontUrl");
  if (orbitronUrl === ORBITRON_URL) {
    pass("Sanity – fontUrl property round-trip for Text element");
  } else {
    fail("Sanity – fontUrl round-trip mismatch: got=" + orbitronUrl);
  }

  var playwriteUrl = ui.getElementProperty("exo2Input", "fontUrl");
  if (playwriteUrl === PLAYWRITE_URL) {
    pass("Sanity – fontUrl property round-trip for InputBox element");
  } else {
    fail("Sanity – fontUrl round-trip mismatch (InputBox): got=" + playwriteUrl);
  }

  ui.setElementProperties("status", { text: "Tests completed — see console for results." });

  pass("OnlineFontTest completed");
});
