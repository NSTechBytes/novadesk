#include "JSEngine.h"

#include "quickjs.h"

#include <string>
#include <unordered_map>
#include <vector>

#include "../../domain/Widget.h"
#include "../../shared/FileUtils.h"
#include "../../shared/Logging.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Utils.h"
#include "../modules/ModuleSystem.h"
#include "../modules/WidgetUiBindings.h"

namespace JSEngine {
namespace {
HWND g_messageWindow = nullptr;
JSRuntime* g_runtime = nullptr;
JSContext* g_context = nullptr;
std::wstring g_lastScriptPath;
std::vector<JSValue> g_eventCallbacks;
std::unordered_map<Widget*, std::unordered_map<std::string, std::vector<int>>> g_widgetEventListeners;
std::unordered_map<std::wstring, std::unordered_map<int, JSValue>> g_widgetContextMenuCallbacks;
std::vector<JSValue> g_mainIpcListeners;
std::vector<JSValue> g_uiIpcListeners;
std::unordered_map<std::string, std::vector<JSValue>> g_mainIpcChannelListeners;
std::unordered_map<std::string, std::vector<JSValue>> g_uiIpcChannelListeners;
std::unordered_map<std::string, JSValue> g_mainIpcHandlers;

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

void ClearCallbacks(std::vector<JSValue>& list) {
    for (JSValue& v : list) {
        JS_FreeValue(g_context, v);
    }
    list.clear();
}

void ClearChannelMap(std::unordered_map<std::string, std::vector<JSValue>>& map) {
    for (auto& kv : map) {
        for (JSValue& v : kv.second) {
            JS_FreeValue(g_context, v);
        }
    }
    map.clear();
}

void ClearHandlerMap(std::unordered_map<std::string, JSValue>& map) {
    for (auto& kv : map) {
        JS_FreeValue(g_context, kv.second);
    }
    map.clear();
}

void ClearWidgetEventListeners() {
    g_widgetEventListeners.clear();
}

void ClearAllWidgetContextMenuCallbacks() {
    if (!g_context) return;
    for (auto& wkv : g_widgetContextMenuCallbacks) {
        for (auto& ckv : wkv.second) {
            JS_FreeValue(g_context, ckv.second);
        }
    }
    g_widgetContextMenuCallbacks.clear();
}

int RegisterCallback(std::vector<JSValue>& list, JSContext* ctx, JSValueConst fn) {
    if (!ctx || !JS_IsFunction(ctx, fn) || ctx != g_context) {
        return -1;
    }
    list.push_back(JS_DupValue(g_context, fn));
    return static_cast<int>(list.size() - 1);
}

bool GetChannelArg(JSContext* ctx, JSValueConst v, std::string& out) {
    const char* s = JS_ToCString(ctx, v);
    if (!s || !*s) {
        if (s) JS_FreeCString(ctx, s);
        return false;
    }
    out = s;
    JS_FreeCString(ctx, s);
    return true;
}

void RegisterChannelListener(std::unordered_map<std::string, std::vector<JSValue>>& map, JSContext* ctx, const std::string& channel, JSValueConst fn) {
    map[channel].push_back(JS_DupValue(ctx, fn));
}

JSValue BuildIpcMessage(JSContext* ctx, JSValueConst typeVal, JSValueConst payloadVal, const char* from, const char* to, const char* channel) {
    JSValue msg = JS_NewObject(ctx);

    std::string type = "message";
    const char* t = JS_ToCString(ctx, typeVal);
    if (t && *t) {
        type = t;
    }
    if (t) JS_FreeCString(ctx, t);

    JS_SetPropertyStr(ctx, msg, "type", JS_NewString(ctx, type.c_str()));
    JS_SetPropertyStr(ctx, msg, "payload", JS_DupValue(ctx, payloadVal));
    JS_SetPropertyStr(ctx, msg, "from", JS_NewString(ctx, from));
    JS_SetPropertyStr(ctx, msg, "to", JS_NewString(ctx, to));
    JS_SetPropertyStr(ctx, msg, "channel", JS_NewString(ctx, channel ? channel : type.c_str()));
    return msg;
}

void DispatchIpc(std::vector<JSValue>& listeners, JSValueConst message) {
    for (JSValue& cb : listeners) {
        JSValue argv[1] = { JS_DupValue(g_context, message) };
        JSValue ret = JS_Call(g_context, cb, JS_UNDEFINED, 1, argv);
        JS_FreeValue(g_context, argv[0]);
        if (JS_IsException(ret)) {
            LogQuickJsException(g_context);
        } else {
            JS_FreeValue(g_context, ret);
        }
    }
}

void DispatchChannelIpc(std::unordered_map<std::string, std::vector<JSValue>>& listeners, const std::string& channel, const char* from, const char* to, JSValueConst payload) {
    auto it = listeners.find(channel);
    if (it == listeners.end()) return;

    JSValue channelVal = JS_NewString(g_context, channel.c_str());
    JSValue eventObj = BuildIpcMessage(g_context, channelVal, payload, from, to, channel.c_str());
    JS_FreeValue(g_context, channelVal);

    for (JSValue& cb : it->second) {
        JSValue argv[2] = { JS_DupValue(g_context, eventObj), JS_DupValue(g_context, payload) };
        JSValue ret = JS_Call(g_context, cb, JS_UNDEFINED, 2, argv);
        JS_FreeValue(g_context, argv[0]);
        JS_FreeValue(g_context, argv[1]);
        if (JS_IsException(ret)) {
            LogQuickJsException(g_context);
        } else {
            JS_FreeValue(g_context, ret);
        }
    }

    JS_FreeValue(g_context, eventObj);
}

JSValue JsMainIpcOn(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "ipcMain.on requires (channel, listener)");
    }
    std::string channel;
    if (!GetChannelArg(ctx, argv[0], channel)) {
        return JS_ThrowTypeError(ctx, "ipcMain.on channel must be non-empty string");
    }
    RegisterChannelListener(g_mainIpcChannelListeners, ctx, channel, argv[1]);
    return JS_UNDEFINED;
}

