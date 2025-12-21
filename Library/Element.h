/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ELEMENT_H__
#define __NOVADESK_ELEMENT_H__

#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <string>

/*
** Content type enumeration.
*/
enum ElementType
{
    ELEMENT_IMAGE,
    ELEMENT_TEXT
};

/*
** Base class for all widget elements (Text, Image, etc.)
*/
class Element
{
public:
    Element(ElementType type, const std::wstring& id, int x, int y, int width, int height)
        : m_Type(type), m_Id(id), m_X(x), m_Y(y)
    {
        m_Width = (width > 0) ? width : 0;
        m_Height = (height > 0) ? height : 0;
        m_WDefined = (width > 0);
        m_HDefined = (height > 0);
    }

    virtual ~Element() {}

    /*
    ** Render the element to the GDI+ Graphics context.
    */
    virtual void Render(Gdiplus::Graphics& graphics) = 0;

    /*
    ** Getters
    */
    ElementType GetType() const { return m_Type; }
    const std::wstring& GetId() const { return m_Id; }
    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }
    
    int GetWidth() { 
        if (m_WDefined) return m_Width;
        return GetAutoWidth();
    }
    
    int GetHeight() { 
        if (m_HDefined) return m_Height;
        return GetAutoHeight();
    }

    /*
    ** Setters
    */
    void SetPosition(int x, int y) { m_X = x; m_Y = y; }
    void SetSize(int w, int h) { 
        m_Width = w; 
        m_Height = h; 
        m_WDefined = (w > 0);
        m_HDefined = (h > 0);
    }

    /*
    ** Auto-size calculation (to be overriden by subclasses)
    */
    virtual int GetAutoWidth() { return 0; }
    virtual int GetAutoHeight() { return 0; }

    /*
    ** Check if a point is within the element's bounds.
    */
    virtual bool HitTest(int x, int y) {
        int w = GetWidth();
        int h = GetHeight();
        return (x >= m_X && x < m_X + w && y >= m_Y && y < m_Y + h);
    }

    /*
    ** Check if this element should be hit even if it's transparent.
    ** (e.g. for SolidColor in Rainmeter)
    */
    virtual bool IsTransparentHit() const { return false; }

    /*
    ** Check if the element has an action associated with a specific mouse message.
    */
    bool HasAction(UINT message, WPARAM wParam) const {
        switch (message)
        {
        case WM_LBUTTONUP:     return !m_OnLeftMouseUp.empty();
        case WM_LBUTTONDOWN:   return !m_OnLeftMouseDown.empty();
        case WM_LBUTTONDBLCLK: return !m_OnLeftDoubleClick.empty();
        case WM_RBUTTONUP:     return !m_OnRightMouseUp.empty();
        case WM_RBUTTONDOWN:   return !m_OnRightMouseDown.empty();
        case WM_RBUTTONDBLCLK: return !m_OnRightDoubleClick.empty();
        case WM_MBUTTONUP:     return !m_OnMiddleMouseUp.empty();
        case WM_MBUTTONDOWN:   return !m_OnMiddleMouseDown.empty();
        case WM_MBUTTONDBLCLK: return !m_OnMiddleDoubleClick.empty();
        case WM_XBUTTONUP:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return !m_OnX1MouseUp.empty();
            else return !m_OnX2MouseUp.empty();
        case WM_XBUTTONDOWN:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return !m_OnX1MouseDown.empty();
            else return !m_OnX2MouseDown.empty();
        case WM_XBUTTONDBLCLK:
            if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return !m_OnX1DoubleClick.empty();
            else return !m_OnX2DoubleClick.empty();
        case WM_MOUSEWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return !m_OnScrollUp.empty();
            else return !m_OnScrollDown.empty();
        case WM_MOUSEHWHEEL:
            if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return !m_OnScrollRight.empty();
            else return !m_OnScrollLeft.empty();
        case WM_MOUSEMOVE:
            return !m_OnMouseOver.empty() || !m_OnMouseLeave.empty();
        }
        return false;
    }

    // Mouse Actions
    std::wstring m_OnLeftMouseUp;
    std::wstring m_OnLeftMouseDown;
    std::wstring m_OnLeftDoubleClick;
    std::wstring m_OnRightMouseUp;
    std::wstring m_OnRightMouseDown;
    std::wstring m_OnRightDoubleClick;
    std::wstring m_OnMiddleMouseUp;
    std::wstring m_OnMiddleMouseDown;
    std::wstring m_OnMiddleDoubleClick;
    std::wstring m_OnX1MouseUp;
    std::wstring m_OnX1MouseDown;
    std::wstring m_OnX1DoubleClick;
    std::wstring m_OnX2MouseUp;
    std::wstring m_OnX2MouseDown;
    std::wstring m_OnX2DoubleClick;
    std::wstring m_OnScrollUp;
    std::wstring m_OnScrollDown;
    std::wstring m_OnScrollLeft;
    std::wstring m_OnScrollRight;
    std::wstring m_OnMouseOver;
    std::wstring m_OnMouseLeave;

    bool m_IsMouseOver = false;

protected:
    ElementType m_Type;
    std::wstring m_Id;
    int m_X, m_Y;
    int m_Width, m_Height;
    bool m_WDefined, m_HDefined;
};

#endif
