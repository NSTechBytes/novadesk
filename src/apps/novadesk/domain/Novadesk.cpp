/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "framework.h"
#include "Novadesk.h"
#include "Widget.h"
#include "DesktopManager.h"
#include "Settings.h"
#include "Resource.h"
#include <vector>
#include <shellapi.h>
#include <fcntl.h>
#include <io.h>
#include "../shared/MenuUtils.h"
#include <windef.h>
#include "Utils.h"
#include "PathUtils.h"
#include <commctrl.h>
#include "Direct2DHelper.h"
#include "FontManager.h"
#include "../shared/Logging.h"
#include "../scripting/quickjs/engine/JSEngine.h"

#pragma comment(lib, "comctl32.lib")

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
duk_context *ctx = nullptr;
std::vector<Widget*> widgets;
NOTIFYICONDATAW nid = {};

// Forward declarations of functions included in this code module:
void InitTrayIcon(HWND hWnd);
void RemoveTrayIcon();
void ShowTrayMenu(HWND hWnd);
void ShowTrayIconDynamic();
void HideTrayIconDynamic();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    // Attach to parent console for logging if present
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fDummy;
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        _setmode(_fileno(stdout), _O_U16TEXT);
    }

    // Clear log file on startup
    std::wstring logPath = PathUtils::GetAppDataPath() + L"logs.log";
    DeleteFileW(logPath.c_str());

    // Enable DPI awareness with runtime fallback for older SDK headers/toolchains.
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        using SetProcessDpiAwarenessContextFn = BOOL(WINAPI*)(HANDLE);
        using SetProcessDPIAwareFn = BOOL(WINAPI*)();
        auto setDpiCtx = reinterpret_cast<SetProcessDpiAwarenessContextFn>(
            GetProcAddress(user32, "SetProcessDpiAwarenessContext"));
        if (setDpiCtx) {
            HANDLE kPerMonitorAwareV2 = reinterpret_cast<HANDLE>(static_cast<intptr_t>(-4));
            setDpiCtx(kPerMonitorAwareV2);
        } else {
            auto setDpiAware = reinterpret_cast<SetProcessDPIAwareFn>(
                GetProcAddress(user32, "SetProcessDPIAware"));
            if (setDpiAware) {
                setDpiAware();
            }
        }
    }

    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(nCmdShow);

    std::wstring appTitle = PathUtils::GetProductName();
    std::wstring mutexName = L"Global\\NovadeskMutex_" + appTitle;
    std::wstring className = L"NovadeskTrayClass_" + appTitle;

    // Single instance enforcement
    // We use a NULL DACL to allow both Admin and User instances to share the same Mutex
    SECURITY_DESCRIPTOR sd;
    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);
    SetSecurityDescriptorDacl(&sd, TRUE, NULL, FALSE);
    SECURITY_ATTRIBUTES sa = { sizeof(sa), &sd, FALSE };

    HANDLE hMutex = CreateMutexW(&sa, TRUE, mutexName.c_str());
    DWORD dwError = GetLastError();

    if (dwError == ERROR_ALREADY_EXISTS || dwError == ERROR_ACCESS_DENIED)
    {
        // Another instance is running, check arguments for commands
        bool handledCommand = false;
        int argc = 0;
        LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
        if (argv && argc > 1)
        {
            std::wstring cmd;
            for (int i = 1; i < argc; ++i) {
                if (!cmd.empty()) cmd += L" ";
                cmd += argv[i];
            }
            HWND hExisting = FindWindowW(className.c_str(), NULL);

            if (hExisting)
            {
                if (cmd.find(L"/exit") != std::wstring::npos || 
                         cmd.find(L"-exit") != std::wstring::npos ||
                         cmd.find(L"--exit") != std::wstring::npos)
                {
                    SendMessage(hExisting, WM_COMMAND, ID_TRAY_EXIT, 0);
                    handledCommand = true;
                }
            }
        }
        if (argv) {
            LocalFree(argv);
        }

        if (!handledCommand)
        {
            std::wstring message = appTitle + L" is already running.";
            MessageBoxW(nullptr, message.c_str(), appTitle.c_str(), MB_OK | MB_ICONINFORMATION);
        }

        if (hMutex) CloseHandle(hMutex);
        return 0;
    }

    Logging::Log(LogLevel::Info, L"Application starting...");

    // Initialize Common Controls
    INITCOMMONCONTROLSEX icce;
    icce.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icce.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&icce);

    System::Initialize(hInstance);

    // Initialize Direct2D
    Direct2D::Initialize();
    FontManager::Initialize();

    // Initialize Settings
    Settings::Initialize();

    // Initialize global strings
    wcscpy_s(szTitle, MAX_LOADSTRING, appTitle.c_str());
    hInst = hInstance;

    // Create a hidden message-only window for tray icon
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.lpfnWndProc = [](HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) -> LRESULT {
        // Route custom JSEngine messages and timers
        JSEngine::OnMessage(message, wParam, lParam);

        switch (message)
        {
        case WM_TIMER:
            JSEngine::OnTimer(wParam);
            break;
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP)
            {
                ShowTrayMenu(hWnd);
            }
            break;
        case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            if (wmId == ID_TRAY_EXIT)
            {
                DestroyWindow(hWnd);
            }
            else if (wmId == ID_TRAY_REFRESH)
            {
                JSEngine::Reload();
            }
            else if (wmId >= 2000)
            {
                JSEngine::OnTrayCommand(wmId);
            }

        }
        break;
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
        }
        return 0;
    };
    wcex.hInstance = hInstance;
    wcex.lpszClassName = className.c_str();

    RegisterClassExW(&wcex);

    HWND hWnd = CreateWindowW(className.c_str(), szTitle, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

    if (!hWnd) {
        Logging::Log(LogLevel::Error, L"Failed to create message window");
        return FALSE;
    }

    // Initialize tray icon and JS message routing
    JSEngine::SetMessageWindow(hWnd);
    InitTrayIcon(hWnd);

    // Scripting runtime context is handled by the migrated QuickJS path.
    ctx = nullptr;

    // Initialize JavaScript API
    JSEngine::InitializeJavaScriptAPI(ctx);

    // Parse command line for custom script path using argv semantics.
    // argv[0] is the executable path; the first user arg is argv[1].
    std::wstring scriptPath;
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv)
    {
        if (argc > 1)
        {
            scriptPath = argv[1];
            if (!scriptPath.empty())
            {
                Logging::Log(LogLevel::Info, L"Using custom script: %s", scriptPath.c_str());
            }
        }
        LocalFree(argv);
    }

    // Load and execute script (with optional custom path)
    if (!JSEngine::LoadAndExecuteScript(ctx, scriptPath.empty() ? L"" : scriptPath))
    {
        Logging::Log(LogLevel::Error, L"Script execution failed. See QuickJS exception logs above.");
    }

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    std::vector<Widget*> widgetsCopy = widgets;
    widgets.clear();
    for (auto w : widgetsCopy) delete w;
    
    ctx = nullptr;
    System::Finalize();
    
    // Convert GDI+ shutdown
    FontManager::Cleanup();
    Direct2D::Cleanup();

    // Close mutex
    if (hMutex) CloseHandle(hMutex);

    return (int) msg.wParam;
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR, int nCmdShow)
{
    // Forward an empty command line; wWinMain now reads argv directly via GetCommandLineW().
    return wWinMain(hInstance, hPrevInstance, nullptr, nCmdShow);
}

