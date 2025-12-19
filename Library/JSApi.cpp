/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSApi.h"
#include "Widget.h"
#include "Logging.h"
#include "ColorUtil.h"
#include "Utils.h"
#include <vector>
#include <fstream>
#include <sstream>
#include <Windows.h>

extern std::vector<Widget*> widgets;

namespace JSApi {

    // JS API: novadesk.log(...)
    duk_ret_t js_log(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Info, L"%s", msg.c_str());
        return 0;
    }

    // JS API: novadesk.error(...)
    duk_ret_t js_error(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Error, L"%s", msg.c_str());
        return 0;
    }

    // JS API: novadesk.debug(...)
    duk_ret_t js_debug(duk_context* ctx) {
        duk_idx_t n = duk_get_top(ctx);
        std::wstring msg;
        for (duk_idx_t i = 0; i < n; i++) {
            if (i > 0) msg += L" ";
            msg += Utils::ToWString(duk_safe_to_string(ctx, i));
        }
        Logging::Log(LogLevel::Debug, L"%s", msg.c_str());
        return 0;
    }

    // JS API: new widgetWindow(options)
    duk_ret_t js_create_widget_window(duk_context* ctx) {
        Logging::Log(LogLevel::Debug, L"js_create_widget_window called");
        if (!duk_is_object(ctx, 0)) return DUK_RET_TYPE_ERROR;

        WidgetOptions options;
        options.width = 400;
        options.height = 300;
        options.backgroundColor = L"rgba(255,255,255,255)";
        options.zPos = ZPOSITION_NORMAL;
        options.alpha = 255;
        options.color = RGB(255, 255, 255);
        options.draggable = false;
        options.clickThrough = false;
        options.keepOnScreen = false;
        options.snapEdges = false;

        if (duk_get_prop_string(ctx, 0, "width")) options.width = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "height")) options.height = duk_get_int(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "backgroundColor")) {
            options.backgroundColor = Utils::ToWString(duk_get_string(ctx, -1));
            ColorUtil::ParseRGBA(options.backgroundColor, options.color, options.alpha);
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "zPos")) {
            std::string zPosStr = duk_get_string(ctx, -1);
            if (zPosStr == "ondesktop") options.zPos = ZPOSITION_ONDESKTOP;
            else if (zPosStr == "ontop") options.zPos = ZPOSITION_ONTOP;
            else if (zPosStr == "onbottom") options.zPos = ZPOSITION_ONBOTTOM;
            else if (zPosStr == "ontopmost") options.zPos = ZPOSITION_ONTOPMOST;
            else if (zPosStr == "normal") options.zPos = ZPOSITION_NORMAL;
        }
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "draggable")) options.draggable = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "clickThrough")) options.clickThrough = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "keepOnScreen")) options.keepOnScreen = duk_get_boolean(ctx, -1);
        duk_pop(ctx);
        if (duk_get_prop_string(ctx, 0, "snapEdges")) options.snapEdges = duk_get_boolean(ctx, -1);
        duk_pop(ctx);

        Logging::Log(LogLevel::Debug, L"Widget Creation Options:");
        Logging::Log(LogLevel::Debug, L"  - Size: %dx%d", options.width, options.height);
        Logging::Log(LogLevel::Debug, L"  - Draggable: %s", options.draggable ? L"true" : L"false");
        Logging::Log(LogLevel::Debug, L"  - ClickThrough: %s", options.clickThrough ? L"true" : L"false");
        Logging::Log(LogLevel::Debug, L"  - KeepOnScreen: %s", options.keepOnScreen ? L"true" : L"false");
        Logging::Log(LogLevel::Debug, L"  - SnapEdges: %s", options.snapEdges ? L"true" : L"false");
        Logging::Log(LogLevel::Debug, L"  - Color: RGB(%d,%d,%d) Alpha: %d", GetRValue(options.color), GetGValue(options.color), GetBValue(options.color), options.alpha);

        Widget* widget = new Widget(options);
        if (widget->Create()) {
            widget->Show();
            widgets.push_back(widget);
            Logging::Log(LogLevel::Info, L"Widget created and shown. HWND: 0x%p", widget->GetWindow());
        }
        else {
            Logging::Log(LogLevel::Error, L"Failed to create widget.");
            delete widget;
        }

        return 0;
    }

    void InitializeJavaScriptAPI(duk_context* ctx) {
        // Register novadesk object and methods
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_log, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "log");
        duk_push_c_function(ctx, js_error, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "error");
        duk_push_c_function(ctx, js_debug, DUK_VARARGS);
        duk_put_prop_string(ctx, -2, "debug");
        duk_put_global_string(ctx, "novadesk");

        // Register widgetWindow constructor
        duk_push_c_function(ctx, js_create_widget_window, 1);
        duk_put_global_string(ctx, "widgetWindow");

        Logging::Log(LogLevel::Info, L"JavaScript API initialized");
    }

    bool LoadAndExecuteScript(duk_context* ctx) {
        // Get executable path to find index.js
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        std::wstring exePath = path;
        size_t lastBackslash = exePath.find_last_of(L"\\");
        std::wstring scriptPath = exePath.substr(0, lastBackslash + 1) + L"index.js";

        Logging::Log(LogLevel::Info, L"Loading script from: %s", scriptPath.c_str());

        std::ifstream t(scriptPath);
        if (!t.is_open()) {
            Logging::Log(LogLevel::Error, L"Failed to open index.js at %s", scriptPath.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << t.rdbuf();
        std::string content = buffer.str();
        Logging::Log(LogLevel::Info, L"Script loaded, size: %zu", content.size());

        if (duk_peval_string(ctx, content.c_str()) != 0) {
            Logging::Log(LogLevel::Error, L"Script execution failed: %S", duk_safe_to_string(ctx, -1));
            duk_pop(ctx);
            return false;
        }

        Logging::Log(LogLevel::Info, L"Script execution successful. Calling novadeskAppReady()...");

        // Call novadeskAppReady if defined
        duk_get_global_string(ctx, "novadeskAppReady");
        if (duk_is_function(ctx, -1)) {
            if (duk_pcall(ctx, 0) != 0) {
                Logging::Log(LogLevel::Error, L"novadeskAppReady failed: %S", duk_safe_to_string(ctx, -1));
                duk_pop(ctx);
                return false;
            }
        }
        else {
            Logging::Log(LogLevel::Info, L"novadeskAppReady not defined.");
        }
        duk_pop(ctx);
        duk_pop(ctx);

        return true;
    }

    void ReloadScripts(duk_context* ctx) {
        Logging::Log(LogLevel::Info, L"Reloading scripts...");

        // Destroy all existing widgets
        for (auto w : widgets) {
            delete w;
        }
        widgets.clear();

        // Reload and execute script
        LoadAndExecuteScript(ctx);
    }

}
