/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WindowStyler.h"
#include "ApiState.h"
#include "WinVersion.h"

#pragma comment(lib, "dwmapi.lib")
#include <dwmapi.h>

// ============================================================================
// Internal structures (mirror SetWindowCompositionAttribute ABI)
// ============================================================================

namespace {

static const int WCA_ACCENT_POLICY = 19;

struct AccentPolicy {
  BB::Accent Accent;
  uint32_t   Flags;    // Effect bits (low byte)
  uint32_t   Color;
  BYTE       Reserved;
};

struct CompositionAttribute {
  int           Attribute; // WCA_ACCENT_POLICY
  AccentPolicy *Data;
  DWORD         Size;
};

// ---- Helpers ----------------------------------------------------------------

void ApplyComposition(HWND hwnd, CompositionAttribute attr) noexcept {
  if (!ApiState::SetWindowCompositionAttribute) return;
  ApiState::SetWindowCompositionAttribute(hwnd, &attr);
}

void ApplyAttribute(HWND hwnd, DWORD attribute, const int &value) noexcept {
  if (!ApiState::SetWindowAttribute) return;
  ApiState::SetWindowAttribute(hwnd, attribute, &value, sizeof(value));
}

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

namespace WindowStyler {

void SetAccent(HWND hwnd,
               BB::Accent accent,
               BB::Effect effect) noexcept {
  if (!hwnd || !ApiState::IsUser32Loaded()) return;

  AccentPolicy policy  = {};
  policy.Accent        = accent;
  policy.Flags         = static_cast<uint32_t>(static_cast<uint8_t>(effect));
  policy.Color         = (accent == BB::Accent::DEFAULT) ? 0 : 0x01000000;
  policy.Reserved      = 0;

  CompositionAttribute data = {};
  data.Attribute = WCA_ACCENT_POLICY;
  data.Data      = &policy;
  data.Size      = sizeof(policy);

  ApplyComposition(hwnd, data);
}

void SetCorner(HWND hwnd, BB::Corner corner) noexcept {
  if (!hwnd || !ApiState::IsDwmapiLoaded() || !WinVersion::IsWin11()) return;
  const int value = static_cast<int>(corner);
  ApplyAttribute(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE, value);
}

void SetStroke(HWND hwnd, BB::Stroke stroke) noexcept {
  if (!hwnd || !ApiState::IsDwmapiLoaded() || !WinVersion::IsWin11()) return;
  const int value = static_cast<int>(stroke);
  ApplyAttribute(hwnd, DWMWA_BORDER_COLOR, value);
}

void Disable(HWND hwnd) noexcept {
  if (!hwnd) return;
  SetAccent(hwnd, BB::Accent::DEFAULT, BB::Effect::DEFAULT);
}

} // namespace WindowStyler
