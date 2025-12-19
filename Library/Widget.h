/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_WIDGET_H__
#define __NOVADESK_WIDGET_H__

#include <windows.h>
#include <string>
#include "System.h"

struct WidgetOptions
{
    int width;
    int height;
    std::wstring backgroundColor;
    ZPOSITION zPos;
    BYTE alpha;
    COLORREF color;
    bool draggable;
    bool clickThrough;
    bool keepOnScreen;
    bool snapEdges;
};

class Widget
{
public:
    /*
    ** Construct a new Widget with the specified options.
    ** Options include size, position, colors, z-order, and behavior flags.
    */
    Widget(const WidgetOptions& options);

    /*
    ** Destructor. Cleans up window resources and removes from system tracking.
    */
    ~Widget();

    /*
    ** Create the widget window.
    ** Registers the window class if needed and creates the actual window.
    ** Returns true on success, false on failure.
    */
    bool Create();

    /*
    ** Show the widget window.
    ** Makes the window visible and applies the configured z-order position.
    */
    void Show();

    /*
    ** Change the z-order position of this widget.
    ** If all is true, affects all widgets in the same z-order group.
    */
    void ChangeZPos(ZPOSITION zPos, bool all = false);

    /*
    ** Change the z-order position of a single widget.
    ** Similar to ChangeZPos but only affects this specific widget.
    */
    void ChangeSingleZPos(ZPOSITION zPos, bool all = false);
    
    /*
    ** Get the window handle for this widget.
    */
    HWND GetWindow() const { return m_hWnd; }

    /*
    ** Get the current z-order position of this widget.
    */
    ZPOSITION GetWindowZPosition() const { return m_WindowZPosition; }

private:
    /*
    ** Window procedure for handling widget window messages.
    ** Handles painting, mouse input, dragging, and z-order management.
    */
    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    /*
    ** Register the widget window class.
    ** Only needs to be called once per application instance.
    */
    static bool Register();

    /*
    ** Retrieve the Widget instance associated with a window handle.
    */
    static Widget* GetWidgetFromHWND(HWND hWnd);
    
    HWND m_hWnd;
    WidgetOptions m_Options;
    ZPOSITION m_WindowZPosition;
    HBRUSH m_hBackBrush;

    static const UINT_PTR TIMER_TOPMOST = 2;
};

#endif
