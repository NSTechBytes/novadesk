/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WidgetContextMenuHelper.h"

#include <string>
#include <cstdlib>
#include <commctrl.h>
#include <shellapi.h>

#include "Widget.h"
#include "SettingsPanel.h"
#include "../shared/MenuUtils.h"
#include "PathUtils.h"
#include "../render/FontCache.h"
#include "../render/ImageCache.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "Settings.h"
#include "../../common/Localization.h"
#include "../../common/WindowsTheme.h"

namespace {
constexpr int CMD_REFRESH = 1001;
constexpr int CMD_CLOSE = 1002;
constexpr int CMD_EXIT = 1003;

constexpr int CMD_MANAGE_ZPOS_NORMAL = 1101;
constexpr int CMD_MANAGE_ZPOS_DESKTOP = 1102;
constexpr int CMD_MANAGE_ZPOS_BOTTOM = 1103;
constexpr int CMD_MANAGE_ZPOS_ONTOP = 1104;
constexpr int CMD_MANAGE_ZPOS_ONTOPMOST = 1105;

constexpr int CMD_MANAGE_OPACITY_START = 1110;

constexpr int CMD_MANAGE_DRAGGABLE = 1130;
constexpr int CMD_MANAGE_CLICKTHROUGH = 1131;
constexpr int CMD_MANAGE_SNAPEDGES = 1132;
constexpr int CMD_MANAGE_KEEPOFFSCREEN = 1133;

constexpr int CMD_SETTINGS_ENABLE_LOGGING = 1140;
constexpr int CMD_SETTINGS_ENABLE_DEBUGGING = 1141;
constexpr int CMD_SETTINGS_SAVE_LOG_TO_FILE = 1142;
constexpr int CMD_SETTINGS_USE_HW_ACCEL = 1143;
constexpr int CMD_SETTINGS_CLEAR_IMAGE_CACHE = 1144;
constexpr int CMD_SETTINGS_CLEAR_FONT_CACHE = 1145;
constexpr int CMD_OPEN_SETTINGS = 1146;

const std::wstring &Loc(const char *key, const wchar_t *fallback) {
  return Localization::TextOr(key, fallback);
}

static void PromptRestartForHardwareAcceleration(HWND hwndParent) {
  const std::wstring appTitle = PathUtils::GetProductName();
  const std::wstring mainInstruction =
      appTitle + L" needs restart for this change.";
  const std::wstring content =
      L"Hardware acceleration changes apply after restarting " + appTitle +
      L".";

  const int kRestartNowButtonId = 1001;
  const TASKDIALOG_BUTTON buttons[] = {
      {kRestartNowButtonId, L"Restart Now"},
      {IDCANCEL, L"Later"},
  };

  TASKDIALOGCONFIG config{};
  config.cbSize = sizeof(config);
  config.hwndParent = hwndParent;
  config.dwFlags =
      TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
  config.dwCommonButtons = 0;
  config.pszWindowTitle = appTitle.c_str();
  config.pszMainInstruction = mainInstruction.c_str();
  config.pszContent = content.c_str();
  config.cButtons = ARRAYSIZE(buttons);
  config.pButtons = buttons;
  config.nDefaultButton = kRestartNowButtonId;

  auto doRestart = []() {
    Settings::Flush();
    const wchar_t *rawCmd = GetCommandLineW();
    std::wstring restartCmd = L"/c ping 127.0.0.1 -n 2 > nul & " +
                              std::wstring(rawCmd ? rawCmd : L"");
    ShellExecuteW(nullptr, L"open", L"cmd.exe", restartCmd.c_str(), nullptr,
                  SW_HIDE);
    PostQuitMessage(0);
  };

  int selectedButton = IDCANCEL;
  HRESULT hr = TaskDialogIndirect(&config, &selectedButton, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    if (selectedButton == kRestartNowButtonId) {
      doRestart();
    }
    return;
  }

  const std::wstring fallbackMsg = mainInstruction + L"\n\nRestart now?";
  const int fallback =
      MessageBoxW(hwndParent, fallbackMsg.c_str(), appTitle.c_str(),
                  MB_YESNO | MB_ICONINFORMATION);
  if (fallback == IDYES) {
    doRestart();
  }
}
} // namespace

