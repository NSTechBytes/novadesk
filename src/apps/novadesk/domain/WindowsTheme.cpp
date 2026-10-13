/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WindowsTheme.h"
#include "../shared/Settings.h"
#include "../shared/System.h"

#include <atomic>
#include <dwmapi.h>
#include <uxtheme.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

namespace WindowsTheme {

namespace {

enum PreferredAppMode {
  Default,
  AllowDark,
  ForceDark,
  ForceLight,
  Max
};

using fnSetPreferredAppMode = PreferredAppMode(WINAPI *)(PreferredAppMode);
using fnAllowDarkModeForWindow = bool(WINAPI *)(HWND, bool);
using fnAllowDarkModeForApp = bool(WINAPI *)(bool);
using fnRefreshImmersiveColorPolicyState = void(WINAPI *)();
using fnFlushMenuThemes = void(WINAPI *)();
using fnSetWindowTheme = HRESULT(WINAPI *)(HWND, LPCWSTR, LPCWSTR);

struct ThemeState {
  std::atomic<bool> initialized{false};
  std::atomic<bool> isDarkMode{false};
  Mode mode{Mode::System};

  fnSetPreferredAppMode pfnSetPreferredAppMode = nullptr;
  fnAllowDarkModeForWindow pfnAllowDarkModeForWindow = nullptr;
  fnAllowDarkModeForApp pfnAllowDarkModeForApp = nullptr;
  fnRefreshImmersiveColorPolicyState pfnRefreshImmersiveColorPolicyState = nullptr;
  fnFlushMenuThemes pfnFlushMenuThemes = nullptr;
};

ThemeState &GetState() {
  static ThemeState state;
  return state;
}

BOOL CALLBACK ApplyDialogChild(HWND window, LPARAM lParam) {
  WindowsTheme::ApplyToWindow(window);
  return TRUE;
}

LRESULT CALLBACK CBTDialogHookProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code == HCBT_ACTIVATE || code == HCBT_CREATEWND) {
    HWND window = reinterpret_cast<HWND>(wParam);
    if (window && IsWindow(window)) {
      WCHAR className[64] = {};
      if (GetClassNameW(window, className, ARRAYSIZE(className)) > 0) {
        if (wcscmp(className, L"#32770") == 0) {
          WindowsTheme::ApplyToDialog(window);
          EnumChildWindows(window, ApplyDialogChild, 0);
        }
      }
    }
  }
  return CallNextHookEx(nullptr, code, wParam, lParam);
}

} // namespace

void Initialize() {
  auto &state = GetState();
  if (state.initialized.exchange(true)) {
    Refresh();
    return;
  }

  HMODULE hUxtheme = GetModuleHandleW(L"uxtheme.dll");
  if (!hUxtheme) {
    hUxtheme = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  }

  if (hUxtheme) {
    state.pfnSetPreferredAppMode = reinterpret_cast<fnSetPreferredAppMode>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135)));
    state.pfnAllowDarkModeForWindow = reinterpret_cast<fnAllowDarkModeForWindow>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(133)));
    state.pfnAllowDarkModeForApp = reinterpret_cast<fnAllowDarkModeForApp>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(132)));
    state.pfnRefreshImmersiveColorPolicyState =
        reinterpret_cast<fnRefreshImmersiveColorPolicyState>(
            GetProcAddress(hUxtheme, MAKEINTRESOURCEA(104)));
    state.pfnFlushMenuThemes = reinterpret_cast<fnFlushMenuThemes>(
        GetProcAddress(hUxtheme, MAKEINTRESOURCEA(136)));
  }

  Refresh();
}

void Refresh() {
  auto &state = GetState();

  std::string pref = Settings::GetGlobalString("windowsTheme", "");
  if (pref.empty()) {
    pref = Settings::GetGlobalString("theme", "system");
  }

  if (pref == "dark") {
    state.mode = Mode::Dark;
    state.isDarkMode = true;
  } else if (pref == "light") {
    state.mode = Mode::Light;
    state.isDarkMode = false;
  } else {
    state.mode = Mode::System;
    state.isDarkMode = novadesk::shared::system::IsSystemDarkMode();
  }

  PreferredAppMode appMode = PreferredAppMode::Default;
  if (state.mode == Mode::Dark) {
    appMode = PreferredAppMode::ForceDark;
  } else if (state.mode == Mode::Light) {
    appMode = PreferredAppMode::ForceLight;
  } else {
    appMode = PreferredAppMode::AllowDark;
  }

  if (state.pfnSetPreferredAppMode) {
    state.pfnSetPreferredAppMode(appMode);
  } else if (state.pfnAllowDarkModeForApp) {
    state.pfnAllowDarkModeForApp(state.isDarkMode);
  }

  if (state.pfnRefreshImmersiveColorPolicyState) {
    state.pfnRefreshImmersiveColorPolicyState();
  }

  if (state.pfnFlushMenuThemes) {
    state.pfnFlushMenuThemes();
  }
}

bool IsUsingDarkMode() {
  return GetState().isDarkMode.load();
}

Mode GetMode() {
  return GetState().mode;
}

void SetMode(Mode mode) {
  auto &state = GetState();
  state.mode = mode;
  switch (mode) {
  case Mode::Dark:
    Settings::SetGlobalString("windowsTheme", "dark");
    break;
  case Mode::Light:
    Settings::SetGlobalString("windowsTheme", "light");
    break;
  case Mode::System:
  default:
    Settings::SetGlobalString("windowsTheme", "system");
    break;
  }
  Refresh();
}

void SetMode(const std::string &modeStr) {
  Settings::SetGlobalString("windowsTheme", modeStr);
  Refresh();
}

void ApplyToWindow(HWND window) {
  if (!window || !IsWindow(window)) return;

  auto &state = GetState();
  const BOOL dark = state.isDarkMode.load() ? TRUE : FALSE;

  // Titlebar / Non-client area
  DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &dark, sizeof(dark));

  // Per-window dark mode toggle for Win32 menus and scrollbars
  if (state.pfnAllowDarkModeForWindow) {
    state.pfnAllowDarkModeForWindow(window, dark != FALSE);
  }

  // Common controls theme subclass
  if (dark) {
    SetWindowTheme(window, L"DarkMode_Explorer", nullptr);
  } else {
    SetWindowTheme(window, L"Explorer", nullptr);
  }
}

void ApplyToDialog(HWND dialog) {
  if (!dialog || !IsWindow(dialog)) return;
  ApplyToWindow(dialog);
}

void PrepareMenu(HWND owner) {
  Refresh();
  auto &state = GetState();
  if (owner && IsWindow(owner)) {
    if (state.pfnAllowDarkModeForWindow) {
      state.pfnAllowDarkModeForWindow(owner, state.isDarkMode.load());
    }
  }
  if (state.pfnFlushMenuThemes) {
    state.pfnFlushMenuThemes();
  }
}

DialogHookScope::DialogHookScope() {
  WindowsTheme::Initialize();
  m_Hook = SetWindowsHookExW(WH_CBT, CBTDialogHookProc, nullptr, GetCurrentThreadId());
}

DialogHookScope::~DialogHookScope() {
  if (m_Hook) {
    UnhookWindowsHookEx(m_Hook);
    m_Hook = nullptr;
  }
}

} // namespace WindowsTheme
