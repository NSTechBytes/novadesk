import { app, widgetWindow } from "novadesk";

// ── Robust Argv Parser Helper ────────────────────────────────────────────────
function parseArgs(argv) {
    const raw = argv.slice(2); // user-supplied args
    const flags = new Set();
    const values = {};
    const positionals = [];

    for (let i = 0; i < raw.length; i++) {
        const arg = raw[i];
        if (arg.startsWith("--") || arg.startsWith("-")) {
            if (arg.includes("=")) {
                const eqIdx = arg.indexOf("=");
                const key = arg.slice(0, eqIdx);
                const val = arg.slice(eqIdx + 1);
                values[key] = val;
            } else {
                // Check if next item is a value (does not start with -)
                if (i + 1 < raw.length && !raw[i + 1].startsWith("-")) {
                    values[arg] = raw[++i];
                } else {
                    flags.add(arg);
                }
            }
        } else {
            positionals.push(arg);
        }
    }
    return { raw, flags, values, positionals };
}

const argv = app.argv;
const rawArgv = app.rawArgv;
const parsed = parseArgs(argv);
const results = [];

// 1. Structure Tests
results.push({
    id: "t1_is_array",
    name: "app.argv is an Array",
    passed: Array.isArray(argv),
    detail: `Type: ${typeof argv}`
});

results.push({
    id: "t2_raw_is_array",
    name: "app.rawArgv is an Array",
    passed: Array.isArray(rawArgv) && rawArgv.length >= argv.length,
    detail: `raw length = ${rawArgv ? rawArgv.length : 0}`
});

results.push({
    id: "t3_exe_path",
    name: "argv[0] is Novadesk.exe executable path",
    passed: typeof argv[0] === "string" && argv[0].toLowerCase().endsWith(".exe"),
    detail: argv[0] ?? ""
});

results.push({
    id: "t4_entry_script",
    name: "argv[1] is the entry script path",
    passed: typeof argv[1] === "string" && (argv[1].toLowerCase().endsWith(".js") || argv[1].toLowerCase().includes("appargvtest")),
    detail: argv[1] ?? ""
});

// 2. Boolean Flag Tests
results.push({
    id: "t5_flag_test",
    name: "Flag '--test-flag' detected",
    passed: parsed.flags.has("--test-flag"),
    detail: parsed.flags.has("--test-flag") ? "Present" : "Missing (--test-flag)"
});

results.push({
    id: "t6_flag_verbose",
    name: "Flag '--verbose' / '-v' detected",
    passed: parsed.flags.has("--verbose") || parsed.flags.has("-v"),
    detail: parsed.flags.has("--verbose") ? "--verbose" : (parsed.flags.has("-v") ? "-v" : "Missing (--verbose / -v)")
});

results.push({
    id: "t7_flag_debug",
    name: "Flag '--debug' detected",
    passed: parsed.flags.has("--debug"),
    detail: parsed.flags.has("--debug") ? "Present" : "Missing (--debug)"
});

// 3. Key=Value Tests
results.push({
    id: "t8_kv_name",
    name: "Key=Value '--name=Novadesk'",
    passed: parsed.values["--name"] === "Novadesk",
    detail: parsed.values["--name"] ? `Value: ${parsed.values["--name"]}` : "Missing (--name=Novadesk)"
});

results.push({
    id: "t9_kv_env",
    name: "Key=Value '--env=production'",
    passed: parsed.values["--env"] === "production",
    detail: parsed.values["--env"] ? `Value: ${parsed.values["--env"]}` : "Missing (--env=production)"
});

results.push({
    id: "t10_kv_port",
    name: "Key=Value '--port=8080' (numeric parsing)",
    passed: parsed.values["--port"] === "8080" && Number(parsed.values["--port"]) === 8080,
    detail: parsed.values["--port"] ? `Port: ${parsed.values["--port"]}` : "Missing (--port=8080)"
});

// 4. Space-Separated Key Value Tests
results.push({
    id: "t11_space_greet",
    name: "Space-separated '--greet World'",
    passed: parsed.values["--greet"] === "World",
    detail: parsed.values["--greet"] ? `Value: ${parsed.values["--greet"]}` : "Missing (--greet World)"
});

results.push({
    id: "t11_space_title",
    name: "Quoted space-separated '--title \"Novadesk App\"'",
    passed: typeof parsed.values["--title"] === "string" && parsed.values["--title"].includes("Novadesk"),
    detail: parsed.values["--title"] ? `Title: ${parsed.values["--title"]}` : "Missing (--title \"Novadesk App\")"
});

// 5. Complex JSON & Comma list tests
let jsonParsedOk = false;
try {
    if (parsed.values["--data"]) {
        let rawStr = parsed.values["--data"];
        // If PowerShell stripped quotes, fix keys and booleans: {\active\:true} -> {"active":true}
        if (rawStr.includes("\\")) {
            rawStr = rawStr.replace(/\\([a-zA-Z0-9_]+)\\/g, '"$1"');
        }
        const d = JSON.parse(rawStr);
        jsonParsedOk = d && d.active === true;
    }
} catch (e) {}

results.push({
    id: "t12_json_data",
    name: "JSON Argument '--data={\"active\":true}'",
    passed: jsonParsedOk,
    detail: parsed.values["--data"] ? `JSON: ${parsed.values["--data"]}` : "Missing (--data={\"active\":true})"
});

const tags = parsed.values["--tags"] ? parsed.values["--tags"].split(",") : [];
results.push({
    id: "t13_tags_array",
    name: "Comma-separated list '--tags=ui,system,desktop'",
    passed: tags.includes("ui") && tags.includes("desktop"),
    detail: tags.length > 0 ? `Tags: [${tags.join(", ")}]` : "Missing (--tags=ui,system,desktop)"
});

// 6. Positional Arguments Tests
results.push({
    id: "t14_positionals",
    name: "Trailing Positional Arg 'run-all'",
    passed: parsed.positionals.includes("run-all"),
    detail: parsed.positionals.length > 0 ? `Positionals: [${parsed.positionals.join(", ")}]` : "Missing (run-all)"
});

// Print test summary to console
console.log("\n=======================================================");
console.log("             NOVADESK APP.ARGV TEST REPORT             ");
console.log("=======================================================");
console.log("app.argv count     :", argv.length);
console.log("app.rawArgv count  :", rawArgv ? rawArgv.length : 0);
console.log("User arguments     :", JSON.stringify(argv.slice(2)));
console.log("Parsed Flags       :", [...parsed.flags].join(", ") || "(none)");
console.log("Parsed Values      :", JSON.stringify(parsed.values));
console.log("Parsed Positionals :", JSON.stringify(parsed.positionals));
console.log("-------------------------------------------------------");

let passCount = 0;
results.forEach((r, idx) => {
    if (r.passed) passCount++;
    console.log(`[${r.passed ? "PASS" : "FAIL"}] #${idx + 1} ${r.name} -> ${r.detail}`);
});
console.log("-------------------------------------------------------");
console.log(`Summary: ${passCount}/${results.length} tests passed`);
console.log("=======================================================\n");

// Expose results via IPC for UI rendering
ipcMain.handle("get-argv-results", () => ({
    argv,
    rawArgv,
    parsed,
    results,
    passCount,
    totalCount: results.length
}));

new widgetWindow({
    id: "AppArgvTestWindow",
    x: 200,
    y: 120,
    width: 680,
    height: 640,
    backgroundColor: "rgba(14, 18, 30, 0.98)",
    script: "./script.ui.js",
    show: true
}).on("close", function () {
    app.exit();
});
