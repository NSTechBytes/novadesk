#include <filesystem>
#include <iostream>
#include <string>

#include "quickjs.h"
#include "domain/Widget.h"
#include "scripting/quickjs/ModuleSystem.h"
#include "scripting/quickjs/parser/PropertyParser.h"
#include "shared/Utils.h"

namespace {
bool g_debug = false;

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
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--debug") {
            g_debug = true;
        }
    }

    std::filesystem::path exePath = argc > 0 ? std::filesystem::path(argv[0]) : std::filesystem::current_path();
    exePath = std::filesystem::absolute(exePath).parent_path();
    if (g_debug) {
        std::cerr << "[novadesk] exe dir: " << exePath.string() << std::endl;
    }

    const std::filesystem::path metaPath = exePath / "meta.json";
    if (g_debug) {
        std::cerr << "[novadesk] meta path: " << metaPath.string() << std::endl;
    }

    const std::string mainFile = novadesk::scripting::quickjs::parser::ParseMainFromMeta(
        novadesk::shared::utils::ReadTextFile(metaPath));
    if (mainFile.empty()) {
        std::cerr << "meta.json missing or invalid. Expected key: \"main\"" << std::endl;
        return 1;
    }

    const std::filesystem::path mainPath = exePath / mainFile;
    if (g_debug) {
        std::cerr << "[novadesk] main path: " << mainPath.string() << std::endl;
    }

    const std::string scriptSource = novadesk::shared::utils::ReadTextFile(mainPath);
    if (scriptSource.empty()) {
        std::cerr << "Cannot read main script: " << mainPath.string() << std::endl;
        return 1;
    }

    JSRuntime* rt = JS_NewRuntime();
    JSContext* ctx = JS_NewContext(rt);
    if (!rt || !ctx) {
        std::cerr << "QuickJS runtime init failed" << std::endl;
        return 1;
    }

    novadesk::scripting::quickjs::SetModuleSystemDebug(g_debug);
    JS_SetModuleLoaderFunc(
        rt,
        novadesk::scripting::quickjs::ModuleNormalizeName,
        novadesk::scripting::quickjs::ModuleLoader,
        nullptr);

    JSValue compiled = JS_Eval(
        ctx,
        scriptSource.c_str(),
        scriptSource.size(),
        mainPath.string().c_str(),
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
    );

    if (JS_IsException(compiled)) {
        if (g_debug) {
            std::cerr << "[novadesk] JS compile failed" << std::endl;
        }
        PrintException(ctx);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }

    JSValue result = JS_EvalFunction(ctx, compiled);
    if (JS_IsException(result)) {
        if (g_debug) {
            std::cerr << "[novadesk] JS run failed" << std::endl;
        }
        PrintException(ctx);
        JS_FreeContext(ctx);
        JS_FreeRuntime(rt);
        return 1;
    }
    JS_FreeValue(ctx, result);

    while (novadesk::domain::widget::HasOpenWindows()) {
        novadesk::domain::widget::PollAndUpdateWindows();
    }

    if (g_debug) {
        std::cerr << "[novadesk] exiting: no windows left" << std::endl;
    }

    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);
    return 0;
}
