// ── Header ────────────────────────────────────────────────────────────────────
ui.addShape({
    id: "headerBg",
    type: "rectangle",
    x: 0, y: 0, width: 660, height: 54,
    fillColor: "rgba(22, 30, 52, 1)"
});
ui.addText({
    id: "title",
    x: 20, y: 15, width: 500, height: 26,
    text: "app.argv — Test Suite",
    fontSize: 17,
    fontColor: "#e8eaf6",
    fontWeight: "bold"
});

// ── Fetch results ─────────────────────────────────────────────────────────────
const data    = ipcRenderer.invoke("get-argv-results") ?? { argv: [], results: [] };
const argv    = data.argv    ?? [];
const results = data.results ?? [];

// ── argv raw display ──────────────────────────────────────────────────────────
ui.addText({
    id: "rawLabel",
    x: 20, y: 64, width: 620, height: 18,
    text: "app.argv  →  " + JSON.stringify(argv),
    fontSize: 11,
    fontColor: "#7986cb",
    overflow: "ellipsis"
});

// ── Test cards ────────────────────────────────────────────────────────────────
const CARD_H   = 42;
const CARD_GAP = 6;
let y = 92;

const passColor = "rgba(30, 110, 65, 0.90)";
const failColor = "rgba(160, 40, 40, 0.90)";
const passStroke = "rgba(46, 200, 110, 0.35)";
const failStroke = "rgba(230, 60, 60, 0.35)";

results.forEach(function (res, idx) {
    const ok = res.passed;

    ui.addShape({
        id: "card_bg_" + idx,
        type: "rectangle",
        x: 16, y: y,
        width: 628, height: CARD_H,
        radius: 6,
        fillColor: "rgba(20, 26, 42, 0.95)",
        strokeColor: ok ? passStroke : failStroke,
        strokeWidth: 1
    });

    // Badge
    ui.addShape({
        id: "badge_" + idx,
        type: "rectangle",
        x: 26, y: y + 9,
        width: 58, height: 24,
        radius: 4,
        fillColor: ok ? passColor : failColor
    });
    ui.addText({
        id: "badge_txt_" + idx,
        x: 32, y: y + 13,
        width: 46, height: 16,
        text: ok ? "PASS" : "FAIL",
        fontSize: 11,
        fontColor: "#ffffff",
        fontWeight: "bold"
    });

    // Test name
    ui.addText({
        id: "name_" + idx,
        x: 96, y: y + 6,
        width: 530, height: 18,
        text: (idx + 1) + ". " + res.name,
        fontSize: 12,
        fontColor: "#cfd8dc",
        fontWeight: ok ? "normal" : "bold"
    });

    // Detail
    ui.addText({
        id: "detail_" + idx,
        x: 96, y: y + 23,
        width: 530, height: 14,
        text: res.detail ?? "",
        fontSize: 10,
        fontColor: ok ? "#80cbc4" : "#ef9a9a",
        overflow: "ellipsis"
    });

    y += CARD_H + CARD_GAP;
});

// ── Summary footer ────────────────────────────────────────────────────────────
const passed = results.filter(function (r) { return r.passed; }).length;
const total  = results.length;

ui.addShape({
    id: "footer_bg",
    type: "rectangle",
    x: 16, y: y + 4,
    width: 628, height: 34,
    radius: 6,
    fillColor: passed === total ? "rgba(30, 100, 60, 0.5)" : "rgba(120, 40, 40, 0.5)"
});
ui.addText({
    id: "summary",
    x: 30, y: y + 12,
    width: 600, height: 18,
    text: passed + " / " + total + " tests passed"
        + (passed === total ? "  ✔  All OK" : "  — check console for details"),
    fontSize: 13,
    fontColor: passed === total ? "#a5d6a7" : "#ef9a9a",
    fontWeight: "bold"
});
