/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include "quickjs.h"

class Widget;

/**
 * @brief Widget UI bindings for QuickJS JavaScript runtime.
 *
 * @note Provides the WidgetWindow JavaScript class for creating and
 *       managing widgets from script.
 */
namespace novadesk::scripting::quickjs {

/**
 * @brief Wrapper holding a widget pointer and stable instance ID.
 */
struct WidgetWrapper {
  Widget *widget = nullptr; ///< The widget instance.
  uint64_t instanceId = 0;  ///< Stable ID across HWND reuse.
};

/// Enables or disables debug logging for widget UI operations.
void SetWidgetUiDebug(bool debug);

/// Registers the WidgetWindow JavaScript class.
JSClassID EnsureWidgetWindowClass(JSContext *ctx);

/// JavaScript constructor for WidgetWindow.
JSValue JsWidgetWindowCtor(JSContext *ctx, JSValueConst thisVal, int argc,
                           JSValueConst *argv);

/**
 * @brief Executes a widget's UI script.
 *
 * @param ctx The QuickJS context.
 * @param widget The widget to execute the script for.
 * @param scriptPath Path to the UI script.
 *
 * @return True if execution succeeded.
 */
bool ExecuteWidgetUiScript(JSContext *ctx, Widget *widget,
                           const std::wstring &scriptPath);

} // namespace novadesk::scripting::quickjs
