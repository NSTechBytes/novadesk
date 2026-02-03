#include <windows.h>
#include <winreg.h>
#include <commctrl.h>
#include <shlobj.h>
#include <shobjidl.h>
#include <objbase.h>
#include <atlbase.h>
#include <atlcomcli.h>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <cstring>
#include "..\..\Library\json\json.hpp"

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "advapi32.lib")

namespace fs = std::filesystem;
using ATL::CComPtr;

static const std::string INSTALLER_MAGIC = "NWSFX1";

#pragma pack(push, 1)
struct InstallerFooter {
    char magic[8];
    uint64_t payloadSize;
    uint64_t manifestSize;
};
#pragma pack(pop)

static_assert(sizeof(InstallerFooter) == 24, "InstallerFooter size mismatch");

std::wstring ToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

std::wstring ExpandEnv(const std::wstring& value) {
    if (value.empty()) return L"";
    DWORD needed = ExpandEnvironmentStringsW(value.c_str(), nullptr, 0);
    if (needed == 0) return value;
    std::wstring buffer(needed, L'\0');
    DWORD written = ExpandEnvironmentStringsW(value.c_str(), buffer.data(), needed);
    if (written == 0) return value;
    buffer.resize(written - 1);
    return buffer;
}

bool ReadInstallerFooter(const fs::path& exePath, InstallerFooter& footer, uint64_t& footerOffset) {
    std::ifstream in(exePath, std::ios::binary | std::ios::ate);
    if (!in) return false;
    std::streamoff size = in.tellg();
    if (size < (std::streamoff)sizeof(InstallerFooter)) return false;
    footerOffset = static_cast<uint64_t>(size) - sizeof(InstallerFooter);
    in.seekg(static_cast<std::streamoff>(footerOffset));
    in.read(reinterpret_cast<char*>(&footer), sizeof(footer));
    if (!in) return false;
    return std::memcmp(footer.magic, INSTALLER_MAGIC.c_str(), INSTALLER_MAGIC.size()) == 0;
}

bool IsRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                 DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin == TRUE;
}

bool RelaunchAsAdmin(const std::wstring& arguments) {
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    SHELLEXECUTEINFOW sei = {};
    sei.cbSize = sizeof(sei);
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.lpVerb = L"runas";
    sei.lpFile = exePathBuf;
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

bool CreateShortcut(const std::wstring& targetPath, const std::wstring& shortcutPath, const std::wstring& workingDir, const std::wstring& description) {
    CComPtr<IShellLinkW> shellLink;
    HRESULT hr = CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&shellLink));
    if (FAILED(hr)) return false;

    shellLink->SetPath(targetPath.c_str());
    shellLink->SetWorkingDirectory(workingDir.c_str());
    shellLink->SetDescription(description.c_str());
    shellLink->SetIconLocation(targetPath.c_str(), 0);

    CComPtr<IPersistFile> persistFile;
    hr = shellLink->QueryInterface(IID_PPV_ARGS(&persistFile));
    if (FAILED(hr)) return false;

    return SUCCEEDED(persistFile->Save(shortcutPath.c_str(), TRUE));
}

namespace {
    HWND g_hwnd = nullptr;
    HWND g_title = nullptr;
    HWND g_status = nullptr;
    HWND g_progress = nullptr;
    HFONT g_titleFont = nullptr;
    HFONT g_textFont = nullptr;
    std::atomic<uint64_t> g_done{0};
    std::atomic<uint64_t> g_total{1};
    std::atomic<bool> g_finished{false};
    std::atomic<bool> g_uninstall{false};
    std::mutex g_statusMutex;
    std::wstring g_statusText = L"Preparing...";
    std::wstring g_appName = L"";

    void UpdateStatus(const std::wstring& text) {
        std::lock_guard<std::mutex> lock(g_statusMutex);
        g_statusText = text;
    }

    void RefreshUi() {
        if (!g_hwnd) return;
        uint64_t total = g_total.load();
        uint64_t done = g_done.load();
        if (total == 0) total = 1;
        int percent = static_cast<int>((done * 100) / total);
        SendMessageW(g_progress, PBM_SETPOS, percent, 0);
        std::lock_guard<std::mutex> lock(g_statusMutex);
        SetWindowTextW(g_status, g_statusText.c_str());
    }
}

std::wstring LoadAppNameFromManifest() {
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    fs::path exePath = exePathBuf;

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

    uint64_t payloadStart = exeSize - sizeof(InstallerFooter) - manifestSize - payloadSize;
    uint64_t manifestOffset = payloadStart + payloadSize;

    std::ifstream in(exePath, std::ios::binary);
    if (!in) return L"";
    in.seekg(static_cast<std::streamoff>(manifestOffset));
    std::string manifestJson(manifestSize, '\0');
    in.read(manifestJson.data(), static_cast<std::streamsize>(manifestSize));
    if (!in) return L"";

    nlohmann::json manifest = nlohmann::json::parse(manifestJson, nullptr, false);
    if (manifest.is_discarded()) return L"";
    std::string appName = manifest.value("appName", "");
    return ToWString(appName);
}

