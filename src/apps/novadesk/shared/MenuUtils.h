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

namespace MenuUtils {

/**
 * @brief Builds a Win32 HMENU from a vector of MenuItem definitions.
 *
 * @param hMenu The menu handle to populate.
 * @param items Vector of MenuItem structures defining the menu layout.
 *
 * @note Recursively creates submenus for nested items.
 */
void BuildMenu(HMENU hMenu, const std::vector<MenuItem> &items);

} // namespace MenuUtils
