/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <windowsx.h>
#include <shlobj.h>
#include <shellapi.h>
#include <shlwapi.h>
#include <wininet.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <deque>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <cstring>
#include <sstream>
#include <fstream>
#include <iterator>
#include <cwctype>
#include "../../third_party/json/json.hpp"

#include "resource.h"
#include "../../common/WindowsTheme.h"
#include "../../common/Dialog.h"
#include "../../common/DpiUtil.h"
#include "../../common/AppPaths.h"
#include "../../common/FontUtil.h"
#include "../../common/NovadeskPaths.h"
#include "../../common/CommonSettings.h"
#include "../../common/Localization.h"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shlwapi.lib")

// ============================================================================
// Data Structures
// ============================================================================

struct WidgetEntry {
  std::wstring name;
  std::wstring scriptPath;
  bool loaded = false;
};

struct AddonEntry {
  std::wstring name;
  std::wstring author;
  std::wstring version;
  std::wstring copyright;
};

// ============================================================================
// UI Globals & Constants
// ============================================================================

static HWND g_list = nullptr;
static std::vector<WidgetEntry> g_widgets;
static HFONT g_buttonFont = nullptr;
static const int kWindowWidth = 900;
static const int kWindowHeight = 560;
static const UINT kTrayMessage = WM_APP + 1;
static const UINT kTrayMenuCommand = WM_APP + 2;
static const int kTrayMenuExitId = 500;
static const int kTrayMenuShowHideId = 501;
static const int kTrayMenuHomeWebsiteId = 510;
static const int kTrayMenuHomeDonateId = 511;
static const int kTrayMenuDocsMainId = 520;
static const int kTrayMenuDocsApiId = 521;
static const int kTrayMenuSettingsOpenId = 530;
static const int kTrayMenuSettingsCheckUpdatesId = 531;
static NOTIFYICONDATAW g_tray{};
static PROCESS_INFORMATION g_novadeskProcess{};
static bool g_novadeskRunning = false;
static HMENU g_trayMenu = nullptr;
static HWND g_btnRefreshList = nullptr;
static HWND g_btnClearLogs = nullptr;
static HWND g_btnLoad = nullptr;
static HWND g_btnRefresh = nullptr;
static HWND g_btnRefreshAll = nullptr;
static HWND g_btnClose = nullptr;
static HWND g_tab = nullptr;
static HWND g_logsList = nullptr;
static HIMAGELIST g_logsImageList = nullptr;
static HWND g_addonsList = nullptr;
static HWND g_settingsPanel = nullptr;
static HWND g_aboutPanel = nullptr;
static HWND g_settingsTitle = nullptr;
static HWND g_chkRunOnStartup = nullptr;
static HWND g_btnCheckUpdates = nullptr;
static HWND g_chkAutoUpdate = nullptr;
static HWND g_chkEnableLogging = nullptr;
static HWND g_chkEnableDebugging = nullptr;
static HWND g_chkSaveLogToFile = nullptr;
static HWND g_chkUseHardwareAcceleration = nullptr;
static HWND g_lblWindowsTheme = nullptr;
static HWND g_cboWindowsTheme = nullptr;
static bool g_isDarkMode = false;
static HWND g_lblLanguage = nullptr;
static HWND g_cboLanguage = nullptr;
static HBRUSH g_hbrWindowBg = nullptr;
static HBRUSH g_hbrControlBg = nullptr;
static HWND g_aboutLogo = nullptr;
static HWND g_aboutAppName = nullptr;
static HWND g_aboutVersion = nullptr;
static HWND g_aboutCopyright = nullptr;
static HWND g_aboutDivider = nullptr;
static HWND g_aboutHomeLabel = nullptr;
static HWND g_aboutHomeLink = nullptr;
static HWND g_aboutDocsLabel = nullptr;
static HWND g_aboutDocsLink = nullptr;
static HWND g_aboutSupportGroup = nullptr;
static HWND g_aboutSupportTitle = nullptr;
static HWND g_aboutSupportText = nullptr;
static HWND g_aboutDonateButton = nullptr;
static HWND g_aboutBottomNote = nullptr;
static HICON g_windowIconLarge = nullptr;
static HICON g_windowIconSmall = nullptr;
static HWND g_mainWindow = nullptr;
static HANDLE g_novadeskLogRead = nullptr;
static HANDLE g_novadeskLogWrite = nullptr;
static HANDLE g_novadeskLogThread = nullptr;
static std::wstring g_pendingLogLine;
struct ManagedLogRow {
  std::wstring time;
  std::wstring source;
  std::wstring level;
  std::wstring message;
};
// Newest entries at the front. Survives lazy Logs-tab creation so pipe
// output captured before the list exists is not discarded.
static std::deque<ManagedLogRow> g_logRows;
static int g_activeTab = 0;
static int g_logsMessageColumnWidth = 600;
static std::unordered_map<std::wstring, std::wstring> g_savedLoadedScripts;
static bool g_manageAutoCheckForUpdates = false;
static std::wstring g_lastNotifiedUpdateVersion;
static bool g_updateDialogOpen = false;
static bool g_isCheckingForUpdates = false;
static bool g_forceExitOnClose = false;
static ULONG_PTR g_gdiplusToken = 0;
static HBITMAP g_aboutLogoBitmap = nullptr;
static HFONT g_aboutTitleFont = nullptr;
static HFONT g_aboutSectionFont = nullptr;
static HFONT g_aboutBodyFont = nullptr;

static const int kControlIdRefreshList = 101;
static const int kControlIdClearLogs = 108;
static const int kControlIdLoad = 102;
static const int kControlIdRefresh = 104;
static const int kControlIdRefreshAll = 105;
static const int kControlIdClose = 107;
static const int kControlIdTabs = 106;
static const int kControlIdRunOnStartup = 301;
static const int kControlIdCheckUpdates = 302;
static const int kControlIdAutoCheckUpdates = 303;
static const int kControlIdEnableLogging = 304;
static const int kControlIdEnableDebugging = 305;
static const int kControlIdSaveLogToFile = 306;
static const int kControlIdUseHardwareAcceleration = 307;
static const int kControlIdWindowsTheme = 308;
static const int kControlIdLanguage = 309;
static const int kControlIdAboutHome = 201;
static const int kControlIdAboutDocs = 202;
static const int kControlIdAboutDonate = 203;
static const int kLogMenuCopyMessage = 401;
static const int kLogMenuCopyFullLog = 402;
static const int kLogMenuCopyAllLogs = 403;
static const UINT kLogAppendMessage = WM_APP + 20;
static const UINT_PTR kLogsRefreshTimerId = 1;
static const UINT_PTR kAutoUpdateTimerId = 2;
static const UINT_PTR kStartupSyncTimerId = 3;
static const UINT_PTR kProcessMonitorTimerId = 4;
static const UINT kProcessMonitorIntervalMs = 500;
static const UINT kStartupSyncDelayMs = 120;
static const UINT kAutoUpdateIntervalMs = 60 * 1000; // 1 minute
static const int kMaxLogRows = 2000;
static const wchar_t *kCurrentVersion = L"0.9.13.0";
static const wchar_t *kSingleInstanceLockArg =
    L"--request-single-instance-lock";
static const wchar_t *kManageWindowClassName = L"NovadeskManagerWindow";
static const wchar_t *kManageCloseMessageName = L"Novadesk.Manage.RequestClose";
static UINT g_manageCloseMessage = 0;
static std::wstring g_manageMutexName;
static std::wstring g_manageWindowScopeProperty;
static int ShowManageMessageBox(const std::wstring &message,
                                const std::wstring &title, UINT type);
static bool ExecuteNovadeskCommand(const std::wstring &cmd,
                                   const std::wstring &path);
static bool ExecuteNovadeskCommandNoPath(const std::wstring &cmd);
static std::wstring GetAboutLogoPath();
static void ConfigureAutoUpdateTimer(HWND hWnd);
static void RequestAppExit(HWND hWnd);
static bool HasCommandLineFlag(const wchar_t *flag);
static bool RequestExistingManageWindowClose();
static bool RestartManageWindow();
static void ResendFontsToAllControls();
static void ApplyLocalizedText();
static void RecreateManagerTabsForLayoutDirection(int activeTab);
static void PopulateWindowsThemeCombo();
static UINT g_languageChangedMessage = 0;
static UINT g_themeChangedMessage = 0;
static HWND FindManageWindowForCurrentInstallation() {
  struct Search {
    const wchar_t *property;
    HWND window = nullptr;
  } search{g_manageWindowScopeProperty.c_str()};
  EnumWindows(
      [](HWND window, LPARAM value) -> BOOL {
        auto *search = reinterpret_cast<Search *>(value);
        if (GetPropW(window, search->property)) {
          search->window = window;
          return FALSE;
        }
        return TRUE;
      },
      reinterpret_cast<LPARAM>(&search));
  return search.window;
}
static const std::wstring &Tr(const char *key, const wchar_t *fallback) {
  return Localization::TextOr(key, fallback);
}

static void InitGdiPlus() {
  if (g_gdiplusToken != 0) {
    return;
  }

  Gdiplus::GdiplusStartupInput gdiplusStartupInput;
  Gdiplus::GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);
}

static void ShutdownGdiPlus() {
  if (g_gdiplusToken != 0) {
    Gdiplus::GdiplusShutdown(g_gdiplusToken);
    g_gdiplusToken = 0;
  }
}

static HBITMAP ScaleGdiplusBitmap(Gdiplus::Bitmap &source, int maxWidth,
                                  int maxHeight) {
  if (maxWidth <= 0 || maxHeight <= 0)
    return nullptr;

  const UINT srcW = source.GetWidth();
  const UINT srcH = source.GetHeight();
  if (srcW == 0 || srcH == 0)
    return nullptr;

  const double scaleW =
      static_cast<double>(maxWidth) / static_cast<double>(srcW);
  const double scaleH =
      static_cast<double>(maxHeight) / static_cast<double>(srcH);
  const double scale = (scaleW < scaleH) ? scaleW : scaleH;
  int dstW = static_cast<int>(srcW * scale);
  int dstH = static_cast<int>(srcH * scale);
  if (dstW < 1)
    dstW = 1;
  if (dstH < 1)
    dstH = 1;

  Gdiplus::Bitmap canvas(dstW, dstH, PixelFormat32bppPARGB);
  Gdiplus::Graphics graphics(&canvas);
  graphics.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
  graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
  graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
  graphics.DrawImage(&source, Gdiplus::Rect(0, 0, dstW, dstH));

  HBITMAP outBitmap = nullptr;
  if (canvas.GetHBITMAP(Gdiplus::Color(0, 0, 0, 0), &outBitmap) !=
      Gdiplus::Ok) {
    return nullptr;
  }
  return outBitmap;
}

static HBITMAP LoadScaledBitmapFromPng(const std::wstring &path, int maxWidth,
                                       int maxHeight) {
  if (path.empty())
    return nullptr;
  Gdiplus::Bitmap source(path.c_str());
  if (source.GetLastStatus() != Gdiplus::Ok)
    return nullptr;
  return ScaleGdiplusBitmap(source, maxWidth, maxHeight);
}

static HBITMAP LoadScaledBitmapFromPngResource(int resourceId, int maxWidth,
                                               int maxHeight) {
  HMODULE module = GetModuleHandleW(nullptr);
  HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), L"PNG");
  if (!resource)
    return nullptr;
  const DWORD size = SizeofResource(module, resource);
  HGLOBAL handle = LoadResource(module, resource);
  if (!handle || size == 0)
    return nullptr;
  const void *data = LockResource(handle);
  if (!data)
    return nullptr;

  IStream *stream = SHCreateMemStream(static_cast<const BYTE *>(data), size);
  if (!stream)
    return nullptr;

  Gdiplus::Bitmap source(stream);
  stream->Release();
  if (source.GetLastStatus() != Gdiplus::Ok)
    return nullptr;
  return ScaleGdiplusBitmap(source, maxWidth, maxHeight);
}

enum LogIconIndex : int {
  kLogIconDebug = 0,
  kLogIconInfo = 1,
  kLogIconWarning = 2,
  kLogIconError = 3,
  kLogIconCount = 4,
};

static constexpr int kLogIconSize = 16;

static int LogLevelToIconIndex(const std::wstring &level) {
  std::wstring normalized = level;
  while (!normalized.empty() &&
         (normalized.front() == L'[' || normalized.front() == L' '))
    normalized.erase(normalized.begin());
  while (!normalized.empty() &&
         (normalized.back() == L']' || normalized.back() == L' '))
    normalized.pop_back();
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 ::towupper);

  if (normalized == L"DEBUG" || normalized == L"DBG")
    return kLogIconDebug;
  if (normalized == L"WARN" || normalized == L"WARNING")
    return kLogIconWarning;
  if (normalized == L"ERROR" || normalized == L"ERR" ||
      normalized == L"FATAL" || normalized == L"CRITICAL")
    return kLogIconError;
  // INFO / LOG / unknown → info icon
  return kLogIconInfo;
}

static void DestroyLogsImageList() {
  if (g_logsImageList) {
    ImageList_Destroy(g_logsImageList);
    g_logsImageList = nullptr;
  }
}

static HIMAGELIST EnsureLogsImageList() {
  if (g_logsImageList)
    return g_logsImageList;

  g_logsImageList =
      ImageList_Create(kLogIconSize, kLogIconSize, ILC_COLOR32 | ILC_MASK,
                       kLogIconCount, 1);
  if (!g_logsImageList)
    return nullptr;

  const int resourceIds[kLogIconCount] = {
      IDB_LOG_DEBUG, IDB_LOG_INFO, IDB_LOG_WARNING, IDB_LOG_ERROR};
  for (int i = 0; i < kLogIconCount; ++i) {
    HBITMAP bitmap =
        LoadScaledBitmapFromPngResource(resourceIds[i], kLogIconSize,
                                        kLogIconSize);
    if (!bitmap) {
      DestroyLogsImageList();
      return nullptr;
    }
    if (ImageList_Add(g_logsImageList, bitmap, nullptr) < 0) {
      DeleteObject(bitmap);
      DestroyLogsImageList();
      return nullptr;
    }
    DeleteObject(bitmap);
  }
  return g_logsImageList;
}

static const int kAboutLogoSize = 110;

static std::wstring GetAboutVersionText() {
  return Tr("settingspanel.versionPrefix", L"Version ") +
         std::wstring(kCurrentVersion) + L" " +
         Tr("manage.about.beta", L"(Beta)");
}

static void ApplyAboutLocalizedText() {
  if (g_aboutAppName)
    SetWindowTextW(g_aboutAppName,
                   Tr("settingspanel.appName", L"Novadesk").c_str());
  if (g_aboutVersion)
    SetWindowTextW(g_aboutVersion, GetAboutVersionText().c_str());
  if (g_aboutCopyright)
    SetWindowTextW(g_aboutCopyright,
                   Tr("manage.about.copyright",
                      L"(c) 2026 OfficialNovadesk. All rights reserved.")
                       .c_str());
  if (g_aboutHomeLabel)
    SetWindowTextW(g_aboutHomeLabel,
                   Tr("manage.about.home", L"Home:").c_str());
  if (g_aboutDocsLabel)
    SetWindowTextW(g_aboutDocsLabel,
                   Tr("manage.about.docs", L"Docs:").c_str());
  if (g_aboutSupportTitle)
    SetWindowTextW(g_aboutSupportTitle,
                   Tr("manage.about.supportTitle",
                      L"Support & Contribution")
                       .c_str());
  if (g_aboutSupportText)
    SetWindowTextW(
        g_aboutSupportText,
        Tr("manage.about.supportText",
           L"We appreciate your support. If you find Novadesk helpful,\r\n"
           L"consider contributing to its ongoing development.")
            .c_str());
  if (g_aboutDonateButton)
    SetWindowTextW(g_aboutDonateButton,
                   Tr("manage.about.donate",
                      L"Support Project via Donation")
                       .c_str());
  if (g_aboutSupportGroup)
    SetWindowTextW(g_aboutSupportGroup,
                   Tr("manage.about.supportSection", L"Support Section")
                       .c_str());
  if (g_aboutBottomNote)
    SetWindowTextW(g_aboutBottomNote,
                   Tr("manage.about.bottomNote",
                      L"Development depends on user support.")
                       .c_str());
}

static void UpdateAboutLogo() {
  if (!g_aboutLogo) {
    return;
  }

  if (g_aboutLogoBitmap) {
    DeleteObject(g_aboutLogoBitmap);
    g_aboutLogoBitmap = nullptr;
  }

  g_aboutLogoBitmap =
      LoadScaledBitmapFromPng(GetAboutLogoPath(), kAboutLogoSize, kAboutLogoSize);
  SendMessageW(g_aboutLogo, STM_SETIMAGE, IMAGE_BITMAP,
               reinterpret_cast<LPARAM>(g_aboutLogoBitmap));
}

