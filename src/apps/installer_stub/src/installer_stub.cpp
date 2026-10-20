/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

// StdAfx.h pulls in windows.h, commctrl.h, uxtheme.h, dwmapi.h, shlobj.h etc.
#include "StdAfx.h"
#include "WindowsTheme.h"
#include "DpiUtil.h"
#include "FontUtil.h"
#include "AppPaths.h"
#include "NovadeskPaths.h"
#include <winreg.h>
#include <shobjidl.h>
#include <objbase.h>
#include <wrl/client.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <tlhelp32.h>
#include "../../../third_party/json/json.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;
using Microsoft::WRL::ComPtr;

// ============================================================================
// Data Structures & Constants
// ============================================================================

static const std::string INSTALLER_MAGIC = "NWSFX1";

#pragma pack(push, 1)
struct InstallerFooter {
  char magic[8];
  uint64_t payloadSize;
  uint64_t manifestSize;
};
#pragma pack(pop)

static_assert(sizeof(InstallerFooter) == 24, "InstallerFooter size mismatch");

// ============================================================================
// Utility Functions (String Conversion, Environment Expansion)
// ============================================================================

std::wstring ToWString(const std::string &str) {
  if (str.empty())
    return L"";
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

std::wstring ExpandEnv(const std::wstring &value) {
  if (value.empty())
    return L"";
  DWORD needed = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
  if (needed == 0)
    return value;
  std::wstring buffer(needed, L'\0');
  DWORD written =
      ExpandEnvironmentStringsW(value.c_str(), buffer.data(), needed);
  if (written == 0)
    return value;
  buffer.resize(written - 1);
  return buffer;
}

bool ReadInstallerFooter(const fs::path &exePath, InstallerFooter &footer,
                         uint64_t &footerOffset) {
  // Read the installer footer (magic + sizes) from the end of the executable.
  // The footer indicates where embedded payload and manifest data reside.
  std::ifstream in(exePath, std::ios::binary | std::ios::ate);
  if (!in)
    return false;
  std::streamoff size = in.tellg();
  if (size < (std::streamoff)sizeof(InstallerFooter))
    return false;
  footerOffset = static_cast<uint64_t>(size) - sizeof(InstallerFooter);
  in.seekg(static_cast<std::streamoff>(footerOffset));
  in.read(reinterpret_cast<char *>(&footer), sizeof(footer));
  if (!in)
    return false;
  return std::memcmp(footer.magic, INSTALLER_MAGIC.c_str(),
                     INSTALLER_MAGIC.size()) == 0;
}

// ============================================================================
// Admin & Privilege Management
// ============================================================================

bool IsRunningAsAdmin() {
  BOOL isAdmin = FALSE;
  PSID adminGroup = nullptr;
  SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
  if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &adminGroup)) {
    CheckTokenMembership(nullptr, adminGroup, &isAdmin);
    FreeSid(adminGroup);
  }
  return isAdmin == TRUE;
}

bool RelaunchAsAdmin(const std::wstring &arguments) {
  const fs::path exePath = AppPaths::GetExePath();
  if (exePath.empty())
    return false;
  SHELLEXECUTEINFOW sei = {};
  sei.cbSize = sizeof(sei);
  sei.fMask = SEE_MASK_NOCLOSEPROCESS;
  sei.lpVerb = L"runas";
  sei.lpFile = exePath.c_str();
  sei.lpParameters = arguments.c_str();
  sei.nShow = SW_SHOWNORMAL;
  if (!ShellExecuteExW(&sei)) {
    return false;
  }
  if (sei.hProcess) {
    CloseHandle(sei.hProcess);
  }
  return true;
}

// ============================================================================
// Shortcut & Registry Operations
// ============================================================================

bool CreateShortcut(const std::wstring &targetPath,
                    const std::wstring &shortcutPath,
                    const std::wstring &workingDir,
                    const std::wstring &description) {
  ComPtr<IShellLinkW> shellLink;
  HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                IID_PPV_ARGS(&shellLink));
  if (FAILED(hr))
    return false;

  shellLink->SetPath(targetPath.c_str());
  shellLink->SetWorkingDirectory(workingDir.c_str());
  shellLink->SetDescription(description.c_str());
  shellLink->SetIconLocation(targetPath.c_str(), 0);

  ComPtr<IPersistFile> persistFile;
  hr = shellLink->QueryInterface(IID_PPV_ARGS(&persistFile));
  if (FAILED(hr))
    return false;

  return SUCCEEDED(persistFile->Save(shortcutPath.c_str(), TRUE));
}

bool ExtractEmbeddedStub(const fs::path &outPath) {
  HRSRC hRes = FindResourceW(nullptr, L"INSTALLER_STUB", RT_RCDATA);
  if (!hRes)
    return false;
  HGLOBAL hData = LoadResource(nullptr, hRes);
  if (!hData)
    return false;
  DWORD size = SizeofResource(nullptr, hRes);
  void *data = LockResource(hData);
  if (!data || size == 0)
    return false;

  std::ofstream out(outPath, std::ios::binary);
  if (!out)
    return false;
  out.write(reinterpret_cast<const char *>(data),
            static_cast<std::streamsize>(size));
  return true;
}

