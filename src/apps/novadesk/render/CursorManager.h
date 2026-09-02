/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>
#include <string>
#include <unordered_map>

class Element;

/**
 * @brief Manages custom cursor loading and caching for elements.
 *
 * @note Custom cursors are loaded from .cur or .ani files and cached
 *       by full path to avoid redundant file I/O.
 */
class CursorManager {
public:
  CursorManager() = default;
  ~CursorManager();

  /**
   * @brief Gets the cursor for an element based on its cursor configuration.
   *
   * @param element The element to get the cursor for.
   *
   * @return Handle to the cursor, or nullptr for default arrow.
   */
  HCURSOR GetCursorForElement(Element *element);

private:
  /**
   * @brief Loads a custom cursor from a file path.
   *
   * @param fullPath Full path to the .cur or .ani file.
   *
   * @return Handle to the loaded cursor, or nullptr on failure.
   */
  HCURSOR LoadCustomCursorFile(const std::wstring &fullPath);

  std::unordered_map<std::wstring, HCURSOR> m_CustomCursorCache; ///< Cached cursor handles.
};
