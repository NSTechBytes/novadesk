/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

/**
 * @brief Creates fonts derived from the Windows system UI (menu) font.
 *
 * All Novadesk Win32 apps build their UI fonts from the same source Windows
 * uses for its own chrome, so text metrics match native UI at any DPI.
 */
namespace FontUtil {

/**
 * @brief Creates a font based on the system UI (menu) font, scaled to a DPI.
 *
 * Fetches SPI_GETNONCLIENTMETRICS, rescales lfHeight from the system DPI to
 * @p dpi (per-monitor DPI support), then optionally overrides the weight and
 * applies an additional size multiplier.
 *
 * @param dpi      Target DPI the font will be used at (e.g. GetDpiForWindow).
 * @param weight   Font weight (FW_NORMAL, FW_BOLD, FW_SEMIBOLD, ...) or a
 *                 negative value to keep the menu font's own weight.
 * @param sizeScalePercent Extra size multiplier applied after DPI scaling;
 *                 100 keeps the size unchanged (e.g. 160 for a ~1.6x title).
 *
 * @return A new HFONT, or nullptr on failure.
 *
 * @warning Caller is responsible for calling DeleteObject on the returned HFONT.
 */
HFONT CreateSystemFont(UINT dpi, int weight = -1, int sizeScalePercent = 100);

}  // namespace FontUtil
