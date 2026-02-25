#include "NovadeskModule.h"

#include <string>
#include <vector>
#include <windows.h>

#include "../../../Version.h"
#include "../../domain/Novadesk.h"
#include "../../shared/Logging.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Settings.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include "WidgetUiBindings.h"

namespace novadesk::scripting::quickjs {
namespace {
bool g_moduleDebug = false;
int g_nextTrayCommandId = 1;

std::wstring GetVersionProperty(const std::wstring& propertyName) {
    std::wstring exePath = PathUtils::GetExePath();
    DWORD handle = 0;
    DWORD size = GetFileVersionInfoSizeW(exePath.c_str(), &handle);
    if (size == 0) return L"";

    std::vector<BYTE> buffer(size);
    if (!GetFileVersionInfoW(exePath.c_str(), handle, size, buffer.data())) {
        return L"";
    }

    struct LANGANDCODEPAGE {
        WORD wLanguage;
        WORD wCodePage;
    } *translate = nullptr;
    UINT cbTranslate = 0;
    if (!VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<LPVOID*>(&translate), &cbTranslate) ||
        cbTranslate < sizeof(LANGANDCODEPAGE)) {
        return L"";
    }

    wchar_t subBlock[128];
    swprintf_s(subBlock, L"\\StringFileInfo\\%04x%04x\\%s",
        translate[0].wLanguage, translate[0].wCodePage, propertyName.c_str());

    LPWSTR value = nullptr;
    UINT sizeOut = 0;
    if (VerQueryValueW(buffer.data(), subBlock, reinterpret_cast<LPVOID*>(&value), &sizeOut) && value) {
        return value;
    }
    return L"";
}

bool ParseTrayMenuItems(JSContext* ctx, JSValueConst arr, std::vector<MenuItem>& out) {
    if (!JS_IsArray(arr)) return false;
    uint32_t len = 0;
    JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
    if (JS_ToUint32(ctx, &len, lenV) != 0) {
        JS_FreeValue(ctx, lenV);
        return false;
    }
    JS_FreeValue(ctx, lenV);

    for (uint32_t i = 0; i < len; ++i) {
        JSValue itemV = JS_GetPropertyUint32(ctx, arr, i);
        if (!JS_IsObject(itemV)) {
            JS_FreeValue(ctx, itemV);
            continue;
        }

        MenuItem item{};
        item.id = 0;

        JSValue typeV = JS_GetPropertyStr(ctx, itemV, "type");
        const char* typeS = JS_ToCString(ctx, typeV);
        if (typeS && std::string(typeS) == "separator") item.isSeparator = true;
        if (typeS) JS_FreeCString(ctx, typeS);
        JS_FreeValue(ctx, typeV);

        if (!item.isSeparator) {
            JSValue textV = JS_GetPropertyStr(ctx, itemV, "text");
            const char* textS = JS_ToCString(ctx, textV);
            if (textS) {
                item.text = Utils::ToWString(textS);
                JS_FreeCString(ctx, textS);
            }
            JS_FreeValue(ctx, textV);

            JSValue checkedV = JS_GetPropertyStr(ctx, itemV, "checked");
            int checked = JS_ToBool(ctx, checkedV);
            if (checked >= 0) item.checked = (checked != 0);
            JS_FreeValue(ctx, checkedV);

            JSValue actionV = JS_GetPropertyStr(ctx, itemV, "action");
            if (JS_IsFunction(ctx, actionV)) {
                item.id = 2000 + g_nextTrayCommandId++;
                JSEngine::RegisterTrayCommandCallback(ctx, item.id, actionV);
            }
            JS_FreeValue(ctx, actionV);

            JSValue childV = JS_GetPropertyStr(ctx, itemV, "items");
            if (JS_IsArray(childV)) {
                ParseTrayMenuItems(ctx, childV, item.children);
            }
            JS_FreeValue(ctx, childV);
        }

        out.push_back(std::move(item));
        JS_FreeValue(ctx, itemV);
    }

    return true;
}

JSValue JsAppReload(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    (void)ctx;
    JSEngine::Reload();
    return JS_UNDEFINED;
}

JSValue JsAppExit(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    (void)ctx;
    PostQuitMessage(0);
    return JS_UNDEFINED;
}

JSValue JsAppHideTrayIcon(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    bool hide = true;
    if (argc > 0) {
        int b = JS_ToBool(ctx, argv[0]);
        if (b >= 0) hide = (b != 0);
    }
    Settings::SetGlobalBool("hideTrayIcon", hide);
    if (hide) ::HideTrayIconDynamic();
    else ::ShowTrayIconDynamic();
    return JS_UNDEFINED;
}

JSValue JsAppGetProductVersion(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    return JS_NewString(ctx, Utils::ToString(GetVersionProperty(L"ProductVersion")).c_str());
}

JSValue JsAppGetFileVersion(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    return JS_NewString(ctx, Utils::ToString(GetVersionProperty(L"FileVersion")).c_str());
}

JSValue JsAppGetNovadeskVersion(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    return JS_NewString(ctx, NOVADESK_VERSION);
}

JSValue JsAppSetTrayMenu(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsArray(argv[0])) {
        return JS_ThrowTypeError(ctx, "app.setTrayMenu: expected items array");
    }
    JSEngine::ClearTrayCommandCallbacks();
    std::vector<MenuItem> menu;
    if (!ParseTrayMenuItems(ctx, argv[0], menu)) {
        return JS_ThrowTypeError(ctx, "app.setTrayMenu: invalid items");
    }
    SetTrayMenu(menu);
    return JS_UNDEFINED;
}

