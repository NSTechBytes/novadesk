/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <Windows.h>

// ============================================================================
// WinVersion — lazy-initialised Windows build detection
// ============================================================================

namespace WinVersion {

/// Must be called once before any Is* query.
void Initialize() noexcept;

/// Returns true if running on Windows 10 build 16299 (Fall Creators Update) or later.
bool IsWin10() noexcept;

/// Returns true if running on Windows 11 build 22000 or later.
bool IsWin11() noexcept;

} // namespace WinVersion