JSValue JsMainIpcHandle(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "ipcMain.handle requires (channel, handler)");
    }
    std::string channel;
    if (!GetChannelArg(ctx, argv[0], channel)) {
        return JS_ThrowTypeError(ctx, "ipcMain.handle channel must be non-empty string");
    }
    auto it = g_mainIpcHandlers.find(channel);
    if (it != g_mainIpcHandlers.end()) {
        JS_FreeValue(ctx, it->second);
    }
    g_mainIpcHandlers[channel] = JS_DupValue(ctx, argv[1]);
    return JS_UNDEFINED;
}

JSValue JsMainIpcSend(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "ipc.sendToUi requires at least a type");
    }
    JSValue payload = (argc > 1) ? argv[1] : JS_UNDEFINED;
    JSValue msg = BuildIpcMessage(ctx, argv[0], payload, "main", "ui", nullptr);
    DispatchIpc(g_uiIpcListeners, msg);
    JS_FreeValue(ctx, msg);
    std::string channel;
    if (GetChannelArg(ctx, argv[0], channel)) {
        DispatchChannelIpc(g_uiIpcChannelListeners, channel, "main", "ui", payload);
    }
    return JS_UNDEFINED;
}

JSValue JsUiIpcOn(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2 || !JS_IsFunction(ctx, argv[1])) {
        return JS_ThrowTypeError(ctx, "ipcRenderer.on requires (channel, listener)");
    }
    std::string channel;
    if (!GetChannelArg(ctx, argv[0], channel)) {
        return JS_ThrowTypeError(ctx, "ipcRenderer.on channel must be non-empty string");
    }
    RegisterChannelListener(g_uiIpcChannelListeners, ctx, channel, argv[1]);
    return JS_UNDEFINED;
}

JSValue JsUiIpcSend(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "ipc.sendToMain requires at least a type");
    }
    JSValue payload = (argc > 1) ? argv[1] : JS_UNDEFINED;
    JSValue msg = BuildIpcMessage(ctx, argv[0], payload, "ui", "main", nullptr);
    DispatchIpc(g_mainIpcListeners, msg);
    JS_FreeValue(ctx, msg);
    std::string channel;
    if (GetChannelArg(ctx, argv[0], channel)) {
        DispatchChannelIpc(g_mainIpcChannelListeners, channel, "ui", "main", payload);
    }
    return JS_UNDEFINED;
}