// BS_GROUPBOX is a sibling, not a container. With WS_CLIPSIBLINGS (forced by
// WindowsTheme), controls framed by the group box are clipped away and appear
// blank unless the group box stays below them in z-order.
static void EnsureAboutSupportZOrder() {
  if (!g_aboutSupportGroup) {
    return;
  }

  const HWND framed[] = {g_aboutSupportTitle, g_aboutSupportText,
                         g_aboutDonateButton};
  for (HWND h : framed) {
    if (!h) {
      continue;
    }
    const LONG_PTR style = GetWindowLongPtrW(h, GWL_STYLE);
    if (style & WS_CLIPSIBLINGS) {
      SetWindowLongPtrW(h, GWL_STYLE, style & ~WS_CLIPSIBLINGS);
    }
  }

  SetWindowPos(g_aboutSupportGroup, HWND_BOTTOM, 0, 0, 0, 0,
               SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

  HWND insertAfter = g_aboutSupportGroup;
  for (HWND h : framed) {
    if (!h) {
      continue;
    }
    SetWindowPos(h, insertAfter, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    insertAfter = h;
  }
}

static void OpenUrl(const wchar_t *url) {
  if (!url || !url[0]) {
    return;
  }
  ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

static std::wstring GetExeDir() {
  return AppPaths::GetExeDir().wstring();
}

static std::wstring JoinPath(const std::wstring &a, const std::wstring &b) {
  std::filesystem::path p(a);
  p /= b;
  return p.wstring();
}

static std::wstring GetAboutLogoPath() {
  return JoinPath(GetExeDir(), L"images\\Novadesk.png");
}

static std::wstring EnsureSingleInstanceArg(const std::wstring &args) {
  if (args.find(kSingleInstanceLockArg) != std::wstring::npos) {
    return args;
  }
  if (args.empty()) {
    return std::wstring(kSingleInstanceLockArg);
  }
  return args + L" " + kSingleInstanceLockArg;
}

static void PromptRestartForHardwareAcceleration() {
  const int kRestartNowButtonId = 1001;
  const TASKDIALOG_BUTTON buttons[] = {
      {kRestartNowButtonId, L"Restart Now"},
      {IDCANCEL, L"Later"},
  };

  TASKDIALOGCONFIG config{};
  config.cbSize = sizeof(config);
  config.hwndParent = g_mainWindow;
  config.dwFlags = TDF_ALLOW_DIALOG_CANCELLATION;
  config.dwCommonButtons = 0;
  config.pszWindowTitle = L"Manage Novadesk";
  config.pszMainInstruction = L"Novadesk needs restart for this change.";
  config.pszContent =
      L"Hardware acceleration changes apply after restarting Novadesk.";
  config.cButtons = ARRAYSIZE(buttons);
  config.pButtons = buttons;
  config.nDefaultButton = kRestartNowButtonId;

  int selectedButton = IDCANCEL;
  HRESULT hr = TaskDialogIndirect(&config, &selectedButton, nullptr, nullptr);
  if (SUCCEEDED(hr)) {
    if (selectedButton == kRestartNowButtonId) {
      RestartManageWindow();
    }
    return;
  }

  const int fallback = ShowManageMessageBox(
      L"Novadesk needs restart for this change.\n\nRestart now?",
      L"Manage Novadesk", MB_YESNO | MB_ICONINFORMATION);
  if (fallback == IDYES) {
    RestartManageWindow();
  }
}

static void LoadWindowIcons(HINSTANCE hInstance) {
  if (!g_windowIconLarge) {
    g_windowIconLarge = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK), IMAGE_ICON,
        GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
  }
  if (!g_windowIconSmall) {
    g_windowIconSmall = static_cast<HICON>(LoadImageW(
        hInstance, MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK), IMAGE_ICON,
        GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
  }
}

static void DestroyOwnedIcon(HICON &icon) {
  if (icon) {
    DestroyIcon(icon);
    icon = nullptr;
  }
}

static void DeleteOwnedFont(HFONT &font) {
  if (font) {
    DeleteObject(font);
    font = nullptr;
  }
}

static void DestroyWindowIcons() {
  DestroyOwnedIcon(g_windowIconLarge);
  DestroyOwnedIcon(g_windowIconSmall);
}

static void DeleteManageFonts() {
  DeleteOwnedFont(g_buttonFont);
  DeleteOwnedFont(g_aboutTitleFont);
  DeleteOwnedFont(g_aboutSectionFont);
  g_aboutBodyFont = nullptr; // alias of g_buttonFont, already deleted above
}

static void CreateManageFonts(UINT dpi = USER_DEFAULT_SCREEN_DPI) {
  // Delete existing fonts so they are recreated at the new DPI.
  g_aboutBodyFont = nullptr; // alias of g_buttonFont — clear before deleting buttonFont
  DeleteOwnedFont(g_buttonFont);
  DeleteOwnedFont(g_aboutTitleFont);
  DeleteOwnedFont(g_aboutSectionFont);

  g_buttonFont = FontUtil::CreateSystemFont(dpi);

  // Bold variant used for the "Manage Settings" title.
  g_aboutSectionFont = FontUtil::CreateSystemFont(dpi, FW_BOLD);

  // Larger bold title for the About app name (~1.6x bigger).
  g_aboutTitleFont = FontUtil::CreateSystemFont(dpi, FW_BOLD, 160);

  // Body font for About section text — same as regular.
  g_aboutBodyFont = g_buttonFont;
}

// Re-send WM_SETFONT to every control after fonts are recreated at a new DPI.
// Controls that have not been initialized yet (tab pages not yet shown) will
// get their font set when they initialize, so only live HWNDs are updated here.
static void ResendFontsToAllControls() {
  auto setFont = [](HWND h, HFONT f) {
    if (h && f) SendMessageW(h, WM_SETFONT, (WPARAM)f, TRUE);
  };
  // Main window controls
  setFont(g_tab,            g_buttonFont);
  setFont(g_btnRefreshList, g_buttonFont);
  setFont(g_btnClearLogs,   g_buttonFont);
  setFont(g_btnLoad,        g_buttonFont);
  setFont(g_btnRefresh,     g_buttonFont);
  setFont(g_btnRefreshAll,  g_buttonFont);
  setFont(g_btnClose,       g_buttonFont);
  // Widgets tab
  setFont(g_list,           g_buttonFont);
  // Logs tab
  setFont(g_logsList,       g_buttonFont);
  // Addons tab
  setFont(g_addonsList,     g_buttonFont);
  // Settings tab
  setFont(g_settingsTitle,             g_aboutSectionFont);
  setFont(g_chkRunOnStartup,           g_buttonFont);
  setFont(g_btnCheckUpdates,           g_buttonFont);
  setFont(g_chkAutoUpdate,             g_buttonFont);
  setFont(g_chkEnableLogging,          g_buttonFont);
  setFont(g_chkEnableDebugging,        g_buttonFont);
  setFont(g_chkSaveLogToFile,          g_buttonFont);
  setFont(g_chkUseHardwareAcceleration,g_buttonFont);
  setFont(g_lblWindowsTheme,           g_buttonFont);
  setFont(g_cboWindowsTheme,           g_buttonFont);
  setFont(g_lblLanguage,               g_buttonFont);
  setFont(g_cboLanguage,               g_buttonFont);
  setFont(g_aboutAppName,      g_aboutTitleFont);
  setFont(g_aboutVersion,      g_aboutBodyFont);
  setFont(g_aboutCopyright,    g_aboutBodyFont);
  setFont(g_aboutHomeLabel,    g_aboutBodyFont);
  setFont(g_aboutHomeLink,     g_aboutBodyFont);
  setFont(g_aboutDocsLabel,    g_aboutBodyFont);
  setFont(g_aboutDocsLink,     g_aboutBodyFont);
  setFont(g_aboutSupportGroup, g_aboutBodyFont);
  setFont(g_aboutSupportTitle, g_aboutSectionFont);
  setFont(g_aboutSupportText,  g_aboutBodyFont);
  setFont(g_aboutDonateButton, g_aboutBodyFont);
  setFont(g_aboutBottomNote,   g_aboutSectionFont);
}

static void LogLine(const std::wstring &line) {
  OutputDebugStringW((line + L"\n").c_str());
}

static std::wstring Win32ErrorToString(DWORD err) {
  wchar_t buf[512] = {};
  FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf,
                 512, nullptr);
  return std::wstring(buf);
}

static std::wstring NormalizePathLower(const std::wstring &p) {
  std::error_code ec;
  std::filesystem::path fp(p);
  fp = std::filesystem::absolute(fp, ec).lexically_normal();
  std::wstring out = fp.wstring();
  std::transform(out.begin(), out.end(), out.begin(), ::towlower);
  return out;
}

static std::wstring BytesToWide(const std::string &bytes) {
  if (bytes.empty())
    return L"";

  auto convert = [&](UINT cp) -> std::wstring {
    int needed = MultiByteToWideChar(
        cp, 0, bytes.c_str(), static_cast<int>(bytes.size()), nullptr, 0);
    if (needed <= 0)
      return L"";
    std::wstring wide(needed, L'\0');
    MultiByteToWideChar(cp, 0, bytes.c_str(), static_cast<int>(bytes.size()),
                        wide.data(), needed);
    return wide;
  };

  std::wstring utf8 = convert(CP_UTF8);
  if (!utf8.empty())
    return utf8;
  return convert(CP_ACP);
}

static std::string WideToUtf8(const std::wstring &wide) {
  if (wide.empty())
    return std::string();
  int needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(),
                                   static_cast<int>(wide.size()), nullptr, 0,
                                   nullptr, nullptr);
  if (needed <= 0)
    return std::string();
  std::string out(needed, '\0');
  WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()),
                      out.data(), needed, nullptr, nullptr);
  return out;
}

static std::wstring Utf8ToWide(const std::string &utf8) {
  if (utf8.empty())
    return std::wstring();
  int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(),
                                   static_cast<int>(utf8.size()), nullptr, 0);
  if (needed <= 0)
    return std::wstring();
  std::wstring out(needed, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()),
                      out.data(), needed);
  return out;
}

static DWORD WINAPI NovadeskLogReaderThreadProc(LPVOID) {
  HANDLE readPipe = g_novadeskLogRead;
  if (!readPipe)
    return 0;

  std::vector<char> buffer(4096);
  DWORD bytesRead = 0;
  while (ReadFile(readPipe, buffer.data(), static_cast<DWORD>(buffer.size()),
                  &bytesRead, nullptr) &&
         bytesRead > 0) {
    std::wstring text = BytesToWide(std::string(buffer.data(), bytesRead));
    if (text.empty())
      continue;

    auto *payload = new std::wstring(std::move(text));
    if (!PostMessageW(g_mainWindow, kLogAppendMessage, 0,
                      reinterpret_cast<LPARAM>(payload))) {
      delete payload;
      break;
    }
  }
  return 0;
}

static void StopNovadeskLogCapture() {
  if (g_novadeskLogRead) {
    CloseHandle(g_novadeskLogRead);
    g_novadeskLogRead = nullptr;
  }
  if (g_novadeskLogWrite) {
    CloseHandle(g_novadeskLogWrite);
    g_novadeskLogWrite = nullptr;
  }
  if (g_novadeskLogThread) {
    WaitForSingleObject(g_novadeskLogThread, 1500);
    CloseHandle(g_novadeskLogThread);
    g_novadeskLogThread = nullptr;
  }
}

static void RequestAppExit(HWND hWnd) {
  g_forceExitOnClose = true;
  PostMessageW(hWnd, WM_CLOSE, 0, 0);
}

static bool IsPortableMode() {
  const std::wstring exeDir = GetExeDir();
  const std::wstring settingsPath =
      JoinPath(exeDir, L"manage_novadesk_settings.json");
  std::error_code ec;
  if (!std::filesystem::exists(settingsPath, ec) || ec) {
    return false;
  }

  std::ifstream in(std::filesystem::path(settingsPath), std::ios::binary);
  if (!in.is_open()) {
    return true;
  }

  nlohmann::json doc;
  try {
    in >> doc;
  } catch (...) {
    return true;
  }

  if (doc.is_object() && doc.contains("isPortable") &&
      doc["isPortable"].is_boolean()) {
    return doc["isPortable"].get<bool>();
  }

  return true;
}

static std::wstring GetWidgetsRoot() {
  if (IsPortableMode()) {
    return JoinPath(GetExeDir(), L"Widgets");
  }
  return NovadeskPaths::GetWidgetsDir().wstring();
}

static std::wstring GetAddonsRoot() {
  if (IsPortableMode()) {
    return JoinPath(GetExeDir(), L"Addons");
  }
  return NovadeskPaths::GetAddonsDir().wstring();
}

static std::wstring GetAddonVersionField(const std::wstring &dllPath,
                                         const wchar_t *fieldName) {
  if (!fieldName || !fieldName[0]) {
    return L"";
  }

  DWORD handle = 0;
  const DWORD infoSize = GetFileVersionInfoSizeW(dllPath.c_str(), &handle);
  if (infoSize == 0) {
    return L"";
  }

  std::vector<BYTE> buffer(infoSize);
  if (!GetFileVersionInfoW(dllPath.c_str(), 0, infoSize, buffer.data())) {
    return L"";
  }

  struct LangCodePage {
    WORD language;
    WORD codePage;
  };

  LangCodePage *translation = nullptr;
  UINT translationLen = 0;
  if (!VerQueryValueW(buffer.data(), L"\\VarFileInfo\\Translation",
                      reinterpret_cast<LPVOID *>(&translation),
                      &translationLen) ||
      !translation || translationLen < sizeof(LangCodePage)) {
    return L"";
  }

  wchar_t queryPath[256] = {};
  swprintf_s(queryPath, L"\\StringFileInfo\\%04x%04x\\%s",
             translation[0].language, translation[0].codePage, fieldName);

  LPWSTR value = nullptr;
  UINT valueLen = 0;
  if (!VerQueryValueW(buffer.data(), queryPath,
                      reinterpret_cast<LPVOID *>(&value), &valueLen) ||
      !value || valueLen == 0) {
    return L"";
  }

  return std::wstring(value);
}

static std::vector<AddonEntry> LoadAddons() {
  std::vector<AddonEntry> addons;
  const std::wstring root = GetAddonsRoot();
  if (root.empty()) {
    return addons;
  }

  std::error_code ec;
  if (!std::filesystem::exists(root, ec) || ec) {
    return addons;
  }

  for (const auto &entry : std::filesystem::directory_iterator(root, ec)) {
    if (ec) {
      break;
    }
    if (!entry.is_regular_file()) {
      continue;
    }

    std::wstring ext = entry.path().extension().wstring();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    if (ext != L".dll") {
      continue;
    }

    const std::wstring dllPath = entry.path().wstring();
    const std::wstring fileStem = entry.path().stem().wstring();
    AddonEntry addon{};
    addon.name = GetAddonVersionField(dllPath, L"FileDescription");
    if (addon.name.empty()) {
      addon.name = fileStem;
    }
    addon.author = GetAddonVersionField(dllPath, L"CompanyName");
    if (addon.author.empty()) {
      addon.author = L"-";
    }
    addon.version = GetAddonVersionField(dllPath, L"FileVersion");
    if (addon.version.empty()) {
      addon.version = L"-";
    }
    addon.copyright = GetAddonVersionField(dllPath, L"LegalCopyright");
    if (addon.copyright.empty()) {
      addon.copyright = L"-";
    }

    addons.push_back(std::move(addon));
  }

  std::sort(addons.begin(), addons.end(),
            [](const AddonEntry &a, const AddonEntry &b) {
              return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0;
            });
  return addons;
}

static void RefreshAddonsView() {
  if (!g_addonsList) {
    return;
  }

  ListView_DeleteAllItems(g_addonsList);
  const auto addons = LoadAddons();
  for (int i = 0; i < static_cast<int>(addons.size()); ++i) {
    const auto &addon = addons[static_cast<size_t>(i)];
    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = i;
    item.pszText = const_cast<wchar_t *>(addon.name.c_str());
    const int row = ListView_InsertItem(g_addonsList, &item);
    ListView_SetItemText(g_addonsList, row, 1,
                         const_cast<wchar_t *>(addon.author.c_str()));
    ListView_SetItemText(g_addonsList, row, 2,
                         const_cast<wchar_t *>(addon.version.c_str()));
    ListView_SetItemText(g_addonsList, row, 3,
                         const_cast<wchar_t *>(addon.copyright.c_str()));
  }
}

static std::wstring GetNovadeskExePath() {
  return JoinPath(GetExeDir(), L"Novadesk.exe");
}

static std::wstring GetManageExePath() {
  return AppPaths::GetExePath().wstring();
}

static std::wstring GetManageSettingsPath() {
  const std::wstring exePath =
      JoinPath(GetExeDir(), L"manage_novadesk_settings.json");
  std::error_code ec;
  if (std::filesystem::exists(std::filesystem::path(exePath), ec) && !ec) {
    return exePath;
  }

  const std::wstring appDataDir = NovadeskPaths::GetAppDataDir().wstring();
  if (!appDataDir.empty()) {
    std::filesystem::create_directories(std::filesystem::path(appDataDir), ec);
    if (!ec) {
      return JoinPath(appDataDir, L"manage_novadesk_settings.json");
    }
  }

  // Fallback: keep using EXE directory path if AppData path is unavailable.
  return exePath;
}

static std::wstring GetNovadeskAppDataDir() {
  return NovadeskPaths::GetAppDataDir().wstring();
}

static std::wstring GetNovadeskSettingsPath() {
  const std::wstring exePath = JoinPath(GetExeDir(), L"settings.json");
  std::error_code ec;
  if (std::filesystem::exists(std::filesystem::path(exePath), ec) && !ec) {
    return exePath;
  }

  const std::wstring appDataDir = GetNovadeskAppDataDir();
  if (!appDataDir.empty()) {
    const std::wstring appDataPath = JoinPath(appDataDir, L"settings.json");
    if (std::filesystem::exists(std::filesystem::path(appDataPath), ec) &&
        !ec) {
      return appDataPath;
    }
    return appDataPath;
  }

  return exePath;
}

static bool LoadNovadeskSettings(nlohmann::json &doc, std::wstring &pathUsed) {
  pathUsed = GetNovadeskSettingsPath();
  std::ifstream in(std::filesystem::path(pathUsed), std::ios::binary);
  if (!in.is_open()) {
    doc = nlohmann::json::object();
    return false;
  }

  try {
    in >> doc;
    if (!doc.is_object()) {
      doc = nlohmann::json::object();
    }
    return true;
  } catch (...) {
    doc = nlohmann::json::object();
    return false;
  }
}

