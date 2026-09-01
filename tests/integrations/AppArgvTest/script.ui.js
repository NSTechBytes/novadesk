// ── Header ────────────────────────────────────────────────────────────────────
ui.addShape({
    id: "headerBg",
    type: "rectangle",
    x: 0, y: 0, width: 680, height: 50,
    fillColor: "rgba(22, 30, 52, 1)"
});

ui.addText({
    id: "title",
    x: 20, y: 14, width: 640, height: 26,
    text: "Novadesk app.argv Comprehensive Test Suite",
    fontSize: 16,
    fontColor: "#e8eaf6"
});

// ── Fetch results ─────────────────────────────────────────────────────────────
const data = ipcRenderer.invoke("get-argv-results") ?? { argv: [], results: [] };
const argv = data.argv ?? [];
const results = data.results ?? [];
const passCount = data.passCount ?? 0;
const totalCount = data.totalCount ?? results.length;

// ── Stats Summary Bar ─────────────────────────────────────────────────────────
const isAllPassed = passCount === totalCount;
ui.addShape({
    id: "statsBg",
    type: "rectangle",
    x: 20, y: 60, width: 640, height: 32,
    radius: 4,
    fillColor: isAllPassed ? "rgba(30, 120, 65, 0.4)" : "rgba(180, 50, 50, 0.35)",
    strokeColor: isAllPassed ? "rgba(46, 200, 110, 0.5)" : "rgba(240, 70, 70, 0.5)",
    strokeWidth: 1
});

ui.addText({
    id: "statsText",
    x: 35, y: 68, width: 610, height: 18,
    text: `Passed: ${passCount} / ${totalCount} tests  |  Arguments received: ${argv.length}`,
    fontSize: 12,
    fontColor: isAllPassed ? "#a5d6a7" : "#ef9a9a"
});

// ── Test List ────────────────────────────────────────────────────────────────
let y = 102;
const CARD_H = 32;
const GAP = 4;

results.forEach(function (res, idx) {
    const ok = res.passed;

    ui.addShape({
        id: "card_" + idx,
        type: "rectangle",
        x: 20, y: y,
        width: 640, height: CARD_H,
        radius: 4,
        fillColor: "rgba(22, 28, 44, 0.95)",
        strokeColor: ok ? "rgba(40, 180, 90, 0.25)" : "rgba(240, 60, 60, 0.25)",
        strokeWidth: 1
    });

    // Badge
    ui.addShape({
        id: "badge_" + idx,
        type: "rectangle",
        x: 28, y: y + 5,
        width: 50, height: 22,
        radius: 3,
        fillColor: ok ? "rgba(35, 120, 70, 0.9)" : "rgba(180, 50, 50, 0.9)"
    });

    ui.addText({
        id: "badge_txt_" + idx,
        x: 34, y: y + 8,
        width: 38, height: 16,
        text: ok ? "PASS" : "FAIL",
        fontSize: 10,
        fontColor: "#ffffff"
    });

    // Test Name
    ui.addText({
        id: "name_" + idx,
        x: 88, y: y + 8,
        width: 310, height: 16,
        text: (idx + 1) + ". " + res.name,
        fontSize: 11,
        fontColor: ok ? "#eceff1" : "#ffcdd2"
    });

    // Detail
    ui.addText({
        id: "detail_" + idx,
        x: 405, y: y + 8,
        width: 245, height: 16,
        text: res.detail ?? "",
        fontSize: 10,
        fontColor: ok ? "#80cbc4" : "#ff8a80"
    });

    y += CARD_H + GAP;
});
