/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>

struct WidgetOptions;

/**
 * @brief Manages window chrome customization (toolbar style, icon, title).
 *
 * @note Static utility class for Win32 window properties.
 */
namespace WidgetWindowChromeHelper {

/**
 * @brief Applies the toolbar window style to a widget window.
 *
 * @param hWnd The widget window handle.
 * @param showInToolbar True to enable toolbar mode.
 */
void ApplyToolbarStyle(HWND hWnd, bool showInToolbar);

/**
 * @brief Destroys the toolbar icon handle.
 *
 * @param iconHandle Reference to the icon handle to destroy.
 * @param iconOwned Reference to the ownership flag.
 */
void DestroyToolbarIcon(HICON &iconHandle, bool &iconOwned);

/**
 * @brief Loads and applies the toolbar icon from widget options.
 *
 * @param hWnd The widget window handle.
 * @param options Widget configuration options.
 * @param iconHandle Reference to store the loaded icon handle.
 * @param iconOwned Reference to set the ownership flag.
 */
void ApplyToolbarIcon(HWND hWnd, const WidgetOptions &options,
                      HICON &iconHandle, bool &iconOwned);

/**
 * @brief Applies the toolbar title text to a widget window.
 *
 * @param hWnd The widget window handle.
 * @param options Widget configuration options.
 */
void ApplyToolbarTitle(HWND hWnd, const WidgetOptions &options);

} // namespace WidgetWindowChromeHelper
