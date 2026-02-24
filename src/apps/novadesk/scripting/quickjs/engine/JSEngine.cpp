#include "JSEngine.h"

#include "quickjs.h"

#include <string>

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
    (void)callbackId;
    (void)widget;
    (void)data;
}
}  // namespace JSApi
