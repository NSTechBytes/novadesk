/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>
#include <string>

namespace WindowsTheme {

enum class Mode {
  System,
  Dark,
  Light
};

/// Initialize theme hooks and uxtheme/dwmapi exports.
void Initialize();

/// Refresh theme mode (e.g. after setting change or WM_SETTINGCHANGE).
void Refresh();

/// Returns whether dark mode is currently effective.
bool IsUsingDarkMode();

/// Returns current configured mode.
Mode GetMode();

/// Sets preferred mode (Mode::System, Mode::Dark, Mode::Light).
void SetMode(Mode mode);

/// Sets mode by string ("system", "dark", "light").
void SetMode(const std::string &modeStr);

/// Apply theme (dark / light) to a standard Win32 window (including titlebar).
void ApplyToWindow(HWND window);

/// Apply theme to a dialog or control hierarchy.
void ApplyToDialog(HWND dialog);

/// Prepare and update menu theme right before showing a popup menu.
void PrepareMenu(HWND owner);

/// Scope-based helper to theme dialogs created in the current thread (e.g. MessageBox, FileDialog).
class DialogHookScope {
public:
  DialogHookScope();
  ~DialogHookScope();

  DialogHookScope(const DialogHookScope &) = delete;
  DialogHookScope &operator=(const DialogHookScope &) = delete;

private:
  HHOOK m_Hook = nullptr;
};

} // namespace WindowsTheme
