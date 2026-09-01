import { app, widgetWindow } from "novadesk";

// ── Helper: parse argv ────────────────────────────────────────────────────────
function parseArgs(argv) {
    const raw = argv.slice(2); // drop exe + script path
    const flags = new Set();
    const values = {};

    for (let i = 0; i < raw.length; i++) {
        const arg = raw[i];
        if (arg.includes("=")) {
            const [key, ...rest] = arg.split("=");
            values[key] = rest.join("=");
        } else if (arg.startsWith("--")) {
            if (i + 1 < raw.length && !raw[i + 1].startsWith("--")) {
                values[arg] = raw[++i];
            } else {
                flags.add(arg);
            }
        }
    }
    return { raw, flags, values };
}

// ── Run tests ─────────────────────────────────────────────────────────────────
const argv   = app.argv;
const parsed = parseArgs(argv);
const results = [];

// Test 1 — app.argv is an array
results.push({
    name: "app.argv is an Array",
    passed: Array.isArray(argv),
    detail: typeof argv
});

// Test 2 — argv[0] is the executable path (Novadesk.exe)
const exePath = argv[0] ?? "";
results.push({
    name: "argv[0] is Novadesk.exe path",
    passed: exePath.toLowerCase().endsWith(".exe"),
    detail: exePath
});

// Test 3 — argv[1] is this script's path (index.js)
const scriptPath = argv[1] ?? "";
results.push({
    name: "argv[1] is the entry script path",
    passed: scriptPath.toLowerCase().endsWith(".js"),
    detail: scriptPath
});

// Test 4 — argv.length >= 2 (always has exe + script)
results.push({
    name: "argv.length >= 2",
    passed: argv.length >= 2,
    detail: `length = ${argv.length}`
});

// Test 5 — argv.slice(2) gives user args only
const userArgs = argv.slice(2);
results.push({
    name: "argv.slice(2) returns user args",
    passed: Array.isArray(userArgs),
    detail: JSON.stringify(userArgs)
});

// Test 6 — --test-flag detection
const hasFlag = parsed.flags.has("--test-flag");
results.push({
    name: "Flag --test-flag detected",
    passed: hasFlag,
    detail: hasFlag ? "found" : "not found — run with: nwm run AppArgvTest -- --test-flag"
});

// Test 7 — --name=value detection
const nameVal = parsed.values["--name"] ?? null;
results.push({
    name: "--name=Novadesk value parsed",
    passed: nameVal === "Novadesk",
    detail: nameVal !== null ? `--name=${nameVal}` : "not found — run with: nwm run AppArgvTest -- --name=Novadesk"
});

// Test 8 — --greet value (space-separated)
const greetVal = parsed.values["--greet"] ?? null;
results.push({
    name: "--greet World (space-separated) parsed",
    passed: greetVal === "World",
    detail: greetVal !== null ? `--greet ${greetVal}` : "not found — run with: nwm run AppArgvTest -- --greet World"
});

// Test 9 — argv is immutable snapshot (modifying copy doesn't affect re-read)
const copy = app.argv;
results.push({
    name: "app.argv returns consistent values",
    passed: app.argv.length === argv.length && app.argv[0] === argv[0],
    detail: `lengths: ${app.argv.length} == ${argv.length}`
});

// Log results to console
console.log("=== app.argv Test Suite ===");
console.log("Raw argv:", JSON.stringify(argv, null, 2));
console.log("Parsed flags:", [...parsed.flags]);
console.log("Parsed values:", parsed.values);
results.forEach((r, i) => {
    const icon = r.passed ? "✔" : "✘";
    console.log(`[Test ${i + 1}] ${icon} ${r.name} — ${r.detail}`);
});

// Expose to UI
ipcMain.handle("get-argv-results", () => ({ argv, results }));

new widgetWindow({
    id: "AppArgvTestWindow",
    x: 200,
    y: 150,
    width: 660,
    height: 520,
    backgroundColor: "rgba(14, 18, 30, 0.97)",
    script: "./script.ui.js",
    show: true
}).on("close", function () {
    app.exit();
});
