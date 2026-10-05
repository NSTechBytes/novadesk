/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>

/**
 * @brief All colors used by the settings panel, resolved from the current
 *        theme preference (dark / light / system).
 */
struct ThemePalette {
  COLORREF background;    ///< Panel window background fill.
  COLORREF sidebar;       ///< Left sidebar background.
  COLORREF sidebarBorder; ///< Thin right border of the sidebar.
  COLORREF text;          ///< Primary text (title, input values).
  COLORREF label;         ///< Row label text.
  COLORREF muted;         ///< Close button, placeholders, secondary info.
  COLORREF accent;        ///< Highlights, toggle-on, active tab indicator, app name.
  COLORREF controlFill;   ///< Input box / dropdown fill.
  COLORREF controlBorder; ///< Input box / dropdown border.
  COLORREF toggleOff;     ///< Toggle pill color when off.
  COLORREF divider;       ///< Horizontal rule / separator.
  COLORREF tabActive;     ///< Active tab label text.
  COLORREF tabInactive;   ///< Inactive tab label text.
  COLORREF tabBar;        ///< Tab strip background (unused in sidebar mode, kept for compat).
  BYTE     bgAlpha;       ///< Panel background alpha (always 255).
};

/// @return Hard-coded dark palette (existing default colors).
ThemePalette DarkPalette();

/// @return Light palette for use on bright system themes.
ThemePalette LightPalette();

/**
 * @brief Reads the "theme" global setting and returns the matching palette.
 *
 * - "dark"   → DarkPalette()
 * - "light"  → LightPalette()
 * - "system" → DarkPalette() or LightPalette() based on IsSystemDarkMode()
 */
ThemePalette ResolveTheme();
