#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <fstream>
#include <iterator>
#include "../../third_party/json/json.hpp"

#include "resource.h"

#pragma comment(lib, "comctl32.lib")


struct WidgetEntry
{
    std::wstring name;
    std::wstring scriptPath;
    bool loaded = false;
};

static HWND g_list = nullptr;
static std::vector<WidgetEntry> g_widgets;
static HFONT g_buttonFont = nullptr;
static const int kWindowWidth = 720;
static const int kWindowHeight = 480;
static const UINT kTrayMessage = WM_APP + 1;
static const UINT kTrayMenuCommand = WM_APP + 2;
static const int kTrayMenuCloseId = 100;
static NOTIFYICONDATAW g_tray{};
static PROCESS_INFORMATION g_novadeskProcess{};
static bool g_novadeskRunning = false;
static HMENU g_trayMenu = nullptr;
static HWND g_btnRefreshList = nullptr;
static HWND g_btnLoad = nullptr;
static HWND g_btnUnload = nullptr;
static HWND g_btnRefresh = nullptr;
static HWND g_btnRefreshAll = nullptr;
static HWND g_tab = nullptr;
static HWND g_logsList = nullptr;
static HICON g_windowIconLarge = nullptr;
static HICON g_windowIconSmall = nullptr;
static HWND g_mainWindow = nullptr;
static HANDLE g_novadeskLogRead = nullptr;
static HANDLE g_novadeskLogWrite = nullptr;
static HANDLE g_novadeskLogThread = nullptr;
static std::wstring g_pendingLogLine;
static int g_activeTab = 0;
static std::unordered_map<std::wstring, std::wstring> g_savedLoadedScripts;

static const int kControlIdRefreshList = 101;
static const int kControlIdLoad = 102;
static const int kControlIdUnload = 103;
static const int kControlIdRefresh = 104;
static const int kControlIdRefreshAll = 105;
static const int kControlIdTabs = 106;
static const UINT kLogAppendMessage = WM_APP + 20;
static const UINT_PTR kLogsRefreshTimerId = 1;
static const int kMaxLogRows = 2000;
static void ExecuteNovadeskCommand(const std::wstring &cmd, const std::wstring &path);
static void ExecuteNovadeskCommandNoPath(const std::wstring &cmd);

static std::wstring GetExeDir()
{
    wchar_t buf[MAX_PATH + 1] = {};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::filesystem::path p(buf);
    return p.parent_path().wstring();
}

static std::wstring JoinPath(const std::wstring &a, const std::wstring &b)
{
    std::filesystem::path p(a);
    p /= b;
    return p.wstring();
}

static void LoadWindowIcons(HINSTANCE hInstance)
{
    if (!g_windowIconLarge)
    {
        g_windowIconLarge = static_cast<HICON>(
            LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK), IMAGE_ICON,
                       GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
    }
    if (!g_windowIconSmall)
    {
        g_windowIconSmall = static_cast<HICON>(
            LoadImageW(hInstance, MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK), IMAGE_ICON,
                       GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    }
}

static void LogLine(const std::wstring &line)
{
    const std::wstring logPath = JoinPath(GetExeDir(), L"manage_novadesk.log");
    std::wofstream out(logPath, std::ios::app);
    if (out)
        out << line << L"\n";
    OutputDebugStringW((line + L"\n").c_str());
}

static std::wstring Win32ErrorToString(DWORD err)
{
    wchar_t buf[512] = {};
    FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                   buf, 512, nullptr);
    return std::wstring(buf);
}

static std::wstring NormalizePathLower(const std::wstring &p)
{
    std::error_code ec;
    std::filesystem::path fp(p);
    fp = std::filesystem::absolute(fp, ec).lexically_normal();
    std::wstring out = fp.wstring();
    std::transform(out.begin(), out.end(), out.begin(), ::towlower);
    return out;
}

static std::wstring BytesToWide(const std::string &bytes)
{
    if (bytes.empty())
        return L"";

    auto convert = [&](UINT cp) -> std::wstring
    {
        int needed = MultiByteToWideChar(cp, 0, bytes.c_str(), static_cast<int>(bytes.size()), nullptr, 0);
        if (needed <= 0)
            return L"";
        std::wstring wide(needed, L'\0');
        MultiByteToWideChar(cp, 0, bytes.c_str(), static_cast<int>(bytes.size()), wide.data(), needed);
        return wide;
    };

    std::wstring utf8 = convert(CP_UTF8);
    if (!utf8.empty())
        return utf8;
    return convert(CP_ACP);
}