static bool SaveNovadeskSettings(const nlohmann::json &doc,
                                 const std::wstring &path) {
  std::error_code ec;
  std::filesystem::create_directories(std::filesystem::path(path).parent_path(),
                                      ec);
  std::ofstream out(std::filesystem::path(path),
                    std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    return false;
  }
  out << doc.dump(4);
  return true;
}

static bool GetNovadeskSettingBool(const std::string &key, bool defaultValue) {
  nlohmann::json doc;
  std::wstring path;
  LoadNovadeskSettings(doc, path);
  if (doc.contains(key) && doc[key].is_boolean()) {
    return doc[key].get<bool>();
  }
  return defaultValue;
}

static bool SetNovadeskSettingBool(const std::string &key, bool value) {
  nlohmann::json doc;
  std::wstring path;
  LoadNovadeskSettings(doc, path);
  doc[key] = value;
  return SaveNovadeskSettings(doc, path);
}

static std::string GetNovadeskSettingString(const std::string &key,
                                            const std::string &def) {
  nlohmann::json doc;
  std::wstring path;
  LoadNovadeskSettings(doc, path);
  if (doc.contains(key) && doc[key].is_string()) {
    return doc[key].get<std::string>();
  }
  return def;
}

static bool SetNovadeskSettingString(const std::string &key,
                                     const std::string &value) {
  nlohmann::json doc;
  std::wstring path;
  LoadNovadeskSettings(doc, path);
  doc[key] = value;
  return SaveNovadeskSettings(doc, path);
}

static bool IsRunOnStartupEnabled() {
  HKEY hKey = nullptr;
  if (RegOpenKeyExW(HKEY_CURRENT_USER,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                    KEY_READ, &hKey) != ERROR_SUCCESS) {
    return false;
  }

  wchar_t value[2048] = {};
  DWORD valueSize = sizeof(value);
  DWORD type = 0;
  const LONG rc = RegQueryValueExW(hKey, L"Novadesk", nullptr, &type,
                                   reinterpret_cast<LPBYTE>(value), &valueSize);
  RegCloseKey(hKey);

  if (rc != ERROR_SUCCESS || type != REG_SZ) {
    return false;
  }

  const std::wstring expectedExe = GetManageExePath();
  std::wstring configured = value;
  configured.erase(std::remove(configured.begin(), configured.end(), L'"'),
                   configured.end());
  return _wcsicmp(configured.c_str(), expectedExe.c_str()) == 0;
}

static bool SetRunOnStartupEnabled(bool enabled) {
  HKEY hKey = nullptr;
  if (RegCreateKeyExW(HKEY_CURRENT_USER,
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0,
                      nullptr, 0, KEY_SET_VALUE, nullptr, &hKey,
                      nullptr) != ERROR_SUCCESS) {
    return false;
  }

  bool ok = true;
  if (enabled) {
    const std::wstring value = L"\"" + GetManageExePath() + L"\"";
    const LONG rc = RegSetValueExW(
        hKey, L"Novadesk", 0, REG_SZ,
        reinterpret_cast<const BYTE *>(value.c_str()),
        static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t)));
    ok = (rc == ERROR_SUCCESS);
  } else {
    const LONG rc = RegDeleteValueW(hKey, L"Novadesk");
    ok = (rc == ERROR_SUCCESS || rc == ERROR_FILE_NOT_FOUND);
  }

  RegCloseKey(hKey);
  return ok;
}

static std::wstring HttpGetText(const std::wstring &url) {
  HINTERNET hInternet = InternetOpenW(
      L"ManageNovadesk/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (!hInternet) {
    return L"";
  }

  HINTERNET hUrl =
      InternetOpenUrlW(hInternet, url.c_str(), nullptr, 0,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                           INTERNET_FLAG_SECURE,
                       0);
  if (!hUrl) {
    InternetCloseHandle(hInternet);
    return L"";
  }

  std::string bytes;
  char buffer[4096];
  DWORD read = 0;
  while (InternetReadFile(hUrl, buffer, static_cast<DWORD>(sizeof(buffer)),
                          &read) &&
         read > 0) {
    bytes.append(buffer, buffer + read);
  }

  InternetCloseHandle(hUrl);
  InternetCloseHandle(hInternet);
  return BytesToWide(bytes);
}

static std::vector<int> ParseVersion(const std::wstring &value) {
  std::wstring s = value;
  if (!s.empty() && (s[0] == L'v' || s[0] == L'V')) {
    s.erase(s.begin());
  }
  for (wchar_t &ch : s) {
    if (!(ch >= L'0' && ch <= L'9') && ch != L'.') {
      ch = L'.';
    }
  }

  std::vector<int> parts;
  std::wstringstream ss(s);
  std::wstring token;
  while (std::getline(ss, token, L'.')) {
    if (token.empty())
      continue;
    try {
      parts.push_back(std::stoi(token));
    } catch (...) {
      parts.push_back(0);
    }
  }
  return parts;
}

static bool IsVersionNewer(const std::wstring &candidate,
                           const std::wstring &current) {
  std::vector<int> a = ParseVersion(candidate);
  std::vector<int> b = ParseVersion(current);
  const size_t n = (a.size() > b.size()) ? a.size() : b.size();
  a.resize(n, 0);
  b.resize(n, 0);
  for (size_t i = 0; i < n; ++i) {
    if (a[i] > b[i])
      return true;
    if (a[i] < b[i])
      return false;
  }
  return false;
}

static bool GetLatestGithubRelease(std::wstring &tag, std::wstring &assetUrl,
                                   std::wstring &assetName) {
  tag.clear();
  assetUrl.clear();
  assetName.clear();

  const std::wstring jsonText =
      HttpGetText(L"https://api.github.com/repos/Official-Novadesk/novadesk/"
                  L"releases/latest");
  if (jsonText.empty()) {
    return false;
  }
  try {
    nlohmann::json doc = nlohmann::json::parse(WideToUtf8(jsonText));
    if (doc.contains("tag_name") && doc["tag_name"].is_string()) {
      tag = Utf8ToWide(doc["tag_name"].get<std::string>());
    }
    if (doc.contains("assets") && doc["assets"].is_array()) {
      for (const auto &asset : doc["assets"]) {
        if (!asset.is_object())
          continue;
        if (!asset.contains("name") || !asset.contains("browser_download_url"))
          continue;
        if (!asset["name"].is_string() ||
            !asset["browser_download_url"].is_string())
          continue;

        const std::wstring name = Utf8ToWide(asset["name"].get<std::string>());
        const std::wstring url =
            Utf8ToWide(asset["browser_download_url"].get<std::string>());
        std::wstring lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                       ::towlower);
        if (lowerName.find(L".exe") != std::wstring::npos ||
            lowerName.find(L".msi") != std::wstring::npos) {
          assetName = name;
          assetUrl = url;
          break;
        }
      }
      if (assetUrl.empty()) {
        for (const auto &asset : doc["assets"]) {
          if (!asset.is_object())
            continue;
          if (!asset.contains("name") ||
              !asset.contains("browser_download_url"))
            continue;
          if (!asset["name"].is_string() ||
              !asset["browser_download_url"].is_string())
            continue;
          assetName = Utf8ToWide(asset["name"].get<std::string>());
          assetUrl =
              Utf8ToWide(asset["browser_download_url"].get<std::string>());
          break;
        }
      }
    }
  } catch (...) {
    return false;
  }
  return !tag.empty();
}

static bool DownloadFileToPath(const std::wstring &url,
                               const std::wstring &destinationPath) {
  HINTERNET hInternet = InternetOpenW(
      L"ManageNovadesk/1.0", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
  if (!hInternet)
    return false;

  HINTERNET hUrl =
      InternetOpenUrlW(hInternet, url.c_str(), nullptr, 0,
                       INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE |
                           INTERNET_FLAG_SECURE,
                       0);
  if (!hUrl) {
    InternetCloseHandle(hInternet);
    return false;
  }

  std::ofstream out(std::filesystem::path(destinationPath),
                    std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    InternetCloseHandle(hUrl);
    InternetCloseHandle(hInternet);
    return false;
  }

  char buffer[8192];
  DWORD read = 0;
  while (InternetReadFile(hUrl, buffer, static_cast<DWORD>(sizeof(buffer)),
                          &read) &&
         read > 0) {
    out.write(buffer, read);
  }
  out.close();
  InternetCloseHandle(hUrl);
  InternetCloseHandle(hInternet);
  return std::filesystem::exists(std::filesystem::path(destinationPath));
}

static void SetSettingsStatusText(const std::wstring &text) {
  UNREFERENCED_PARAMETER(text);
}

static int ShowManageMessageBox(const std::wstring &message,
                                const std::wstring &title, UINT type) {
  HWND owner = nullptr;
  if (g_mainWindow && IsWindow(g_mainWindow) && IsWindowVisible(g_mainWindow)) {
    owner = g_mainWindow;
  }

  const UINT flags =
      type | MB_SETFOREGROUND | MB_TOPMOST | (owner ? 0 : MB_TASKMODAL);
  return MessageBoxW(owner, message.c_str(), title.c_str(), flags);
}

static void SaveManageSettings() {
  const std::wstring path = GetManageSettingsPath();
  std::vector<std::wstring> paths;
  paths.reserve(g_savedLoadedScripts.size());
  for (const auto &kv : g_savedLoadedScripts) {
    paths.push_back(kv.second);
  }
  std::sort(paths.begin(), paths.end());

  nlohmann::json doc = nlohmann::json::object();
  doc["autoCheckForUpdates"] = g_manageAutoCheckForUpdates;
  doc["loadedScripts"] = nlohmann::json::array();
  for (const auto &p : paths) {
    doc["loadedScripts"].push_back(WideToUtf8(p));
  }

  std::ofstream out(std::filesystem::path(path),
                    std::ios::binary | std::ios::trunc);
  if (!out.is_open()) {
    LogLine(L"[Manage] Failed to write settings file: " + path);
    return;
  }
  out << doc.dump(2);
}

static void RememberLoadedScriptState(const std::wstring &scriptPath,
                                      bool loaded) {
  const std::wstring norm = NormalizePathLower(scriptPath);
  if (loaded) {
    g_savedLoadedScripts[norm] = scriptPath;
  } else {
    g_savedLoadedScripts.erase(norm);
  }
  SaveManageSettings();
}

static void LoadManageSettings() {
  g_savedLoadedScripts.clear();
  g_manageAutoCheckForUpdates = false;
  const std::wstring path = GetManageSettingsPath();
  std::ifstream in(std::filesystem::path(path), std::ios::binary);
  if (!in.is_open()) {
    return;
  }

  nlohmann::json doc;
  try {
    in >> doc;
  } catch (...) {
    return;
  }

  if (!doc.is_object()) {
    return;
  }

  if (doc.contains("autoCheckForUpdates") &&
      doc["autoCheckForUpdates"].is_boolean()) {
    g_manageAutoCheckForUpdates = doc["autoCheckForUpdates"].get<bool>();
  }

  if (!doc.contains("loadedScripts") || !doc["loadedScripts"].is_array()) {
    return;
  }

  for (const auto &v : doc["loadedScripts"]) {
    if (!v.is_string())
      continue;
    std::wstring pathValue = Utf8ToWide(v.get<std::string>());
    if (pathValue.empty()) {
      continue;
    }
    g_savedLoadedScripts[NormalizePathLower(pathValue)] = pathValue;
  }
}

static void ApplySavedLoadedScripts() {
  for (const auto &kv : g_savedLoadedScripts) {
    ExecuteNovadeskCommand(L"--load", kv.second);
  }
}

static std::wstring CreateTempListPath() {
  wchar_t tempPath[MAX_PATH + 1] = {};
  if (!GetTempPathW(MAX_PATH, tempPath))
    return L"";
  wchar_t filePath[MAX_PATH + 1] = {};
  if (!GetTempFileNameW(tempPath, L"nds", 0, filePath))
    return L"";
  return std::wstring(filePath);
}

static void StartNovadesk() {
  if (g_novadeskRunning)
    return;

  const std::wstring exe = GetNovadeskExePath();
  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;
  if (!CreatePipe(&g_novadeskLogRead, &g_novadeskLogWrite, &sa, 0)) {
    g_novadeskLogRead = nullptr;
    g_novadeskLogWrite = nullptr;
    LogLine(L"[Manage] Failed to create output pipe for Novadesk.");
  } else {
    SetHandleInformation(g_novadeskLogRead, HANDLE_FLAG_INHERIT, 0);
  }

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  if (g_novadeskLogWrite) {
    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = g_novadeskLogWrite;
    si.hStdError = g_novadeskLogWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
  }
  PROCESS_INFORMATION pi{};
  std::wstring cmdLine = L"\"" + exe + L"\" " + EnsureSingleInstanceArg(L"");

  if (CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
                     CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
    g_novadeskProcess = pi;
    g_novadeskRunning = true;
    if (g_novadeskLogWrite) {
      CloseHandle(g_novadeskLogWrite);
      g_novadeskLogWrite = nullptr;
      g_novadeskLogThread = CreateThread(
          nullptr, 0, NovadeskLogReaderThreadProc, nullptr, 0, nullptr);
    }
    LogLine(L"[Manage] Started Novadesk: " + exe);
  } else {
    DWORD err = GetLastError();
    LogLine(L"[Manage] Failed to start Novadesk (" + std::to_wstring(err) +
            L"): " + Win32ErrorToString(err));
    StopNovadeskLogCapture();
  }
}

static void StopNovadesk() {
  if (!g_novadeskRunning) {
    StopNovadeskLogCapture();
    return;
  }

  TerminateProcess(g_novadeskProcess.hProcess, 0);
  CloseHandle(g_novadeskProcess.hProcess);
  CloseHandle(g_novadeskProcess.hThread);
  g_novadeskProcess = {};
  g_novadeskRunning = false;
  StopNovadeskLogCapture();
  LogLine(L"[Manage] Stopped Novadesk process.");
}

static void AddTrayIcon(HWND hWnd) {
  g_tray = {};
  g_tray.cbSize = sizeof(g_tray);
  g_tray.hWnd = hWnd;
  g_tray.uID = 1;
  g_tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  g_tray.uCallbackMessage = kTrayMessage;
  g_tray.hIcon = static_cast<HICON>(LoadImageW(
      GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK),
      IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
      0));
  wcscpy_s(g_tray.szTip, L"Manage Novadesk");

  Shell_NotifyIconW(NIM_ADD, &g_tray);
}

static void RemoveTrayIcon() {
  if (g_tray.hWnd)
    Shell_NotifyIconW(NIM_DELETE, &g_tray);
  if (g_tray.hIcon)
    DestroyIcon(g_tray.hIcon);
  g_tray = {};
}

static void EnsureTrayMenu() {
  if (g_trayMenu)
    return;
  g_trayMenu = CreatePopupMenu();
  HMENU homeMenu = CreatePopupMenu();
  AppendMenuW(homeMenu, MF_STRING, kTrayMenuHomeWebsiteId, Tr("manage.menu.website", L"Official Website").c_str());
  AppendMenuW(homeMenu, MF_STRING, kTrayMenuHomeDonateId, Tr("manage.menu.donate", L"Donate").c_str());

  HMENU docsMenu = CreatePopupMenu();
  AppendMenuW(docsMenu, MF_STRING, kTrayMenuDocsMainId, Tr("manage.menu.documentation", L"Documentation").c_str());
  AppendMenuW(docsMenu, MF_STRING, kTrayMenuDocsApiId, Tr("manage.menu.apiReference", L"API Reference").c_str());

  HMENU settingsMenu = CreatePopupMenu();
  AppendMenuW(settingsMenu, MF_STRING, kTrayMenuSettingsOpenId,
              Tr("manage.menu.openSettings", L"Open Settings").c_str());
  AppendMenuW(settingsMenu, MF_STRING, kTrayMenuSettingsCheckUpdatesId,
              Tr("manage.menu.checkUpdates", L"Check for Updates").c_str());

  AppendMenuW(g_trayMenu, MF_STRING, kTrayMenuShowHideId, Tr("manage.button.show", L"Show").c_str());
  AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_trayMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(homeMenu),
              Tr("manage.menu.home", L"Home").c_str());
  AppendMenuW(g_trayMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(docsMenu),
              Tr("manage.menu.docs", L"Docs").c_str());
  AppendMenuW(g_trayMenu, MF_POPUP, reinterpret_cast<UINT_PTR>(settingsMenu),
              Tr("tab.settings", L"Settings").c_str());
  AppendMenuW(g_trayMenu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(g_trayMenu, MF_STRING, kTrayMenuExitId, Tr("manage.button.exit", L"Exit").c_str());
}

#ifndef TPM_LAYOUTRTL
#define TPM_LAYOUTRTL 0x00008000L
#endif

