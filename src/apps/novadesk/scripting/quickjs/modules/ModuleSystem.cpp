#include "ModuleSystem.h"

#include <cstring>
#include <filesystem>
#include <iostream>
#include <string>

#include "NovadeskModule.h"
#include "SystemModule.h"
#include "../../shared/FileUtils.h"
#include "../../shared/Utils.h"

namespace novadesk::scripting::quickjs
{
    namespace
    {
        bool g_moduleSystemDebug = false;
        thread_local bool g_uiScriptImportRestricted = false;

        bool IsBuiltinModuleName(const std::string &name)
        {
            return name == "novadesk" || name == "system";
        }

        std::string NormalizeModuleNameImpl(const std::string &baseName, const std::string &moduleName)
        {
            if (IsBuiltinModuleName(moduleName))
            {
                return moduleName;
            }

            std::filesystem::path modulePath(moduleName);
            if (modulePath.is_absolute())
            {
                return modulePath.lexically_normal().string();
            }

            std::filesystem::path base(baseName);
            if (!base.empty())
            {
                base = base.parent_path();
            }
            return (base / modulePath).lexically_normal().string();
        }

    } // namespace

    void SetModuleSystemDebug(bool debug)
    {
        g_moduleSystemDebug = debug;
        SetModuleDebug(debug);
    }

    void SetUiScriptImportRestricted(bool restricted)
    {
        g_uiScriptImportRestricted = restricted;
    }

    char *ModuleNormalizeName(JSContext *ctx, const char *baseName, const char *name, void *)
    {
        const std::string normalized = NormalizeModuleNameImpl(baseName ? baseName : "", name ? name : "");
        char *out = static_cast<char *>(js_malloc(ctx, normalized.size() + 1));
        if (!out)
        {
            return nullptr;
        }
        std::memcpy(out, normalized.c_str(), normalized.size() + 1);
        return out;
    }

    JSModuleDef *ModuleLoader(JSContext *ctx, const char *moduleName, void *)
    {
        if (g_moduleSystemDebug)
        {
            std::cerr << "[novadesk] ModuleLoader: " << (moduleName ? moduleName : "<null>") << std::endl;
        }

        const std::string requested = moduleName ? std::string(moduleName) : std::string();
        if (g_uiScriptImportRestricted && IsBuiltinModuleName(requested))
        {
            JS_ThrowReferenceError(ctx, "Module not allowed in ui script: %s", moduleName ? moduleName : "<null>");
            return nullptr;
        }

        if (moduleName && requested == "novadesk")
        {
            return EnsureNovadeskModule(ctx, moduleName);
        }
        if (moduleName && requested == "system")
        {
            return EnsureSystemModule(ctx, moduleName);
        }
        const std::wstring modulePath = Utils::ToWString(moduleName ? moduleName : "");
        const std::string source = FileUtils::ReadFileContent(modulePath);
        if (source.empty())
        {
            JS_ThrowReferenceError(ctx, "Cannot load module: %s", moduleName ? moduleName : "<null>");
            return nullptr;
        }

        JSValue func = JS_Eval(
            ctx,
            source.c_str(),
            source.size(),
            moduleName ? moduleName : "<module>",
            JS_EVAL_TYPE_MODULE | JS_EVAL_FLAG_COMPILE_ONLY);

        if (JS_IsException(func))
        {
            return nullptr;
        }

        return static_cast<JSModuleDef *>(JS_VALUE_GET_PTR(func));
    }
} // namespace novadesk::scripting::quickjs