static std::string WideToUtf8(const std::wstring &wide)
{
    if (wide.empty())
        return std::string();
    int needed = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), nullptr, 0, nullptr, nullptr);
    if (needed <= 0)
        return std::string();
    std::string out(needed, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), static_cast<int>(wide.size()), out.data(), needed, nullptr, nullptr);
    return out;
}

static std::wstring Utf8ToWide(const std::string &utf8)
{
    if (utf8.empty())
        return std::wstring();
    int needed = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    if (needed <= 0)
        return std::wstring();
    std::wstring out(needed, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), out.data(), needed);
    return out;
}

static DWORD WINAPI NovadeskLogReaderThreadProc(LPVOID)
{
    HANDLE readPipe = g_novadeskLogRead;
    if (!readPipe)
        return 0;

    std::vector<char> buffer(4096);
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0)
    {
        std::wstring text = BytesToWide(std::string(buffer.data(), bytesRead));
        if (text.empty())
            continue;

        auto *payload = new std::wstring(std::move(text));
        if (!PostMessageW(g_mainWindow, kLogAppendMessage, 0, reinterpret_cast<LPARAM>(payload)))
        {
            delete payload;
            break;
        }
    }
    return 0;
}

static void StopNovadeskLogCapture()
{
    if (g_novadeskLogRead)
    {
        CloseHandle(g_novadeskLogRead);
        g_novadeskLogRead = nullptr;
    }
    if (g_novadeskLogWrite)
    {
        CloseHandle(g_novadeskLogWrite);
        g_novadeskLogWrite = nullptr;
    }
    if (g_novadeskLogThread)
    {
        WaitForSingleObject(g_novadeskLogThread, 1500);
        CloseHandle(g_novadeskLogThread);
        g_novadeskLogThread = nullptr;
    }
}

static std::wstring GetWidgetsRoot()
{
    const std::wstring exeDir = GetExeDir();
    const std::wstring portableFlag = JoinPath(exeDir, L"manage_window_settings.json");
    if (std::filesystem::exists(portableFlag))
    {
        return JoinPath(exeDir, L"Widgets");
    }

    PWSTR docs = nullptr;
    std::wstring out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &docs)) && docs)
    {
        out = std::wstring(docs) + L"\\Novadesk\\Widgets";
        CoTaskMemFree(docs);
    }
    return out;
}

static std::wstring GetNovadeskExePath()
{
    return JoinPath(GetExeDir(), L"Novadesk.exe");
}

static std::wstring GetManageSettingsPath()
{
    const std::wstring exePath = JoinPath(GetExeDir(), L"manage_window_settings.json");
    std::error_code ec;
    if (std::filesystem::exists(std::filesystem::path(exePath), ec) && !ec)
    {
        return exePath;
    }

    PWSTR roaming = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming)) && roaming)
    {
        const std::wstring appDataDir = std::wstring(roaming) + L"\\Novadesk";
        CoTaskMemFree(roaming);
        std::filesystem::create_directories(std::filesystem::path(appDataDir), ec);
        if (!ec)
        {
            return JoinPath(appDataDir, L"manage_window_settings.json");
        }
    }

    // Fallback: keep using EXE directory path if AppData path is unavailable.
    return exePath;
}

static void SaveManageSettings()
{
    const std::wstring path = GetManageSettingsPath();
    std::vector<std::wstring> paths;
    paths.reserve(g_savedLoadedScripts.size());
    for (const auto &kv : g_savedLoadedScripts)
    {
        paths.push_back(kv.second);
    }
    std::sort(paths.begin(), paths.end());

    nlohmann::json doc = nlohmann::json::object();
    doc["loadedScripts"] = nlohmann::json::array();
    for (const auto &p : paths)
    {
        doc["loadedScripts"].push_back(WideToUtf8(p));
    }

    std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
    if (!out.is_open())
    {
        LogLine(L"[Manage] Failed to write settings file: " + path);
        return;
    }
    out << doc.dump(2);
}

static void RememberLoadedScriptState(const std::wstring &scriptPath, bool loaded)
{
    const std::wstring norm = NormalizePathLower(scriptPath);
    if (loaded)
    {
        g_savedLoadedScripts[norm] = scriptPath;
    }
    else
    {
        g_savedLoadedScripts.erase(norm);
    }
    SaveManageSettings();
}