bool InstallFromSelf() {
    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    fs::path exePath = exePathBuf;

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

    uint64_t payloadStart = exeSize - sizeof(InstallerFooter) - manifestSize - payloadSize;
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
    std::string appExeRel = manifest.value("appExeRel", "");
    auto setup = manifest.value("setup", nlohmann::json::object());
    std::string installDir = setup.value("installDir", "");
    std::string startMenuFolder = setup.value("startMenuFolder", "");
    std::string setupName = setup.value("setupName", "");
    std::string setupIcon = setup.value("setupIcon", "");
    if (appName.empty() || version.empty() || appExeRel.empty() ||
        installDir.empty() || startMenuFolder.empty() || setupName.empty() || setupIcon.empty()) {
        UpdateStatus(L"Missing required metadata.");
        return false;
    }

    bool createDesktopShortcut = setup.value("createDesktopShortcut", true);
    bool createStartupShortcut = setup.value("createStartupShortcut", false);
    bool runOnStartup = setup.value("runOnStartup", false);
    bool enableUninstall = setup.value("enableUninstall", true);

    std::wstring installDirW = ExpandEnv(ToWString(installDir));

    fs::path installDirPath = fs::path(installDirW);
    fs::create_directories(installDirPath);

    auto files = manifest.value("files", nlohmann::json::array());
    uint64_t totalBytes = 0;
    for (const auto& entry : files) {
        totalBytes += entry.value("size", 0ull);
    }

    UpdateStatus(L"Installing files...");

    uint64_t doneBytes = 0;
    for (const auto& entry : files) {
        std::string relPath = entry.value("path", "");
        uint64_t offset = entry.value("offset", 0ull);
        uint64_t size = entry.value("size", 0ull);
        if (relPath.empty() || size == 0) continue;

        fs::path outPath = installDirPath / fs::path(relPath);
        fs::create_directories(outPath.parent_path());

        std::ofstream out(outPath, std::ios::binary);
        if (!out) continue;

        in.seekg(static_cast<std::streamoff>(payloadStart + offset));
        uint64_t remaining = size;
        std::vector<char> buffer(1024 * 1024);
        while (remaining > 0) {
            uint64_t toRead = std::min<uint64_t>(remaining, buffer.size());
            in.read(buffer.data(), static_cast<std::streamsize>(toRead));
            std::streamsize got = in.gcount();
            if (got <= 0) break;
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
    std::wstring appNameW = ToWString(appName);

    HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (SUCCEEDED(hr)) {
        UpdateStatus(L"Creating shortcuts...");
        if (!startMenuFolder.empty()) {
            PWSTR programsPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &programsPath))) {
                fs::path startMenuDir = fs::path(programsPath) / fs::path(startMenuFolder);
                fs::create_directories(startMenuDir);
                fs::path shortcutPath = startMenuDir / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(programsPath);
            }
        }

        if (createDesktopShortcut) {
            PWSTR desktopPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktopPath))) {
                fs::path shortcutPath = fs::path(desktopPath) / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(desktopPath);
            }
        }

        if (createStartupShortcut || runOnStartup) {
            PWSTR startupPath = nullptr;
            if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &startupPath))) {
                fs::path shortcutPath = fs::path(startupPath) / (appName + ".lnk");
                CreateShortcut(appExeW, shortcutPath.wstring(), workingDirW, appNameW);
                CoTaskMemFree(startupPath);
            }
        }

        if (enableUninstall) {
            UpdateStatus(L"Registering uninstall...");
            fs::path uninstallPath = installDirPath / "Uninstall.exe";
            try {
                fs::copy_file(exePath, uninstallPath, fs::copy_options::overwrite_existing);
            } catch (...) {
            }

            HKEY hKey = nullptr;
            std::wstring uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appNameW;
            if (RegCreateKeyExW(HKEY_LOCAL_MACHINE, uninstallKey.c_str(), 0, nullptr, 0, KEY_WRITE, nullptr, &hKey, nullptr) == ERROR_SUCCESS) {
                std::wstring displayName = appNameW;
                std::wstring uninstallCmd = L"\"" + uninstallPath.wstring() + L"\" --uninstall";
                RegSetValueExW(hKey, L"DisplayName", 0, REG_SZ, reinterpret_cast<const BYTE*>(displayName.c_str()), static_cast<DWORD>((displayName.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"UninstallString", 0, REG_SZ, reinterpret_cast<const BYTE*>(uninstallCmd.c_str()), static_cast<DWORD>((uninstallCmd.size() + 1) * sizeof(wchar_t)));
                RegSetValueExW(hKey, L"InstallLocation", 0, REG_SZ, reinterpret_cast<const BYTE*>(installDirPath.wstring().c_str()), static_cast<DWORD>((installDirPath.wstring().size() + 1) * sizeof(wchar_t)));
                RegCloseKey(hKey);
            }
        }

        CoUninitialize();
    }

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

    wchar_t exePathBuf[MAX_PATH];
    GetModuleFileNameW(NULL, exePathBuf, MAX_PATH);
    fs::path exePath = exePathBuf;

    InstallerFooter footer{};
    uint64_t footerOffset = 0;
    if (!ReadInstallerFooter(exePath, footer, footerOffset)) {
        return true;
    }

    uint64_t exeSize = footerOffset + sizeof(InstallerFooter);
    uint64_t payloadSize = footer.payloadSize;
    uint64_t manifestSize = footer.manifestSize;
    if (manifestSize == 0 || payloadSize == 0) {
        return true;
    }

    uint64_t payloadStart = exeSize - sizeof(InstallerFooter) - manifestSize - payloadSize;
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
    std::string appExeRel = manifest.value("appExeRel", "");
    auto setup = manifest.value("setup", nlohmann::json::object());
    std::string installDir = setup.value("installDir", "");
    std::string startMenuFolder = setup.value("startMenuFolder", "");
    std::string setupName = setup.value("setupName", "");
    std::string setupIcon = setup.value("setupIcon", "");
    if (appName.empty() || version.empty() || appExeRel.empty() ||
        installDir.empty() || startMenuFolder.empty() || setupName.empty() || setupIcon.empty()) {
        UpdateStatus(L"Missing required metadata.");
        return false;
    }
    bool runOnStartup = setup.value("runOnStartup", false);
    bool createStartupShortcut = setup.value("createStartupShortcut", false);

    fs::path installDir = exePath.parent_path();

    if (installDir.empty()) {
        return true;
    }

    UpdateStatus(L"Removing shortcuts...");
    std::wstring appNameW = ToWString(appName);

    PWSTR programsPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Programs, 0, nullptr, &programsPath))) {
        fs::path startMenuDir = fs::path(programsPath) / ToWString(startMenuFolder);
        fs::remove_all(startMenuDir);
        CoTaskMemFree(programsPath);
    }

    PWSTR desktopPath = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktopPath))) {
        fs::remove(fs::path(desktopPath) / (appNameW + L".lnk"));
        CoTaskMemFree(desktopPath);
    }

    if (createStartupShortcut || runOnStartup) {
        PWSTR startupPath = nullptr;
        if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &startupPath))) {
            fs::remove(fs::path(startupPath) / (appNameW + L".lnk"));
            CoTaskMemFree(startupPath);
        }
    }

    UpdateStatus(L"Removing files...");
    try {
        fs::remove_all(installDir);
    } catch (...) {
    }

    HKEY hUninstall = nullptr;
    std::wstring uninstallKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" + appNameW;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_WRITE, &hUninstall) == ERROR_SUCCESS) {
        RegDeleteTreeW(hUninstall, appNameW.c_str());
        RegCloseKey(hUninstall);
    }

    UpdateStatus(L"Done");
    return true;
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TIMER:
            RefreshUi();
            if (g_finished.load()) {
                DestroyWindow(hwnd);
            }
            return 0;
        case WM_CLOSE:
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
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"--uninstall") == 0) {
            g_uninstall.store(true);
            break;
        }
    }
    if (argv) LocalFree(argv);

    g_appName = LoadAppNameFromManifest();
    if (g_appName.empty()) {
        g_appName = L"Installer";
    }

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);

    const wchar_t* className = L"NovadeskInstallerWindow";
    WNDCLASSW wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    int width = 520;
    int height = 220;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    std::wstring windowTitle = g_appName + (g_uninstall.load() ? L" Uninstaller" : L" Installer");
    g_hwnd = CreateWindowExW(
        0, className, windowTitle.c_str(),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_hwnd) {
        return InstallFromSelf() ? 0 : 1;
    }

    std::wstring headerText = g_uninstall.load() ? (L"Uninstalling " + g_appName) : (L"Installing " + g_appName);
    g_title = CreateWindowW(L"STATIC", headerText.c_str(),
                            WS_CHILD | WS_VISIBLE,
                            24, 20, 460, 28,
                            g_hwnd, nullptr, hInstance, nullptr);
    g_titleFont = CreateFontW(22, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    g_textFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
    SendMessageW(g_title, WM_SETFONT, (WPARAM)g_titleFont, TRUE);

    g_status = CreateWindowW(L"STATIC", L"Preparing...",
                             WS_CHILD | WS_VISIBLE,
                             24, 60, 460, 20,
                             g_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(g_status, WM_SETFONT, (WPARAM)g_textFont, TRUE);

    g_progress = CreateWindowW(PROGRESS_CLASSW, nullptr,
                               WS_CHILD | WS_VISIBLE,
                               24, 92, 460, 22,
                               g_hwnd, nullptr, hInstance, nullptr);
    SendMessageW(g_progress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    std::thread worker([&]() {
        if (g_uninstall.load()) {
            UpdateStatus(L"Removing files...");
            UninstallSelf();
        } else {
            InstallFromSelf();
        }
        g_finished.store(true);
    });
    worker.detach();

    SetTimer(g_hwnd, 1, 100, nullptr);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    if (g_titleFont) DeleteObject(g_titleFont);
    if (g_textFont) DeleteObject(g_textFont);
    return 0;
}