JSValue JsUiIpcInvoke(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) {
        return JS_ThrowTypeError(ctx, "ipcRenderer.invoke requires at least a channel");
    }
    std::string channel;
    if (!GetChannelArg(ctx, argv[0], channel)) {
        return JS_ThrowTypeError(ctx, "ipcRenderer.invoke channel must be non-empty string");
    }
    auto it = g_mainIpcHandlers.find(channel);
    if (it == g_mainIpcHandlers.end()) {
        return JS_ThrowReferenceError(ctx, "No ipcMain handler for channel: %s", channel.c_str());
    }

    JSValue payload = (argc > 1) ? argv[1] : JS_UNDEFINED;
    JSValue channelVal = JS_NewString(ctx, channel.c_str());
    JSValue eventObj = BuildIpcMessage(ctx, channelVal, payload, "ui", "main", channel.c_str());
    JS_FreeValue(ctx, channelVal);

    JSValue callArgs[2] = { eventObj, JS_DupValue(ctx, payload) };
    JSValue ret = JS_Call(ctx, it->second, JS_UNDEFINED, 2, callArgs);
    JS_FreeValue(ctx, callArgs[0]);
    JS_FreeValue(ctx, callArgs[1]);
    return ret;
}

JSValue CreateMainIpcObject(JSContext* ctx) {
    JSValue ipc = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, ipc, "on", JS_NewCFunction(ctx, JsMainIpcOn, "on", 2));
    JS_SetPropertyStr(ctx, ipc, "handle", JS_NewCFunction(ctx, JsMainIpcHandle, "handle", 2));
    JS_SetPropertyStr(ctx, ipc, "send", JS_NewCFunction(ctx, JsMainIpcSend, "send", 2));
    return ipc;
}

JSValue CreateUiIpcObjectImpl(JSContext* ctx) {
    JSValue ipc = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, ipc, "on", JS_NewCFunction(ctx, JsUiIpcOn, "on", 2));
    JS_SetPropertyStr(ctx, ipc, "send", JS_NewCFunction(ctx, JsUiIpcSend, "send", 2));
    JS_SetPropertyStr(ctx, ipc, "invoke", JS_NewCFunction(ctx, JsUiIpcInvoke, "invoke", 2));
    return ipc;
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
    g_mainIpcListeners.clear();
    g_uiIpcListeners.clear();
    g_mainIpcChannelListeners.clear();
    g_uiIpcChannelListeners.clear();
    g_mainIpcHandlers.clear();
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
    ClearCallbacks(g_mainIpcListeners);
    ClearCallbacks(g_uiIpcListeners);
    ClearChannelMap(g_mainIpcChannelListeners);
    ClearChannelMap(g_uiIpcChannelListeners);
    ClearHandlerMap(g_mainIpcHandlers);
    ClearWidgetEventListeners();
    ClearAllWidgetContextMenuCallbacks();

    const std::string script = FileUtils::ReadFileContent(finalScriptPath);
    if (script.empty()) {
        Logging::Log(LogLevel::Error, L"Failed to load script: %s", finalScriptPath.c_str());
        return false;
    }

    const std::string fileName = Utils::ToString(finalScriptPath);
    JSValue global = JS_GetGlobalObject(g_context);
    JSValue mainIpc = CreateMainIpcObject(g_context);
    JS_SetPropertyStr(g_context, global, "ipcMain", JS_DupValue(g_context, mainIpc));
    JS_FreeValue(g_context, mainIpc);
    JS_FreeValue(g_context, global);

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
    auto wit = g_widgetContextMenuCallbacks.find(widgetId);
    if (wit == g_widgetContextMenuCallbacks.end()) return;
    auto cit = wit->second.find(commandId);
    if (cit == wit->second.end()) return;

    JSValue ret = JS_Call(g_context, cit->second, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(ret)) {
        LogQuickJsException(g_context);
    } else {
        JS_FreeValue(g_context, ret);
    }
}

void TriggerWidgetEvent(Widget* widget, const char* eventName, const MouseEventData* data) {
    if (!widget || !eventName || !*eventName) {
        return;
    }

    auto widgetIt = g_widgetEventListeners.find(widget);
    if (widgetIt == g_widgetEventListeners.end()) {
        return;
    }

    const std::string eventKey(eventName);
    auto eventIt = widgetIt->second.find(eventKey);
    if (eventIt == widgetIt->second.end()) {
        return;
    }

    for (int callbackId : eventIt->second) {
        CallEventCallback(callbackId, widget, data);
    }
}

