#include "JSEngine.h"

#include "quickjs.h"

#include <string>
#include <vector>

#include "../../domain/Widget.h"
#include "../../shared/FileUtils.h"
#include "../../shared/Logging.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Utils.h"
#include "../modules/ModuleSystem.h"

namespace JSApi {
namespace {
HWND g_messageWindow = nullptr;
JSRuntime* g_runtime = nullptr;
JSContext* g_context = nullptr;
std::wstring g_lastScriptPath;
std::vector<JSValue> g_eventCallbacks;

enum class ConsoleLevel {
    Log = 0,
    Info = 1,
    Warn = 2,
    Error = 3,
    Debug = 4
};

std::wstring JsValueToWString(JSContext* ctx, JSValueConst value) {
    const char* s = JS_ToCString(ctx, value);
    if (!s) {
        return L"<unprintable>";
    }
    std::wstring out = Utils::ToWString(s);
    JS_FreeCString(ctx, s);
    return out;
}

JSValue JsConsoleWrite(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv, int magic) {
    std::wstring msg;
    for (int i = 0; i < argc; ++i) {
        if (!msg.empty()) msg += L" ";
        msg += JsValueToWString(ctx, argv[i]);
    }

    LogLevel lvl = LogLevel::Info;
    switch (static_cast<ConsoleLevel>(magic)) {
    case ConsoleLevel::Log:   lvl = LogLevel::Info; break;
    case ConsoleLevel::Info:  lvl = LogLevel::Info; break;
    case ConsoleLevel::Warn:  lvl = LogLevel::Warn; break;
    case ConsoleLevel::Error: lvl = LogLevel::Error; break;
    case ConsoleLevel::Debug: lvl = LogLevel::Debug; break;
    }
    Logging::Log(lvl, L"%s", msg.c_str());
    return JS_UNDEFINED;
}

void RegisterConsoleBindings(JSContext* ctx) {
    JSValue global = JS_GetGlobalObject(ctx);

    JSValue printFn = JS_NewCFunction(ctx, [](JSContext* c, JSValueConst thisVal, int argc, JSValueConst* argv) -> JSValue {
        return JsConsoleWrite(c, thisVal, argc, argv, static_cast<int>(ConsoleLevel::Log));
    }, "print", 1);
    JS_SetPropertyStr(ctx, global, "print", printFn);

    JSValue consoleObj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, consoleObj, "log",
        JS_NewCFunctionMagic(ctx, JsConsoleWrite, "log", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Log)));
    JS_SetPropertyStr(ctx, consoleObj, "info",
        JS_NewCFunctionMagic(ctx, JsConsoleWrite, "info", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Info)));
    JS_SetPropertyStr(ctx, consoleObj, "warn",
        JS_NewCFunctionMagic(ctx, JsConsoleWrite, "warn", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Warn)));
    JS_SetPropertyStr(ctx, consoleObj, "error",
        JS_NewCFunctionMagic(ctx, JsConsoleWrite, "error", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Error)));
    JS_SetPropertyStr(ctx, consoleObj, "debug",
        JS_NewCFunctionMagic(ctx, JsConsoleWrite, "debug", 1, JS_CFUNC_generic_magic, static_cast<int>(ConsoleLevel::Debug)));
    JS_SetPropertyStr(ctx, global, "console", consoleObj);

    JS_FreeValue(ctx, global);
}

void LogQuickJsException(JSContext* ctx) {
    JSValue ex = JS_GetException(ctx);
    const char* msg = JS_ToCString(ctx, ex);
    if (msg) {
        Logging::Log(LogLevel::Error, L"QuickJS error: %S", msg);
        JS_FreeCString(ctx, msg);
    } else {
        Logging::Log(LogLevel::Error, L"QuickJS error (unknown)");
    }
    JS_FreeValue(ctx, ex);
}

bool EnsureRuntime() {
    if (g_runtime && g_context) {
        return true;
    }

    g_runtime = JS_NewRuntime();
    if (!g_runtime) {
        Logging::Log(LogLevel::Error, L"Failed to create QuickJS runtime");
        return false;
    }

    g_context = JS_NewContext(g_runtime);
    if (!g_context) {
        Logging::Log(LogLevel::Error, L"Failed to create QuickJS context");
        JS_FreeRuntime(g_runtime);
        g_runtime = nullptr;
        return false;
    }

    JS_SetModuleLoaderFunc(g_runtime, novadesk::scripting::quickjs::ModuleNormalizeName, novadesk::scripting::quickjs::ModuleLoader, nullptr);
    novadesk::scripting::quickjs::SetModuleSystemDebug(false);
    RegisterConsoleBindings(g_context);
    g_eventCallbacks.clear();
    g_eventCallbacks.push_back(JS_UNDEFINED);  // callback id 0 is invalid
    return true;
}

std::wstring ResolveEntryScript(const std::wstring&) {
    return PathUtils::ResolvePath(L"index.js", PathUtils::GetWidgetsDir());
}
}  // namespace