static void LoadManageSettings()
{
    g_savedLoadedScripts.clear();
    const std::wstring path = GetManageSettingsPath();
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in.is_open())
    {
        return;
    }

    nlohmann::json doc;
    try
    {
        in >> doc;
    }
    catch (...)
    {
        return;
    }

    if (!doc.is_object() || !doc.contains("loadedScripts") || !doc["loadedScripts"].is_array())
    {
        return;
    }

    for (const auto &v : doc["loadedScripts"])
    {
        if (!v.is_string())
            continue;
        std::wstring pathValue = Utf8ToWide(v.get<std::string>());
        if (pathValue.empty())
        {
            continue;
        }
        g_savedLoadedScripts[NormalizePathLower(pathValue)] = pathValue;
    }
}

static void ApplySavedLoadedScripts()
{
    for (const auto &kv : g_savedLoadedScripts)
    {
        ExecuteNovadeskCommand(L"--load", kv.second);
    }
}

static std::wstring CreateTempListPath()
{
    wchar_t tempPath[MAX_PATH + 1] = {};
    if (!GetTempPathW(MAX_PATH, tempPath))
        return L"";
    wchar_t filePath[MAX_PATH + 1] = {};
    if (!GetTempFileNameW(tempPath, L"nds", 0, filePath))
        return L"";
    return std::wstring(filePath);
}