void CallEventCallback(int callbackId, Widget* widget, const MouseEventData* data) {
    if (!g_context || callbackId <= 0 || callbackId >= static_cast<int>(g_eventCallbacks.size())) {
        return;
    }

    JSValue callback = g_eventCallbacks[callbackId];
    if (JS_IsUndefined(callback) || JS_IsNull(callback)) {
        return;
    }

    JSValue arg = JS_NewObject(g_context);
    JS_SetPropertyStr(g_context, arg, "clientX", JS_NewInt32(g_context, 0));
    JS_SetPropertyStr(g_context, arg, "clientY", JS_NewInt32(g_context, 0));
    JS_SetPropertyStr(g_context, arg, "screenX", JS_NewInt32(g_context, 0));
    JS_SetPropertyStr(g_context, arg, "screenY", JS_NewInt32(g_context, 0));
    JS_SetPropertyStr(g_context, arg, "offsetX", JS_NewInt32(g_context, 0));
    JS_SetPropertyStr(g_context, arg, "offsetY", JS_NewInt32(g_context, 0));
    JS_SetPropertyStr(g_context, arg, "offsetXPercent", JS_NewInt32(g_context, 0));
    JS_SetPropertyStr(g_context, arg, "offsetYPercent", JS_NewInt32(g_context, 0));
    if (widget) {
        JS_SetPropertyStr(g_context, arg, "widgetId",
            JS_NewString(g_context, Utils::ToString(widget->GetOptions().id).c_str()));
    }

    int argc = 1;
    if (data) {
        JS_SetPropertyStr(g_context, arg, "clientX", JS_NewInt32(g_context, data->clientX));
        JS_SetPropertyStr(g_context, arg, "clientY", JS_NewInt32(g_context, data->clientY));
        JS_SetPropertyStr(g_context, arg, "screenX", JS_NewInt32(g_context, data->screenX));
        JS_SetPropertyStr(g_context, arg, "screenY", JS_NewInt32(g_context, data->screenY));
        JS_SetPropertyStr(g_context, arg, "offsetX", JS_NewInt32(g_context, data->offsetX));
        JS_SetPropertyStr(g_context, arg, "offsetY", JS_NewInt32(g_context, data->offsetY));
        JS_SetPropertyStr(g_context, arg, "offsetXPercent", JS_NewInt32(g_context, data->offsetXPercent));
        JS_SetPropertyStr(g_context, arg, "offsetYPercent", JS_NewInt32(g_context, data->offsetYPercent));
    }

    JSValue argv[1] = { arg };
    JSValue ret = JS_Call(g_context, callback, JS_UNDEFINED, argc, argv);
    JS_FreeValue(g_context, arg);
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

bool RegisterWidgetEventListener(JSContext* ctx, Widget* widget, const std::string& eventName, JSValueConst fn) {
    if (!widget || eventName.empty()) {
        return false;
    }

    const int callbackId = RegisterEventCallback(ctx, fn);
    if (callbackId < 0) {
        return false;
    }

    g_widgetEventListeners[widget][eventName].push_back(callbackId);
    return true;
}

bool RegisterWidgetContextMenuCallback(JSContext* ctx, const std::wstring& widgetId, int commandId, JSValueConst fn) {
    if (!ctx || ctx != g_context || widgetId.empty() || commandId <= 0 || !JS_IsFunction(ctx, fn)) {
        return false;
    }

    auto& bucket = g_widgetContextMenuCallbacks[widgetId];
    auto it = bucket.find(commandId);
    if (it != bucket.end()) {
        JS_FreeValue(ctx, it->second);
    }
    bucket[commandId] = JS_DupValue(ctx, fn);
    return true;
}

void ClearWidgetContextMenuCallbacks(const std::wstring& widgetId) {
    auto it = g_widgetContextMenuCallbacks.find(widgetId);
    if (it == g_widgetContextMenuCallbacks.end()) return;
    for (auto& kv : it->second) {
        JS_FreeValue(g_context, kv.second);
    }
    g_widgetContextMenuCallbacks.erase(it);
}

bool ExecuteWidgetScript(Widget* widget) {
    if (!widget || !g_context) {
        return false;
    }
    const std::wstring& scriptPath = widget->GetOptions().scriptPath;
    if (scriptPath.empty()) {
        return false;
    }
    return novadesk::scripting::quickjs::ExecuteWidgetUiScript(g_context, widget, scriptPath);
}

JSValue CreateUiIpcObject(JSContext* ctx) {
    return CreateUiIpcObjectImpl(ctx);
}
}  // namespace JSEngine