void InitializeJavaScriptAPI(duk_context* ctx) {
    (void)ctx;
    EnsureRuntime();
}

bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath) {
    (void)ctx;
    if (!EnsureRuntime()) {
        return false;
    }

    const std::wstring finalScriptPath = ResolveEntryScript(scriptPath);
    g_lastScriptPath = finalScriptPath;

    for (size_t i = 1; i < g_eventCallbacks.size(); ++i) {
        JS_FreeValue(g_context, g_eventCallbacks[i]);
    }
    g_eventCallbacks.clear();
    g_eventCallbacks.push_back(JS_UNDEFINED);

    const std::string script = FileUtils::ReadFileContent(finalScriptPath);
    if (script.empty()) {
        Logging::Log(LogLevel::Error, L"Failed to load script: %s", finalScriptPath.c_str());
        return false;
    }

    const std::string fileName = Utils::ToString(finalScriptPath);
    JSValue result = JS_Eval(
        g_context,
        script.c_str(),
        script.size(),
        fileName.c_str(),
        JS_EVAL_TYPE_MODULE
    );

    if (JS_IsException(result)) {
        LogQuickJsException(g_context);
        JS_FreeValue(g_context, result);
        return false;
    }
    JS_FreeValue(g_context, result);

    JSContext* ctx1 = nullptr;
    int err = 0;
    while (JS_IsJobPending(g_runtime)) {
        err = JS_ExecutePendingJob(g_runtime, &ctx1);
        if (err < 0) {
            LogQuickJsException(ctx1 ? ctx1 : g_context);
            return false;
        }
    }

    return true;
}

void Reload() {
    LoadAndExecuteScript(nullptr, g_lastScriptPath);
}

void OnTimer(UINT_PTR id) {
    (void)id;
}

void OnMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_NOVADESK_DISPATCH) {
        using DispatchFn = void (*)(void*);
        DispatchFn fn = reinterpret_cast<DispatchFn>(wParam);
        if (fn) {
            fn(reinterpret_cast<void*>(lParam));
        }
    }
}

void SetMessageWindow(HWND hWnd) {
    g_messageWindow = hWnd;
}

HWND GetMessageWindow() {
    return g_messageWindow;
}

void OnTrayCommand(int commandId) {
    (void)commandId;
}

void OnWidgetContextCommand(const std::wstring& widgetId, int commandId) {
    (void)widgetId;
    (void)commandId;
}

void TriggerWidgetEvent(Widget* widget, const char* eventName, const MouseEventData* data) {
    (void)widget;
    (void)eventName;
    (void)data;
}

void CallEventCallback(int callbackId, Widget* widget, const MouseEventData* data) {
    if (!g_context || callbackId <= 0 || callbackId >= static_cast<int>(g_eventCallbacks.size())) {
        return;
    }

    JSValue callback = g_eventCallbacks[callbackId];
    if (JS_IsUndefined(callback) || JS_IsNull(callback)) {
        return;
    }

    JSValue arg = JS_UNDEFINED;
    int argc = 0;
    if (data) {
        arg = JS_NewObject(g_context);
        JS_SetPropertyStr(g_context, arg, "clientX", JS_NewInt32(g_context, data->clientX));
        JS_SetPropertyStr(g_context, arg, "clientY", JS_NewInt32(g_context, data->clientY));
        JS_SetPropertyStr(g_context, arg, "screenX", JS_NewInt32(g_context, data->screenX));
        JS_SetPropertyStr(g_context, arg, "screenY", JS_NewInt32(g_context, data->screenY));
        JS_SetPropertyStr(g_context, arg, "offsetX", JS_NewInt32(g_context, data->offsetX));
        JS_SetPropertyStr(g_context, arg, "offsetY", JS_NewInt32(g_context, data->offsetY));
        JS_SetPropertyStr(g_context, arg, "offsetXPercent", JS_NewInt32(g_context, data->offsetXPercent));
        JS_SetPropertyStr(g_context, arg, "offsetYPercent", JS_NewInt32(g_context, data->offsetYPercent));
        if (widget) {
            JS_SetPropertyStr(g_context, arg, "widgetId",
                JS_NewString(g_context, Utils::ToString(widget->GetOptions().id).c_str()));
        }
        argc = 1;
    }

    JSValue argv[1] = { arg };
    JSValue ret = JS_Call(g_context, callback, JS_UNDEFINED, argc, argv);
    if (argc == 1) {
        JS_FreeValue(g_context, arg);
    }
    if (JS_IsException(ret)) {
        LogQuickJsException(g_context);
    } else {
        JS_FreeValue(g_context, ret);
    }
}

int RegisterEventCallback(JSContext* ctx, JSValueConst fn) {
    if (!ctx || !JS_IsFunction(ctx, fn)) {
        return -1;
    }
    if (!EnsureRuntime() || !g_context) {
        return -1;
    }
    if (ctx != g_context) {
        return -1;
    }

    g_eventCallbacks.push_back(JS_DupValue(g_context, fn));
    return static_cast<int>(g_eventCallbacks.size() - 1);
}
}  // namespace JSApi