// Match Novadesk MenuUtils::TrackPopup / Rainmeter DisplayMenu.
// Manage dialogs use WS_EX_LAYOUTRTL; that conflicts with TPM_LAYOUTRTL and
// leaves popup item text LTR. Clear the owner style for the menu loop only.
static int TrackRtlAwarePopup(HMENU menu, UINT flags, int x, int y,
                              HWND owner) {
  if (!menu)
    return 0;

  DWORD previousLayout = 0;
  LONG_PTR previousExStyle = 0;
  bool clearedOwnerRtl = false;
  const bool rtl = Localization::IsRtl();

  if (rtl) {
    GetProcessDefaultLayout(&previousLayout);
    SetProcessDefaultLayout(LAYOUT_RTL);
    flags = (flags & ~TPM_RIGHTALIGN) | TPM_LEFTALIGN | TPM_LAYOUTRTL;
  }

  if (owner && IsWindow(owner)) {
    if (rtl) {
      previousExStyle = GetWindowLongPtrW(owner, GWL_EXSTYLE);
      if (previousExStyle & WS_EX_LAYOUTRTL) {
        SetWindowLongPtrW(owner, GWL_EXSTYLE,
                          previousExStyle & ~static_cast<LONG_PTR>(WS_EX_LAYOUTRTL));
        clearedOwnerRtl = true;
      }
    }

    HWND foregroundWindow = GetForegroundWindow();
    if (foregroundWindow && foregroundWindow != owner) {
      const DWORD foregroundThreadID =
          GetWindowThreadProcessId(foregroundWindow, nullptr);
      const DWORD currentThreadID = GetCurrentThreadId();
      AttachThreadInput(currentThreadID, foregroundThreadID, TRUE);
      SetForegroundWindow(owner);
      AttachThreadInput(currentThreadID, foregroundThreadID, FALSE);
    } else {
      SetForegroundWindow(owner);
    }

    WindowsTheme::PrepareMenu(owner);
  }

  const int result = static_cast<int>(
      TrackPopupMenu(menu, flags, x, y, 0, owner, nullptr));

  if (clearedOwnerRtl && owner && IsWindow(owner))
    SetWindowLongPtrW(owner, GWL_EXSTYLE, previousExStyle);

  if (rtl)
    SetProcessDefaultLayout(previousLayout);

  if (owner && IsWindow(owner))
    PostMessageW(owner, WM_NULL, 0, 0);

  return result;
}

static void ShowTrayMenu(HWND hWnd) {
  EnsureTrayMenu();
  ModifyMenuW(g_trayMenu, kTrayMenuShowHideId, MF_BYCOMMAND | MF_STRING,
              kTrayMenuShowHideId, IsWindowVisible(hWnd) ? Tr("manage.button.hide", L"Hide").c_str() : Tr("manage.button.show", L"Show").c_str());
  POINT pt{};
  GetCursorPos(&pt);
  TrackRtlAwarePopup(g_trayMenu,
                     TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x,
                     pt.y, hWnd);
  PostMessageW(hWnd, kTrayMenuCommand, 0, 0);
}

static std::wstring RunProcessCaptureStdout(const std::wstring &exePath,
                                            const std::wstring &args) {
  std::wstring cmd = L"\"" + exePath + L"\" " + EnsureSingleInstanceArg(args);
  LogLine(L"[Manage] Run capture: " + cmd);

  SECURITY_ATTRIBUTES sa{};
  sa.nLength = sizeof(sa);
  sa.bInheritHandle = TRUE;
  sa.lpSecurityDescriptor = nullptr;

  HANDLE hRead = nullptr;
  HANDLE hWrite = nullptr;
  if (!CreatePipe(&hRead, &hWrite, &sa, 0))
    return L"";
  SetHandleInformation(hRead, HANDLE_FLAG_INHERIT, 0);

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdOutput = hWrite;
  si.hStdError = hWrite;

  PROCESS_INFORMATION pi{};
  std::wstring cmdLine = cmd;
  if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE,
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
    CloseHandle(hRead);
    CloseHandle(hWrite);
    DWORD err = GetLastError();
    LogLine(L"[Manage] CreateProcess failed (" + std::to_wstring(err) + L"): " +
            Win32ErrorToString(err));
    return L"";
  }

  CloseHandle(hWrite);

  std::wstring output;
  std::vector<char> buffer(4096);
  DWORD bytesRead = 0;
  while (ReadFile(hRead, buffer.data(), static_cast<DWORD>(buffer.size()),
                  &bytesRead, nullptr) &&
         bytesRead > 0) {
    // Assume UTF-16LE from wcout
    if (bytesRead % 2 != 0)
      bytesRead -= 1;
    output.append(reinterpret_cast<wchar_t *>(buffer.data()), bytesRead / 2);
  }

  CloseHandle(hRead);
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);

  LogLine(L"[Manage] Capture output length: " + std::to_wstring(output.size()));
  return output;
}

static bool RunProcessWait(const std::wstring &exePath,
                           const std::wstring &args) {
  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  std::wstring cmdLine =
      L"\"" + exePath + L"\" " + EnsureSingleInstanceArg(args);
  LogLine(L"[Manage] Run wait: " + cmdLine);
  if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE,
                      CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
    DWORD err = GetLastError();
    LogLine(L"[Manage] Run wait failed (" + std::to_wstring(err) + L"): " +
            Win32ErrorToString(err));
    return false;
  }
  WaitForSingleObject(pi.hProcess, INFINITE);
  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}

static void UpdateButtonState();
static void RefreshLogsView();
static void ApplyTabState();
static void OnToggleLoadSelected();
static void OnClearLogs();
static std::wstring GetListViewText(HWND list, int row, int subItem);
static bool CopyTextToClipboard(HWND owner, const std::wstring &text);
static void ShowLogsContextMenu(HWND hWnd, int x, int y);

static void AutoExpandLogsMessageColumn(const std::wstring &message);

static std::wstring GetListViewText(HWND list, int row, int subItem) {
  if (!list || row < 0 || subItem < 0)
    return L"";

  int capacity = 256;
  for (int attempt = 0; attempt < 6; ++attempt) {
    std::vector<wchar_t> buf(static_cast<size_t>(capacity), L'\0');
    LVITEMW item{};
    item.iSubItem = subItem;
    item.pszText = buf.data();
    item.cchTextMax = capacity;
    const int copied = static_cast<int>(
        SendMessageW(list, LVM_GETITEMTEXTW, static_cast<WPARAM>(row),
                     reinterpret_cast<LPARAM>(&item)));
    if (copied < capacity - 1) {
      return std::wstring(buf.data(), static_cast<size_t>(copied));
    }
    capacity *= 2;
  }
  return L"";
}

static bool CopyTextToClipboard(HWND owner, const std::wstring &text) {
  if (text.empty())
    return false;

  if (!OpenClipboard(owner))
    return false;

  EmptyClipboard();
  const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
  HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, bytes);
  if (!mem) {
    CloseClipboard();
    return false;
  }

  void *dst = GlobalLock(mem);
  if (!dst) {
    GlobalFree(mem);
    CloseClipboard();
    return false;
  }

  memcpy(dst, text.c_str(), bytes);
  GlobalUnlock(mem);

  if (!SetClipboardData(CF_UNICODETEXT, mem)) {
    GlobalFree(mem);
    CloseClipboard();
    return false;
  }

  CloseClipboard();
  return true;
}

static void AutoExpandLogsMessageColumn(const std::wstring &message) {
  if (!g_logsList || message.empty())
    return;

  const int textWidth = ListView_GetStringWidth(g_logsList, message.c_str());
  if (textWidth <= 0)
    return;

  const int desiredWidth = textWidth + 24;
  if (desiredWidth > g_logsMessageColumnWidth) {
    g_logsMessageColumnWidth = desiredWidth;
    // Columns: Level, Time, Source, Message
    ListView_SetColumnWidth(g_logsList, 3, g_logsMessageColumnWidth);
  }
}

static std::wstring FormatLogRowLine(const ManagedLogRow &row) {
  return L"[" + row.time + L"] [" + row.source + L"] [" + row.level + L"] " +
         row.message;
}

static std::wstring BuildAllLogsClipboardText() {
  // g_logRows is newest-first; emit oldest-first for readable paste order.
  std::wstring out;
  for (auto it = g_logRows.rbegin(); it != g_logRows.rend(); ++it) {
    if (!out.empty())
      out += L"\r\n";
    out += FormatLogRowLine(*it);
  }
  return out;
}

static void ShowLogsContextMenu(HWND hWnd, int x, int y) {
  if (!g_logsList)
    return;

  POINT pt{x, y};
  if (pt.x == -1 && pt.y == -1) {
    const int selected = ListView_GetNextItem(g_logsList, -1, LVNI_SELECTED);
    if (selected >= 0) {
      RECT rc{};
      if (ListView_GetItemRect(g_logsList, selected, &rc, LVIR_BOUNDS)) {
        POINT localPt{rc.left + 6, rc.top + 6};
        ClientToScreen(g_logsList, &localPt);
        pt = localPt;
      }
    } else {
      POINT localPt{8, 8};
      ClientToScreen(g_logsList, &localPt);
      pt = localPt;
    }
  } else {
    POINT clientPt = pt;
    ScreenToClient(g_logsList, &clientPt);
    LVHITTESTINFO hit{};
    hit.pt = clientPt;
    const int hitIndex = ListView_SubItemHitTest(g_logsList, &hit);
    if (hitIndex >= 0) {
      ListView_SetItemState(g_logsList, -1, 0, LVIS_SELECTED);
      ListView_SetItemState(g_logsList, hitIndex, LVIS_SELECTED | LVIS_FOCUSED,
                            LVIS_SELECTED | LVIS_FOCUSED);
    }
  }

  const int row = ListView_GetNextItem(g_logsList, -1, LVNI_SELECTED);
  const bool hasSelection = row >= 0;
  const bool hasAnyLogs = !g_logRows.empty();
  if (!hasSelection && !hasAnyLogs)
    return;

  std::wstring message;
  std::wstring full;
  if (hasSelection) {
    const std::wstring level = GetListViewText(g_logsList, row, 0);
    const std::wstring time = GetListViewText(g_logsList, row, 1);
    const std::wstring source = GetListViewText(g_logsList, row, 2);
    message = GetListViewText(g_logsList, row, 3);
    full = L"[" + time + L"] [" + source + L"] [" + level + L"] " + message;
  }

  HMENU menu = CreatePopupMenu();
  if (!menu)
    return;

  const UINT rowFlags = MF_STRING | (hasSelection ? 0 : MF_GRAYED);
  const UINT allFlags = MF_STRING | (hasAnyLogs ? 0 : MF_GRAYED);
  AppendMenuW(menu, rowFlags, kLogMenuCopyMessage,
              Tr("manage.context.copyMessage", L"Copy Message").c_str());
  AppendMenuW(menu, rowFlags, kLogMenuCopyFullLog,
              Tr("manage.context.copyFullLog", L"Copy Full Log").c_str());
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(menu, allFlags, kLogMenuCopyAllLogs,
              Tr("manage.context.copyAllLogs", L"Copy All Logs").c_str());
  // Same flags as Novadesk widget menus / Rainmeter DisplayMenu.
  const UINT cmd = static_cast<UINT>(TrackRtlAwarePopup(
      menu,
      TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON | TPM_LEFTALIGN, pt.x,
      pt.y, hWnd));
  DestroyMenu(menu);

  if (cmd == kLogMenuCopyMessage) {
    if (hasSelection)
      CopyTextToClipboard(hWnd, message);
  } else if (cmd == kLogMenuCopyFullLog) {
    if (hasSelection)
      CopyTextToClipboard(hWnd, full);
  } else if (cmd == kLogMenuCopyAllLogs) {
    CopyTextToClipboard(hWnd, BuildAllLogsClipboardText());
  }
}

static void InsertLogRowIntoList(const ManagedLogRow &row) {
  if (!g_logsList)
    return;

  LVITEMW item{};
  item.mask = LVIF_TEXT | LVIF_IMAGE;
  item.iItem = 0;
  item.iImage = LogLevelToIconIndex(row.level);
  item.pszText = const_cast<wchar_t *>(row.level.c_str());
  const int index = ListView_InsertItem(g_logsList, &item);
  ListView_SetItemText(g_logsList, index, 1,
                       const_cast<wchar_t *>(row.time.c_str()));
  ListView_SetItemText(g_logsList, index, 2,
                       const_cast<wchar_t *>(row.source.c_str()));
  ListView_SetItemText(g_logsList, index, 3,
                       const_cast<wchar_t *>(row.message.c_str()));
  AutoExpandLogsMessageColumn(row.message);

  while (ListView_GetItemCount(g_logsList) > kMaxLogRows) {
    const int last = ListView_GetItemCount(g_logsList) - 1;
    if (last >= 0)
      ListView_DeleteItem(g_logsList, last);
    else
      break;
  }

  if (ListView_GetItemCount(g_logsList) > 0)
    ListView_EnsureVisible(g_logsList, 0, FALSE);
}

static void AppendLogRow(const std::wstring &time, const std::wstring &source,
                         const std::wstring &level,
                         const std::wstring &message) {
  if (!g_logRows.empty()) {
    const ManagedLogRow &first = g_logRows.front();
    if (time == first.time && source == first.source && level == first.level &&
        message == first.message) {
      return;
    }
  }

  g_logRows.push_front(ManagedLogRow{time, source, level, message});
  while (static_cast<int>(g_logRows.size()) > kMaxLogRows)
    g_logRows.pop_back();

  // Logs list is created lazily with the Logs tab. Buffer until then so
  // Novadesk stdout captured at startup is not dropped.
  if (g_logsList)
    InsertLogRowIntoList(g_logRows.front());
}

static void ParseAndAppendLogLine(const std::wstring &line) {
  if (line.empty())
    return;

  std::wstring trimmed = line;
  while (!trimmed.empty() &&
         (trimmed.back() == L'\r' || trimmed.back() == L'\n')) {
    trimmed.pop_back();
  }
  if (trimmed.empty())
    return;

  std::wstring time;
  std::wstring source;
  std::wstring level;
  std::wstring message = trimmed;

  size_t p0s = trimmed.find(L'[');
  size_t p0e = (p0s == std::wstring::npos) ? std::wstring::npos
                                           : trimmed.find(L']', p0s + 1);
  size_t p1s = (p0e == std::wstring::npos) ? std::wstring::npos
                                           : trimmed.find(L'[', p0e + 1);
  size_t p1e = (p1s == std::wstring::npos) ? std::wstring::npos
                                           : trimmed.find(L']', p1s + 1);
  size_t p2s = (p1e == std::wstring::npos) ? std::wstring::npos
                                           : trimmed.find(L'[', p1e + 1);
  size_t p2e = (p2s == std::wstring::npos) ? std::wstring::npos
                                           : trimmed.find(L']', p2s + 1);

  if (p0s != std::wstring::npos && p0e != std::wstring::npos &&
      p1s != std::wstring::npos && p1e != std::wstring::npos &&
      p2s != std::wstring::npos && p2e != std::wstring::npos) {
    time = trimmed.substr(p0s + 1, p0e - p0s - 1);
    source = trimmed.substr(p1s + 1, p1e - p1s - 1);
    level = trimmed.substr(p2s + 1, p2e - p2s - 1);
    size_t msgStart = p2e + 1;
    while (msgStart < trimmed.size() && trimmed[msgStart] == L' ') {
      ++msgStart;
    }
    message = trimmed.substr(msgStart);
  }

  AppendLogRow(time, source, level, message);
}

static void ProcessLogChunk(const std::wstring &chunk) {
  if (chunk.empty())
    return;

  g_pendingLogLine += chunk;
  size_t start = 0;
  for (;;) {
    size_t nl = g_pendingLogLine.find(L'\n', start);
    if (nl == std::wstring::npos)
      break;
    std::wstring line = g_pendingLogLine.substr(start, nl - start);
    ParseAndAppendLogLine(line);
    start = nl + 1;
  }
  if (start > 0) {
    g_pendingLogLine.erase(0, start);
  }
}

static std::unordered_set<std::wstring> GetRunningScripts() {
  std::unordered_set<std::wstring> out;
  std::wstring exe = GetNovadeskExePath();
  const std::wstring tempList = CreateTempListPath();
  if (tempList.empty()) {
    LogLine(L"[Manage] list-scripts temp file not created.");
    return out;
  }
  const std::wstring args = L"--list-scripts-file \"" + tempList + L"\"";
  RunProcessWait(exe, args);

  std::wifstream in(tempList.c_str());
  if (in.is_open()) {
    std::wstring line;
    while (std::getline(in, line)) {
      if (line.empty())
        continue;
      LogLine(L"[Manage] list-scripts: " + line);
      out.insert(NormalizePathLower(line));
    }
    in.close();
  } else {
    LogLine(L"[Manage] list-scripts failed to open: " + tempList);
  }
  DeleteFileW(tempList.c_str());
  LogLine(L"[Manage] Running scripts count: " + std::to_wstring(out.size()));
  return out;
}

static std::vector<WidgetEntry> LoadWidgets() {
  std::vector<WidgetEntry> list;
  const std::wstring root = GetWidgetsRoot();
  if (root.empty())
    return list;

  std::error_code ec;
  if (!std::filesystem::exists(root, ec))
    return list;

  auto running = GetRunningScripts();

  for (const auto &entry : std::filesystem::directory_iterator(root, ec)) {
    if (ec)
      break;
    if (!entry.is_directory())
      continue;

    std::filesystem::path dir = entry.path();
    std::filesystem::path index = dir / L"index.js";
    if (!std::filesystem::exists(index, ec))
      continue;

    WidgetEntry w{};
    w.name = dir.filename().wstring();
    w.scriptPath = index.wstring();
    w.loaded = running.find(NormalizePathLower(w.scriptPath)) != running.end();
    list.push_back(std::move(w));
  }

  return list;
}

static bool ExecuteNovadeskCommand(const std::wstring &cmd,
                                   const std::wstring &path) {
  const std::wstring exe = GetNovadeskExePath();
  const std::wstring args = cmd + L" \"" + path + L"\"";
  const bool ok = RunProcessWait(exe, args);
  LogLine(ok ? L"[Manage] Exec OK." : L"[Manage] Exec failed.");
  return ok;
}

static int GetSelectedIndex() {
  return ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
}

