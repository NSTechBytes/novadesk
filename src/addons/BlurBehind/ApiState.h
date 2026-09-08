/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <Windows.h>
#include <cstdint>

// ============================================================================
// ApiState — reference-counted loader for user32.dll and dwmapi.dll
// ============================================================================
// Multiple addon instances can share a single DLL load.  The first call to
// Initialize() loads the DLLs; the last call to Finalize() unloads them.
// ============================================================================

namespace ApiState {

// Raw function-pointer typedefs (kept in the header so WindowStyler can use them)
using PSET_COMPOSITION = void(__stdcall *)(HWND hWnd, void *pData);
using PSET_ATTRIBUTE   = HRESULT(__stdcall *)(HWND hWnd, DWORD dwAttribute, LPCVOID pvAttribute, DWORD cbAttribute);

/// Loaded function pointers — nullptr until Initialize() succeeds.
extern PSET_COMPOSITION SetWindowCompositionAttribute;
extern PSET_ATTRIBUTE   SetWindowAttribute;

/// Load DLLs and increment the reference count.
void Initialize() noexcept;

/// Returns true if user32.dll was loaded successfully.
bool IsUser32Loaded() noexcept;

/// Returns true if dwmapi.dll was loaded successfully.
bool IsDwmapiLoaded() noexcept;

/// Decrement the reference count; unload DLLs when it reaches zero.
void Finalize() noexcept;

} // namespace ApiState