JSValue JsAppClearTrayMenu(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    (void)ctx;
    JSEngine::ClearTrayCommandCallbacks();
    ClearTrayMenu();
    return JS_UNDEFINED;
}

JSValue JsAppShowDefaultTrayItems(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    bool show = true;
    if (argc > 0) {
        int b = JS_ToBool(ctx, argv[0]);
        if (b >= 0) show = (b != 0);
    }
    SetShowDefaultTrayItems(show);
    return JS_UNDEFINED;
}

int InitAppExport(JSContext* ctx, JSModuleDef* m) {
    JSValue app = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, app, "reload", JS_NewCFunction(ctx, JsAppReload, "reload", 0));
    JS_SetPropertyStr(ctx, app, "exit", JS_NewCFunction(ctx, JsAppExit, "exit", 0));
    JS_SetPropertyStr(ctx, app, "hideTrayIcon", JS_NewCFunction(ctx, JsAppHideTrayIcon, "hideTrayIcon", 1));
    JS_SetPropertyStr(ctx, app, "getProductVersion", JS_NewCFunction(ctx, JsAppGetProductVersion, "getProductVersion", 0));
    JS_SetPropertyStr(ctx, app, "getFileVersion", JS_NewCFunction(ctx, JsAppGetFileVersion, "getFileVersion", 0));
    JS_SetPropertyStr(ctx, app, "getNovadeskVersion", JS_NewCFunction(ctx, JsAppGetNovadeskVersion, "getNovadeskVersion", 0));
    JS_SetPropertyStr(ctx, app, "setTrayMenu", JS_NewCFunction(ctx, JsAppSetTrayMenu, "setTrayMenu", 1));
    JS_SetPropertyStr(ctx, app, "clearTrayMenu", JS_NewCFunction(ctx, JsAppClearTrayMenu, "clearTrayMenu", 0));
    JS_SetPropertyStr(ctx, app, "showDefaultTrayItems", JS_NewCFunction(ctx, JsAppShowDefaultTrayItems, "showDefaultTrayItems", 1));
    JS_SetModuleExport(ctx, m, "app", app);
    return 0;
}

int NovadeskModuleInit(JSContext* ctx, JSModuleDef* m) {
    EnsureWidgetWindowClass(ctx);
    JSValue ctor = JS_NewCFunction2(ctx, JsWidgetWindowCtor, "WidgetWindow", 1, JS_CFUNC_constructor, 0);
    JSValue proto = JS_GetClassProto(ctx, EnsureWidgetWindowClass(ctx));
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetModuleExport(ctx, m, "WidgetWindow", ctor);
    JS_FreeValue(ctx, proto);
    InitAppExport(ctx, m);
    return 0;
}
}  // namespace

void SetModuleDebug(bool debug) {
    g_moduleDebug = debug;
    SetWidgetUiDebug(debug);
}

JSModuleDef* EnsureNovadeskModule(JSContext* ctx, const char* moduleName) {
    JSModuleDef* m = JS_NewCModule(ctx, moduleName, NovadeskModuleInit);
    if (!m) {
        return nullptr;
    }
    if (JS_AddModuleExport(ctx, m, "WidgetWindow") < 0) {
        return nullptr;
    }
    if (JS_AddModuleExport(ctx, m, "app") < 0) {
        return nullptr;
    }
    return m;
}
}  // namespace novadesk::scripting::quickjs
