/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include "quickjs.h"

/**
 * @brief Widget window event bindings for QuickJS JavaScript runtime.
 *
 * @note Provides JavaScript methods for widget window events (mouse, drag,
 *       context menu, etc.) that are attached to the WidgetWindow prototype.
 */
namespace novadesk::scripting::quickjs {

/// Initializes the widget window event binding system.
void InitWidgetWindowEventBindings(JSClassID widgetWindowClassId);

/// Attaches event methods to the WidgetWindow JavaScript prototype.
void AttachWidgetWindowEventMethods(JSContext *ctx, JSValue proto);

/**
 * @brief Invokes a widget context menu callback from JavaScript.
 *
 * @param widgetId The widget identifier.
 * @param commandId The selected command ID.
 */
void InvokeWidgetContextMenuCallback(const std::wstring &widgetId,
                                     int commandId);

} // namespace novadesk::scripting::quickjs
