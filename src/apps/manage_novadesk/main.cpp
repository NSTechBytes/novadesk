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
#include <filesystem>
#include <algorithm>
#include <sstream>
#include <fstream>

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
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring cmdLine = L"\"" + exe + L"\"";

    if (CreateProcessW(nullptr, cmdLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
    {
        g_novadeskProcess = pi;
        g_novadeskRunning = true;
        LogLine(L"[Manage] Started Novadesk: " + exe);
    }
    else
    {
        DWORD err = GetLastError();
        LogLine(L"[Manage] Failed to start Novadesk (" + std::to_wstring(err) + L"): " + Win32ErrorToString(err));
    }
}

static void StopNovadesk()
{
    if (!g_novadeskRunning)
        return;

    TerminateProcess(g_novadeskProcess.hProcess, 0);
    CloseHandle(g_novadeskProcess.hProcess);
    CloseHandle(g_novadeskProcess.hThread);
    g_novadeskProcess = {};
    g_novadeskRunning = false;
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

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
    {
        LogLine(L"[Manage] Window created.");
        RECT rc{};
        GetClientRect(hWnd, &rc);

        StartNovadesk();
        AddTrayIcon(hWnd);

        g_buttonFont = CreateFontW(-12, 0, 0, 0, FW_LIGHT, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");

        g_list = CreateWindowExW(0, WC_LISTVIEWW, L"",
                                 WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL,
                                 10, 10, rc.right - 20, rc.bottom - 60,
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

        g_btnRefreshList = CreateWindowW(L"BUTTON", L"Refresh List", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                         10, rc.bottom - 40, 110, 28, hWnd, (HMENU)1, GetModuleHandleW(nullptr), nullptr);
        g_btnLoad = CreateWindowW(L"BUTTON", L"Load", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                  130, rc.bottom - 40, 80, 28, hWnd, (HMENU)2, GetModuleHandleW(nullptr), nullptr);
        g_btnUnload = CreateWindowW(L"BUTTON", L"Unload", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                    220, rc.bottom - 40, 80, 28, hWnd, (HMENU)3, GetModuleHandleW(nullptr), nullptr);
        g_btnRefresh = CreateWindowW(L"BUTTON", L"Refresh", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                                     310, rc.bottom - 40, 80, 28, hWnd, (HMENU)4, GetModuleHandleW(nullptr), nullptr);

        if (g_buttonFont)
        {
            SendMessageW(g_btnRefreshList, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnLoad, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnUnload, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
            SendMessageW(g_btnRefresh, WM_SETFONT, (WPARAM)g_buttonFont, TRUE);
        }

        RefreshListView();
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
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1:
            RefreshListView();
            break;
        case 2:
            OnLoadSelected();
            break;
        case 3:
            OnUnloadSelected();
            break;
        case 4:
            OnRefreshSelected();
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
        break;
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
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    const wchar_t *className = L"NovadeskManagerWindow";
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
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
