/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>
#include <vector>

/**
 * @brief Represents a single item in a context menu or dropdown menu.
 *
 * @note Supports hierarchical menus via the children vector. Separators
 *       are represented by setting isSeparator = true.
 */
struct MenuItem {
  std::wstring text;              ///< Display text for the menu item.
  int id;                         ///< Unique identifier for command dispatch.
  bool isSeparator = false;       ///< True if this item is a visual separator.
  bool checked = false;           ///< True to show a checkmark indicator.
  bool disabled = false;          ///< True to gray out the item.
  std::vector<MenuItem> children; ///< Submenu items (empty for leaf items).
};
