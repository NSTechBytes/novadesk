/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SYSTEM_H__
#define __NOVADESK_SYSTEM_H__

#include <windows.h>
#include <string>
#include <vector>

/**
 * @brief Window Z-order position constants.
 */
enum ZPOSITION {
  ZPOSITION_ONDESKTOP = -2, ///< Behind the desktop icons.
  ZPOSITION_ONBOTTOM = -1,  ///< Below all other windows.
  ZPOSITION_NORMAL = 0,     ///< Standard window layering.
  ZPOSITION_ONTOP = 1,      ///< Above normal windows.
  ZPOSITION_ONTOPMOST = 2   ///< Always on top of all windows.
};

/**
 * @brief Information about a single display monitor.
 */
struct MonitorInfo {
  bool active;              ///< True if this is the primary monitor.
  HMONITOR handle;          ///< Win32 monitor handle.
  RECT screen;              ///< Full screen bounds.
  RECT work;                ///< Work area (excluding taskbar).
  std::wstring deviceName;  ///< Device name (e.g., "\\.\DISPLAY1").
  std::wstring monitorName; ///< Friendly monitor name from EDID.
};

/**
 * @brief Aggregate information about all connected monitors.
 */
struct MultiMonitorInfo {
  int vsT, vsL, vsH, vsW; ///< Virtual screen top, left, height, width.
  int primary;             ///< Primary monitor handle.
  int primaryIndex;        ///< Index of the primary monitor.
  std::vector<MonitorInfo> monitors; ///< List of all monitors.
};

/**
 * @brief Manages the desktop environment, monitor detection, and window layering.
 *
 * @note Must call Initialize() before use and Finalize() on shutdown.
 */
class System {
public:
  /**
   * @brief Initializes the system and enumerates monitors.
   *
   * @param instance Application instance handle.
   */
  static void Initialize(HINSTANCE instance);

  /// Cleans up system resources.
  static void Finalize();

  /// @return The backmost top-level window handle.
  static HWND GetBackmostTopWindow();

  /**
   * @brief Gets the helper window handle for desktop icon management.
   *
   * @return The helper window HWND.
   */
  static HWND GetHelperWindow() { return c_HelperWindow; }

  /**
   * @brief Gets the main system window handle.
   *
   * @return The system window HWND.
   */
  static HWND GetWindow() { return c_Window; }

  /**
   * @brief Prepares the helper window for desktop icon integration.
   *
   * @param desktopIconsHostWindow Optional host window for desktop icons.
   */
  static void PrepareHelperWindow(HWND desktopIconsHostWindow = nullptr);

  /// @return The desktop icons host window handle.
  static HWND GetDesktopIconsHostWindow();

  /**
   * @brief Gets the current "Show Desktop" state.
   *
   * @return True if Show Desktop mode is active.
   */
  static bool GetShowDesktop() { return c_ShowDesktop; }

  /**
   * @brief Sets the "Show Desktop" state.
   *
   * @param show True to activate Show Desktop mode.
   */
  static void SetShowDesktop(bool show) { c_ShowDesktop = show; }

  /**
   * @brief Checks if the desktop state has changed.
   *
   * @param desktopIconsHostWindow The desktop icons host window.
   *
   * @return True if state changed and requires handling.
   */
  static bool CheckDesktopState(HWND desktopIconsHostWindow);

  /// Updates Z-order of windows based on current desktop state.
  static void ChangeZPosInOrder();

  /**
   * @brief Gets information about all connected monitors.
   *
   * @return Const reference to the multi-monitor info structure.
   */
  static const MultiMonitorInfo &GetMultiMonitorInfo() { return c_Monitors; }

  /**
   * @brief Gets the number of connected monitors.
   *
   * @return Monitor count.
   */
  static size_t GetMonitorCount() { return c_Monitors.monitors.size(); }

  /**
   * @brief Executes a process.
   *
   * @param target Path to the executable.
   * @param parameters Command-line parameters.
   * @param workingDir Working directory.
   * @param show Window show command.
   *
   * @return True if the process was started.
   */
  static bool Execute(const std::wstring &target,
                      const std::wstring &parameters = L"",
                      const std::wstring &workingDir = L"",
                      int show = SW_SHOWNORMAL);

  /**
   * @brief Sets the desktop wallpaper.
   *
   * @param imagePath Path to the image file.
   * @param style Wallpaper style ("fill", "fit", "stretch", etc.).
   *
   * @return True if successful.
   */
  static bool SetWallpaper(const std::wstring &imagePath,
                           const std::wstring &style = L"fill");

  /**
   * @brief Gets the current wallpaper path.
   *
   * @param outPath Receives the wallpaper path.
   *
   * @return True if successful.
   */
  static bool GetCurrentWallpaperPath(std::wstring &outPath);

  /// Window procedure for the system window.
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);

private:
  static HWND GetDefaultShellWindow();
  static bool ShouldUseShellWindowAsDesktopIconsHost();

  static HWND c_Window;          ///< Main system window.
  static HWND c_HelperWindow;    ///< Helper window for desktop integration.
  static MultiMonitorInfo c_Monitors; ///< Monitor information cache.
  static bool c_ShowDesktop;     ///< Current Show Desktop state.

  static const UINT_PTR TIMER_SHOWDESKTOP = 1;
  static const int INTERVAL_SHOWDESKTOP = 100;
  static const int INTERVAL_RESTOREWINDOWS = 50;
};

#endif
