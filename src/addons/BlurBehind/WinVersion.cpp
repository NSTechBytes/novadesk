/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WinVersion.h"

#include <Windows.h>

// ============================================================================
// Build constants
// ============================================================================

static constexpr DWORD BUILD_WIN10 = 16299; // Fall Creators Update
static constexpr DWORD BUILD_WIN11 = 22000; // Windows 11 initial release

// ============================================================================
// State
// ============================================================================

namespace {
bool g_initialized = false;
bool g_isWin10 = false;
bool g_isWin11 = false;

using RtlGetVersionFn = LONG(WINAPI *)(PRTL_OSVERSIONINFOW);

} // anonymous namespace

// ============================================================================
// Public API
// ============================================================================

namespace WinVersion {

void Initialize() noexcept {
  if (g_initialized)
    return;
  g_initialized = true;

  HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
  if (ntdll) {
    auto pRtlGetVersion = reinterpret_cast<RtlGetVersionFn>(
        GetProcAddress(ntdll, "RtlGetVersion"));
    if (pRtlGetVersion) {
      RTL_OSVERSIONINFOW rovi = {};
      rovi.dwOSVersionInfoSize = sizeof(rovi);
      if (pRtlGetVersion(&rovi) == 0) {
        const DWORD build = rovi.dwBuildNumber;
        g_isWin10 = (build >= BUILD_WIN10);
        g_isWin11 = (build >= BUILD_WIN11);
        return;
      }
    }
  }

  // Fallback using VerifyVersionInfoW
  OSVERSIONINFOEXW osvi = {};
  osvi.dwOSVersionInfoSize = sizeof(osvi);
  osvi.dwMajorVersion = 10;
  osvi.dwBuildNumber = BUILD_WIN10;
  DWORDLONG mask = VerSetConditionMask(0, VER_MAJORVERSION, VER_GREATER_EQUAL);
  mask = VerSetConditionMask(mask, VER_BUILDNUMBER, VER_GREATER_EQUAL);

  g_isWin10 = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_BUILDNUMBER,
                                 mask) != FALSE;
  osvi.dwBuildNumber = BUILD_WIN11;
  g_isWin11 = VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_BUILDNUMBER,
                                 mask) != FALSE;
}

bool IsWin10() noexcept { return g_isWin10; }
bool IsWin11() noexcept { return g_isWin11; }

} // namespace WinVersion
