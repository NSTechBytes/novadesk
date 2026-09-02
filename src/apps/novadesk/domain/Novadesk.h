/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "../Resource.h"
#include "../../shared/MenuItem.h"

/**
 * @brief Creates a system tray icon.
 *
 * @param path Path to the tray icon image.
 *
 * @return Tray ID for subsequent operations.
 */
int TrayCreate(const std::wstring &path);

/**
 * @brief Removes a system tray icon.
 *
 * @param trayId The tray ID returned by TrayCreate.
 */
void TrayDestroy(int trayId);

/**
 * @brief Updates the tray icon image.
 *
 * @param trayId The tray ID.
 * @param path Path to the new icon image.
 */
void TraySetImage(int trayId, const std::wstring &path);

/**
 * @brief Updates the tray icon tooltip text.
 *
 * @param trayId The tray ID.
 * @param toolTip The tooltip text to display.
 */
void TraySetToolTip(int trayId, const std::wstring &toolTip);

/**
 * @brief Sets the context menu for a tray icon.
 *
 * @param trayId The tray ID.
 * @param menu Vector of menu items.
 */
void TraySetContextMenu(int trayId, const std::vector<MenuItem> &menu);

/**
 * @brief Acquires a single-instance application lock.
 *
 * @return True if lock was acquired (first instance).
 */
bool RequestSingleInstanceLock();

/// Releases the single-instance application lock.
void ReleaseSingleInstanceLock();