// ============================================================================
// Process & Installation Checks
// ============================================================================

bool IsProcessRunning(const std::wstring &exeName) {
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE)
    return false;
  PROCESSENTRY32W entry = {};
  entry.dwSize = sizeof(entry);
  if (Process32FirstW(snapshot, &entry)) {
    do {
      if (_wcsicmp(entry.szExeFile, exeName.c_str()) == 0) {
        CloseHandle(snapshot);
        return true;
      }
    } while (Process32NextW(snapshot, &entry));
  }
  CloseHandle(snapshot);
  return false;
}

bool IsAppInstalled(const std::wstring &appName) {
  std::wstring uninstallKey =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appName;
  HKEY hKey = nullptr;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, uninstallKey.c_str(), 0, KEY_READ,
                    &hKey) == ERROR_SUCCESS) {
    RegCloseKey(hKey);
    return true;
  }
  return false;
}

namespace {
// ============================================================================
// UI Globals & State Management
// ============================================================================

HWND g_hwnd = nullptr;
HWND g_title = nullptr;
HWND g_status = nullptr;
HWND g_progress = nullptr;
HWND g_repairButton = nullptr;
HWND g_uninstallButton = nullptr;
HWND g_closeButton = nullptr;
HFONT g_titleFont = nullptr;
HFONT g_textFont = nullptr;
std::atomic<uint64_t> g_done{0};
std::atomic<uint64_t> g_total{1};
std::atomic<bool> g_finished{false};
std::atomic<bool> g_uninstall{false};
std::atomic<bool> g_forceRepair{false};
std::atomic<bool> g_showActions{false};
std::atomic<bool> g_allowClose{false};
std::atomic<int> g_action{0}; // 0 none, 1 repair, 2 uninstall
std::mutex g_statusMutex;
std::mutex g_actionMutex;
std::condition_variable g_actionCv;
std::wstring g_statusText = L"Preparing...";
std::wstring g_appName = L"";
std::wstring g_appNameUi = L"Installer";
std::wstring g_launchExe;
std::wstring g_launchDir;
bool g_launchAfterInstall = false;

// ============================================================================
// UI Helper Functions
// ============================================================================

void UpdateStatus(const std::wstring &text) {
  std::lock_guard<std::mutex> lock(g_statusMutex);
  g_statusText = text;
}

void SetCloseEnabled(HWND hwnd, bool enabled) {
  HMENU hMenu = GetSystemMenu(hwnd, FALSE);
  if (!hMenu)
    return;
  EnableMenuItem(hMenu, SC_CLOSE,
                 MF_BYCOMMAND | (enabled ? MF_ENABLED : MF_GRAYED));
  DrawMenuBar(hwnd);
}

void RefreshUi() {
  if (!g_hwnd)
    return;
  uint64_t total = g_total.load();
  uint64_t done = g_done.load();
  if (total == 0)
    total = 1;
  int percent = static_cast<int>((done * 100) / total);
  SendMessageW(g_progress, PBM_SETPOS, percent, 0);
  std::lock_guard<std::mutex> lock(g_statusMutex);
  SetWindowTextW(g_status, g_statusText.c_str());
}
} // namespace

// ============================================================================
// Installation & Uninstallation Core
// ============================================================================

std::wstring LoadAppNameFromManifest() {
  const fs::path exePath = AppPaths::GetExePath();

  InstallerFooter footer{};
  uint64_t footerOffset = 0;
  if (!ReadInstallerFooter(exePath, footer, footerOffset)) {
    return L"";
  }

  uint64_t exeSize = footerOffset + sizeof(InstallerFooter);
  uint64_t payloadSize = footer.payloadSize;
  uint64_t manifestSize = footer.manifestSize;
  if (manifestSize == 0 || payloadSize == 0) {
    return L"";
  }

  uint64_t payloadStart =
      exeSize - sizeof(InstallerFooter) - manifestSize - payloadSize;
  uint64_t manifestOffset = payloadStart + payloadSize;

  std::ifstream in(exePath, std::ios::binary);
  if (!in)
    return L"";
  in.seekg(static_cast<std::streamoff>(manifestOffset));
  std::string manifestJson(manifestSize, '\0');
  in.read(manifestJson.data(), static_cast<std::streamsize>(manifestSize));
  if (!in)
    return L"";

  nlohmann::json manifest = nlohmann::json::parse(manifestJson, nullptr, false);
  if (manifest.is_discarded())
    return L"";
  std::string appName = manifest.value("appName", "");
  return ToWString(appName);
}

