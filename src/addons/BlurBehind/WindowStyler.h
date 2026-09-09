/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "BlurBehind.h"
#include <Windows.h>

// ============================================================================
// WindowStyler — all DWM / SetWindowCompositionAttribute calls
// ============================================================================

namespace WindowStyler {

/// Apply accent policy (blur/acrylic) via SetWindowCompositionAttribute.
void SetAccent(HWND hwnd,
               BB::Accent accent,
               BB::Effect effect) noexcept;

/// Set rounded corners (Win11+).
void SetCorner(HWND hwnd, BB::Corner corner) noexcept;

/// Set border / stroke colour (Win11+).
void SetStroke(HWND hwnd, BB::Stroke stroke) noexcept;

/// Clear all composition effects on the window.
void Disable(HWND hwnd) noexcept;

} // namespace WindowStyler