static void RefreshListView(const std::wstring &preferredSelection = L"") {
  std::wstring selectedScript = preferredSelection;
  if (selectedScript.empty()) {
    const int prevSel = GetSelectedIndex();
    if (prevSel >= 0 && prevSel < static_cast<int>(g_widgets.size())) {
      selectedScript = g_widgets[prevSel].scriptPath;
    }
  }

  g_widgets = LoadWidgets();
  LogLine(L"[Manage] Refresh list. Widgets: " +
          std::to_wstring(g_widgets.size()));

  // Suppress redraws while rebuilding to prevent flicker
  SendMessage(g_list, WM_SETREDRAW, FALSE, 0);

  ListView_DeleteAllItems(g_list);
  int newSelectIdx = -1;
  int idx = 0;
  for (const auto &w : g_widgets) {
    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = idx;
    item.pszText = const_cast<wchar_t *>(w.name.c_str());
    ListView_InsertItem(g_list, &item);

    ListView_SetItemText(g_list, idx, 1,
                         const_cast<wchar_t *>(w.scriptPath.c_str()));
    std::wstring status = w.loaded ? L"Loaded" : L"Not Loaded";
    ListView_SetItemText(g_list, idx, 2, const_cast<wchar_t *>(status.c_str()));
    LogLine(L"[Manage] Widget: " + w.name + L" | " + w.scriptPath + L" | " +
            (w.loaded ? L"Loaded" : L"Not Loaded"));

    if (!selectedScript.empty() &&
        NormalizePathLower(w.scriptPath) == NormalizePathLower(selectedScript)) {
      newSelectIdx = idx;
    }
    ++idx;
  }

  if (newSelectIdx >= 0) {
    ListView_SetItemState(g_list, newSelectIdx,
                          LVIS_SELECTED | LVIS_FOCUSED,
                          LVIS_SELECTED | LVIS_FOCUSED);
    ListView_EnsureVisible(g_list, newSelectIdx, FALSE);
  }

  // Re-enable drawing and repaint in one shot (no flicker)
  SendMessage(g_list, WM_SETREDRAW, TRUE, 0);
  RedrawWindow(g_list, NULL, NULL,
               RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);

  UpdateButtonState();
}

static void UpdateButtonState() {
  if (g_activeTab != 0) {
    EnableWindow(g_btnLoad, FALSE);
    EnableWindow(g_btnRefresh, FALSE);
    EnableWindow(g_btnRefreshAll, FALSE);
    SetWindowTextW(g_btnLoad, L"Load");
    return;
  }

  EnableWindow(g_btnRefreshAll, TRUE);

  const int idx = GetSelectedIndex();
  if (idx < 0 || idx >= static_cast<int>(g_widgets.size())) {
    EnableWindow(g_btnLoad, FALSE);
    EnableWindow(g_btnRefresh, FALSE);
    SetWindowTextW(g_btnLoad, L"Load");
    return;
  }

  const bool isLoaded = g_widgets[idx].loaded;
  SetWindowTextW(g_btnLoad, isLoaded ? L"Unload" : L"Load");
  EnableWindow(g_btnLoad, TRUE);
  EnableWindow(g_btnRefresh, isLoaded ? TRUE : FALSE);
}

static void RefreshLogsView() {
  if (!g_logsList)
    return;

  // Already in sync with the in-memory buffer (live AppendLogRow path).
  if (ListView_GetItemCount(g_logsList) == static_cast<int>(g_logRows.size()))
    return;

  SendMessageW(g_logsList, WM_SETREDRAW, FALSE, 0);
  ListView_DeleteAllItems(g_logsList);
  // g_logRows is newest-first; insert from back so index 0 stays newest.
  for (auto it = g_logRows.rbegin(); it != g_logRows.rend(); ++it)
    InsertLogRowIntoList(*it);
  SendMessageW(g_logsList, WM_SETREDRAW, TRUE, 0);
  RedrawWindow(g_logsList, nullptr, nullptr,
               RDW_ERASE | RDW_FRAME | RDW_INVALIDATE | RDW_ALLCHILDREN);
  if (!g_logRows.empty())
    ListView_EnsureVisible(g_logsList, 0, FALSE);
}

static void OnClearLogs() {
  g_logRows.clear();
  g_pendingLogLine.clear();
  if (g_logsList)
    ListView_DeleteAllItems(g_logsList);
}

static bool ExecuteNovadeskCommandNoPath(const std::wstring &cmd) {
  const std::wstring exe = GetNovadeskExePath();
  const bool ok = RunProcessWait(exe, cmd);
  LogLine(ok ? L"[Manage] Exec OK." : L"[Manage] Exec failed.");
  return ok;
}

static void RefreshSettingsControls() {
  if (!g_chkRunOnStartup)
    return;

  SendMessageW(g_chkRunOnStartup, BM_SETCHECK,
               IsRunOnStartupEnabled() ? BST_CHECKED : BST_UNCHECKED, 0);
  SendMessageW(g_chkAutoUpdate, BM_SETCHECK,
               g_manageAutoCheckForUpdates ? BST_CHECKED : BST_UNCHECKED, 0);
  SendMessageW(g_chkEnableLogging, BM_SETCHECK,
               GetNovadeskSettingBool("disableLogging", false) ? BST_UNCHECKED
                                                               : BST_CHECKED,
               0);
  SendMessageW(g_chkEnableDebugging, BM_SETCHECK,
               GetNovadeskSettingBool("enableDebugging", false) ? BST_CHECKED
                                                                : BST_UNCHECKED,
               0);
  SendMessageW(g_chkSaveLogToFile, BM_SETCHECK,
               GetNovadeskSettingBool("saveLogToFile", false) ? BST_CHECKED
                                                              : BST_UNCHECKED,
               0);
  SendMessageW(g_chkUseHardwareAcceleration, BM_SETCHECK,
               GetNovadeskSettingBool("useHardwareAcceleration", false)
                   ? BST_CHECKED
                   : BST_UNCHECKED,
               0);

  // Never CB_SETCURSEL while the drop-down is open — the settings timer
  // otherwise snaps the highlight back to the saved theme (e.g. Dark) on
  // every tick. Language combo is not refreshed here, which is why it was fine.
  if (g_cboWindowsTheme &&
      !SendMessageW(g_cboWindowsTheme, CB_GETDROPPEDSTATE, 0, 0)) {
    const std::string theme = CommonSettings::GetWindowsTheme();
    int selected = 0; // system
    if (theme == "light")
      selected = 1;
    else if (theme == "dark")
      selected = 2;
    const int current = static_cast<int>(
        SendMessageW(g_cboWindowsTheme, CB_GETCURSEL, 0, 0));
    if (current != selected)
      SendMessageW(g_cboWindowsTheme, CB_SETCURSEL, selected, 0);
  }
}

static void PopulateWindowsThemeCombo() {
  if (!g_cboWindowsTheme) return;

  SendMessageW(g_cboWindowsTheme, CB_RESETCONTENT, 0, 0);

  struct ThemeOption {
    const char *tag;
    const char *key;
    const wchar_t *fallback;
  };
  static const ThemeOption kOptions[] = {
      {"system", "settings.theme.system", L"System"},
      {"light", "settings.theme.light", L"Light"},
      {"dark", "settings.theme.dark", L"Dark"},
  };

  for (const auto &option : kOptions) {
    const std::wstring label = Tr(option.key, option.fallback);
    const int added = static_cast<int>(SendMessageW(
        g_cboWindowsTheme, CB_ADDSTRING, 0,
        reinterpret_cast<LPARAM>(label.c_str())));
    SendMessageW(g_cboWindowsTheme, CB_SETITEMDATA, added,
                 reinterpret_cast<LPARAM>(option.tag));
  }

  const std::string theme = CommonSettings::GetWindowsTheme();
  int selected = 0;
  if (theme == "light")
    selected = 1;
  else if (theme == "dark")
    selected = 2;
  SendMessageW(g_cboWindowsTheme, CB_SETCURSEL, selected, 0);
}

static void CheckForUpdates(bool silent) {
  if (g_updateDialogOpen) {
    return;
  }

  SetSettingsStatusText(L"Checking for updates...");
  std::wstring latest;
  std::wstring assetUrl;
  std::wstring assetName;
  if (!GetLatestGithubRelease(latest, assetUrl, assetName)) {
    SetSettingsStatusText(L"Update check failed.");
    if (!silent) {
      ShowManageMessageBox(L"Unable to check latest version from GitHub.",
                           L"Novadesk Update", MB_OK | MB_ICONWARNING);
    }
    return;
  }

  if (IsVersionNewer(latest, kCurrentVersion)) {
    const std::wstring msg = L"New version available: " + latest +
                             L" (current: " + std::wstring(kCurrentVersion) +
                             L")";
    SetSettingsStatusText(msg);
    const bool shouldShowDialog =
        !silent ||
        (g_manageAutoCheckForUpdates && latest != g_lastNotifiedUpdateVersion);
    if (shouldShowDialog) {
      g_updateDialogOpen = true;
      const int rc = ShowManageMessageBox(
          msg + L"\n\nDownload latest release to temp and run setup now?",
          L"Novadesk Update", MB_YESNO | MB_ICONINFORMATION);
      g_updateDialogOpen = false;
      g_lastNotifiedUpdateVersion = latest;
      if (rc == IDYES) {
        if (assetUrl.empty()) {
          ShowManageMessageBox(
              L"No downloadable asset found for the latest release.",
              L"Novadesk Update", MB_OK | MB_ICONWARNING);
          return;
        }

        wchar_t tempPath[MAX_PATH + 1] = {};
        if (!GetTempPathW(MAX_PATH, tempPath)) {
          ShowManageMessageBox(L"Could not access temp folder.",
                               L"Novadesk Update", MB_OK | MB_ICONWARNING);
          return;
        }

        std::wstring fileName =
            assetName.empty() ? L"novadesk_latest_setup.exe" : assetName;
        std::wstring destination = JoinPath(tempPath, fileName);
        SetSettingsStatusText(L"Downloading update...");
        if (!DownloadFileToPath(assetUrl, destination)) {
          SetSettingsStatusText(L"Download failed.");
          ShowManageMessageBox(L"Failed to download latest release.",
                               L"Novadesk Update", MB_OK | MB_ICONWARNING);
          return;
        }

        SetSettingsStatusText(L"Launching setup...");
        ShellExecuteW(nullptr, L"open", destination.c_str(), nullptr, nullptr,
                      SW_SHOWNORMAL);
      }
    }
  } else {
    const std::wstring msg =
        L"You are up to date (" + std::wstring(kCurrentVersion) + L").";
    SetSettingsStatusText(msg);
    if (!silent) {
      ShowManageMessageBox(msg, L"Novadesk Update", MB_OK | MB_ICONINFORMATION);
    }
  }
}

struct UpdateCheckParams {
  bool silent;
};

static DWORD WINAPI CheckForUpdatesThreadProc(LPVOID lpParam) {
  UpdateCheckParams *params = static_cast<UpdateCheckParams *>(lpParam);
  if (!params)
    return 0;

  bool silent = params->silent;
  delete params;

  CheckForUpdates(silent);

  g_isCheckingForUpdates = false;
  return 0;
}

static void StartAsyncUpdateCheck(bool silent) {
  if (g_isCheckingForUpdates)
    return;

  g_isCheckingForUpdates = true;
  UpdateCheckParams *params = new UpdateCheckParams{silent};
  HANDLE hThread =
      CreateThread(nullptr, 0, CheckForUpdatesThreadProc, params, 0, nullptr);
  if (hThread) {
    CloseHandle(hThread);
  } else {
    delete params;
    g_isCheckingForUpdates = false;
  }
}

static void OnSettingsControlClicked(int controlId) {
  // Controls live in the tab child window — use global HWNDs directly.
  HWND ctrl = nullptr;
  switch (controlId) {
  case kControlIdRunOnStartup:           ctrl = g_chkRunOnStartup; break;
  case kControlIdAutoCheckUpdates:       ctrl = g_chkAutoUpdate; break;
  case kControlIdEnableLogging:          ctrl = g_chkEnableLogging; break;
  case kControlIdEnableDebugging:        ctrl = g_chkEnableDebugging; break;
  case kControlIdSaveLogToFile:          ctrl = g_chkSaveLogToFile; break;
  case kControlIdUseHardwareAcceleration: ctrl = g_chkUseHardwareAcceleration; break;
  default: break;
  }
  if (!ctrl) return;
  const bool checked = (SendMessageW(ctrl, BM_GETCHECK, 0, 0) == BST_CHECKED);
  switch (controlId) {
  case kControlIdRunOnStartup:
    if (!SetRunOnStartupEnabled(checked)) {
      MessageBoxW(g_mainWindow, L"Failed to update startup entry.",
                  L"Manage Novadesk", MB_OK | MB_ICONWARNING);
    }
    break;
  case kControlIdAutoCheckUpdates:
    g_manageAutoCheckForUpdates = checked;
    SaveManageSettings();
    ConfigureAutoUpdateTimer(g_mainWindow);
    if (checked) {
      CheckForUpdates(true);
    }
    break;
  case kControlIdEnableLogging:
    SetNovadeskSettingBool("disableLogging", !checked);
    ExecuteNovadeskCommandNoPath(checked ? L"--enable-logging"
                                         : L"--disable-logging");
    break;
  case kControlIdEnableDebugging:
    SetNovadeskSettingBool("enableDebugging", checked);
    ExecuteNovadeskCommandNoPath(checked ? L"--enable-debugging"
                                         : L"--disable-debugging");
    break;
  case kControlIdSaveLogToFile:
    SetNovadeskSettingBool("saveLogToFile", checked);
    ExecuteNovadeskCommandNoPath(checked ? L"--enable-save-log-to-file"
                                         : L"--disable-save-log-to-file");
    break;
  case kControlIdUseHardwareAcceleration: {
    const bool previous =
        GetNovadeskSettingBool("useHardwareAcceleration", false);
    SetNovadeskSettingBool("useHardwareAcceleration", checked);
    if (previous != checked) {
      PromptRestartForHardwareAcceleration();
    }
    break;
  }
  default:
    break;
  }

  RefreshSettingsControls();
}

static void ConfigureAutoUpdateTimer(HWND hWnd) {
  if (!hWnd)
    return;

  KillTimer(hWnd, kAutoUpdateTimerId);
  if (g_manageAutoCheckForUpdates) {
    SetTimer(hWnd, kAutoUpdateTimerId, kAutoUpdateIntervalMs, nullptr);
  }
}

static void OnToggleLoadSelected() {
  int idx = GetSelectedIndex();
  if (idx < 0 || idx >= static_cast<int>(g_widgets.size())) {
    LogLine(L"[Manage] Toggle load ignored: no selection.");
    return;
  }

  const std::wstring targetScript = g_widgets[idx].scriptPath;
  const bool isLoaded = g_widgets[idx].loaded;
  bool ok = false;
  if (isLoaded) {
    LogLine(L"[Manage] Unload: " + targetScript);
    ok = ExecuteNovadeskCommand(L"--unload", targetScript);
  } else {
    LogLine(L"[Manage] Load: " + targetScript);
    ok = ExecuteNovadeskCommand(L"--load", targetScript);
  }
  if (ok) {
    RememberLoadedScriptState(targetScript, !isLoaded);
  } else {
    ShowManageMessageBox(L"Failed to send command to Novadesk.",
                         L"Manage Novadesk", MB_OK | MB_ICONWARNING);
  }
  RefreshListView(targetScript);
}

static void OnRefreshSelected() {
  int idx = GetSelectedIndex();
  if (idx < 0 || idx >= static_cast<int>(g_widgets.size())) {
    LogLine(L"[Manage] Refresh ignored: no selection.");
    return;
  }
  const std::wstring targetScript = g_widgets[idx].scriptPath;
  LogLine(L"[Manage] Refresh: " + targetScript);
  ExecuteNovadeskCommand(L"--refresh", targetScript);
  RefreshListView(targetScript);
}

static void OnRefreshAll() {
  int idx = GetSelectedIndex();
  std::wstring targetScript;
  if (idx >= 0 && idx < static_cast<int>(g_widgets.size())) {
    targetScript = g_widgets[idx].scriptPath;
  }
  LogLine(L"[Manage] Refresh All");
  ExecuteNovadeskCommandNoPath(L"--refresh-all");
  RefreshListView(targetScript);
}

class DialogManage;
static void ApplyThemeToManageWindow(HWND hWnd);

// ============================================================================
// DialogManage
// ============================================================================

class DialogManage : public Dialog {
public:
  class TabWidgets : public Dialog::Tab {
  public:
    TabWidgets() = default;
    virtual ~TabWidgets() override = default;

    virtual void Create(HWND owner) override {
      RECT tabRectOnParent{};
      GetClientRect(DialogManage::Get()->m_TabControl, &tabRectOnParent);
      TabCtrl_AdjustRect(DialogManage::Get()->m_TabControl, FALSE, &tabRectOnParent);
      MapWindowPoints(DialogManage::Get()->m_TabControl, owner, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
      CreateTabWindowPixel(tabRectOnParent, owner);
    }

    virtual void Initialize() override {
      m_Initialized = true;
      RECT rc{};
      GetClientRect(m_Window, &rc);
      g_list = CreateWindowExW(
          0, WC_LISTVIEWW, L"",
          WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_CLIPSIBLINGS,
          0, 0, rc.right, rc.bottom, m_Window, nullptr,
          GetModuleHandleW(nullptr), nullptr);
      ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
      LVCOLUMNW col{};
      col.mask = LVCF_TEXT | LVCF_WIDTH;
      col.pszText = const_cast<wchar_t *>(Tr("manage.column.widget", L"Widget").c_str());
      col.cx = 160;
      ListView_InsertColumn(g_list, 0, &col);
      col.pszText = const_cast<wchar_t *>(Tr("manage.column.scriptPath", L"Script Path").c_str());
      col.cx = 360;
      ListView_InsertColumn(g_list, 1, &col);
      col.pszText = const_cast<wchar_t *>(Tr("manage.column.status", L"Status").c_str());
      col.cx = 120;
      ListView_InsertColumn(g_list, 2, &col);

      if (g_buttonFont) {
        SendMessageW(g_list, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      }
      RefreshListView();
      UpdateButtonState();
    }

    virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
      if (uMsg == WM_NOTIFY) {
        LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr && hdr->hwndFrom == g_list) {
          if (hdr->code == LVN_ITEMCHANGED) {
            UpdateButtonState();
            return TRUE;
          } else if (hdr->code == NM_DBLCLK) {
            OnToggleLoadSelected();
            return TRUE;
          }
        }
      }
      return FALSE;
    }