static void StartNovadesk()
{
    if (g_novadeskRunning)
        return;

    const std::wstring exe = GetNovadeskExePath();
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;
    if (!CreatePipe(&g_novadeskLogRead, &g_novadeskLogWrite, &sa, 0))
    {
        g_novadeskLogRead = nullptr;
        g_novadeskLogWrite = nullptr;
        LogLine(L"[Manage] Failed to create output pipe for Novadesk.");
    }
    else
    {
        SetHandleInformation(g_novadeskLogRead, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    if (g_novadeskLogWrite)
    {
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdOutput = g_novadeskLogWrite;
        si.hStdError = g_novadeskLogWrite;
        si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    }
    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = L"\"" + exe + L"\"";

    if (CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        g_novadeskProcess = pi;
        g_novadeskRunning = true;
        if (g_novadeskLogWrite)
        {
            CloseHandle(g_novadeskLogWrite);
            g_novadeskLogWrite = nullptr;
            g_novadeskLogThread = CreateThread(nullptr, 0, NovadeskLogReaderThreadProc, nullptr, 0, nullptr);
        }
        LogLine(L"[Manage] Started Novadesk: " + exe);
    }
    else
    {
        DWORD err = GetLastError();
        LogLine(L"[Manage] Failed to start Novadesk (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
        StopNovadeskLogCapture();
    }
}

static void StopNovadesk()
{
    if (!g_novadeskRunning)
    {
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

static void AddTrayIcon(HWND hWnd)
{
    g_tray = {};
    g_tray.cbSize = sizeof(g_tray);
    g_tray.hWnd = hWnd;
    g_tray.uID = 1;
    g_tray.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    g_tray.uCallbackMessage = kTrayMessage;
    g_tray.hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_MANAGE_NOVADESK));
    wcscpy_s(g_tray.szTip, L"Manage Novadesk");

    Shell_NotifyIconW(NIM_ADD, &g_tray);
}

static void RemoveTrayIcon()
{
    if (g_tray.hWnd)
        Shell_NotifyIconW(NIM_DELETE, &g_tray);
    if (g_tray.hIcon)
        DestroyIcon(g_tray.hIcon);
    g_tray = {};
}

static void EnsureTrayMenu()
{
    if (g_trayMenu)
        return;
    g_trayMenu = CreatePopupMenu();
    AppendMenuW(g_trayMenu, MF_STRING, kTrayMenuCloseId, L"Close");
}

static void ShowTrayMenu(HWND hWnd)
{
    EnsureTrayMenu();
    POINT pt{};
    GetCursorPos(&pt);
    SetForegroundWindow(hWnd);
    TrackPopupMenu(g_trayMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    PostMessageW(hWnd, kTrayMenuCommand, 0, 0);
}

static std::wstring RunProcessCaptureStdout(const std::wstring &exePath, const std::wstring &args)
{
    std::wstring cmd = L"\"" + exePath + L"\" " + args;
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
    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(hRead);
        CloseHandle(hWrite);
        DWORD err = GetLastError();
        LogLine(L"[Manage] CreateProcess failed (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
        return L"";
    }

    CloseHandle(hWrite);

    std::wstring output;
    std::vector<char> buffer(4096);
    DWORD bytesRead = 0;
    while (ReadFile(hRead, buffer.data(), static_cast<DWORD>(buffer.size()), &bytesRead, nullptr) && bytesRead > 0)
    {
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

static bool RunProcessWait(const std::wstring &exePath, const std::wstring &args)
{
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = L"\"" + exePath + L"\" " + args;
    LogLine(L"[Manage] Run wait: " + cmdLine);
    if (!CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        DWORD err = GetLastError();
        LogLine(L"[Manage] Run wait failed (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
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

static void AppendLogRow(const std::wstring &time, const std::wstring &source, const std::wstring &level, const std::wstring &message)
{
    if (!g_logsList)
        return;

    const int last = ListView_GetItemCount(g_logsList) - 1;
    if (last >= 0)
    {
        wchar_t tbuf[256] = {};
        wchar_t sbuf[256] = {};
        wchar_t lbuf[128] = {};
        wchar_t mbuf[2048] = {};
        ListView_GetItemText(g_logsList, last, 0, tbuf, static_cast<int>(std::size(tbuf)));
        ListView_GetItemText(g_logsList, last, 1, sbuf, static_cast<int>(std::size(sbuf)));
        ListView_GetItemText(g_logsList, last, 2, lbuf, static_cast<int>(std::size(lbuf)));
        ListView_GetItemText(g_logsList, last, 3, mbuf, static_cast<int>(std::size(mbuf)));
        if (time == tbuf && source == sbuf && level == lbuf && message == mbuf)
        {
            return;
        }
    }

    LVITEMW item{};
    item.mask = LVIF_TEXT;
    item.iItem = ListView_GetItemCount(g_logsList);
    item.pszText = const_cast<wchar_t *>(time.c_str());
    int row = ListView_InsertItem(g_logsList, &item);
    ListView_SetItemText(g_logsList, row, 1, const_cast<wchar_t *>(source.c_str()));
    ListView_SetItemText(g_logsList, row, 2, const_cast<wchar_t *>(level.c_str()));
    ListView_SetItemText(g_logsList, row, 3, const_cast<wchar_t *>(message.c_str()));

    while (ListView_GetItemCount(g_logsList) > kMaxLogRows)
    {
        ListView_DeleteItem(g_logsList, 0);
    }

    const int count = ListView_GetItemCount(g_logsList);
    if (count > 0)
    {
        ListView_EnsureVisible(g_logsList, count - 1, FALSE);
    }
}

static void ParseAndAppendLogLine(const std::wstring &line)
{
    if (line.empty())
        return;

    std::wstring trimmed = line;
    while (!trimmed.empty() && (trimmed.back() == L'\r' || trimmed.back() == L'\n'))
    {
        trimmed.pop_back();
    }
    if (trimmed.empty())
        return;

    std::wstring time;
    std::wstring source;
    std::wstring level;
    std::wstring message = trimmed;

    size_t p0s = trimmed.find(L'[');
    size_t p0e = (p0s == std::wstring::npos) ? std::wstring::npos : trimmed.find(L']', p0s + 1);
    size_t p1s = (p0e == std::wstring::npos) ? std::wstring::npos : trimmed.find(L'[', p0e + 1);
    size_t p1e = (p1s == std::wstring::npos) ? std::wstring::npos : trimmed.find(L']', p1s + 1);
    size_t p2s = (p1e == std::wstring::npos) ? std::wstring::npos : trimmed.find(L'[', p1e + 1);
    size_t p2e = (p2s == std::wstring::npos) ? std::wstring::npos : trimmed.find(L']', p2s + 1);

    if (p0s != std::wstring::npos && p0e != std::wstring::npos &&
        p1s != std::wstring::npos && p1e != std::wstring::npos &&
        p2s != std::wstring::npos && p2e != std::wstring::npos)
    {
        time = trimmed.substr(p0s + 1, p0e - p0s - 1);
        source = trimmed.substr(p1s + 1, p1e - p1s - 1);
        level = trimmed.substr(p2s + 1, p2e - p2s - 1);
        size_t msgStart = p2e + 1;
        while (msgStart < trimmed.size() && trimmed[msgStart] == L' ')
        {
            ++msgStart;
        }
        message = trimmed.substr(msgStart);
    }

    AppendLogRow(time, source, level, message);
}

static void ProcessLogChunk(const std::wstring &chunk)
{
    if (chunk.empty())
        return;

    g_pendingLogLine += chunk;
    size_t start = 0;
    for (;;)
    {
        size_t nl = g_pendingLogLine.find(L'\n', start);
        if (nl == std::wstring::npos)
            break;
        std::wstring line = g_pendingLogLine.substr(start, nl - start);
        ParseAndAppendLogLine(line);
        start = nl + 1;
    }
    if (start > 0)
    {
        g_pendingLogLine.erase(0, start);
    }
}

static std::unordered_set<std::wstring> GetRunningScripts()
{
    std::unordered_set<std::wstring> out;
    std::wstring exe = GetNovadeskExePath();
    const std::wstring tempList = CreateTempListPath();
    if (tempList.empty())
    {
        LogLine(L"[Manage] list-scripts temp file not created.");
        return out;
    }
    const std::wstring args = L"--list-scripts-file \"" + tempList + L"\"";
    RunProcessWait(exe, args);

    std::wifstream in(tempList.c_str());
    if (in.is_open())
    {
        std::wstring line;
        while (std::getline(in, line))
        {
            if (line.empty())
                continue;
            LogLine(L"[Manage] list-scripts: " + line);
            out.insert(NormalizePathLower(line));
        }
        in.close();
    }
    else
    {
        LogLine(L"[Manage] list-scripts failed to open: " + tempList);
    }
    DeleteFileW(tempList.c_str());
    LogLine(L"[Manage] Running scripts count: " + std::to_wstring(out.size()));
    return out;
}

static std::vector<WidgetEntry> LoadWidgets()
{
    std::vector<WidgetEntry> list;
    const std::wstring root = GetWidgetsRoot();
    if (root.empty())
        return list;

    std::error_code ec;
    if (!std::filesystem::exists(root, ec))
        return list;

    auto running = GetRunningScripts();

    for (const auto &entry : std::filesystem::directory_iterator(root, ec))
    {
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

static void ExecuteNovadeskCommand(const std::wstring &cmd, const std::wstring &path)
{
    const std::wstring exe = GetNovadeskExePath();
    std::wstring args = cmd + L" \"" + path + L"\"";

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::wstring cmdLine = L"\"" + exe + L"\" " + args;
    LogLine(L"[Manage] Exec: " + cmdLine);
    if (CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        LogLine(L"[Manage] Exec OK.");
    }
    else
    {
        DWORD err = GetLastError();
        LogLine(L"[Manage] Exec failed (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
    }
}

static void RefreshListView()
{
    g_widgets = LoadWidgets();
    LogLine(L"[Manage] Refresh list. Widgets: " + std::to_wstring(g_widgets.size()));

    ListView_DeleteAllItems(g_list);
    int idx = 0;
    for (const auto &w : g_widgets)
    {
        LVITEMW item{};
        item.mask = LVIF_TEXT;
        item.iItem = idx;
        item.pszText = const_cast<wchar_t *>(w.name.c_str());
        ListView_InsertItem(g_list, &item);

        ListView_SetItemText(g_list, idx, 1, const_cast<wchar_t *>(w.scriptPath.c_str()));
        std::wstring status = w.loaded ? L"Loaded" : L"Not Loaded";
        ListView_SetItemText(g_list, idx, 2, const_cast<wchar_t *>(status.c_str()));
        LogLine(L"[Manage] Widget: " + w.name + L" | " + w.scriptPath + L" | " + (w.loaded ? L"Loaded" : L"Not Loaded"));
        ++idx;
    }
    UpdateButtonState();
}

static int GetSelectedIndex()
{
    return ListView_GetNextItem(g_list, -1, LVNI_SELECTED);
}

static void UpdateButtonState()
{
    if (g_activeTab != 0)
    {
        EnableWindow(g_btnLoad, FALSE);
        EnableWindow(g_btnUnload, FALSE);
        EnableWindow(g_btnRefresh, FALSE);
        EnableWindow(g_btnRefreshAll, FALSE);
        return;
    }

    EnableWindow(g_btnRefreshAll, TRUE);

    const int idx = GetSelectedIndex();
    if (idx < 0 || idx >= static_cast<int>(g_widgets.size()))
    {
        EnableWindow(g_btnLoad, FALSE);
        EnableWindow(g_btnUnload, FALSE);
        EnableWindow(g_btnRefresh, FALSE);
        return;
    }

    const bool isLoaded = g_widgets[idx].loaded;
    EnableWindow(g_btnLoad, isLoaded ? FALSE : TRUE);
    EnableWindow(g_btnUnload, isLoaded ? TRUE : FALSE);
    EnableWindow(g_btnRefresh, isLoaded ? TRUE : FALSE);
}

static void RefreshLogsView()
{
    if (!g_logsList)
        return;

    if (ListView_GetItemCount(g_logsList) == 0)
    {
        AppendLogRow(L"", L"Manage", L"INFO", L"Waiting for live output from novadesk.exe...");
    }
}

static void ExecuteNovadeskCommandNoPath(const std::wstring &cmd)
{
    const std::wstring exe = GetNovadeskExePath();

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::wstring cmdLine = L"\"" + exe + L"\" " + cmd;
    LogLine(L"[Manage] Exec: " + cmdLine);
    if (CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        LogLine(L"[Manage] Exec OK.");
    }
    else
    {
        DWORD err = GetLastError();
        LogLine(L"[Manage] Exec failed (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
    }
}

static void ApplyTabState()
{
    const bool widgetsTab = (g_activeTab == 0);
    ShowWindow(g_list, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_logsList, widgetsTab ? SW_HIDE : SW_SHOW);
    ShowWindow(g_btnLoad, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnUnload, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefresh, widgetsTab ? SW_SHOW : SW_HIDE);
    ShowWindow(g_btnRefreshAll, widgetsTab ? SW_SHOW : SW_HIDE);

    if (!widgetsTab)
    {
        RefreshLogsView();
    }
    UpdateButtonState();
}

static void OnLoadSelected()
{
    int idx = GetSelectedIndex();
    if (idx < 0 || idx >= static_cast<int>(g_widgets.size()))
    {
        LogLine(L"[Manage] Load ignored: no selection.");
        return;
    }
    LogLine(L"[Manage] Load: " + g_widgets[idx].scriptPath);
    ExecuteNovadeskCommand(L"--load", g_widgets[idx].scriptPath);
    RememberLoadedScriptState(g_widgets[idx].scriptPath, true);
    RefreshListView();
    UpdateButtonState();
}

static void OnUnloadSelected()
{
    int idx = GetSelectedIndex();
    if (idx < 0 || idx >= static_cast<int>(g_widgets.size()))
    {
        LogLine(L"[Manage] Unload ignored: no selection.");
        return;
    }
    LogLine(L"[Manage] Unload: " + g_widgets[idx].scriptPath);
    ExecuteNovadeskCommand(L"--unload", g_widgets[idx].scriptPath);
    RememberLoadedScriptState(g_widgets[idx].scriptPath, false);
    RefreshListView();
    UpdateButtonState();
}

static void OnRefreshSelected()
{
    int idx = GetSelectedIndex();
    if (idx < 0 || idx >= static_cast<int>(g_widgets.size()))
    {
        LogLine(L"[Manage] Refresh ignored: no selection.");
        return;
    }
    LogLine(L"[Manage] Refresh: " + g_widgets[idx].scriptPath);
    ExecuteNovadeskCommand(L"--refresh", g_widgets[idx].scriptPath);
    RefreshListView();
    UpdateButtonState();
}

static void OnRefreshAll()
{
    LogLine(L"[Manage] Refresh All");
    ExecuteNovadeskCommandNoPath(L"--refresh-all");
    RefreshListView();
    UpdateButtonState();
}

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        LogLine(L"[Manage] Window created.");
        g_mainWindow = hWnd;
        g_pendingLogLine.clear();
        SendMessageW(hWnd, WM_SETICON, ICON_BIG, reinterpret_cast<LPARAM>(g_windowIconLarge));
        SendMessageW(hWnd, WM_SETICON, ICON_SMALL, reinterpret_cast<LPARAM>(g_windowIconSmall));
        RECT rc{};
        GetClientRect(hWnd, &rc);

        StartNovadesk();
        AddTrayIcon(hWnd);
        LoadManageSettings();
        ApplySavedLoadedScripts();

        g_buttonFont = CreateFontW(-12, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        g_tab = CreateWindowExW(0, WC_TABCONTROLW, L"",
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                10, 10, rc.right - 20, rc.bottom - 60,
                                hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdTabs)), GetModuleHandleW(nullptr), nullptr);
        TCITEMW tabItem{};
        tabItem.mask = TCIF_TEXT;
        tabItem.pszText = const_cast<wchar_t *>(L"Widgets");
        TabCtrl_InsertItem(g_tab, 0, &tabItem);
        tabItem.pszText = const_cast<wchar_t *>(L"Logs");
        TabCtrl_InsertItem(g_tab, 1, &tabItem);

        RECT tabRectOnParent{};
        GetWindowRect(g_tab, &tabRectOnParent);
        MapWindowPoints(HWND_DESKTOP, hWnd, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
        RECT pageRect{};
        GetClientRect(g_tab, &pageRect);
        TabCtrl_AdjustRect(g_tab, FALSE, &pageRect);
        OffsetRect(&pageRect, tabRectOnParent.left, tabRectOnParent.top);

        g_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                 pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top,
                                 hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);
        g_logsList = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
                                     WS_CHILD | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS,
                                     pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top,
                                     hWnd, nullptr, GetModuleHandleW(nullptr), nullptr);

        ListView_SetExtendedListViewStyle(g_list, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

        LVCOLUMNW col{};
        col.mask = LVCF_TEXT | LVCF_WIDTH;

        col.pszText = const_cast<wchar_t *>(L"Widget");
        col.cx = 160;
        ListView_InsertColumn(g_list, 0, &col);

        col.pszText = const_cast<wchar_t *>(L"Script Path");
        col.cx = 360;
        ListView_InsertColumn(g_list, 1, &col);

        col.pszText = const_cast<wchar_t *>(L"Status");
        col.cx = 120;
        ListView_InsertColumn(g_list, 2, &col);

        ListView_SetExtendedListViewStyle(g_logsList, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        LVCOLUMNW lcol{};
        lcol.mask = LVCF_TEXT | LVCF_WIDTH;
        lcol.pszText = const_cast<wchar_t *>(L"Time");
        lcol.cx = 190;
        ListView_InsertColumn(g_logsList, 0, &lcol);
        lcol.pszText = const_cast<wchar_t *>(L"Source");
        lcol.cx = 120;
        ListView_InsertColumn(g_logsList, 1, &lcol);
        lcol.pszText = const_cast<wchar_t *>(L"Level");
        lcol.cx = 80;
        ListView_InsertColumn(g_logsList, 2, &lcol);
        lcol.pszText = const_cast<wchar_t *>(L"Message");
        lcol.cx = 600;
        ListView_InsertColumn(g_logsList, 3, &lcol);

        g_btnRefreshList = CreateWindowW(L"BUTTON", L"Refresh List", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         10, rc.bottom - 40, 110, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefreshList)), GetModuleHandleW(nullptr), nullptr);
        g_btnLoad = CreateWindowW(L"BUTTON", L"Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  130, rc.bottom - 40, 80, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdLoad)), GetModuleHandleW(nullptr), nullptr);
        g_btnUnload = CreateWindowW(L"BUTTON", L"Unload", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    220, rc.bottom - 40, 80, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdUnload)), GetModuleHandleW(nullptr), nullptr);
        g_btnRefresh = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     310, rc.bottom - 40, 80, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefresh)), GetModuleHandleW(nullptr), nullptr);
        g_btnRefreshAll = CreateWindowW(L"BUTTON", L"Refresh All", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                        400, rc.bottom - 40, 100, 28, hWnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(kControlIdRefreshAll)), GetModuleHandleW(nullptr), nullptr);

        if (g_buttonFont)
        {
            SendMessageW(g_tab, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnRefreshList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnLoad, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnUnload, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnRefresh, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnRefreshAll, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_logsList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        }

        RefreshListView();
        ApplyTabState();
        SetTimer(hWnd, kLogsRefreshTimerId, 700, nullptr);
        UpdateButtonState();
        return 0;
    }
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO *mmi = reinterpret_cast<MINMAXINFO *>(lParam);
        mmi->ptMinTrackSize.x = kWindowWidth;
        mmi->ptMinTrackSize.y = kWindowHeight;
        mmi->ptMaxTrackSize.x = kWindowWidth;
        mmi->ptMaxTrackSize.y = kWindowHeight;
        return 0;
    }
    case WM_SIZE:
    {
        RECT rc{};
        GetClientRect(hWnd, &rc);
        if (g_tab)
        {
            MoveWindow(g_tab, 10, 10, rc.right - 20, rc.bottom - 60, TRUE);
            RECT tabRectOnParent{};
            GetWindowRect(g_tab, &tabRectOnParent);
            MapWindowPoints(HWND_DESKTOP, hWnd, reinterpret_cast<LPPOINT>(&tabRectOnParent), 2);
            RECT pageRect{};
            GetClientRect(g_tab, &pageRect);
            TabCtrl_AdjustRect(g_tab, FALSE, &pageRect);
            OffsetRect(&pageRect, tabRectOnParent.left, tabRectOnParent.top);
            if (g_list)
            {
                MoveWindow(g_list, pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top, TRUE);
            }
            if (g_logsList)
            {
                MoveWindow(g_logsList, pageRect.left, pageRect.top, pageRect.right - pageRect.left, pageRect.bottom - pageRect.top, TRUE);
            }
        }
        if (g_btnRefreshList)
        {
            MoveWindow(g_btnRefreshList, 10, rc.bottom - 40, 110, 28, TRUE);
            MoveWindow(g_btnLoad, 130, rc.bottom - 40, 80, 28, TRUE);
            MoveWindow(g_btnUnload, 220, rc.bottom - 40, 80, 28, TRUE);
            MoveWindow(g_btnRefresh, 310, rc.bottom - 40, 80, 28, TRUE);
            MoveWindow(g_btnRefreshAll, 400, rc.bottom - 40, 100, 28, TRUE);
        }
        return 0;
    }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case kControlIdRefreshList:
            RefreshListView();
            break;
        case kControlIdLoad:
            OnLoadSelected();
            break;
        case kControlIdUnload:
            OnUnloadSelected();
            break;
        case kControlIdRefresh:
            OnRefreshSelected();
            break;
        case kControlIdRefreshAll:
            OnRefreshAll();
            break;
        case kTrayMenuCloseId:
            DestroyWindow(hWnd);
            break;
        default:
            break;
        }
        break;
    case WM_NOTIFY:
    {
        LPNMHDR hdr = reinterpret_cast<LPNMHDR>(lParam);
        if (hdr && hdr->hwndFrom == g_list && hdr->code == LVN_ITEMCHANGED)
        {
            UpdateButtonState();
        }
        else if (hdr && hdr->hwndFrom == g_tab && hdr->code == TCN_SELCHANGE)
        {
            g_activeTab = TabCtrl_GetCurSel(g_tab);
            ApplyTabState();
        }
        break;
    }
    case WM_TIMER:
        if (wParam == kLogsRefreshTimerId && g_activeTab == 1)
        {
            RefreshLogsView();
        }
        return 0;
    case kLogAppendMessage:
    {
        auto *chunk = reinterpret_cast<std::wstring *>(lParam);
        if (!chunk)
            return 0;
        ProcessLogChunk(*chunk);
        delete chunk;
        if (g_activeTab == 1)
        {
            RefreshLogsView();
        }
        return 0;
    }
    case WM_CLOSE:
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    case WM_APP + 1:
        if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK)
        {
            ShowWindow(hWnd, SW_SHOW);
            SetForegroundWindow(hWnd);
        }
        else if (lParam == WM_RBUTTONUP)
        {
            ShowTrayMenu(hWnd);
        }
        return 0;
    case WM_DESTROY:
        g_mainWindow = nullptr;
        KillTimer(hWnd, kLogsRefreshTimerId);
        if (g_windowIconLarge)
        {
            DestroyIcon(g_windowIconLarge);
            g_windowIconLarge = nullptr;
        }
        if (g_windowIconSmall)
        {
            DestroyIcon(g_windowIconSmall);
            g_windowIconSmall = nullptr;
        }
        if (g_buttonFont)
        {
            DeleteObject(g_buttonFont);
            g_buttonFont = nullptr;
        }
        if (g_trayMenu)
        {
            DestroyMenu(g_trayMenu);
            g_trayMenu = nullptr;
        }
        RemoveTrayIcon();
        StopNovadesk();
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProcW(hWnd, message, wParam, lParam);
    }
    return 0;
}

int APIENTRY WinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE,
                     _In_ LPSTR,
                     _In_ int nCmdShow)
{
    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES;
    InitCommonControlsEx(&icc);

    const wchar_t *className = L"NovadeskManagerWindow";
    LoadWindowIcons(hInstance);
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = g_windowIconLarge;
    wc.hIconSm = g_windowIconSmall;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = className;
    RegisterClassExW(&wc);

    const DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    RECT wr{0, 0, kWindowWidth, kWindowHeight};
    AdjustWindowRectEx(&wr, style, FALSE, 0);

    HWND hWnd = CreateWindowExW(0, className, L"Manage Novadesk Widgets",
                                style,
                                CW_USEDEFAULT, CW_USEDEFAULT,
                                wr.right - wr.left, wr.bottom - wr.top,
                                nullptr, nullptr, hInstance, nullptr);

    if (!hWnd)
        return 0;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return (int)msg.wParam;
}
