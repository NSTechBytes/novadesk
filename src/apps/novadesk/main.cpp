#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>

#include "quickjs.h"

#define RGFW_IMPLEMENTATION
#include "RGFW.h"

namespace {
std::vector<RGFW_window*> g_windows;

std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}

std::string ReadMainFromMeta(const std::filesystem::path& meta_path) {
    const std::string meta = ReadTextFile(meta_path);
    if (meta.empty()) {
        return {};
    }

    std::smatch match;
    const std::regex main_regex("\"main\"\\s*:\\s*\"([^\"]+)\"");
    if (std::regex_search(meta, match, main_regex) && match.size() > 1) {
        return match[1].str();
    }
    return {};
}

JSValue JsCreateWindow(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    int width = 800;
    int height = 600;

    if (argc > 0) {
        JS_ToInt32(ctx, &width, argv[0]);
    }
    if (argc > 1) {
        JS_ToInt32(ctx, &height, argv[1]);
    }

    RGFW_window* win = RGFW_createWindow("Novadesk", 100, 100, width, height, 0);
    if (!win) {
        return JS_ThrowInternalError(ctx, "Failed to create RGFW window");
    }

    g_windows.push_back(win);
    return JS_UNDEFINED;
}

std::string NormalizeModuleName(const std::string& base_name, const std::string& module_name) {
    if (module_name == "novadesk") {
        return module_name;
    }

    const std::filesystem::path module_path(module_name);
    if (module_path.is_absolute()) {
        return module_path.lexically_normal().string();
    }

    std::filesystem::path base(base_name);
    if (!base.empty()) {
        base = base.parent_path();
    }
    return (base / module_path).lexically_normal().string();
}

char* ModuleNormalizeName(JSContext* ctx, const char* base_name, const char* name, void*) {
    const std::string normalized = NormalizeModuleName(base_name ? base_name : "", name ? name : "");
    char* out = static_cast<char*>(js_malloc(ctx, normalized.size() + 1));
    if (!out) {
        return nullptr;
    }
    std::memcpy(out, normalized.c_str(), normalized.size() + 1);
    return out;
}

JSModuleDef* ModuleLoader(JSContext* ctx, const char* module_name, void*) {
    std::string source;

    if (std::string(module_name) == "novadesk") {
        source =
            "export class WidgetWindow {\n"
            "  constructor(opts = {}) {\n"
            "    const width = Number(opts.width ?? 800);\n"
            "    const height = Number(opts.height ?? 600);\n"
            "    globalThis.__nativeCreateWindow(width, height);\n"
            "  }\n"
            "}\n";
    } else {
        source = ReadTextFile(module_name);
        if (source.empty()) {
            JS_ThrowReferenceError(ctx, "Cannot load module: %s", module_name);
            return nullptr;
        }
    }

    JSValue func = JS_Eval(
        ctx,
        source.c_str(),
        source.size(),
        module_name,
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
    );

    if (JS_IsException(func)) {
        return nullptr;
    }

    return static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func));
}

void PrintException(JSContext* ctx) {
    JSValue exception = JS_GetException(ctx);
    const char* err = JS_ToCString(ctx, exception);
    if (err) {
        std::cerr << err << std::endl;
        JS_FreeCString(ctx, err);
    }
    JS_FreeValue(ctx, exception);
}
}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path exe_path = argc > 0 ? std::filesystem::path(argv[0]) : std::filesystem::current_path();
    exe_path = std::filesystem::absolute(exe_path).parent_path();

    const std::filesystem::path meta_path = exe_path / "meta.json";
    const std::string main_file = ReadMainFromMeta(meta_path);
    if (main_file.empty()) {
        std::cerr << "meta.json missing or invalid. Expected key: \"main\"" << std::endl;
        return 1;
    }

    const std::filesystem::path main_path = exe_path / main_file;
    const std::string script_source = ReadTextFile(main_path);
    if (script_source.empty()) {
        std::cerr << "Cannot read main script: " << main_path.string() << std::endl;
        return 1;
    }

    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    if (!rt || !ctx) {
        std::cerr << "QuickJS runtime init failed" << std::endl;
        return 1;
    }

    JS_SetModuleLoaderFunc(rt, ModuleNormalizeName, ModuleLoader, nullptr);

    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(ctx, global, "__nativeCreateWindow",
                      JS_NewCFunction(ctx, JsCreateWindow, "__nativeCreateWindow", 2));
    JS_FreeValue(ctx, global);

    JSValue compiled = JS_Eval(
        ctx,
        script_source.c_str(),
        script_source.size(),
        main_path.string().c_str(),
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
    );
    if (JS_IsException(compiled)) {
        PrintException(ctx);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (JS_IsException(result)) {
        PrintException(ctx);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }
    JS_FreeValue(ctx, result);

    RGFW_event event;
    while (!g_windows.empty()) {
        for (auto it = g_windows.begin(); it != g_windows.end();) {
            RGFW_window* win = *it;
            while (RGFW_window_checkEvent(win, &event)) {
                if (event.type == RGFW_quit) {
                    RGFW_window_close(win);
                    it = g_windows.erase(it);
                    win = nullptr;
                    break;
                }
            }
            if (win) {
                ++it;
            }
        }
        RGFW_pollEvents();
    }

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}