namespace WidgetContextMenuHelper {
int ShowContextMenu(HWND hWnd, const std::vector<MenuItem> &customMenu,
                    bool showDefaultItems, ZPOSITION windowZPos,
                    const WidgetOptions &options, bool hasSettings) {
  POINT pt{};
  GetCursorPos(&pt);

  HMENU hMenu = CreatePopupMenu();
  MenuUtils::BuildMenu(hMenu, customMenu);

  if (showDefaultItems) {
    if (!customMenu.empty()) {
      MenuUtils::AppendItem(hMenu, MF_SEPARATOR, 0, nullptr);
    }

    HMENU hManageMenu = CreatePopupMenu();

    HMENU hZPosMenu = CreatePopupMenu();
    MenuUtils::AppendItem(
        hZPosMenu,
        MF_STRING | (windowZPos == ZPOSITION_NORMAL ? MF_CHECKED : 0),
        CMD_MANAGE_ZPOS_NORMAL, Loc("context.zpos.normal", L"Normal").c_str());
    MenuUtils::AppendItem(
        hZPosMenu,
        MF_STRING | (windowZPos == ZPOSITION_ONDESKTOP ? MF_CHECKED : 0),
        CMD_MANAGE_ZPOS_DESKTOP,
        Loc("context.zpos.onDesktop", L"OnDesktop").c_str());
    MenuUtils::AppendItem(
        hZPosMenu,
        MF_STRING | (windowZPos == ZPOSITION_ONTOP ? MF_CHECKED : 0),
        CMD_MANAGE_ZPOS_ONTOP, Loc("context.zpos.onTop", L"OnTop").c_str());
    MenuUtils::AppendItem(
        hZPosMenu,
        MF_STRING | (windowZPos == ZPOSITION_ONTOPMOST ? MF_CHECKED : 0),
        CMD_MANAGE_ZPOS_ONTOPMOST,
        Loc("context.zpos.onTopMost", L"OnTopMost").c_str());
    MenuUtils::AppendItem(
        hZPosMenu,
        MF_STRING | (windowZPos == ZPOSITION_ONBOTTOM ? MF_CHECKED : 0),
        CMD_MANAGE_ZPOS_BOTTOM, Loc("context.zpos.bottom", L"Bottom").c_str());
    MenuUtils::AppendItem(hManageMenu, MF_POPUP,
                          reinterpret_cast<UINT_PTR>(hZPosMenu),
                          Loc("context.zPosition", L"Zpos").c_str());

    HMENU hOpacityMenu = CreatePopupMenu();
    for (int i = 0; i <= 100; i += 10) {
      const BYTE targetOpacity = static_cast<BYTE>(i * 255 / 100);
      const std::wstring label = std::to_wstring(i) + L"%";
      const bool isCurrent = std::abs(static_cast<int>(options.windowOpacity) -
                                      static_cast<int>(targetOpacity)) < 5;
      MenuUtils::AppendItem(hOpacityMenu,
                            MF_STRING | (isCurrent ? MF_CHECKED : 0),
                            CMD_MANAGE_OPACITY_START + (i / 10), label.c_str());
    }
    MenuUtils::AppendItem(hManageMenu, MF_POPUP,
                          reinterpret_cast<UINT_PTR>(hOpacityMenu),
                          Loc("context.opacity", L"Opacity").c_str());
    MenuUtils::AppendItem(hManageMenu, MF_SEPARATOR, 0, nullptr);

    MenuUtils::AppendItem(hManageMenu,
                          MF_STRING | (options.draggable ? MF_CHECKED : 0),
                          CMD_MANAGE_DRAGGABLE,
                          Loc("context.draggable", L"Draggable").c_str());
    MenuUtils::AppendItem(hManageMenu,
                          MF_STRING | (options.clickThrough ? MF_CHECKED : 0),
                          CMD_MANAGE_CLICKTHROUGH,
                          Loc("context.clickThrough", L"Clickthrough").c_str());
    MenuUtils::AppendItem(hManageMenu,
                          MF_STRING | (options.snapEdges ? MF_CHECKED : 0),
                          CMD_MANAGE_SNAPEDGES,
                          Loc("context.snapEdges", L"Snap to Edges").c_str());
    MenuUtils::AppendItem(hManageMenu,
                          MF_STRING | (options.keepOnScreen ? MF_CHECKED : 0),
                          CMD_MANAGE_KEEPOFFSCREEN,
                          Loc("context.keepOnScreen", L"Keep On Screen").c_str());
    MenuUtils::AppendItem(hMenu, MF_POPUP,
                          reinterpret_cast<UINT_PTR>(hManageMenu),
                          Loc("context.manage", L"Manage").c_str());

    if (hasSettings) {
      MenuUtils::AppendItem(hMenu, MF_STRING, CMD_OPEN_SETTINGS,
                            Loc("context.settingsPanel", L"SettingsPanel")
                                .c_str());
    }

    HMENU hAppMenu = CreatePopupMenu();

    HMENU hSettingsMenu = CreatePopupMenu();
    const bool enableLogging =
        !Settings::GetGlobalBool("disableLogging", false);
    const bool enableDebugging =
        Settings::GetGlobalBool("enableDebugging", false);
    const bool saveLogToFile = Settings::GetGlobalBool("saveLogToFile", false);
    const bool useHwAccel =
        Settings::GetGlobalBool("useHardwareAcceleration", false);

    MenuUtils::AppendItem(
        hSettingsMenu, MF_STRING | (enableLogging ? MF_CHECKED : 0),
        CMD_SETTINGS_ENABLE_LOGGING,
        Loc("context.enableLogging", L"Enable Logging").c_str());
    MenuUtils::AppendItem(
        hSettingsMenu, MF_STRING | (enableDebugging ? MF_CHECKED : 0),
        CMD_SETTINGS_ENABLE_DEBUGGING,
        Loc("context.enableDebugging", L"Enable Debugging").c_str());
    MenuUtils::AppendItem(
        hSettingsMenu, MF_STRING | (saveLogToFile ? MF_CHECKED : 0),
        CMD_SETTINGS_SAVE_LOG_TO_FILE,
        Loc("context.saveLogToFile", L"Save Log to file").c_str());
    MenuUtils::AppendItem(
        hSettingsMenu, MF_STRING | (useHwAccel ? MF_CHECKED : 0),
        CMD_SETTINGS_USE_HW_ACCEL,
        Loc("context.useHardwareAcceleration", L"Use hardware acceleration")
            .c_str());
    MenuUtils::AppendItem(hSettingsMenu, MF_STRING,
                          CMD_SETTINGS_CLEAR_IMAGE_CACHE,
                          Loc("context.clearImageCache", L"Clear Image Cache")
                              .c_str());
    MenuUtils::AppendItem(hSettingsMenu, MF_STRING,
                          CMD_SETTINGS_CLEAR_FONT_CACHE,
                          Loc("context.clearFontCache", L"Clear Font Cache")
                              .c_str());

    MenuUtils::AppendItem(hAppMenu, MF_POPUP,
                          reinterpret_cast<UINT_PTR>(hSettingsMenu),
                          Loc("context.settings", L"Settings").c_str());
    MenuUtils::AppendItem(hAppMenu, MF_SEPARATOR, 0, nullptr);
    MenuUtils::AppendItem(hAppMenu, MF_STRING, CMD_REFRESH,
                          Loc("context.refresh", L"Refresh").c_str());
    MenuUtils::AppendItem(hAppMenu, MF_STRING, CMD_EXIT,
                          Loc("context.exit", L"Exit").c_str());
    std::wstring appTitle = PathUtils::GetProductName();
    MenuUtils::AppendItem(hMenu, MF_POPUP,
                          reinterpret_cast<UINT_PTR>(hAppMenu),
                          appTitle.c_str());
  }

  Widget::SetMenuActive(true);

  HWND foregroundWindow = GetForegroundWindow();
  if (foregroundWindow && foregroundWindow != hWnd) {
    const DWORD foregroundThreadID =
        GetWindowThreadProcessId(foregroundWindow, nullptr);
    const DWORD currentThreadID = GetCurrentThreadId();
    AttachThreadInput(currentThreadID, foregroundThreadID, TRUE);
    SetForegroundWindow(hWnd);
    AttachThreadInput(currentThreadID, foregroundThreadID, FALSE);
  } else {
    SetForegroundWindow(hWnd);
  }

  WindowsTheme::PrepareMenu(hWnd);

  // Same pattern as Rainmeter ContextMenu::DisplayMenu: TPM_LEFTALIGN and
  // TPM_LAYOUTRTL (applied inside MenuUtils::TrackPopup when UI is RTL).
  const int cmd = MenuUtils::TrackPopup(
      hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
      pt.x, pt.y, hWnd);
  DestroyMenu(hMenu);

  Widget::SetMenuActive(false);
  return cmd;
}

void HandleContextCommand(Widget &widget, int cmd) {
  const WidgetOptions &options = widget.GetOptions();
  if (cmd >= 2000) {
    JSEngine::OnWidgetContextCommand(options.id, cmd);
    return;
  }
  if (cmd == CMD_REFRESH) {
    JSEngine::Reload();
    return;
  }
  if (cmd == CMD_CLOSE) {
    DestroyWindow(widget.GetWindow());
    return;
  }
  if (cmd == CMD_EXIT) {
    PostQuitMessage(0);
    return;
  }
  if (cmd == CMD_MANAGE_ZPOS_NORMAL) {
    widget.ChangeZPos(ZPOSITION_NORMAL);
    return;
  }
  if (cmd == CMD_MANAGE_ZPOS_DESKTOP) {
    widget.ChangeZPos(ZPOSITION_ONDESKTOP);
    return;
  }
  if (cmd == CMD_MANAGE_ZPOS_BOTTOM) {
    widget.ChangeZPos(ZPOSITION_ONBOTTOM);
    return;
  }
  if (cmd == CMD_MANAGE_ZPOS_ONTOP) {
    widget.ChangeZPos(ZPOSITION_ONTOP);
    return;
  }
  if (cmd == CMD_MANAGE_ZPOS_ONTOPMOST) {
    widget.ChangeZPos(ZPOSITION_ONTOPMOST);
    return;
  }
  if (cmd >= CMD_MANAGE_OPACITY_START && cmd <= CMD_MANAGE_OPACITY_START + 10) {
    const int percent = (cmd - CMD_MANAGE_OPACITY_START) * 10;
    widget.SetWindowOpacity(static_cast<BYTE>(percent * 255 / 100));
    return;
  }
  if (cmd == CMD_MANAGE_DRAGGABLE) {
    widget.SetDraggable(!options.draggable);
    return;
  }
  if (cmd == CMD_MANAGE_CLICKTHROUGH) {
    widget.SetClickThrough(!options.clickThrough);
    return;
  }
  if (cmd == CMD_MANAGE_SNAPEDGES) {
    widget.SetSnapEdges(!options.snapEdges);
    return;
  }
  if (cmd == CMD_MANAGE_KEEPOFFSCREEN) {
    widget.SetKeepOnScreen(!options.keepOnScreen);
    return;
  }
  if (cmd == CMD_SETTINGS_ENABLE_LOGGING) {
    const bool currentlyEnabled =
        !Settings::GetGlobalBool("disableLogging", false);
    Settings::SetGlobalBool("disableLogging", currentlyEnabled);
    Settings::ApplyGlobalSettings();
    Settings::Save();
    return;
  }
  if (cmd == CMD_SETTINGS_ENABLE_DEBUGGING) {
    const bool current = Settings::GetGlobalBool("enableDebugging", false);
    Settings::SetGlobalBool("enableDebugging", !current);
    Settings::ApplyGlobalSettings();
    Settings::Save();
    return;
  }
  if (cmd == CMD_SETTINGS_SAVE_LOG_TO_FILE) {
    const bool current = Settings::GetGlobalBool("saveLogToFile", false);
    Settings::SetGlobalBool("saveLogToFile", !current);
    Settings::ApplyGlobalSettings();
    Settings::Save();
    return;
  }
  if (cmd == CMD_SETTINGS_USE_HW_ACCEL) {
    const bool current =
        Settings::GetGlobalBool("useHardwareAcceleration", false);
    Settings::SetGlobalBool("useHardwareAcceleration", !current);
    Settings::ApplyGlobalSettings();
    Settings::Save();
    PromptRestartForHardwareAcceleration(widget.GetWindow());
    return;
  }
  if (cmd == CMD_SETTINGS_CLEAR_IMAGE_CACHE) {
    ImageCache::Clear();
    return;
  }
  if (cmd == CMD_SETTINGS_CLEAR_FONT_CACHE) {
    FontCache::Clear();
    return;
  }
  if (cmd == CMD_OPEN_SETTINGS) {
    SettingsPanel::OpenFor(&widget);
    return;
  }
}
} // namespace WidgetContextMenuHelper
