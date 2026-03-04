/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

/*
** Most of the Code taken from Rainmeter (https://github.com/rainmeter/rainmeter/blob/master/Library/System.cpp)
*/

#include "DesktopManager.h"
#include "Widget.h"
#include "../shared/System.h"
#include <algorithm>

extern std::vector<Widget *> widgets; // Defined in Novadesk.cpp

HWND System::c_Window = nullptr;
HWND System::c_HelperWindow = nullptr;
MultiMonitorInfo System::c_Monitors = {0};
bool System::c_ShowDesktop = false;

#define ZPOS_FLAGS (SWP_NOMOVE | SWP_NOSIZE | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_NOSENDCHANGING)

// Internal helper for GetBackmostTopWindow
static Widget *FindWidget(HWND hWnd)
{
    for (auto w : widgets)
    {
        if (w->GetWindow() == hWnd)
            return w;
    }
    return nullptr;
}

/*
** Initialize the System module.
** Sets up helper windows and initializes multi-monitor information.
*/

void System::Initialize(HINSTANCE instance)
{
    // Initialize monitors from shared system metrics to keep a single monitor source of truth.
    c_Monitors.monitors.clear();
    const auto metrics = novadesk::shared::system::GetDisplayMetrics();
    c_Monitors.vsL = metrics.virtualLeft;
    c_Monitors.vsT = metrics.virtualTop;
    c_Monitors.vsW = metrics.virtualWidth;
    c_Monitors.vsH = metrics.virtualHeight;
    c_Monitors.primaryIndex = metrics.primaryIndex;
    c_Monitors.primary = c_Monitors.primaryIndex + 1; // Keep legacy 1-based primary field behavior.

    c_Monitors.monitors.reserve(metrics.monitors.size());
    for (const auto &m : metrics.monitors)
    {
        MonitorInfo info{};
        info.active = m.active;
        info.handle = nullptr; // Handle is not provided by shared metrics.
        info.work.left = m.work.left;
        info.work.top = m.work.top;
        info.work.right = m.work.right;
        info.work.bottom = m.work.bottom;
        info.screen.left = m.screen.left;
        info.screen.top = m.screen.top;
        info.screen.right = m.screen.right;
        info.screen.bottom = m.screen.bottom;
        info.deviceName = m.deviceName;
        info.monitorName = m.monitorName;
        c_Monitors.monitors.push_back(std::move(info));
    }

    // Register a specialized class for system tracking
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = (WNDPROC)System::WndProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"NovadeskSystem";
    RegisterClassW(&wc);

    // Create a dummy window for System tracking
    c_Window = CreateWindowExW(
        WS_EX_TOOLWINDOW,
        L"NovadeskSystem",
        L"System",
        WS_POPUP | WS_DISABLED,
        0, 0, 0, 0,
        nullptr, nullptr, instance, nullptr);

    PrepareHelperWindow();

    SetTimer(c_Window, TIMER_SHOWDESKTOP, INTERVAL_SHOWDESKTOP, nullptr);
}

/*
** Finalize the System module.
** Cleans up resources and destroys helper windows.
*/

void System::Finalize()
{
    if (c_HelperWindow)
    {
        DestroyWindow(c_HelperWindow);
        c_HelperWindow = nullptr;
    }
    if (c_Window)
    {
        DestroyWindow(c_Window);
        c_Window = nullptr;
    }
}

/*
** Get the default shell window.
*/

HWND System::GetDefaultShellWindow()
{
    return GetShellWindow();
}

/*
** Determine if the shell window should be used as the desktop icons host.
*/

bool System::ShouldUseShellWindowAsDesktopIconsHost()
{
    // Check for the existence of GetCurrentMonitorTopologyId, which should be present only
    // on Windows 11 build 10.0.26100.2454.
    static bool result = GetProcAddress(GetModuleHandleW(L"user32"), "GetCurrentMonitorTopologyId") != nullptr;
    return result;
}

/*
** Get the window that hosts desktop icons.
** This is typically the shell window or WorkerW window.
*/

HWND System::GetDesktopIconsHostWindow()
{
    HWND shellW = GetDefaultShellWindow();
    if (!shellW)
        return nullptr;

    if (ShouldUseShellWindowAsDesktopIconsHost())
    {
        if (FindWindowExW(shellW, nullptr, L"SHELLDLL_DefView", L""))
        {
            return shellW;
        }
    }

    HWND workerW = nullptr;
    HWND defView = nullptr;
    while (workerW = FindWindowExW(nullptr, workerW, L"WorkerW", L""))
    {
        if (IsWindowVisible(workerW) &&
            (defView = FindWindowExW(workerW, nullptr, L"SHELLDLL_DefView", L"")))
        {
            break;
        }
    }

    return (defView != nullptr) ? workerW : nullptr;
}