    virtual void Relayout(int w, int h) override {
      if (g_list) {
        MoveWindow(g_list, 0, 0, w, h, TRUE);
      }
    }
  };

  class TabLogs : public Dialog::Tab {
  public:
    TabLogs() = default;
    virtual ~TabLogs() override = default;

    virtual void Create(HWND owner) override {
      RECT tabRectOnParent{};
      GetClientRect(DialogManage::Get()->m_TabControl, &tabRectOnParent);
      TabCtrl_AdjustRect(DialogManage::Get()->m_TabControl, FALSE, &tabRectOnParent);
      MapWindowPoints(DialogManage::Get()->m_TabControl, owner, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
      CreateTabWindowPixel(tabRectOnParent, owner);
    }

    virtual void Initialize() override {
      m_Initialized = true;
      RECT rc{};
      GetClientRect(m_Window, &rc);
      g_logsList = CreateWindowExW(
          0, WC_LISTVIEWW, L"",
          WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_HSCROLL | WS_CLIPSIBLINGS,
          0, 0, rc.right, rc.bottom, m_Window, nullptr,
          GetModuleHandleW(nullptr), nullptr);
      ListView_SetExtendedListViewStyle(g_logsList,
                                        LVS_EX_FULLROWSELECT |
                                            LVS_EX_DOUBLEBUFFER);
      if (HIMAGELIST images = EnsureLogsImageList())
        ListView_SetImageList(g_logsList, images, LVSIL_SMALL);

      LVCOLUMNW lcol{};
      lcol.mask = LVCF_TEXT | LVCF_WIDTH;
      // Level first so the icon + type are visible at the start of each row.
      lcol.pszText = const_cast<wchar_t *>(Tr("manage.column.level", L"Level").c_str());
      lcol.cx = 90;
      ListView_InsertColumn(g_logsList, 0, &lcol);
      lcol.pszText = const_cast<wchar_t *>(Tr("manage.column.time", L"Time").c_str());
      lcol.cx = 190;
      ListView_InsertColumn(g_logsList, 1, &lcol);
      lcol.pszText = const_cast<wchar_t *>(Tr("manage.column.source", L"Source").c_str());
      lcol.cx = 120;
      ListView_InsertColumn(g_logsList, 2, &lcol);
      lcol.pszText = const_cast<wchar_t *>(Tr("manage.column.message", L"Message").c_str());
      lcol.cx = g_logsMessageColumnWidth;
      ListView_InsertColumn(g_logsList, 3, &lcol);

      if (g_buttonFont) {
        SendMessageW(g_logsList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      }
      RefreshLogsView();
    }

    virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
      if (uMsg == WM_CONTEXTMENU) {
        if (reinterpret_cast<HWND>(wParam) == g_logsList) {
          ShowLogsContextMenu(m_Window, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
          return TRUE;
        }
      }
      return FALSE;
    }

    virtual void Relayout(int w, int h) override {
      if (g_logsList) {
        MoveWindow(g_logsList, 0, 0, w, h, TRUE);
      }
    }
  };

  class TabAddons : public Dialog::Tab {
  public:
    TabAddons() = default;
    virtual ~TabAddons() override = default;

    virtual void Create(HWND owner) override {
      RECT tabRectOnParent{};
      GetClientRect(DialogManage::Get()->m_TabControl, &tabRectOnParent);
      TabCtrl_AdjustRect(DialogManage::Get()->m_TabControl, FALSE, &tabRectOnParent);
      MapWindowPoints(DialogManage::Get()->m_TabControl, owner, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
      CreateTabWindowPixel(tabRectOnParent, owner);
    }

    virtual void Initialize() override {
      m_Initialized = true;
      RECT rc{};
      GetClientRect(m_Window, &rc);
      g_addonsList = CreateWindowExW(
          0, WC_LISTVIEWW, L"",
          WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS | WS_CLIPSIBLINGS,
          0, 0, rc.right, rc.bottom, m_Window, nullptr,
          GetModuleHandleW(nullptr), nullptr);
      ListView_SetExtendedListViewStyle(g_addonsList, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
      LVCOLUMNW acol{};
      acol.mask = LVCF_TEXT | LVCF_WIDTH;
      acol.pszText = const_cast<wchar_t *>(L"Addon Name");
      acol.cx = 190;
      ListView_InsertColumn(g_addonsList, 0, &acol);
      acol.pszText = const_cast<wchar_t *>(L"Author");
      acol.cx = 180;
      ListView_InsertColumn(g_addonsList, 1, &acol);
      acol.pszText = const_cast<wchar_t *>(L"Version");
      acol.cx = 120;
      ListView_InsertColumn(g_addonsList, 2, &acol);
      acol.pszText = const_cast<wchar_t *>(L"Copyright");
      acol.cx = 420;
      ListView_InsertColumn(g_addonsList, 3, &acol);

      if (g_buttonFont) {
        SendMessageW(g_addonsList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      }
      RefreshAddonsView();
    }

    virtual void Relayout(int w, int h) override {
      if (g_addonsList) {
        MoveWindow(g_addonsList, 0, 0, w, h, TRUE);
      }
    }
  };

  class TabSettings : public Dialog::Tab {
  public:
    TabSettings() = default;
    virtual ~TabSettings() override = default;

    virtual void Create(HWND owner) override {
      RECT tabRectOnParent{};
      GetClientRect(DialogManage::Get()->m_TabControl, &tabRectOnParent);
      TabCtrl_AdjustRect(DialogManage::Get()->m_TabControl, FALSE, &tabRectOnParent);
      MapWindowPoints(DialogManage::Get()->m_TabControl, owner, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
      CreateTabWindowPixel(tabRectOnParent, owner);
    }

    virtual void Initialize() override {
      m_Initialized = true;
      RECT rc{};
      GetClientRect(m_Window, &rc);
      const int w = rc.right;

      g_settingsTitle = CreateWindowExW(0, L"STATIC", Tr("settings.title", L"Manage Settings").c_str(),
          WS_CHILD | WS_VISIBLE | SS_LEFT | WS_CLIPSIBLINGS,
          16, 12, w - 32, 24, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_chkRunOnStartup = CreateWindowExW(0, L"BUTTON", Tr("manage.setting.runOnStartup", L"Run On Startup").c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | WS_CLIPSIBLINGS,
          16, 46, 240, 24, m_Window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRunOnStartup)),
          GetModuleHandleW(nullptr), nullptr);
      g_btnCheckUpdates = CreateWindowExW(0, L"BUTTON", Tr("manage.setting.checkUpdates", L"Check for Updates").c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON | WS_CLIPSIBLINGS,
          16, 78, 170, 28, m_Window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdCheckUpdates)),
          GetModuleHandleW(nullptr), nullptr);
      g_chkAutoUpdate = CreateWindowExW(0, L"BUTTON", Tr("manage.setting.autoCheckUpdates", L"Automatically check for updates").c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | WS_CLIPSIBLINGS,
          202, 80, w - 218, 24, m_Window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAutoCheckUpdates)),
          GetModuleHandleW(nullptr), nullptr);
      g_chkEnableLogging = CreateWindowExW(0, L"BUTTON", Tr("manage.setting.enableLogging", L"Enable Logging").c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | WS_CLIPSIBLINGS,
          16, 118, 220, 24, m_Window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdEnableLogging)),
          GetModuleHandleW(nullptr), nullptr);
      g_chkEnableDebugging = CreateWindowExW(0, L"BUTTON", Tr("manage.setting.enableDebugging", L"Enable Debugging").c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | WS_CLIPSIBLINGS,
          16, 148, 220, 24, m_Window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdEnableDebugging)),
          GetModuleHandleW(nullptr), nullptr);
      g_chkSaveLogToFile = CreateWindowExW(0, L"BUTTON", Tr("manage.setting.saveLogs", L"Save Logs to File").c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | WS_CLIPSIBLINGS,
          16, 178, 260, 24, m_Window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdSaveLogToFile)),
          GetModuleHandleW(nullptr), nullptr);
      g_chkUseHardwareAcceleration = CreateWindowExW(0, L"BUTTON", Tr("manage.setting.hardwareAcceleration", L"Use Hardware Acceleration").c_str(),
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX | WS_CLIPSIBLINGS,
          16, 236, 300, 24, m_Window, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdUseHardwareAcceleration)),
          GetModuleHandleW(nullptr), nullptr);
      g_lblWindowsTheme = CreateWindowExW(0, L"STATIC", Tr("settings.theme", L"Theme").c_str(),
          WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
          16, 272, 120, 24, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_cboWindowsTheme = CreateWindowExW(
          Localization::IsRtl() ? WS_EX_RTLREADING : 0, WC_COMBOBOXW, L"",
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
          140, 268, 260, 360, m_Window,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdWindowsTheme)),
          GetModuleHandleW(nullptr), nullptr);
      PopulateWindowsThemeCombo();
      g_lblLanguage = CreateWindowExW(0, L"STATIC", Tr("settings.language", L"Language").c_str(),
          WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
          16, 308, 120, 24, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_cboLanguage = CreateWindowExW(
          Localization::IsRtl() ? WS_EX_RTLREADING : 0, WC_COMBOBOXW, L"",
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | WS_VSCROLL,
          140, 304, 360, 360, m_Window,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdLanguage)),
          GetModuleHandleW(nullptr), nullptr);
      const std::string currentLanguage = CommonSettings::GetUiLanguage();
      int selectedLanguage = 0;
      int languageIndex = 0;
      for (const auto &language : Localization::SupportedLanguages()) {
        std::wstring label = language.nativeName;
        if (language.englishName && language.englishName[0] &&
            _wcsicmp(language.nativeName, language.englishName) != 0) {
          label += L" - ";
          label += language.englishName;
        }
        const int added = static_cast<int>(SendMessageW(
            g_cboLanguage, CB_ADDSTRING, 0,
            reinterpret_cast<LPARAM>(label.c_str())));
        SendMessageW(g_cboLanguage, CB_SETITEMDATA, added,
                     reinterpret_cast<LPARAM>(language.tag));
        if (currentLanguage == language.tag) selectedLanguage = languageIndex;
        ++languageIndex;
      }
      SendMessageW(g_cboLanguage, CB_SETCURSEL, selectedLanguage, 0);

      if (g_buttonFont) {
        SendMessageW(g_settingsTitle, WM_SETFONT, (WPARAM)g_aboutSectionFont, TRUE);
        SendMessageW(g_chkRunOnStartup, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_btnCheckUpdates, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_chkAutoUpdate, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_chkEnableLogging, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_chkEnableDebugging, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_chkSaveLogToFile, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_chkUseHardwareAcceleration, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_lblWindowsTheme, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_cboWindowsTheme, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_lblLanguage, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        SendMessageW(g_cboLanguage, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      }
      RefreshSettingsControls();
      if (Localization::IsRtl()) {
        ApplyLocalizedText();
      }
    }

    virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
      if (uMsg == WM_COMMAND) {
        switch (LOWORD(wParam)) {
        case kControlIdRunOnStartup:
        case kControlIdAutoCheckUpdates:
        case kControlIdEnableLogging:
        case kControlIdEnableDebugging:
        case kControlIdSaveLogToFile:
        case kControlIdUseHardwareAcceleration:
          if (HIWORD(wParam) == BN_CLICKED) {
            OnSettingsControlClicked(LOWORD(wParam));
            return TRUE;
          }
          break;
        case kControlIdCheckUpdates:
          StartAsyncUpdateCheck(false);
          return TRUE;
        case kControlIdWindowsTheme:
          // Commit only on CBN_SELENDOK so hover/keyboard preview in the
          // drop-down does not apply theme or stick a temporary label.
          if (HIWORD(wParam) == CBN_DROPDOWN) {
            SetWindowLongPtrW(g_cboWindowsTheme, GWLP_USERDATA,
                              SendMessageW(g_cboWindowsTheme, CB_GETCURSEL, 0, 0));
            return TRUE;
          }
          if (HIWORD(wParam) == CBN_SELENDCANCEL) {
            const int previous = static_cast<int>(
                GetWindowLongPtrW(g_cboWindowsTheme, GWLP_USERDATA));
            if (previous >= 0)
              SendMessageW(g_cboWindowsTheme, CB_SETCURSEL, previous, 0);
            return TRUE;
          }
          if (HIWORD(wParam) == CBN_SELENDOK) {
            const int selected = static_cast<int>(
                SendMessageW(g_cboWindowsTheme, CB_GETCURSEL, 0, 0));
            const char *tag = reinterpret_cast<const char *>(
                SendMessageW(g_cboWindowsTheme, CB_GETITEMDATA, selected, 0));
            if (tag && CommonSettings::SetWindowsTheme(tag)) {
              WindowsTheme::SetMode(tag);
              ApplyThemeToManageWindow(g_mainWindow);
              if (g_themeChangedMessage)
                PostMessageW(HWND_BROADCAST, g_themeChangedMessage, 0, 0);
            }
            return TRUE;
          }
          break;
        case kControlIdLanguage:
          if (HIWORD(wParam) == CBN_DROPDOWN) {
            SetWindowLongPtrW(g_cboLanguage, GWLP_USERDATA,
                              SendMessageW(g_cboLanguage, CB_GETCURSEL, 0, 0));
            return TRUE;
          }
          if (HIWORD(wParam) == CBN_SELENDCANCEL) {
            const int previous = static_cast<int>(
                GetWindowLongPtrW(g_cboLanguage, GWLP_USERDATA));
            if (previous >= 0)
              SendMessageW(g_cboLanguage, CB_SETCURSEL, previous, 0);
            return TRUE;
          }
          if (HIWORD(wParam) == CBN_SELENDOK) {
            const int selected = static_cast<int>(
                SendMessageW(g_cboLanguage, CB_GETCURSEL, 0, 0));
            const char *tag = reinterpret_cast<const char *>(
                SendMessageW(g_cboLanguage, CB_GETITEMDATA, selected, 0));
            if (tag && CommonSettings::SetUiLanguage(tag)) {
              const bool wasRtl = Localization::IsRtl();
              Localization::Initialize();
              const bool directionChanged = wasRtl != Localization::IsRtl();
              SetProcessDefaultLayout(
                  Localization::IsRtl() ? LAYOUT_RTL : 0);
              ApplyLocalizedText();
              if (directionChanged)
                RecreateManagerTabsForLayoutDirection(g_activeTab);
              if (g_languageChangedMessage)
                PostMessageW(HWND_BROADCAST, g_languageChangedMessage, 0, 0);
            }
            return TRUE;
          }
          break;
        }
      }
      return FALSE;
    }

    virtual void Relayout(int w, int h) override {
      const int L = 20;              // left margin
      const int sw = w - L - 20;    // control width to right margin
      const int ch = 26;            // standard control height
      const int bh = 30;            // button height
      if (g_settingsTitle)              MoveWindow(g_settingsTitle,              L,       16, sw,       20, TRUE);
      if (g_chkRunOnStartup)            MoveWindow(g_chkRunOnStartup,            L,       50, sw,       ch, TRUE);
      if (g_btnCheckUpdates)            MoveWindow(g_btnCheckUpdates,            L,       86, 200,      bh, TRUE);
      if (g_chkAutoUpdate)              MoveWindow(g_chkAutoUpdate,          L + 216,    90, sw - 216, ch, TRUE);
      if (g_chkEnableLogging)           MoveWindow(g_chkEnableLogging,           L,      134, sw,       ch, TRUE);
      if (g_chkEnableDebugging)         MoveWindow(g_chkEnableDebugging,         L,      168, sw,       ch, TRUE);
      if (g_chkSaveLogToFile)           MoveWindow(g_chkSaveLogToFile,           L,      202, sw,       ch, TRUE);
      if (g_chkUseHardwareAcceleration) MoveWindow(g_chkUseHardwareAcceleration, L,      236, sw,       ch, TRUE);
      if (g_lblWindowsTheme)            MoveWindow(g_lblWindowsTheme,            L,      272, 120,      24, TRUE);
      if (g_cboWindowsTheme)            MoveWindow(g_cboWindowsTheme,        L + 124,  268, 260,     360, TRUE);
      if (g_lblLanguage)                MoveWindow(g_lblLanguage,                L,      308, 120,      24, TRUE);
      if (g_cboLanguage)                MoveWindow(g_cboLanguage,            L + 124,  304, 360,     360, TRUE);
    }
  };

  class TabAbout : public Dialog::Tab {
  public:
    TabAbout() = default;
    virtual ~TabAbout() override = default;

    virtual void Create(HWND owner) override {
      RECT tabRectOnParent{};
      GetClientRect(DialogManage::Get()->m_TabControl, &tabRectOnParent);
      TabCtrl_AdjustRect(DialogManage::Get()->m_TabControl, FALSE, &tabRectOnParent);
      MapWindowPoints(DialogManage::Get()->m_TabControl, owner, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
      CreateTabWindowPixel(tabRectOnParent, owner);
    }

    virtual void Initialize() override {
      m_Initialized = true;

      // Positions are applied in Relayout so Create and Relayout stay in sync.
      g_aboutLogo = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_BITMAP | SS_CENTERIMAGE | WS_CLIPSIBLINGS,
          0, 0, kAboutLogoSize, kAboutLogoSize, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutAppName = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutVersion = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutCopyright = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutDivider = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_ETCHEDHORZ | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutHomeLabel = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutHomeLink = CreateWindowExW(0, WC_LINK,
          L"<a href=\"https://novadesk.pages.dev/\">https://novadesk.pages.dev/</a>",
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAboutHome)), GetModuleHandleW(nullptr), nullptr);
      g_aboutDocsLabel = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutDocsLink = CreateWindowExW(0, WC_LINK,
          L"<a href=\"https://novadesk-docs.pages.dev/\">https://novadesk-docs.pages.dev/</a>",
          WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAboutDocs)), GetModuleHandleW(nullptr), nullptr);

      // Create framed content before the group box, then push the group box to
      // the bottom of the z-order so WS_CLIPSIBLINGS cannot clip them away.
      g_aboutSupportTitle = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutSupportText = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_LEFT | SS_NOPREFIX,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutDonateButton = CreateWindowExW(0, L"BUTTON", L"",
          WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
          0, 0, 0, 0, m_Window,
          reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdAboutDonate)), GetModuleHandleW(nullptr), nullptr);
      g_aboutSupportGroup = CreateWindowExW(0, L"BUTTON", L"",
          WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      g_aboutBottomNote = CreateWindowExW(0, L"STATIC", L"",
          WS_CHILD | WS_VISIBLE | SS_CENTER | SS_NOPREFIX | WS_CLIPSIBLINGS,
          0, 0, 0, 0, m_Window, nullptr, GetModuleHandleW(nullptr), nullptr);
      ApplyAboutLocalizedText();
      EnsureAboutSupportZOrder();

      if (g_buttonFont) {
        SendMessageW(g_aboutAppName, WM_SETFONT, (WPARAM)g_aboutTitleFont, TRUE);
        SendMessageW(g_aboutVersion, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutCopyright, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutHomeLabel, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutHomeLink, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutDocsLabel, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutDocsLink, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutSupportGroup, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutSupportTitle, WM_SETFONT, (WPARAM)g_aboutSectionFont, TRUE);
        SendMessageW(g_aboutSupportText, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutDonateButton, WM_SETFONT, (WPARAM)g_aboutBodyFont, TRUE);
        SendMessageW(g_aboutBottomNote, WM_SETFONT, (WPARAM)g_aboutSectionFont, TRUE);
      }
      UpdateAboutLogo();
    }

    virtual void OnInitialized() override {
      RECT rc{};
      GetClientRect(m_Window, &rc);
      Relayout(rc.right - rc.left, rc.bottom - rc.top);
      EnsureAboutSupportZOrder();
    }

    virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
      if (uMsg == WM_NOTIFY) {
        auto *hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr && (hdr->hwndFrom == g_aboutHomeLink || hdr->hwndFrom == g_aboutDocsLink) &&
            (hdr->code == NM_CLICK || hdr->code == NM_RETURN)) {
          auto *link = reinterpret_cast<PNMLINK>(lParam);
          if (link) {
            OpenUrl(link->item.szUrl);
            return TRUE;
          }
        }
      } else if (uMsg == WM_COMMAND && LOWORD(wParam) == kControlIdAboutDonate) {
        OpenUrl(L"https://www.patreon.com/cw/officialnovadesk");
        return TRUE;
      }
      return FALSE;
    }

    virtual void Relayout(int w, int h) override {
      const int margin = 16;
      const int logoColWidth = 180;
      const int logoSize = kAboutLogoSize;
      const int logoLeft = margin + (logoColWidth - logoSize) / 2;
      const int infoLeft = margin + logoColWidth + 12;
      const int infoWidth = w - infoLeft - margin;

      if (g_aboutLogo)      MoveWindow(g_aboutLogo,      logoLeft,           margin,            logoSize,          logoSize, TRUE);
      if (g_aboutAppName)   MoveWindow(g_aboutAppName,   infoLeft,           margin,            infoWidth,         36,  TRUE);
      if (g_aboutVersion)   MoveWindow(g_aboutVersion,   infoLeft,           margin + 40,       infoWidth,         22,  TRUE);
      if (g_aboutCopyright) MoveWindow(g_aboutCopyright, infoLeft,           margin + 66,       infoWidth,         22,  TRUE);
      if (g_aboutDivider)   MoveWindow(g_aboutDivider,   infoLeft,           margin + 94,       infoWidth,         2,   TRUE);
      if (g_aboutHomeLabel) MoveWindow(g_aboutHomeLabel, infoLeft,           margin + 106,      70,                22,  TRUE);
      if (g_aboutHomeLink)  MoveWindow(g_aboutHomeLink,  infoLeft + 74,      margin + 106,      infoWidth - 74,    22,  TRUE);
      if (g_aboutDocsLabel) MoveWindow(g_aboutDocsLabel, infoLeft,           margin + 134,      70,                22,  TRUE);
      if (g_aboutDocsLink)  MoveWindow(g_aboutDocsLink,  infoLeft + 74,      margin + 134,      infoWidth - 74,    22,  TRUE);

      const int supportTop         = margin + 180;
      const int supportHeight      = 150;
      const int supportLeft        = margin;
      const int supportWidth       = w - (margin * 2);
      const int supportButtonWidth = 260;
      const int supportButtonHeight= 36;
      const int supportContentLeft = supportLeft + 18;
      const int supportButtonLeft  = supportLeft + supportWidth - supportButtonWidth - 18;
      const int supportTextWidth   = supportButtonLeft - supportContentLeft - 16;

      if (g_aboutSupportGroup)  MoveWindow(g_aboutSupportGroup,  supportLeft,         supportTop,              supportWidth,        supportHeight,        TRUE);
      if (g_aboutSupportTitle)  MoveWindow(g_aboutSupportTitle,  supportContentLeft,  supportTop + 24,         supportTextWidth,    28,                   TRUE);
      if (g_aboutSupportText)   MoveWindow(g_aboutSupportText,   supportContentLeft,  supportTop + 58,         supportTextWidth,    70,                   TRUE);
      if (g_aboutDonateButton)  MoveWindow(g_aboutDonateButton,  supportButtonLeft,   supportTop + 56,         supportButtonWidth,  supportButtonHeight,  TRUE);
      if (g_aboutBottomNote)    MoveWindow(g_aboutBottomNote,    margin,              supportTop + supportHeight + 10, w - margin*2, 22,                   TRUE);
      EnsureAboutSupportZOrder();
    }
  };

  DialogManage() : Dialog(),
    m_TabWidgets(), m_TabLogs(), m_TabAddons(), m_TabSettings(), m_TabAbout() {
    s_instance = this;
  }

  virtual ~DialogManage() override {
    s_instance = nullptr;
  }

  static DialogManage* Get() { return s_instance; }

  static void Open(int tab = 0) {
    if (!s_instance) {
      DialogManage* dlg = new DialogManage();
      dlg->ShowDialogWindow(
          L"Manage Novadesk",
          0, 0, 600, 373,
          DS_CENTER | WS_POPUP | WS_MINIMIZEBOX | WS_CAPTION | WS_SYSMENU,
          WS_EX_APPWINDOW | WS_EX_CONTROLPARENT |
              (Localization::IsRtl() ? WS_EX_LAYOUTRTL : 0),
          nullptr, true);
    }
    if (s_instance) {
      s_instance->SelectTab(tab);
    }
  }

  const std::vector<Tab*>& GetPages() const { return m_Pages; }

  void RecreateTabsForLayoutDirection(int activeTab) {
    // Windows applies inherited RTL layout at native-child creation time.
    // Rebuilding pages keeps the Manager process and its top-level window
    // alive while giving every control a clean native RTL/LTR layout.
    LONG_PTR exStyle = GetWindowLongPtrW(m_Window, GWL_EXSTYLE);
    if (Localization::IsRtl()) exStyle |= WS_EX_LAYOUTRTL;
    else exStyle &= ~static_cast<LONG_PTR>(WS_EX_LAYOUTRTL);
    SetWindowLongPtrW(m_Window, GWL_EXSTYLE, exStyle);
    SetWindowPos(m_Window, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);

    ResetTabs();

    // The tab strip is itself a native child of the manager. Recreate it as
    // well so its item order and text direction inherit the new layout.
    if (g_tab) {
      DestroyWindow(g_tab);
      g_tab = nullptr;
      m_TabControl = nullptr;
    }
    RECT client{};
    GetClientRect(m_Window, &client);
    g_tab = CreateWindowExW(
        0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | TCS_FIXEDWIDTH,
        12, 12, client.right - 24, client.bottom - 64, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdTabs)),
        GetModuleHandleW(nullptr), nullptr);
    m_TabControl = g_tab;
    TabCtrl_SetItemSize(g_tab, 100, 28);
    if (g_buttonFont) SendMessageW(g_tab, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
    WindowsTheme::ApplyToDialog(m_Window);

    g_list = g_logsList = g_addonsList = nullptr;
    g_settingsPanel = g_aboutPanel = nullptr;
    g_settingsTitle = g_chkRunOnStartup = g_btnCheckUpdates = nullptr;
    g_chkAutoUpdate = g_chkEnableLogging = g_chkEnableDebugging = nullptr;
    g_chkSaveLogToFile = g_chkUseHardwareAcceleration = nullptr;
    g_lblLanguage = g_cboLanguage = nullptr;
    g_aboutLogo = g_aboutAppName = g_aboutVersion = g_aboutCopyright = nullptr;
    g_aboutDivider = g_aboutHomeLabel = g_aboutHomeLink = nullptr;
    g_aboutDocsLabel = g_aboutDocsLink = g_aboutSupportGroup = nullptr;
    g_aboutSupportTitle = g_aboutSupportText = g_aboutDonateButton = nullptr;
    g_aboutBottomNote = nullptr;

    AddTab(kControlIdTabs, m_TabWidgets, Tr("tab.widgets", L"Widgets").c_str());
    AddTab(kControlIdTabs, m_TabLogs, Tr("tab.logs", L"Logs").c_str());
    AddTab(kControlIdTabs, m_TabAddons, Tr("tab.addons", L"Addons").c_str());
    AddTab(kControlIdTabs, m_TabSettings, Tr("tab.settings", L"Settings").c_str());
    AddTab(kControlIdTabs, m_TabAbout, Tr("tab.about", L"About").c_str());
    Relayout();
    const int restoredTab = std::clamp(activeTab, 0, 4);
    SelectTab(restoredTab);

    // These controls belong to the main window rather than a tab page, so
    // they survive ResetTabs(). Restore their state explicitly instead of
    // depending on notification ordering while the new tab strip initializes.
    ShowWindow(g_btnRefreshList, restoredTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnLoad, restoredTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefresh, restoredTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefreshAll, restoredTab == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnClearLogs, restoredTab == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnClose, SW_SHOW);
  }

  TabWidgets m_TabWidgets;
  TabLogs m_TabLogs;
  TabAddons m_TabAddons;
  TabSettings m_TabSettings;
  TabAbout m_TabAbout;

  virtual void HandleInitDialog() override {
    g_mainWindow = m_Window;
    SetPropW(m_Window, g_manageWindowScopeProperty.c_str(),
             reinterpret_cast<HANDLE>(1));

    RECT wr{ 0, 0, kWindowWidth, kWindowHeight };
    AdjustWindowRectEx(&wr, (DWORD)GetWindowLongPtrW(m_Window, GWL_STYLE), FALSE, (DWORD)GetWindowLongPtrW(m_Window, GWL_EXSTYLE));    const int winW = wr.right - wr.left;
    const int winH = wr.bottom - wr.top;
    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    const int x = workArea.left + ((workArea.right - workArea.left) - winW) / 2;
    const int y = workArea.top + ((workArea.bottom - workArea.top) - winH) / 2;
    SetWindowPos(m_Window, nullptr, x, y, winW, winH, SWP_NOACTIVATE | SWP_NOZORDER);

    SendMessageW(m_Window, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_windowIconLarge));
    SendMessageW(m_Window, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_windowIconSmall));

    LoadManageSettings();
    StartNovadesk();
    AddTrayIcon(m_Window);
    InitGdiPlus();

    CreateManageFonts(GetDpiForWindow(m_Window));

    RECT rc{};
    GetClientRect(m_Window, &rc);

    g_tab = CreateWindowExW(
        0, WC_TABCONTROLW, L"",
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_TABSTOP | TCS_FIXEDWIDTH,
        12, 12, rc.right - 24, rc.bottom - 64, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdTabs)),
        GetModuleHandleW(nullptr), nullptr);

    TabCtrl_SetItemSize(g_tab, 100, 28);

    g_btnRefreshList = CreateWindowW(
        L"BUTTON", Tr("manage.button.refreshList", L"Refresh List").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        12, rc.bottom - 44, 120, 30, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefreshList)),
        GetModuleHandleW(nullptr), nullptr);
    g_btnClearLogs = CreateWindowW(
        L"BUTTON", Tr("manage.button.clearLogs", L"Clear Logs").c_str(), WS_CHILD | BS_PUSHBUTTON,
        12, rc.bottom - 44, 120, 30, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdClearLogs)),
        GetModuleHandleW(nullptr), nullptr);
    g_btnLoad = CreateWindowW(
        L"BUTTON", Tr("manage.button.load", L"Load").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        142, rc.bottom - 44, 90, 30, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdLoad)),
        GetModuleHandleW(nullptr), nullptr);
    g_btnRefresh = CreateWindowW(
        L"BUTTON", Tr("manage.button.refresh", L"Refresh").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        242, rc.bottom - 44, 90, 30, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefresh)),
        GetModuleHandleW(nullptr), nullptr);
    g_btnRefreshAll = CreateWindowW(
        L"BUTTON", Tr("manage.button.refreshAll", L"Refresh All").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        342, rc.bottom - 44, 110, 30, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefreshAll)),
        GetModuleHandleW(nullptr), nullptr);
    g_btnClose = CreateWindowW(
        L"BUTTON", Tr("button.close", L"Close").c_str(), WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        rc.right - 100, rc.bottom - 44, 88, 30, m_Window,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdClose)),
        GetModuleHandleW(nullptr), nullptr);

    if (g_buttonFont) {
      SendMessageW(g_tab, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      SendMessageW(g_btnRefreshList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      SendMessageW(g_btnClearLogs, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      SendMessageW(g_btnLoad, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      SendMessageW(g_btnRefresh, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      SendMessageW(g_btnRefreshAll, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
      SendMessageW(g_btnClose, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
    }

    AddTab(kControlIdTabs, m_TabWidgets, Tr("tab.widgets", L"Widgets").c_str());
    AddTab(kControlIdTabs, m_TabLogs, Tr("tab.logs", L"Logs").c_str());
    AddTab(kControlIdTabs, m_TabAddons, Tr("tab.addons", L"Addons").c_str());
    AddTab(kControlIdTabs, m_TabSettings, Tr("tab.settings", L"Settings").c_str());
    AddTab(kControlIdTabs, m_TabAbout, Tr("tab.about", L"About").c_str());

    Relayout();

    // Apply theme while the window is still hidden (inside CreateDialogIndirectParam,
    // before ShowDialogWindow calls ShowWindow). m_StartHidden then tells
    // ShowDialogWindow to skip its ShowWindow call entirely, so the window
    // never appears on screen during initialization — it is a tray-only app.
    ApplyThemeToManageWindow(m_Window);
    m_StartHidden = true;

    SetTimer(m_Window, kStartupSyncTimerId, kStartupSyncDelayMs, nullptr);
    SetTimer(m_Window, kLogsRefreshTimerId, 700, nullptr);
    SetTimer(m_Window, kProcessMonitorTimerId, kProcessMonitorIntervalMs, nullptr);
    ConfigureAutoUpdateTimer(m_Window);
    if (g_manageAutoCheckForUpdates) {
      StartAsyncUpdateCheck(true);
    }
  }

  virtual void HandleDpiChange() override {
    const UINT dpi = GetDpiForWindow(m_Window);
    CreateManageFonts(dpi);
    ResendFontsToAllControls();
  }

  virtual void Relayout() override {
    Dialog::Relayout();
    if (!m_Window) return;

    RECT rc{};
    GetClientRect(m_Window, &rc);
    if (m_TabControl) {
      MoveWindow(m_TabControl, 12, 12, rc.right - 24, rc.bottom - 64, TRUE);
      RECT tabRectOnParent{};
      GetClientRect(m_TabControl, &tabRectOnParent);
      TabCtrl_AdjustRect(m_TabControl, FALSE, &tabRectOnParent);
      MapWindowPoints(m_TabControl, m_Window, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);

      const int w = tabRectOnParent.right - tabRectOnParent.left;
      const int h = tabRectOnParent.bottom - tabRectOnParent.top;
      for (auto* page : m_Pages) {
        if (page && page->GetWindow()) {
          page->SetPosition(tabRectOnParent.left, tabRectOnParent.top, w, h);
          page->Relayout(w, h);
        }
      }
    }
    if (g_btnRefreshList) {
      MoveWindow(g_btnRefreshList, 12, rc.bottom - 44, 120, 30, TRUE);
      MoveWindow(g_btnClearLogs,   12, rc.bottom - 44, 120, 30, TRUE);
      MoveWindow(g_btnLoad,       142, rc.bottom - 44,  90, 30, TRUE);
      MoveWindow(g_btnRefresh,    242, rc.bottom - 44,  90, 30, TRUE);
      MoveWindow(g_btnRefreshAll, 342, rc.bottom - 44, 110, 30, TRUE);
      MoveWindow(g_btnClose, rc.right - 100, rc.bottom - 44, 88, 30, TRUE);
    }
  }

  virtual void ActivateTab() override {
    Dialog::ActivateTab();

    const int active = TabCtrl_GetCurSel(m_TabControl);
    g_activeTab = active;

    ShowWindow(g_btnRefreshList, active == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnLoad, active == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefresh, active == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefreshAll, active == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnClearLogs, active == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnClose, SW_SHOW);

    if (active == 0) {
      UpdateButtonState();
    } else if (active == 1) {
      RefreshLogsView();
    } else if (active == 2) {
      RefreshAddonsView();
    } else if (active == 3) {
      RefreshSettingsControls();
    } else if (active == 4) {
      UpdateAboutLogo();
      EnsureAboutSupportZOrder();
    }
  }

  virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override {
    if (g_manageCloseMessage != 0 && uMsg == g_manageCloseMessage) {
      RequestAppExit(m_Window);
      return TRUE;
    }
    if (g_themeChangedMessage != 0 && uMsg == g_themeChangedMessage) {
      ApplyThemeToManageWindow(m_Window);
      if (g_activeTab == 3) RefreshSettingsControls();
      return TRUE;
    }

    switch (uMsg) {
    case WM_INITDIALOG:
      // Do NOT call HandleInitDialog() here — BaseDialog::InitialDlgProc
      // already calls HandleInitDialog() after HandleMessage() returns.
      // Calling it here too was causing every AddTab to fire twice (duplicate tabs).
      return Dialog::HandleMessage(uMsg, wParam, lParam); // DPI setup

    case WM_GETMINMAXINFO: {
      MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
      mmi->ptMinTrackSize.x = kWindowWidth;
      mmi->ptMinTrackSize.y = kWindowHeight;
      mmi->ptMaxTrackSize.x = kWindowWidth;
      mmi->ptMaxTrackSize.y = kWindowHeight;
      return 0;
    }

    case WM_SIZE:
      Relayout();
      return 0;

    case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case kControlIdRefreshList:
        RefreshListView();
        return TRUE;
      case kControlIdClearLogs:
        OnClearLogs();
        return TRUE;
      case kControlIdLoad:
        OnToggleLoadSelected();
        return TRUE;
      case kControlIdRefresh:
        OnRefreshSelected();
        return TRUE;
      case kControlIdRefreshAll:
        OnRefreshAll();
        return TRUE;
      case kControlIdClose:
        ShowWindow(m_Window, SW_HIDE);
        return TRUE;
      case kTrayMenuShowHideId:
        if (IsWindowVisible(m_Window)) {
          ShowWindow(m_Window, SW_HIDE);
        } else {
          ShowWindow(m_Window, SW_SHOW);
          SetForegroundWindow(m_Window);
        }
        return TRUE;
      case kTrayMenuHomeWebsiteId:
        OpenUrl(L"https://novadesk.pages.dev/");
        return TRUE;
      case kTrayMenuHomeDonateId:
        OpenUrl(L"https://www.patreon.com/cw/officialnovadesk");
        return TRUE;
      case kTrayMenuDocsMainId:
        OpenUrl(L"https://novadesk-docs.pages.dev/");
        return TRUE;
      case kTrayMenuDocsApiId:
        OpenUrl(L"https://novadesk-docs.pages.dev/api/logging.html");
        return TRUE;
      case kTrayMenuSettingsOpenId:
        SelectTab(3);
        ShowWindow(m_Window, SW_SHOW);
        SetForegroundWindow(m_Window);
        return TRUE;
      case kTrayMenuSettingsCheckUpdatesId:
        ShowWindow(m_Window, SW_SHOW);
        SetForegroundWindow(m_Window);
        SelectTab(3);
        StartAsyncUpdateCheck(false);
        return TRUE;
      case kTrayMenuExitId:
        RequestAppExit(m_Window);
        return TRUE;
      }
      break;

    case WM_TIMER:
      if (wParam == kLogsRefreshTimerId && g_activeTab == 1) {
        RefreshLogsView();
      } else if (wParam == kAutoUpdateTimerId && g_manageAutoCheckForUpdates) {
        CheckForUpdates(true);
      } else if (wParam == kStartupSyncTimerId) {
        KillTimer(m_Window, kStartupSyncTimerId);
        ApplySavedLoadedScripts();
        RefreshListView();
        UpdateButtonState();
      } else if (wParam == kProcessMonitorTimerId) {
        if (g_activeTab == 3) {
          RefreshSettingsControls();
        }
        if (g_novadeskRunning && g_novadeskProcess.hProcess) {
          DWORD exitCode = 0;
          if (GetExitCodeProcess(g_novadeskProcess.hProcess, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
              LogLine(L"[Manage] Novadesk process exited. Shutting down Manager.");
              RequestAppExit(m_Window);
            }
          }
        }
      }
      return 0;

    case WM_ACTIVATE:
      if (LOWORD(wParam) != WA_INACTIVE && g_activeTab == 3) {
        RefreshSettingsControls();
      }
      break;

    case kLogAppendMessage: {
      auto *chunk = reinterpret_cast<std::wstring *>(lParam);
      if (chunk) {
        ProcessLogChunk(*chunk);
        delete chunk;
        if (g_activeTab == 1) {
          RefreshLogsView();
        }
      }
      return 0;
    }

    case WM_CLOSE:
      if (g_forceExitOnClose) {
        DestroyWindow(m_Window);
      } else {
        ShowWindow(m_Window, SW_HIDE);
      }
      return TRUE;

    case kTrayMessage:
      if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
        ShowWindow(m_Window, SW_SHOW);
        SetForegroundWindow(m_Window);
      } else if (lParam == WM_RBUTTONUP) {
        ShowTrayMenu(m_Window);
      }
      return 0;

    case WM_DESTROY:
      RemovePropW(m_Window, g_manageWindowScopeProperty.c_str());
      g_mainWindow = nullptr;
      KillTimer(m_Window, kLogsRefreshTimerId);
      KillTimer(m_Window, kAutoUpdateTimerId);
      KillTimer(m_Window, kStartupSyncTimerId);
      KillTimer(m_Window, kProcessMonitorTimerId);
      DeleteManageFonts();
      if (g_aboutLogoBitmap) {
        DeleteObject(g_aboutLogoBitmap);
        g_aboutLogoBitmap = nullptr;
      }
      if (g_logsList)
        ListView_SetImageList(g_logsList, nullptr, LVSIL_SMALL);
      DestroyLogsImageList();
      ShutdownGdiPlus();
      if (g_trayMenu) {
        DestroyMenu(g_trayMenu);
        g_trayMenu = nullptr;
      }
      RemoveTrayIcon();
      StopNovadesk();
      PostQuitMessage(0);
      return 0;

    case WM_SETTINGCHANGE:
      ApplyThemeToManageWindow(m_Window);
      return 0;
    }

    return Dialog::HandleMessage(uMsg, wParam, lParam);
  }

private:
  static DialogManage* s_instance;
};

DialogManage* DialogManage::s_instance = nullptr;

static void RecreateManagerTabsForLayoutDirection(int activeTab) {
  if (DialogManage::Get())
    DialogManage::Get()->RecreateTabsForLayoutDirection(activeTab);
}

static void SetListColumnText(HWND list, int index, const std::wstring &text) {
  if (!list) return;
  LVCOLUMNW column{};
  column.mask = LVCF_TEXT;
  column.pszText = const_cast<wchar_t *>(text.c_str());
  ListView_SetColumn(list, index, &column);
}

static void ApplyLocalizedText() {
  if (!g_mainWindow) return;
  SetWindowTextW(g_mainWindow, Tr("app.manage.title", L"Manage Novadesk").c_str());
  SetWindowTextW(g_btnRefreshList, Tr("manage.button.refreshList", L"Refresh List").c_str());
  SetWindowTextW(g_btnClearLogs, Tr("manage.button.clearLogs", L"Clear Logs").c_str());
  SetWindowTextW(g_btnRefresh, Tr("manage.button.refresh", L"Refresh").c_str());
  SetWindowTextW(g_btnRefreshAll, Tr("manage.button.refreshAll", L"Refresh All").c_str());
  SetWindowTextW(g_btnClose, Tr("button.close", L"Close").c_str());
  SetWindowTextW(g_settingsTitle, Tr("settings.title", L"Manage Settings").c_str());
  SetWindowTextW(g_chkRunOnStartup, Tr("manage.setting.runOnStartup", L"Run On Startup").c_str());
  SetWindowTextW(g_btnCheckUpdates, Tr("manage.setting.checkUpdates", L"Check for Updates").c_str());
  SetWindowTextW(g_chkAutoUpdate, Tr("manage.setting.autoCheckUpdates", L"Automatically check for updates").c_str());
  SetWindowTextW(g_chkEnableLogging, Tr("manage.setting.enableLogging", L"Enable Logging").c_str());
  SetWindowTextW(g_chkEnableDebugging, Tr("manage.setting.enableDebugging", L"Enable Debugging").c_str());
  SetWindowTextW(g_chkSaveLogToFile, Tr("manage.setting.saveLogs", L"Save Logs to File").c_str());
  SetWindowTextW(g_chkUseHardwareAcceleration, Tr("manage.setting.hardwareAcceleration", L"Use Hardware Acceleration").c_str());
  SetWindowTextW(g_lblWindowsTheme, Tr("settings.theme", L"Theme").c_str());
  SetWindowTextW(g_lblLanguage, Tr("settings.language", L"Language").c_str());
  PopulateWindowsThemeCombo();
  ApplyAboutLocalizedText();

  const std::wstring tabs[] = {Tr("tab.widgets", L"Widgets"), Tr("tab.logs", L"Logs"),
      Tr("tab.addons", L"Addons"), Tr("tab.settings", L"Settings"), Tr("tab.about", L"About")};
  for (int i = 0; g_tab && i < static_cast<int>(ARRAYSIZE(tabs)); ++i) {
    TCITEMW item{}; item.mask = TCIF_TEXT;
    item.pszText = const_cast<wchar_t *>(tabs[i].c_str());
    TabCtrl_SetItem(g_tab, i, &item);
  }
  SetListColumnText(g_list, 0, Tr("manage.column.widget", L"Widget"));
  SetListColumnText(g_list, 1, Tr("manage.column.scriptPath", L"Script Path"));
  SetListColumnText(g_list, 2, Tr("manage.column.status", L"Status"));
  SetListColumnText(g_logsList, 0, Tr("manage.column.level", L"Level"));
  SetListColumnText(g_logsList, 1, Tr("manage.column.time", L"Time"));
  SetListColumnText(g_logsList, 2, Tr("manage.column.source", L"Source"));
  SetListColumnText(g_logsList, 3, Tr("manage.column.message", L"Message"));

  // Footer controls are owned by the main window. A language update must not
  // leave the Widgets footer visible over another active page.
  ShowWindow(g_btnRefreshList, g_activeTab == 0 ? SW_SHOW : SW_HIDE);
  ShowWindow(g_btnLoad, g_activeTab == 0 ? SW_SHOW : SW_HIDE);
  ShowWindow(g_btnRefresh, g_activeTab == 0 ? SW_SHOW : SW_HIDE);
  ShowWindow(g_btnRefreshAll, g_activeTab == 0 ? SW_SHOW : SW_HIDE);
  ShowWindow(g_btnClearLogs, g_activeTab == 1 ? SW_SHOW : SW_HIDE);
  ShowWindow(g_btnClose, SW_SHOW);

  DrawMenuBar(g_mainWindow);
  RedrawWindow(g_mainWindow, nullptr, nullptr, RDW_INVALIDATE | RDW_ALLCHILDREN);
}

static void ApplyThemeToManageWindow(HWND hWnd) {
  if (!hWnd) return;

  WindowsTheme::Refresh();
  g_isDarkMode = WindowsTheme::IsUsingDarkMode();

  WindowsTheme::ApplyToDialog(hWnd);

  if (DialogManage::Get()) {
    for (auto* page : DialogManage::Get()->GetPages()) {
      if (page && page->GetWindow()) {
        WindowsTheme::ApplyToDialog(page->GetWindow());
      }
    }
  }

  // Theme re-applies WS_CLIPSIBLINGS; keep Support Section content visible.
  EnsureAboutSupportZOrder();

  RedrawWindow(hWnd, nullptr, nullptr,
               RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

static bool HasCommandLineFlag(const wchar_t *flag) {
  if (!flag || !*flag) {
    return false;
  }

  int argc = 0;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!argv) {
    return false;
  }

  bool found = false;
  for (int i = 1; i < argc; ++i) {
    if (argv[i] && _wcsicmp(argv[i], flag) == 0) {
      found = true;
      break;
    }
  }
  LocalFree(argv);
  return found;
}

static bool RequestExistingManageWindowClose() {
  if (g_manageCloseMessage == 0) {
    return false;
  }

  HWND existing = FindManageWindowForCurrentInstallation();
  if (!existing) {
    return true;
  }

  DWORD_PTR result = 0;
  SendMessageTimeoutW(existing, g_manageCloseMessage, 0, 0, SMTO_ABORTIFHUNG,
                      1500, &result);
  for (int i = 0; i < 80; ++i) {
    if (!FindManageWindowForCurrentInstallation()) {
      return true;
    }
    Sleep(100);
  }
  return !FindManageWindowForCurrentInstallation();
}

static bool RestartManageWindow() {
  const std::wstring currentExe = AppPaths::GetExePath().wstring();
  if (currentExe.empty())
    return false;

  const std::wstring commandLine = L"\"" + currentExe + L"\" --restart-manager";
  std::vector<wchar_t> mutableCommand(commandLine.begin(), commandLine.end());
  mutableCommand.push_back(L'\0');

  STARTUPINFOW si{};
  si.cb = sizeof(si);
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE,
                      0, nullptr, GetExeDir().c_str(), &si, &pi)) {
    ShowManageMessageBox(L"Failed to restart Manage Novadesk.",
                         L"Manage Novadesk", MB_OK | MB_ICONWARNING);
    return false;
  }

  CloseHandle(pi.hProcess);
  CloseHandle(pi.hThread);
  return true;
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE, _In_ LPSTR,
                     _In_ int nCmdShow) {
  INITCOMMONCONTROLSEX icc{};
  icc.dwSize = sizeof(icc);
  icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES | ICC_LINK_CLASS;
  InitCommonControlsEx(&icc);

  WindowsTheme::Initialize();
  Localization::Initialize();
  // Match Rainmeter's native-dialog RTL model before any manager window or
  // common control is created.
  SetProcessDefaultLayout(Localization::IsRtl() ? LAYOUT_RTL : 0);

  const std::wstring installationScope = CommonSettings::GetInstallationScopeId();
  g_manageMutexName = L"Global\\NovadeskManageWindowSingleton." + installationScope;
  g_manageWindowScopeProperty = L"Novadesk.Manage.InstallationScope." + installationScope;
  const std::wstring closeMessageName = std::wstring(kManageCloseMessageName) + L"." + installationScope;
  const std::wstring languageMessageName = L"Novadesk.LanguageChanged." + installationScope;
  const std::wstring themeMessageName = L"Novadesk.ThemeChanged." + installationScope;
  g_manageCloseMessage = RegisterWindowMessageW(closeMessageName.c_str());
  g_languageChangedMessage = RegisterWindowMessageW(languageMessageName.c_str());
  g_themeChangedMessage = RegisterWindowMessageW(themeMessageName.c_str());
  if (HasCommandLineFlag(L"--request-close")) {
    return RequestExistingManageWindowClose() ? 0 : 1;
  }
  if (HasCommandLineFlag(L"--restart-manager")) {
    if (!RequestExistingManageWindowClose())
      return 1;

    for (int i = 0; i < 80; ++i) {
      HANDLE existingMutex = OpenMutexW(
          SYNCHRONIZE, FALSE, g_manageMutexName.c_str());
      if (!existingMutex)
        break;
      CloseHandle(existingMutex);
      Sleep(50);
    }

    const std::wstring currentExe = AppPaths::GetExePath().wstring();
    if (currentExe.empty())
      return 1;
    std::wstring commandLine = L"\"" + currentExe + L"\"";
    std::vector<wchar_t> mutableCommand(commandLine.begin(),
                                        commandLine.end());
    mutableCommand.push_back(L'\0');
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    const BOOL started = CreateProcessW(nullptr, mutableCommand.data(),
                                        nullptr, nullptr, FALSE, 0, nullptr,
                                        GetExeDir().c_str(), &si, &pi);
    if (started) {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }
    return started ? 0 : 1;
  }

  HANDLE instanceMutex =
      CreateMutexW(nullptr, FALSE, g_manageMutexName.c_str());
  if (instanceMutex && GetLastError() == ERROR_ALREADY_EXISTS) {
    HWND existing = FindManageWindowForCurrentInstallation();
    if (existing) {
      ShowWindow(existing, SW_SHOW);
      SetForegroundWindow(existing);
    }
    CloseHandle(instanceMutex);
    return 0;
  }

  LoadWindowIcons(hInstance);

  DialogManage::Open(0);

  HWND hWnd = DialogManage::Get() ? DialogManage::Get()->GetWindow() : nullptr;
  if (!hWnd) {
    DestroyWindowIcons();
    if (instanceMutex) {
      CloseHandle(instanceMutex);
    }
    return 0;
  }

  MSG msg{};
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    if (!Dialog::HandleMessage(msg)) {
      TranslateMessage(&msg);
      DispatchMessageW(&msg);
    }
  }

  if (instanceMutex) {
    CloseHandle(instanceMutex);
  }
  DestroyWindowIcons();
  return (int)msg.wParam;
}
