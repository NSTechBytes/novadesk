#include "JSEngine.h"

#include "../../shared/Logging.h"

namespace JSApi {
namespace {
HWND g_messageWindow = nullptr;
}

void InitializeJavaScriptAPI(duk_context* ctx) {
    (void)ctx;
    Logging::Log(LogLevel::Info, L"QuickJS API bridge initialized");
}

bool LoadAndExecuteScript(duk_context* ctx, const std::wstring& scriptPath) {
    (void)ctx;
    Logging::Log(LogLevel::Info, L"QuickJS load requested: %s", scriptPath.empty() ? L"<default>" : scriptPath.c_str());
    return true;
}

void Reload() {
    Logging::Log(LogLevel::Info, L"QuickJS reload requested");
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

