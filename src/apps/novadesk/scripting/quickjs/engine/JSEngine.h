#pragma once

#include <string>
#include <windows.h>

#include "quickjs.h"

struct duk_hthread;
using duk_context = duk_hthread;

class Widget;

namespace JSEngine
{
    struct MouseEventData
    {
        int clientX = 0;
        int clientY = 0;
        int screenX = 0;
        int screenY = 0;
        int offsetX = 0;
        int offsetY = 0;
        int offsetXPercent = 0;
        int offsetYPercent = 0;
    };

    void InitializeJavaScriptAPI(duk_context *ctx);
    bool LoadAndExecuteScript(duk_context *ctx, const std::wstring &scriptPath = L"");
    std::wstring GetEntryScriptDir();
    void Reload();

    void OnTimer(UINT_PTR id);
    void OnMessage(UINT message, WPARAM wParam, LPARAM lParam);
    void SetMessageWindow(HWND hWnd);
    HWND GetMessageWindow();

    void OnTrayCommand(int commandId);
    void OnWidgetContextCommand(const std::wstring &widgetId, int commandId);

    void TriggerWidgetEvent(Widget *widget, const char *eventName, const MouseEventData *data = nullptr);
    void CallEventCallback(int callbackId, Widget *widget = nullptr, const MouseEventData *data = nullptr);
    int RegisterEventCallback(JSContext *ctx, JSValueConst fn);
    bool RegisterWidgetEventListener(JSContext *ctx, Widget *widget, const std::string &eventName, JSValueConst fn);
    bool RegisterWidgetContextMenuCallback(JSContext *ctx, const std::wstring &widgetId, int commandId, JSValueConst fn);
    void ClearWidgetContextMenuCallbacks(const std::wstring &widgetId);
    bool RegisterTrayCommandCallback(JSContext *ctx, int commandId, JSValueConst fn);
    void ClearTrayCommandCallbacks();
    bool ExecuteWidgetScript(Widget *widget);
    JSValue CreateUiIpcObject(JSContext *ctx);

    static const UINT WM_NOVADESK_DISPATCH = WM_USER + 101;
} // namespace JSEngine