void InitTrayIcon(HWND hWnd)
{
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NOVADESK));
    wcscpy_s(nid.szTip, _countof(nid.szTip), szTitle);

    if (Settings::GetGlobalBool("hideTrayIcon", false)) {
        Logging::Log(LogLevel::Info, L"Tray icon hidden by settings");
        return;
    }

    Shell_NotifyIconW(NIM_ADD, &nid);
    Logging::Log(LogLevel::Info, L"Tray icon initialized");
}

void RemoveTrayIcon()
{
    Shell_NotifyIconW(NIM_DELETE, &nid);
    Logging::Log(LogLevel::Info, L"Tray icon removed");
}

std::vector<MenuItem> g_TrayMenu;
bool g_ShowDefaultTrayItems = true;

void ShowTrayMenu(HWND hWnd)
{
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();

    MenuUtils::BuildMenu(hMenu, g_TrayMenu);

    if (g_ShowDefaultTrayItems)
    {
        if (!g_TrayMenu.empty())
        {
        AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
        }
        
        HMENU hSubMenu = CreatePopupMenu();
        AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_REFRESH, L"Refresh");
        AppendMenuW(hSubMenu, MF_STRING, ID_TRAY_EXIT, L"Exit");
        
        AppendMenuW(hMenu, MF_POPUP, (UINT_PTR)hSubMenu, szTitle);
    }

    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hWnd, nullptr);
    DestroyMenu(hMenu);
}

void ShowTrayIconDynamic()
{
    if (nid.hWnd)
    {
        Shell_NotifyIconW(NIM_ADD, &nid);
        Logging::Log(LogLevel::Info, L"Tray icon shown");
    }
}

void HideTrayIconDynamic()
{
    if (nid.hWnd)
    {
        Shell_NotifyIconW(NIM_DELETE, &nid);
        Logging::Log(LogLevel::Info, L"Tray icon hidden");
    }
}


void SetTrayMenu(const std::vector<MenuItem>& menu)
{
    g_TrayMenu = menu;
}

void ClearTrayMenu()
{
    g_TrayMenu.clear();
}

void SetShowDefaultTrayItems(bool show)
{
    g_ShowDefaultTrayItems = show;
}