bool InstallFromSelf(bool skipInstalledCheck) {
  // Extract embedded payload and manifest from self, then perform installation.
  // NOTE: This function requires admin privileges to write to system
  // directories.
  const fs::path exePath = AppPaths::GetExePath();

  InstallerFooter footer{};
  uint64_t footerOffset = 0;
  if (!ReadInstallerFooter(exePath, footer, footerOffset)) {
    return false;
  }

  if (!IsRunningAsAdmin()) {
    if (RelaunchAsAdmin(L"--install-ui")) {
      return true;
    }
    return true;
  }

  uint64_t exeSize = footerOffset + sizeof(InstallerFooter);
  uint64_t payloadSize = footer.payloadSize;
  uint64_t manifestSize = footer.manifestSize;
  if (manifestSize == 0 || payloadSize == 0) {
    return true;
  }

  uint64_t payloadStart =
      exeSize - sizeof(InstallerFooter) - manifestSize - payloadSize;
  uint64_t manifestOffset = payloadStart + payloadSize;

  std::ifstream in(exePath, std::ios::binary);
  if (!in) {
    return true;
  }

  in.seekg(static_cast<std::streamoff>(manifestOffset));
  std::string manifestJson(manifestSize, '\0');
  in.read(manifestJson.data(), static_cast<std::streamsize>(manifestSize));
  if (!in) {
    return true;
  }

  nlohmann::json manifest = nlohmann::json::parse(manifestJson, nullptr, false);
  if (manifest.is_discarded()) {
    return true;
  }

  std::string appName = manifest.value("appName", "");
  std::string version = manifest.value("version", "");
  std::string author = manifest.value("author", "");
  std::string description = manifest.value("description", "");
  std::string appExeRel = manifest.value("appExeRel", "");
  auto setup = manifest.value("setup", nlohmann::json::object());
  std::string installDirStr = setup.value("installDir", "");
  std::string startMenuFolder = setup.value("startMenuFolder", "");
  std::string setupName = setup.value("setupName", "");
  std::string setupIcon = setup.value("setupIcon", "");
  bool launchAfterInstall = setup.value("launchAfterInstall", false);
  if (appName.empty() || version.empty() || author.empty() ||
      description.empty() || appExeRel.empty() || installDirStr.empty() ||
      startMenuFolder.empty() || setupName.empty() || setupIcon.empty()) {
    UpdateStatus(L"Missing required metadata.");
    return false;
  }

  bool createDesktopShortcut = setup.value("createDesktopShortcut", true);
  bool createStartupShortcut = setup.value("createStartupShortcut", false);
  bool runOnStartup = setup.value("runOnStartup", false);
  bool enableUninstall = setup.value("enableUninstall", true);

  std::wstring appNameW = ToWString(appName);
  if (!skipInstalledCheck && IsAppInstalled(appNameW)) {
    UpdateStatus(L"Already installed.");
    return false;
  }

  std::wstring installDirW = ExpandEnv(ToWString(installDirStr));

  fs::path installDirPath = fs::path(installDirW);
  fs::create_directories(installDirPath);

  // Persist manifest for uninstall
  try {
    std::ofstream mf(installDirPath / "install_manifest.json",
                     std::ios::binary);
    if (mf) {
      mf.write(manifestJson.data(),
               static_cast<std::streamsize>(manifestJson.size()));
    }
  } catch (...) {
  }

  auto files = manifest.value("files", nlohmann::json::array());
  uint64_t totalBytes = 0;
  for (const auto &entry : files) {
    totalBytes += entry.value("size", 0ull);
  }

  UpdateStatus(L"Installing files...");

  uint64_t doneBytes = 0;
  for (const auto &entry : files) {
    std::string relPath = entry.value("path", "");
    uint64_t offset = entry.value("offset", 0ull);
    uint64_t size = entry.value("size", 0ull);
    if (relPath.empty() || size == 0)
      continue;

    fs::path outPath = installDirPath / fs::path(relPath);
    fs::create_directories(outPath.parent_path());

    std::ofstream out(outPath, std::ios::binary);
    if (!out)
      continue;

    in.seekg(static_cast<std::streamoff>(payloadStart + offset));
    uint64_t remaining = size;
    std::vector<char> buffer(1024 * 1024);
    while (remaining > 0) {
      uint64_t toRead = std::min<uint64_t>(remaining, buffer.size());
      in.read(buffer.data(), static_cast<std::streamsize>(toRead));
      std::streamsize got = in.gcount();
      if (got <= 0)
        break;
      out.write(buffer.data(), got);
      remaining -= static_cast<uint64_t>(got);
      doneBytes += static_cast<uint64_t>(got);
      g_done.store(doneBytes);
      g_total.store(totalBytes == 0 ? 1 : totalBytes);
    }
  }

  fs::path appExePath = installDirPath / fs::path(appExeRel);
  std::wstring appExeW = appExePath.wstring();
  std::wstring workingDirW = installDirPath.wstring();
  HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (SUCCEEDED(hr)) {
    UpdateStatus(L"Creating shortcuts...");
    if (!startMenuFolder.empty()) {
      const fs::path programsDir = NovadeskPaths::GetProgramsDir();
      if (!programsDir.empty()) {
        fs::path startMenuDir = programsDir / fs::path(startMenuFolder);
        fs::create_directories(startMenuDir);
        fs::path shortcutPath = startMenuDir / (appName + ".lnk");
        CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
      }
    }

    if (createDesktopShortcut) {
      const fs::path desktopDir = NovadeskPaths::GetDesktopDir();
      if (!desktopDir.empty()) {
        fs::path shortcutPath = desktopDir / (appName + ".lnk");
        CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
      }
    }

    if (createStartupShortcut || runOnStartup) {
      const fs::path startupDir = NovadeskPaths::GetStartupDir();
      if (!startupDir.empty()) {
        fs::path shortcutPath = startupDir / (appName + ".lnk");
        CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
      }
    }

    if (enableUninstall) {
      UpdateStatus(L"Registering uninstall...");
      fs::path uninstallPath = installDirPath / "Uninstall.exe";
      try {
        if (!ExtractEmbeddedStub(uninstallPath)) {
          fs::copy_file(exePath, uninstallPath,
                        fs::copy_options::overwrite_existing);
        }
      } catch (...) {
      }

      HKEY hKey = nullptr;
      std::wstring uninstallKey =
          L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" +
          appNameW;
      if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, uninstallKey.c_str(), 0, nullptr,
                          0, KEY_WRITE, nullptr, &hKey,
                          nullptr) == ERROR_SUCCESS) {
        std::wstring displayName = appNameW;
        std::wstring publisher = ToWString(author);
        std::wstring displayVersion = ToWString(version);
        std::wstring displayDesc = ToWString(description);
        std::wstring uninstallCmd =
            L"\"" + uninstallPath.wstring() + L"\" --uninstall";
        std::wstring displayIcon = uninstallPath.wstring();
        RegSetValueExW(
            hKey, L"DisplayName", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(displayName.c_str()),
            static_cast<DWORD>((displayName.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(
            hKey, L"Publisher", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(publisher.c_str()),
            static_cast<DWORD>((publisher.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(
            hKey, L"DisplayVersion", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(displayVersion.c_str()),
            static_cast<DWORD>((displayVersion.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(
            hKey, L"Description", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(displayDesc.c_str()),
            static_cast<DWORD>((displayDesc.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(
            hKey, L"UninstallString", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(uninstallCmd.c_str()),
            static_cast<DWORD>((uninstallCmd.size() + 1) * sizeof(wchar_t)));
        RegSetValueExW(
            hKey, L"InstallLocation", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(installDirPath.wstring().c_str()),
            static_cast<DWORD>((installDirPath.wstring().size() + 1) *
                               sizeof(wchar_t)));
        RegSetValueExW(
            hKey, L"DisplayIcon", 0, REG_SZ,
            reinterpret_cast<const BYTE *>(displayIcon.c_str()),
            static_cast<DWORD>((displayIcon.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
      }
    }

    CoUninitialize();
  }

  g_launchAfterInstall = launchAfterInstall;
  g_launchExe = appExeW;
  g_launchDir = workingDirW;

  UpdateStatus(L"Done");
  return true;
}

bool UninstallSelf() {
  if (!IsRunningAsAdmin()) {
    if (RelaunchAsAdmin(L"--uninstall")) {
      return true;
    }
    return true;
  }

  const fs::path exePath = AppPaths::GetExePath();

  fs::path installDirPath = exePath.parent_path();

  // Try to resolve install location (and uninstall string) from registry using
  // app name if available
  std::wstring appNameHint = g_appName;
  std::wstring uninstallCmdFromReg;
  if (!appNameHint.empty()) {
    HKEY hInstallKey = nullptr;
    std::wstring installKeyPath =
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" +
        appNameHint;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, installKeyPath.c_str(), 0, KEY_READ,
                      &hInstallKey) == ERROR_SUCCESS) {
      wchar_t installPathBuf[MAX_PATH];
      DWORD size = sizeof(installPathBuf);
      if (RegQueryValueExW(hInstallKey, L"InstallLocation", nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(installPathBuf),
                           &size) == ERROR_SUCCESS) {
        if (installPathBuf[0] != L'\0') {
          installDirPath = fs::path(installPathBuf);
        }
      }
      wchar_t uninstallBuf[2048];
      DWORD uninstallSize = sizeof(uninstallBuf);
      if (RegQueryValueExW(hInstallKey, L"UninstallString", nullptr, nullptr,
                           reinterpret_cast<LPBYTE>(uninstallBuf),
                           &uninstallSize) == ERROR_SUCCESS) {
        if (uninstallBuf[0] != L'\0') {
          uninstallCmdFromReg = uninstallBuf;
        }
      }
      RegCloseKey(hInstallKey);
    }
  }

  std::string manifestJson;
  try {
    std::ifstream mf(installDirPath / "install_manifest.json",
                     std::ios::binary);
    if (mf) {
      manifestJson.assign(std::istreambuf_iterator<char>(mf),
                          std::istreambuf_iterator<char>());
    }
  } catch (...) {
  }
  if (manifestJson.empty()) {
    if (!uninstallCmdFromReg.empty()) {
      UpdateStatus(L"Launching uninstaller...");
      std::wstring cmd = L"/C \"\"";
      cmd += uninstallCmdFromReg;
      cmd += L"\"\"";
      SHELLEXECUTEINFOW sei = {};
      sei.cbSize = sizeof(sei);
      sei.fMask = SEE_MASK_NO_CONSOLE;
      sei.lpVerb = L"open";
      sei.lpFile = L"cmd.exe";
      sei.lpParameters = cmd.c_str();
      sei.nShow = SW_HIDE;
      ShellExecuteExW(&sei);
      return true;
    }
    return true;
  }

  nlohmann::json manifest = nlohmann::json::parse(manifestJson, nullptr, false);
  if (manifest.is_discarded()) {
    return true;
  }

  std::string appName = manifest.value("appName", "");
  std::string version = manifest.value("version", "");
  std::string appExeRel = manifest.value("appExeRel", "");
  auto setup = manifest.value("setup", nlohmann::json::object());
  std::string installDir = setup.value("installDir", "");
  std::string startMenuFolder = setup.value("startMenuFolder", "");
  std::string setupName = setup.value("setupName", "");
  std::string setupIcon = setup.value("setupIcon", "");
  if (appName.empty() || version.empty() || appExeRel.empty() ||
      installDir.empty() || startMenuFolder.empty() || setupName.empty() ||
      setupIcon.empty()) {
    UpdateStatus(L"Missing required metadata.");
    return false;
  }
  bool runOnStartup = setup.value("runOnStartup", false);
  bool createStartupShortcut = setup.value("createStartupShortcut", false);

  // If registry has a canonical install location, prefer it
  HKEY hInstallKey = nullptr;
  std::wstring installKeyPath =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" +
      ToWString(appName);
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, installKeyPath.c_str(), 0, KEY_READ,
                    &hInstallKey) == ERROR_SUCCESS) {
    wchar_t installPathBuf[MAX_PATH];
    DWORD size = sizeof(installPathBuf);
    if (RegQueryValueExW(hInstallKey, L"InstallLocation", nullptr, nullptr,
                         reinterpret_cast<LPBYTE>(installPathBuf),
                         &size) == ERROR_SUCCESS) {
      if (installPathBuf[0] != L'\0') {
        installDirPath = fs::path(installPathBuf);
      }
    }
    RegCloseKey(hInstallKey);
  }

  std::wstring appExeNameW = fs::path(appExeRel).filename().wstring();
  if (IsProcessRunning(appExeNameW)) {
    UpdateStatus(L"Close the app before uninstalling.");
    return false;
  }

  // installDirPath already resolved above

  if (installDirPath.empty()) {
    return true;
  }

  UpdateStatus(L"Removing shortcuts...");
  std::wstring appNameW = ToWString(appName);

  const fs::path programsDir = NovadeskPaths::GetProgramsDir();
  if (!programsDir.empty()) {
    fs::path startMenuDir = programsDir / ToWString(startMenuFolder);
    fs::remove_all(startMenuDir);
  }

  const fs::path desktopDir = NovadeskPaths::GetDesktopDir();
  if (!desktopDir.empty()) {
    fs::remove(desktopDir / (appNameW + L".lnk"));
  }

  if (createStartupShortcut || runOnStartup) {
    const fs::path startupDir = NovadeskPaths::GetStartupDir();
    if (!startupDir.empty()) {
      fs::remove(startupDir / (appNameW + L".lnk"));
    }
  }

  UpdateStatus(L"Removing files...");
  bool removalQueued = false;
  try {
    std::error_code ec;
    // Remove every child entry so dynamically created folders are cleaned too.
    if (fs::exists(installDirPath, ec) &&
        fs::is_directory(installDirPath, ec)) {
      for (const auto &entry : fs::directory_iterator(
               installDirPath, fs::directory_options::skip_permission_denied,
               ec)) {
        if (ec)
          break;
        const fs::path &child = entry.path();
        if (_wcsicmp(child.c_str(), exePath.c_str()) == 0) {
          continue;
        }
        fs::remove_all(child, ec);
        ec.clear();
      }
    }
    ec.clear();
    fs::remove_all(installDirPath, ec);
    if (ec || fs::exists(installDirPath)) {
      removalQueued = true;
    }
  } catch (...) {
    removalQueued = true;
  }

  if (removalQueued && !installDirPath.empty()) {
    UpdateStatus(L"Finishing removal...");
    std::wstring deleteCmd =
        L"/C rmdir /s /q \"" + installDirPath.wstring() + L"\"";
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NO_CONSOLE;
    sei.lpVerb = L"open";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = deleteCmd.c_str();
    sei.nShow = SW_HIDE;
    ShellExecuteExW(&sei);
  }

  // Remove Roaming AppData folder if present
  wchar_t appDataPath[MAX_PATH];
  if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr,
                                 SHGFP_TYPE_CURRENT, appDataPath))) {
    try {
      fs::remove_all(fs::path(appDataPath) / appNameW);
    } catch (...) {
    }
  }

  // Self-delete only if we are running from Uninstall.exe (not setup.exe)
  if (_wcsicmp(exePath.filename().c_str(), L"Uninstall.exe") == 0) {
    std::wstring deleteCmd = L"/C ping 127.0.0.1 -n 3 > nul & del /f /q \"" +
                             exePath.wstring() + L"\" & rmdir /s /q \"" +
                             installDirPath.wstring() + L"\"";
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NO_CONSOLE;
    sei.lpVerb = L"open";
    sei.lpFile = L"cmd.exe";
    sei.lpParameters = deleteCmd.c_str();
    sei.nShow = SW_HIDE;
    ShellExecuteExW(&sei);
  }

  HKEY hUninstall = nullptr;
  std::wstring uninstallKey =
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appNameW;
  if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                    L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall",
                    0, KEY_WRITE, &hUninstall) == ERROR_SUCCESS) {
    RegDeleteTreeW(hUninstall, appNameW.c_str());
    RegCloseKey(hUninstall);
  }

  UpdateStatus(L"Done");
  return true;
}
// ============================================================================
// Window Message Handler & Main Entry Point
// ============================================================================

// Apply dark/light theme to the window: DWM titlebar + per-control theming.
// ApplyToControlTree does NOT install the WindowsTheme parent subclass, so
// WndProc keeps ownership of WM_CTLCOLORSTATIC and WM_ERASEBKGND.
static void ApplyInstallerTheme(HWND hwnd) {
  WindowsTheme::ApplyToControlTree(hwnd, g_progress);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_ERASEBKGND: {
    if (WindowsTheme::IsUsingDarkMode()) {
      RECT rc{}; GetClientRect(hwnd, &rc);
      FillRect(reinterpret_cast<HDC>(wParam), &rc,
               WindowsTheme::GetDarkBackgroundBrush());
      return 1;
    }
    break;
  }
  case WM_CTLCOLORSTATIC: {
    HDC hdc = reinterpret_cast<HDC>(wParam);
    SetBkMode(hdc, TRANSPARENT);
    if (WindowsTheme::IsUsingDarkMode()) {
      SetTextColor(hdc, WindowsTheme::GetTextColor());
      SetBkColor(hdc, WindowsTheme::GetDarkBackgroundColor());
      return reinterpret_cast<LRESULT>(WindowsTheme::GetDarkBackgroundBrush());
    }
    SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkColor(hdc, GetSysColor(COLOR_WINDOW));
    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
  }
  case WM_SETTINGCHANGE:
    WindowsTheme::Refresh();
    ApplyInstallerTheme(hwnd);
    RedrawWindow(hwnd, nullptr, nullptr,
                 RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    return 0;
  case WM_DPICHANGED: {
    const UINT dpi = LOWORD(wParam);
    const RECT *r  = reinterpret_cast<const RECT *>(lParam);
    SetWindowPos(hwnd, nullptr, r->left, r->top,
                 r->right - r->left, r->bottom - r->top,
                 SWP_NOZORDER | SWP_NOACTIVATE);
    // Recreate fonts at the new DPI using the system UI font.
    if (g_textFont)  { DeleteObject(g_textFont);  g_textFont  = nullptr; }
    if (g_titleFont) { DeleteObject(g_titleFont); g_titleFont = nullptr; }
    g_textFont  = FontUtil::CreateSystemFont(dpi);
    g_titleFont = FontUtil::CreateSystemFont(dpi, FW_SEMIBOLD, 140);
    // Reposition and resize all controls.
    auto S = [dpi](int v){ return DpiUtil::Scale(v, dpi); };
    if (g_title)         { SendMessageW(g_title,            WM_SETFONT, (WPARAM)g_titleFont, TRUE); SetWindowPos(g_title,            nullptr, S(24),  S(20),  S(460), S(28), SWP_NOZORDER); }
    if (g_status)        { SendMessageW(g_status,           WM_SETFONT, (WPARAM)g_textFont,  TRUE); SetWindowPos(g_status,           nullptr, S(24),  S(60),  S(460), S(20), SWP_NOZORDER); }
    if (g_progress)      {                                                                           SetWindowPos(g_progress,         nullptr, S(24),  S(92),  S(460), S(22), SWP_NOZORDER); }
    if (g_repairButton)  { SendMessageW(g_repairButton,     WM_SETFONT, (WPARAM)g_textFont,  TRUE); SetWindowPos(g_repairButton,     nullptr, S(24),  S(130), S(100), S(28), SWP_NOZORDER); }
    if (g_uninstallButton){SendMessageW(g_uninstallButton,  WM_SETFONT, (WPARAM)g_textFont,  TRUE); SetWindowPos(g_uninstallButton,  nullptr, S(134), S(130), S(100), S(28), SWP_NOZORDER); }
    if (g_closeButton)   { SendMessageW(g_closeButton,      WM_SETFONT, (WPARAM)g_textFont,  TRUE); SetWindowPos(g_closeButton,      nullptr, S(384), S(130), S(100), S(28), SWP_NOZORDER); }
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
    ApplyInstallerTheme(hwnd);
    return 0;
  }
  case WM_TIMER:
    RefreshUi();
    if (g_showActions.load()) {
      ShowWindow(g_repairButton, SW_SHOW);
      ShowWindow(g_uninstallButton, SW_SHOW);
      EnableWindow(g_repairButton, TRUE);
      EnableWindow(g_uninstallButton, TRUE);
      if (!g_finished.load()) {
        g_allowClose.store(true);
        SetCloseEnabled(hwnd, true);
      }
    }
    if (g_finished.load()) {
      g_allowClose.store(true);
      SetCloseEnabled(hwnd, true);
      ShowWindow(g_closeButton, SW_SHOW);
      EnableWindow(g_repairButton, FALSE);
      EnableWindow(g_uninstallButton, FALSE);
    }
    return 0;
  case WM_COMMAND:
    if (LOWORD(wParam) == 1001) {
      g_action.store(1);
      g_showActions.store(false);
      g_allowClose.store(false);
      SetCloseEnabled(hwnd, false);
      EnableWindow(g_repairButton, FALSE);
      EnableWindow(g_uninstallButton, FALSE);
      g_actionCv.notify_one();
      return 0;
    }
    if (LOWORD(wParam) == 1002) {
      g_action.store(2);
      g_showActions.store(false);
      g_allowClose.store(false);
      SetCloseEnabled(hwnd, false);
      EnableWindow(g_repairButton, FALSE);
      EnableWindow(g_uninstallButton, FALSE);
      g_actionCv.notify_one();
      return 0;
    }
    if (LOWORD(wParam) == 1003) {
      if (g_finished.load() && !g_uninstall.load() && g_launchAfterInstall &&
          !g_launchExe.empty()) {
        ShellExecuteW(nullptr, L"open", g_launchExe.c_str(), nullptr,
                      g_launchDir.c_str(), SW_SHOWNORMAL);
      }
      DestroyWindow(hwnd);
      return 0;
    }
    break;
  case WM_CLOSE:
    if (g_allowClose.load()) {
      DestroyWindow(hwnd);
      return 0;
    }
    // Ignore close while installing
    return 0;
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;
  }
  return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int) {
  int argc = 0;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  for (int i = 1; i < argc; ++i) {
    if (wcscmp(argv[i], L"--uninstall") == 0) {
      g_uninstall.store(true);
      break;
    }
    if (wcscmp(argv[i], L"--repair") == 0) {
      g_forceRepair.store(true);
      break;
    }
  }
  if (argv)
    LocalFree(argv);

  if (!IsRunningAsAdmin()) {
    if (g_uninstall.load()) {
      RelaunchAsAdmin(L"--uninstall");
    } else if (g_forceRepair.load()) {
      RelaunchAsAdmin(L"--repair");
    } else {
      RelaunchAsAdmin(L"");
    }
    return 0;
  }

  g_appName = LoadAppNameFromManifest();
  g_appNameUi = g_appName.empty() ? L"Installer" : g_appName;

  INITCOMMONCONTROLSEX icc = {sizeof(icc), ICC_PROGRESS_CLASS};
  InitCommonControlsEx(&icc);

  WindowsTheme::Initialize();

  const wchar_t *className = L"NovadeskInstallerWindow";
  WNDCLASSEXW wc = {};
  wc.cbSize = sizeof(wc);
  wc.lpfnWndProc = WndProc;
  wc.hInstance = hInstance;
  wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
  wc.lpszClassName = className;
  wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
  wc.hIcon = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON, 32,
                               32, LR_DEFAULTCOLOR);
  wc.hIconSm = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101), IMAGE_ICON,
                                 16, 16, LR_DEFAULTCOLOR);
  RegisterClassExW(&wc);

  // Scale the window size to the primary monitor DPI.
  const UINT primaryDpi = GetDpiForSystem();
  const int width  = MulDiv(520, (int)primaryDpi, 96);
  const int height = MulDiv(220, (int)primaryDpi, 96);
  RECT workArea{}; SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
  const int x = workArea.left + ((workArea.right  - workArea.left) - width)  / 2;
  const int y = workArea.top  + ((workArea.bottom - workArea.top)  - height) / 2;

  std::wstring windowTitle =
      g_appNameUi + (g_uninstall.load() ? L" Uninstaller" : L" Installer");
  g_hwnd = CreateWindowExW(0, className, windowTitle.c_str(),
                           WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, x, y, width,
                           height, nullptr, nullptr, hInstance, nullptr);

  if (!g_hwnd) {
    return InstallFromSelf(false) ? 0 : 1;
  }

  HICON hIconBig = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101),
                                     IMAGE_ICON, 32, 32, LR_DEFAULTCOLOR);
  HICON hIconSmall = (HICON)LoadImageW(hInstance, MAKEINTRESOURCEW(101),
                                       IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
  if (hIconBig) {
    SendMessageW(g_hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIconBig);
  }
  if (hIconSmall) {
    SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconSmall);
  } else if (hIconBig) {
    SendMessageW(g_hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIconBig);
  }

  std::wstring headerText = g_uninstall.load()
                                ? (L"Uninstalling " + g_appNameUi)
                                : (L"Installing " + g_appNameUi);

  // Build fonts from the system UI font at the window's actual DPI.
  {
    const UINT dpi = GetDpiForWindow(g_hwnd);
    g_textFont  = FontUtil::CreateSystemFont(dpi);
    g_titleFont = FontUtil::CreateSystemFont(dpi, FW_SEMIBOLD, 140);
  }

  // Create all controls at DPI-scaled positions.
  const UINT cdpi = GetDpiForWindow(g_hwnd);
  auto S = [cdpi](int v){ return DpiUtil::Scale(v, cdpi); };

  g_title = CreateWindowW(L"STATIC", headerText.c_str(), WS_CHILD | WS_VISIBLE,
                          S(24), S(20), S(460), S(28), g_hwnd, nullptr, hInstance, nullptr);
  SendMessageW(g_title, WM_SETFONT, (WPARAM)g_titleFont, TRUE);

  g_status = CreateWindowW(L"STATIC", L"Preparing...", WS_CHILD | WS_VISIBLE,
                           S(24), S(60), S(460), S(20), g_hwnd, nullptr, hInstance, nullptr);
  SendMessageW(g_status, WM_SETFONT, (WPARAM)g_textFont, TRUE);

  g_progress = CreateWindowW(PROGRESS_CLASSW, nullptr, WS_CHILD | WS_VISIBLE,
                             S(24), S(92), S(460), S(22), g_hwnd, nullptr, hInstance, nullptr);
  SendMessageW(g_progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

  g_repairButton = CreateWindowW(L"BUTTON", L"Repair", WS_CHILD | WS_VISIBLE,
                                 S(24), S(130), S(100), S(28), g_hwnd, (HMENU)1001, hInstance, nullptr);
  g_uninstallButton = CreateWindowW(L"BUTTON", L"Uninstall", WS_CHILD | WS_VISIBLE,
                                    S(134), S(130), S(100), S(28), g_hwnd, (HMENU)1002, hInstance, nullptr);
  g_closeButton = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE,
                                S(384), S(130), S(100), S(28), g_hwnd, (HMENU)1003, hInstance, nullptr);
  SendMessageW(g_repairButton,   WM_SETFONT, (WPARAM)g_textFont, TRUE);
  SendMessageW(g_uninstallButton,WM_SETFONT, (WPARAM)g_textFont, TRUE);
  SendMessageW(g_closeButton,    WM_SETFONT, (WPARAM)g_textFont, TRUE);
  ShowWindow(g_repairButton,    SW_HIDE);
  ShowWindow(g_uninstallButton, SW_HIDE);
  ShowWindow(g_closeButton,     SW_HIDE);

  ShowWindow(g_hwnd, SW_SHOW);
  UpdateWindow(g_hwnd);
  // Apply dark/light theme — uses ApplyToControl per child so WndProc
  // retains ownership of WM_CTLCOLORSTATIC and WM_ERASEBKGND.
  ApplyInstallerTheme(g_hwnd);
  SetCloseEnabled(g_hwnd, false);

  std::thread worker([&]() {
    if (g_uninstall.load()) {
      UpdateStatus(L"Removing files...");
      g_done.store(1);
      g_total.store(1);
      UninstallSelf();
      g_done.store(1);
      g_total.store(1);
      g_finished.store(true);
      return;
    }

    if (g_forceRepair.load()) {
      UpdateStatus(L"Repairing...");
      InstallFromSelf(true);
      g_finished.store(true);
      return;
    }

    // If already installed, show actions and wait for user choice
    if (!g_appName.empty() && IsAppInstalled(g_appName)) {
      UpdateStatus(L"Already installed. Choose Repair or Uninstall.");
      g_showActions.store(true);
      std::unique_lock<std::mutex> lock(g_actionMutex);
      g_actionCv.wait(lock, [] { return g_action.load() != 0; });
      if (g_action.load() == 2) {
        g_uninstall.store(true);
        if (g_title) {
          std::wstring headerText = L"Uninstalling " + g_appNameUi;
          SetWindowTextW(g_title, headerText.c_str());
        }
        UpdateStatus(L"Removing files...");
        g_done.store(1);
        g_total.store(1);
        UninstallSelf();
        g_done.store(1);
        g_total.store(1);
        g_finished.store(true);
        return;
      } else {
        UpdateStatus(L"Repairing...");
        InstallFromSelf(true);
        g_finished.store(true);
        return;
      }
    }

    InstallFromSelf(false);
    g_finished.store(true);
  });
  worker.detach();

  SetTimer(g_hwnd, 1, 100, nullptr);

  MSG msg;
  while (GetMessageW(&msg, nullptr, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  if (g_titleFont)
    DeleteObject(g_titleFont);
  if (g_textFont)
    DeleteObject(g_textFont);
  return 0;
}
