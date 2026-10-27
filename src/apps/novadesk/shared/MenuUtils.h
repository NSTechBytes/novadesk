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

#ifndef TPM_LAYOUTRTL
#define TPM_LAYOUTRTL 0x00008000L
#endif

namespace MenuUtils {

/**
 * @brief Builds a Win32 HMENU from a vector of MenuItem definitions.
 */
void BuildMenu(HMENU hMenu, const std::vector<MenuItem> &items);

/**
 * @brief Appends a string/popup/separator item (MF_* flags, matching AppendMenu).
 */
bool AppendItem(HMENU hMenu, UINT flags, UINT_PTR idOrSubMenu, const wchar_t *text);

/**
 * @brief Shows a popup menu.
 *
 * Matches Rainmeter's ContextMenu::DisplayMenu RTL behavior:
 * TPM_LAYOUTRTL when Localization::IsRtl(), plus a temporary
 * SetProcessDefaultLayout(LAYOUT_RTL) for the modal menu loop.
 *
 * @return Selected command id when TPM_RETURNCMD is set; otherwise nonzero on success.
 */
int TrackPopup(HMENU hMenu, UINT flags, int x, int y, HWND owner);

} // namespace MenuUtils
