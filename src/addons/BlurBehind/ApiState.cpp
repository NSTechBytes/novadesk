/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ApiState.h"
#include <Windows.h>

// ============================================================================
// Static state
// ============================================================================

namespace ApiState {

PSET_COMPOSITION SetWindowCompositionAttribute = nullptr;
PSET_ATTRIBUTE   SetWindowAttribute            = nullptr;

} // namespace ApiState

namespace {
static HMODULE   s_user32  = nullptr;
static HMODULE   s_dwmapi  = nullptr;
static uint32_t  s_refs    = 0;
static bool      s_inited  = false;

void LoadUser32() noexcept {
  s_user32 = LoadLibraryW(L"user32.dll");
  if (s_user32) {
    DisableThreadLibraryCalls(s_user32);
    ApiState::SetWindowCompositionAttribute =
        reinterpret_cast<ApiState::PSET_COMPOSITION>(
            GetProcAddress(s_user32, "SetWindowCompositionAttribute"));
  }
}

void UnloadUser32() noexcept {
  ApiState::SetWindowCompositionAttribute = nullptr;
  if (s_user32) {
    FreeLibrary(s_user32);
    s_user32 = nullptr;
  }
}

void LoadDwmapi() noexcept {
  s_dwmapi = LoadLibraryW(L"dwmapi.dll");
  if (s_dwmapi) {
    DisableThreadLibraryCalls(s_dwmapi);
    ApiState::SetWindowAttribute =
        reinterpret_cast<ApiState::PSET_ATTRIBUTE>(
            GetProcAddress(s_dwmapi, "DwmSetWindowAttribute"));
  }
}

void UnloadDwmapi() noexcept {
  ApiState::SetWindowAttribute = nullptr;
  if (s_dwmapi) {
    FreeLibrary(s_dwmapi);
    s_dwmapi = nullptr;
  }
}
} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

namespace ApiState {

void Initialize() noexcept {
  if (!s_inited) {
    s_inited = true;
    LoadUser32();
    LoadDwmapi();
  }
  ++s_refs;
}

bool IsUser32Loaded() noexcept { return s_user32 != nullptr; }
bool IsDwmapiLoaded() noexcept { return s_dwmapi != nullptr; }

void Finalize() noexcept {
  if (s_refs == 0) return;
  if (--s_refs > 0) return;
  s_inited = false;
  UnloadUser32();
  UnloadDwmapi();
}

} // namespace ApiState
