#include "ModuleSystem.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <cstring>

#include "../../shared/Utils.h"
#include "NovadeskModule.h"

namespace novadesk::scripting::quickjs {
namespace {
bool g_moduleSystemDebug = false;

std::string NormalizeModuleNameImpl(const std::string& baseName, const std::string& moduleName) {
    if (moduleName == "novadesk") {
        return moduleName;
    }

    const std::filesystem::path modulePath(moduleName);
    if (modulePath.is_absolute()) {
        return modulePath.lexically_normal().string();
    }

    std::filesystem::path base(baseName);
    if (!base.empty()) {
        base = base.parent_path();
    }
    return (base / modulePath).lexically_normal().string();
}
}  // namespace

void SetModuleSystemDebug(bool debug) {
    g_moduleSystemDebug = debug;
    SetModuleDebug(debug);
}

char* ModuleNormalizeName(JSContext* ctx, const char* baseName, const char* name, void*) {
    const std::string normalized = NormalizeModuleNameImpl(baseName ? baseName : "", name ? name : "");
    char* out = static_cast<char*>(js_malloc(ctx, normalized.size() + 1));
    if (!out) {
        return nullptr;
    }
    std::memcpy(out, normalized.c_str(), normalized.size() + 1);
    return out;
}

JSModuleDef* ModuleLoader(JSContext* ctx, const char* moduleName, void*) {
    if (g_moduleSystemDebug) {
        std::cerr << "[novadesk] ModuleLoader: " << (moduleName ? moduleName : "<null>") << std::endl;
    }

    if (std::string(moduleName) == "novadesk") {
        return EnsureNovadeskModule(ctx, moduleName);
    }

    const std::string source = novadesk::shared::utils::ReadTextFile(moduleName);
    if (source.empty()) {
        JS_ThrowReferenceError(ctx, "Cannot load module: %s", moduleName);
        return nullptr;
    }

    JSValue func = JS_Eval(
        ctx,
        source.c_str(),
        source.size(),
        moduleName,
        JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY
    );

    if (JS_IsException(func)) {
        return nullptr;
    }

    return static_cast<JSModuleDef*>(JS_VALUE_GET_PTR(func));
}
}  // namespace novadesk::scripting::quickjs