/*
** Prepare the helper window for desktop icon management.
** Can optionally specify a custom desktop icons host window.
*/

void System::PrepareHelperWindow(HWND desktopIconsHostWindow)
{
    if (!c_HelperWindow || !IsWindow(c_HelperWindow))
    {
        c_HelperWindow = CreateWindowExW(
            WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
            L"NovadeskSystem", L"PositioningHelper",
            WS_POPUP,
            0, 0, 0, 0,
            nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    }

    if (!desktopIconsHostWindow)
        desktopIconsHostWindow = GetDesktopIconsHostWindow();

    SetWindowPos(c_Window, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);

    if (c_ShowDesktop && desktopIconsHostWindow)
    {
        SetWindowPos(c_HelperWindow, HWND_TOPMOST, 0, 0, 0, 0, ZPOS_FLAGS);
    }
    else if (desktopIconsHostWindow)
    {
        SetWindowPos(c_HelperWindow, desktopIconsHostWindow, 0, 0, 0, 0, ZPOS_FLAGS);
    }
    else
    {
        SetWindowPos(c_HelperWindow, HWND_BOTTOM, 0, 0, 0, 0, ZPOS_FLAGS);
    }
}

/*
** Check if the desktop is currently being shown.
*/

bool System::CheckDesktopState(HWND desktopIconsHostWindow)
{
    HWND hwnd = nullptr;

    if (desktopIconsHostWindow && IsWindowVisible(desktopIconsHostWindow))
    {
        hwnd = FindWindowExW(nullptr, desktopIconsHostWindow, L"NovadeskSystem", L"System");
    }

    bool stateChanged = (hwnd && !c_ShowDesktop) || (!hwnd && c_ShowDesktop);

    if (stateChanged)
    {
        c_ShowDesktop = !c_ShowDesktop;

        PrepareHelperWindow(desktopIconsHostWindow);
        ChangeZPosInOrder();

        SetTimer(c_Window, TIMER_SHOWDESKTOP, c_ShowDesktop ? INTERVAL_RESTOREWINDOWS : INTERVAL_SHOWDESKTOP, nullptr);
    }

    return stateChanged;
}

static BOOL CALLBACK EnumWidgetsProc(HWND hwnd, LPARAM lParam)
{
    std::vector<Widget *> *windowsInZOrder = (std::vector<Widget *> *)lParam;
    Widget *widget = FindWidget(hwnd);
    if (widget)
    {
        windowsInZOrder->push_back(widget);
    }
    return TRUE;
}

/*
** Change z-order positions for all widgets in the correct order.
** Ensures widgets maintain their relative z-order positions.
*/

void System::ChangeZPosInOrder()
{
    std::vector<Widget *> windowsInZOrder;

    // Enumerate all windows to get current Z-order
    EnumWindows(EnumWidgetsProc, (LPARAM)(&windowsInZOrder));

    // Reapply Z-positions in the order they were found (preserves user's stacking)
    for (auto w : windowsInZOrder)
    {
        w->ChangeZPos(w->GetWindowZPosition());
    }
}

/*
** Window procedure for the system helper window.
*/

LRESULT CALLBACK System::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_TIMER:
        if (wParam == TIMER_SHOWDESKTOP)
        {
            CheckDesktopState(GetDesktopIconsHostWindow());
        }
        break;
    }
    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

/*
** Get the backmost top-level window.
** Used for z-order management of desktop widgets.
*/

HWND System::GetBackmostTopWindow()
{
    HWND winPos = c_HelperWindow;
    if (!winPos)
        return HWND_NOTOPMOST;

    // Skip all ZPOSITION_ONDESKTOP, ZPOSITION_ONBOTTOM, and ZPOSITION_NORMAL windows
    while (winPos = ::GetNextWindow(winPos, GW_HWNDPREV))
    {
        Widget *wnd = FindWidget(winPos);
        if (!wnd ||
            (wnd->GetWindowZPosition() != ZPOSITION_NORMAL &&
             wnd->GetWindowZPosition() != ZPOSITION_ONDESKTOP &&
             wnd->GetWindowZPosition() != ZPOSITION_ONBOTTOM))
        {
            break;
        }
    }

    return winPos;
}
