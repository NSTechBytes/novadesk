/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>
#include <vector>

#include "MenuItem.h"
#include "DesktopManager.h"

struct WidgetOptions;
class Widget;

/**
 * @brief Handles widget context menu creation and command dispatch.
 *
 * @note Static utility class for right-click context menu management.
 */
namespace WidgetContextMenuHelper {

/**
 * @brief Displays the widget context menu.
 *
 * @param hWnd The widget window handle.
 * @param customMenu Custom menu items from the widget.
 * @param showDefaultItems Whether to show default menu items.
 * @param windowZPos Current window Z-order position.
 * @param options Widget configuration options.
 *
 * @return The selected menu command ID.
 */
int ShowContextMenu(HWND hWnd, const std::vector<MenuItem> &customMenu,
                    bool showDefaultItems, ZPOSITION windowZPos,
                    const WidgetOptions &options);

/**
 * @brief Handles a context menu command selection.
 *
 * @param widget The widget instance.
 * @param cmd The selected menu command ID.
 */
void HandleContextCommand(Widget &widget, int cmd);

} // namespace WidgetContextMenuHelper
