/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ELEMENT_H__
#define __NOVADESK_ELEMENT_H__

#include <windows.h>
#include <objidl.h>
#include <d2d1_1.h>
#include <string>
#include <vector>
#include <wrl/client.h>

// Helper macros for color extraction from COLORREF (0x00BBGGRR)
#ifndef GetRValue
#define GetRValue(rgb)      (LOBYTE(rgb))
#endif
#ifndef GetGValue
#define GetGValue(rgb)      (LOBYTE((WORD)(rgb) >> 8))
#endif
#ifndef GetBValue
#define GetBValue(rgb)      (LOBYTE((rgb) >> 16))
#endif

enum ElementType
{
    ELEMENT_IMAGE,
    ELEMENT_TEXT,
    ELEMENT_BAR,
    ELEMENT_LINE,
    ELEMENT_ROUNDLINE,
    ELEMENT_SHAPE,
    ELEMENT_HISTOGRAM,
    ELEMENT_BITMAP,
    ELEMENT_BUTTON,
    ELEMENT_ROTATOR,
    ELEMENT_AREA_GRAPH,
    ELEMENT_LAYOUT_BOX,
    ELEMENT_INPUT_BOX,
    ELEMENT_COLOR_PICKER
};

struct GfxRect {
    int X, Y, Width, Height;
    GfxRect() : X(0), Y(0), Width(0), Height(0) {}
    GfxRect(int x, int y, int w, int h) : X(x), Y(y), Width(w), Height(h) {}
};

struct TextShadow {
    float offsetX = 0;
    float offsetY = 0;
    float blur = 0;
    COLORREF color = 0;
    BYTE alpha = 255;
};

struct GradientStop {
    COLORREF color;
    BYTE alpha;
    float position;
};

enum GradientType {
    GRADIENT_NONE,
    GRADIENT_LINEAR,
    GRADIENT_RADIAL
};

struct GradientInfo {
    GradientType type = GRADIENT_NONE;
    std::vector<GradientStop> stops;
    float angle = 0.0f; // For linear
    std::wstring shape = L"circle"; // For radial
};

enum TextCase {
    TEXT_CASE_NORMAL,
    TEXT_CASE_UPPER,
    TEXT_CASE_LOWER,
    TEXT_CASE_CAPITALIZE,
    TEXT_CASE_SENTENCE
};

struct BackdropFilter
{
    float blur = 0.0f;
    float brightness = 1.0f;
    float contrast = 1.0f;
    float grayscale = 0.0f;
    float saturate = 1.0f;
    float sepia = 0.0f;
    float hueRotate = 0.0f;
    float invert = 0.0f;
    float opacity = 1.0f;

    bool IsActive() const;
};

class Element
{
public:
    void SetBackdropFilter(const BackdropFilter &filter) { m_BackdropFilter = filter; }
    const BackdropFilter &GetBackdropFilter() const { return m_BackdropFilter; }
    Element(ElementType type, const std::wstring& id, int x, int y, int width, int height);
    virtual ~Element();

    virtual void Render(ID2D1DeviceContext* context) = 0;

    ElementType GetType() const { return m_Type; }
    const std::wstring& GetId() const { return m_Id; }
    int GetX() const { return m_X; }
    int GetY() const { return m_Y; }
    
    int GetWidth();
    int GetHeight();

    bool IsWDefined() const { return m_WDefined; }
    bool IsHDefined() const { return m_HDefined; }

    void SetPosition(int x, int y) { m_X = x; m_Y = y; }
    void SetSize(int w, int h) { 
        m_Width = w; 
        m_Height = h; 
        m_WDefined = (w > 0);
        m_HDefined = (h > 0);
    }

    virtual int GetAutoWidth() { return 0; }
    virtual int GetAutoHeight() { return 0; }

    void SetOwnerHWND(HWND hWnd)
    {
        m_OwnerHWND = hWnd;
        OnOwnerHWNDSet();
    }
    HWND GetOwnerHWND() const { return m_OwnerHWND; }

    virtual void OnOwnerHWNDSet() {}
    virtual void OnImageDownloaded(const std::wstring& url, const std::vector<BYTE>& buffer) {}
    virtual std::wstring GetImageUrl() const { return L""; }

    virtual GfxRect GetBounds();
    virtual GfxRect GetBackgroundBounds();

    virtual bool HitTest(int x, int y);

    void SetSolidColor(COLORREF color, BYTE alpha) { 
        m_SolidColor = color; 
        m_SolidAlpha = alpha; 
        m_HasSolidColor = true; 
    }

    void SetSolidGradient(const GradientInfo& gradient) {
        m_SolidGradient = gradient;
        m_HasSolidColor = true;
    }

    void SetCornerRadius(int radius) { 
        m_CornerRadius = radius; 
    }

    void SetBevel(int type, int width, COLORREF color, BYTE alpha, COLORREF color2, BYTE alpha2) {
        m_BevelType = type;
        m_BevelWidth = width;
        m_BevelColor = color;
        m_BevelAlpha = alpha;
        m_BevelColor2 = color2;
        m_BevelAlpha2 = alpha2;
    }
    void SetBevelGradient(const GradientInfo& gradient) { m_BevelGradient = gradient; }
    void SetBevelGradient2(const GradientInfo& gradient) { m_BevelGradient2 = gradient; }

    void SetAntiAlias(bool enable) { m_AntiAlias = enable; }
    void SetPixelHitTest(bool enabled) { m_PixelHitTest = enabled; }
    bool GetPixelHitTest() const { return m_PixelHitTest; }
    
    void SetPadding(int left, int top, int right, int bottom);

    void SetRotate(float angle) { m_Rotate = angle; }
    float GetRotate() const { return m_Rotate; }

    void SetTransformMatrix(const float* matrix) {
        if (matrix) {
            memcpy(m_TransformMatrix, matrix, sizeof(float) * 6);
            m_HasTransformMatrix = true;
        } else {
            m_HasTransformMatrix = false;
        }
    }
    bool HasTransformMatrix() const { return m_HasTransformMatrix; }
    const float* GetTransformMatrix() const { return m_TransformMatrix; }

    bool HasSolidColor() const { return m_HasSolidColor; }
    COLORREF GetSolidColor() const { return m_SolidColor; }
    BYTE GetSolidAlpha() const { return m_SolidAlpha; }
    int GetCornerRadius() const { return m_CornerRadius; }

    const GradientInfo& GetSolidGradient() const { return m_SolidGradient; }

    int GetBevelType() const { return m_BevelType; }
    int GetBevelWidth() const { return m_BevelWidth; }
    COLORREF GetBevelColor() const { return m_BevelColor; }
    BYTE GetBevelAlpha() const { return m_BevelAlpha; }
    COLORREF GetBevelColor2() const { return m_BevelColor2; }
    BYTE GetBevelAlpha2() const { return m_BevelAlpha2; }
    const GradientInfo& GetBevelGradient() const { return m_BevelGradient; }
    const GradientInfo& GetBevelGradient2() const { return m_BevelGradient2; }

    int GetPaddingLeft() const { return m_PaddingLeft; }
    int GetPaddingTop() const { return m_PaddingTop; }
    int GetPaddingRight() const { return m_PaddingRight; }
    int GetPaddingBottom() const { return m_PaddingBottom; }

    bool GetAntiAlias() const { return m_AntiAlias; }

    void SetShow(bool show) { m_Show = show; }
    bool IsVisible() const { return m_Show; }

    void SetContainerId(const std::wstring& id) { m_ContainerId = id; }
    const std::wstring& GetContainerId() const { return m_ContainerId; }
    void SetGroupId(const std::wstring& id) { m_GroupId = id; }
    const std::wstring& GetGroupId() const { return m_GroupId; }
    void SetMouseEventCursor(bool enabled) { m_MouseEventCursor = enabled; }
    bool GetMouseEventCursor() const { return m_MouseEventCursor; }
    void SetMouseEventCursorName(const std::wstring& name) { m_MouseEventCursorName = name; }
    const std::wstring& GetMouseEventCursorName() const { return m_MouseEventCursorName; }
    void SetCursorsDir(const std::wstring& dir) { m_CursorsDir = dir; }
    const std::wstring& GetCursorsDir() const { return m_CursorsDir; }
    void SetContainer(Element* container) { m_ContainerElement = container; }
    Element* GetContainer() const { return m_ContainerElement; }
    bool IsContained() const { return m_ContainerElement != nullptr; }

    void AddContainerItem(Element* item) { m_ContainerItems.push_back(item); }
    void RemoveContainerItem(Element* item);
    void ClearContainerItems();
    const std::vector<Element*>& GetContainerItems() const { return m_ContainerItems; }
    bool IsContainer() const { return !m_ContainerItems.empty(); }

    // Scroll properties
    int GetScrollX() const { return m_ScrollX; }
    int GetScrollY() const { return m_ScrollY; }
    void SetScrollX(int x);
    void SetScrollY(int y);
    int GetScrollStep() const { return m_ScrollStep; }
    void SetScrollStep(int step) { m_ScrollStep = step > 0 ? step : 1; }
    int GetContentWidth() const { return m_ContentWidth; }
    int GetContentHeight() const { return m_ContentHeight; }
    int GetMaxScrollX() const;
    int GetMaxScrollY() const;
    void RecalcContentExtents();
    bool IsScrollableX() const;
    bool IsScrollableY() const;
    bool IsScrollable() const { return IsScrollableX() || IsScrollableY(); }

    // Overflow
    enum class OverflowMode { Hidden, Auto, Scroll };
    OverflowMode GetOverflowX() const { return m_OverflowX; }
    OverflowMode GetOverflowY() const { return m_OverflowY; }
    void SetOverflowX(OverflowMode mode) { m_OverflowX = mode; }
    void SetOverflowY(OverflowMode mode) { m_OverflowY = mode; }
    void SetOverflow(const std::wstring& value);

    // Scrollbar appearance
    bool GetShowScrollbar() const { return m_ShowScrollbar; }
    void SetShowScrollbar(bool show) { m_ShowScrollbar = show; }
    bool GetShowScrollbarX() const { return m_ShowScrollbarX; }
    void SetShowScrollbarX(bool show) { m_ShowScrollbarX = show; }
    bool GetShowScrollbarY() const { return m_ShowScrollbarY; }
    void SetShowScrollbarY(bool show) { m_ShowScrollbarY = show; }
    int GetScrollbarWidth() const { return m_ScrollbarWidth; }
    void SetScrollbarWidth(int w) { m_ScrollbarWidth = w > 0 ? w : 1; }
    int GetScrollbarHoverWidth() const { return m_ScrollbarHoverWidth > 0 ? m_ScrollbarHoverWidth : m_ScrollbarWidth; }
    void SetScrollbarHoverWidth(int w) { m_ScrollbarHoverWidth = w; }
    float GetScrollbarRadius() const { return m_ScrollbarRadius; }
    void SetScrollbarRadius(float r) { m_ScrollbarRadius = r; }
    float GetScrollbarTrackRadius() const { return m_ScrollbarTrackRadius >= 0.0f ? m_ScrollbarTrackRadius : m_ScrollbarRadius; }
    void SetScrollbarTrackRadius(float r) { m_ScrollbarTrackRadius = r; }
    float GetScrollbarInset() const { return m_ScrollbarInset; }
    void SetScrollbarInset(float inset) { m_ScrollbarInset = (inset >= 0.0f ? inset : 0.0f); }
    float GetScrollbarMinThumbLength() const { return m_ScrollbarMinThumbLength; }
    void SetScrollbarMinThumbLength(float minLen) { m_ScrollbarMinThumbLength = (minLen >= 4.0f ? minLen : 4.0f); }
    COLORREF GetScrollbarColor() const { return m_ScrollbarColor; }
    BYTE GetScrollbarAlpha() const { return m_ScrollbarAlpha; }
    void SetScrollbarColor(COLORREF color, BYTE alpha) { m_ScrollbarColor = color; m_ScrollbarAlpha = alpha; }
    bool HasScrollbarHoverColor() const { return m_HasScrollbarHoverColor; }
    COLORREF GetScrollbarHoverColor() const { return m_HasScrollbarHoverColor ? m_ScrollbarHoverColor : m_ScrollbarColor; }
    BYTE GetScrollbarHoverAlpha() const { return m_HasScrollbarHoverColor ? m_ScrollbarHoverAlpha : (BYTE)(std::min)(255, (int)m_ScrollbarAlpha + 50); }
    void SetScrollbarHoverColor(COLORREF color, BYTE alpha) { m_ScrollbarHoverColor = color; m_ScrollbarHoverAlpha = alpha; m_HasScrollbarHoverColor = true; }
    bool HasScrollbarActiveColor() const { return m_HasScrollbarActiveColor; }
    COLORREF GetScrollbarActiveColor() const { return m_HasScrollbarActiveColor ? m_ScrollbarActiveColor : GetScrollbarHoverColor(); }
    BYTE GetScrollbarActiveAlpha() const { return m_HasScrollbarActiveColor ? m_ScrollbarActiveAlpha : (BYTE)(std::min)(255, (int)GetScrollbarHoverAlpha() + 40); }
    void SetScrollbarActiveColor(COLORREF color, BYTE alpha) { m_ScrollbarActiveColor = color; m_ScrollbarActiveAlpha = alpha; m_HasScrollbarActiveColor = true; }
    COLORREF GetScrollbarTrackColor() const { return m_ScrollbarTrackColor; }
    BYTE GetScrollbarTrackAlpha() const { return m_ScrollbarTrackAlpha; }
    void SetScrollbarTrackColor(COLORREF color, BYTE alpha) { m_ScrollbarTrackColor = color; m_ScrollbarTrackAlpha = alpha; }

    // Scrollbar Arrow Buttons
    bool GetShowScrollbarButtons() const { return m_ShowScrollbarButtons; }
    void SetShowScrollbarButtons(bool show) { m_ShowScrollbarButtons = show; }
    float GetScrollbarButtonSize() const { return m_ScrollbarButtonSize > 0.0f ? m_ScrollbarButtonSize : (float)GetScrollbarWidth(); }
    void SetScrollbarButtonSize(float size) { m_ScrollbarButtonSize = size > 0.0f ? size : 0.0f; }
    float GetScrollbarButtonRadius() const { return m_ScrollbarButtonRadius >= 0.0f ? m_ScrollbarButtonRadius : 0.0f; }
    void SetScrollbarButtonRadius(float r) { m_ScrollbarButtonRadius = r; }
    COLORREF GetScrollbarArrowColor() const { return m_HasScrollbarArrowColor ? m_ScrollbarArrowColor : GetScrollbarColor(); }
    BYTE GetScrollbarArrowAlpha() const { return m_HasScrollbarArrowColor ? m_ScrollbarArrowAlpha : GetScrollbarAlpha(); }
    void SetScrollbarArrowColor(COLORREF color, BYTE alpha) { m_ScrollbarArrowColor = color; m_ScrollbarArrowAlpha = alpha; m_HasScrollbarArrowColor = true; }
    COLORREF GetScrollbarArrowHoverColor() const { return m_HasScrollbarArrowHoverColor ? m_ScrollbarArrowHoverColor : GetScrollbarHoverColor(); }
    BYTE GetScrollbarArrowHoverAlpha() const { return m_HasScrollbarArrowHoverColor ? m_ScrollbarArrowHoverAlpha : GetScrollbarHoverAlpha(); }
    void SetScrollbarArrowHoverColor(COLORREF color, BYTE alpha) { m_ScrollbarArrowHoverColor = color; m_ScrollbarArrowHoverAlpha = alpha; m_HasScrollbarArrowHoverColor = true; }
    COLORREF GetScrollbarArrowActiveColor() const { return m_HasScrollbarArrowActiveColor ? m_ScrollbarArrowActiveColor : GetScrollbarActiveColor(); }
    BYTE GetScrollbarArrowActiveAlpha() const { return m_HasScrollbarArrowActiveColor ? m_ScrollbarArrowActiveAlpha : GetScrollbarActiveAlpha(); }
    void SetScrollbarArrowActiveColor(COLORREF color, BYTE alpha) { m_ScrollbarArrowActiveColor = color; m_ScrollbarArrowActiveAlpha = alpha; m_HasScrollbarArrowActiveColor = true; }
    COLORREF GetScrollbarButtonBgColor() const { return m_ScrollbarButtonBgColor; }
    BYTE GetScrollbarButtonBgAlpha() const { return m_ScrollbarButtonBgAlpha; }
    void SetScrollbarButtonBgColor(COLORREF color, BYTE alpha) { m_ScrollbarButtonBgColor = color; m_ScrollbarButtonBgAlpha = alpha; }
    COLORREF GetScrollbarButtonHoverBgColor() const { return m_ScrollbarButtonHoverBgColor; }
    BYTE GetScrollbarButtonHoverBgAlpha() const { return m_ScrollbarButtonHoverBgAlpha; }
    void SetScrollbarButtonHoverBgColor(COLORREF color, BYTE alpha) { m_ScrollbarButtonHoverBgColor = color; m_ScrollbarButtonHoverBgAlpha = alpha; }

    virtual bool IsTransparentHit() const { return false; }

    bool HasAction(UINT message, WPARAM wParam) const;
    bool HasMouseAction() const;
    bool HasDragAction() const;

    // Tooltip properties
    void SetToolTip(const std::wstring& text, const std::wstring& title = L"", const std::wstring& icon = L"", int maxWidth = 0, int maxHeight = 0, bool balloon = false) {
        m_ToolTipText = text;
        m_ToolTipTitle = title;
        m_ToolTipIcon = icon;
        m_ToolTipMaxWidth = maxWidth;
        m_ToolTipMaxHeight = maxHeight;
        m_ToolTipBalloon = balloon;
    }

    const std::wstring& GetToolTipText() const { return m_ToolTipText; }
    const std::wstring& GetToolTipTitle() const { return m_ToolTipTitle; }
    const std::wstring& GetToolTipIcon() const { return m_ToolTipIcon; }
    int GetToolTipMaxWidth() const { return m_ToolTipMaxWidth; }
    int GetToolTipMaxHeight() const { return m_ToolTipMaxHeight; }
    bool GetToolTipBalloon() const { return m_ToolTipBalloon; }
    bool GetToolTipDisabled() const { return m_ToolTipDisabled; }
    void SetToolTipDisabled(bool disabled) { m_ToolTipDisabled = disabled; }

    bool HasToolTip() const { return !m_ToolTipText.empty() && !m_ToolTipDisabled; }

    // Mouse Actions
    
    // Callback IDs (initialized to -1)
    int m_OnLeftMouseUpCallbackId = -1;
    int m_OnLeftMouseDownCallbackId = -1;
    int m_OnLeftDoubleClickCallbackId = -1;
    int m_OnRightMouseUpCallbackId = -1;
    int m_OnRightMouseDownCallbackId = -1;
    int m_OnRightDoubleClickCallbackId = -1;
    int m_OnMiddleMouseUpCallbackId = -1;
    int m_OnMiddleMouseDownCallbackId = -1;
    int m_OnMiddleDoubleClickCallbackId = -1;
    int m_OnX1MouseUpCallbackId = -1;
    int m_OnX1MouseDownCallbackId = -1;
    int m_OnX1DoubleClickCallbackId = -1;
    int m_OnX2MouseUpCallbackId = -1;
    int m_OnX2MouseDownCallbackId = -1;
    int m_OnX2DoubleClickCallbackId = -1;
    int m_OnScrollUpCallbackId = -1;
    int m_OnScrollDownCallbackId = -1;
    int m_OnScrollLeftCallbackId = -1;
    int m_OnScrollRightCallbackId = -1;
    int m_OnMouseOverCallbackId = -1;
    int m_OnMouseLeaveCallbackId = -1;
    int m_OnDragStartCallbackId = -1;
    int m_OnDragCallbackId = -1;
    int m_OnDragEndCallbackId = -1;

    bool m_IsMouseOver = false;

protected:
    BackdropFilter m_BackdropFilter;
    ElementType m_Type;
    std::wstring m_Id;
    int m_X, m_Y;
    int m_Width, m_Height;
    bool m_WDefined, m_HDefined;
    
    // Background properties
    bool m_HasSolidColor = false;
    COLORREF m_SolidColor = 0;
    BYTE m_SolidAlpha = 0;
    int m_CornerRadius = 0;

    // Gradient properties
    GradientInfo m_SolidGradient;

    // Bevel properties
    int m_BevelType = 0;
    int m_BevelWidth = 0;
    COLORREF m_BevelColor = RGB(255, 255, 255);
    BYTE m_BevelAlpha = 200;
    COLORREF m_BevelColor2 = RGB(0, 0, 0);
    BYTE m_BevelAlpha2 = 150;
    GradientInfo m_BevelGradient;
    GradientInfo m_BevelGradient2;

    // Rendering properties
    bool m_AntiAlias = true;
    bool m_PixelHitTest = false;
    bool m_Show = true;

    std::wstring m_ContainerId;
    std::wstring m_GroupId;
    bool m_MouseEventCursor = true;
    std::wstring m_MouseEventCursorName;
    std::wstring m_CursorsDir;
    Element* m_ContainerElement = nullptr;
    std::vector<Element*> m_ContainerItems;

    // Scroll state
    int m_ScrollX = 0;
    int m_ScrollY = 0;
    int m_ScrollStep = 24;
    int m_ContentWidth = 0;
    int m_ContentHeight = 0;
    OverflowMode m_OverflowX = OverflowMode::Hidden;
    OverflowMode m_OverflowY = OverflowMode::Hidden;
    bool m_ShowScrollbar = true;
    bool m_ShowScrollbarX = true;
    bool m_ShowScrollbarY = true;
    int m_ScrollbarWidth = 6;
    int m_ScrollbarHoverWidth = -1;
    float m_ScrollbarRadius = 3.0f;
    float m_ScrollbarTrackRadius = -1.0f;
    float m_ScrollbarInset = 2.0f;
    float m_ScrollbarMinThumbLength = 20.0f;
    COLORREF m_ScrollbarColor = RGB(255, 255, 255);
    BYTE m_ScrollbarAlpha = 100;
    bool m_HasScrollbarHoverColor = false;
    COLORREF m_ScrollbarHoverColor = RGB(255, 255, 255);
    BYTE m_ScrollbarHoverAlpha = 180;
    bool m_HasScrollbarActiveColor = false;
    COLORREF m_ScrollbarActiveColor = RGB(255, 255, 255);
    BYTE m_ScrollbarActiveAlpha = 240;
    COLORREF m_ScrollbarTrackColor = RGB(0, 0, 0);
    BYTE m_ScrollbarTrackAlpha = 0;
    bool m_ShowScrollbarButtons = false;
    float m_ScrollbarButtonSize = 14.0f;
    float m_ScrollbarButtonRadius = 2.0f;
    bool m_HasScrollbarArrowColor = false;
    COLORREF m_ScrollbarArrowColor = RGB(255, 255, 255);
    BYTE m_ScrollbarArrowAlpha = 150;
    bool m_HasScrollbarArrowHoverColor = false;
    COLORREF m_ScrollbarArrowHoverColor = RGB(255, 255, 255);
    BYTE m_ScrollbarArrowHoverAlpha = 220;
    bool m_HasScrollbarArrowActiveColor = false;
    COLORREF m_ScrollbarArrowActiveColor = RGB(255, 255, 255);
    BYTE m_ScrollbarArrowActiveAlpha = 255;
    COLORREF m_ScrollbarButtonBgColor = RGB(0, 0, 0);
    BYTE m_ScrollbarButtonBgAlpha = 0;
    COLORREF m_ScrollbarButtonHoverBgColor = RGB(255, 255, 255);
    BYTE m_ScrollbarButtonHoverBgAlpha = 30;
    
    // Padding properties
    int m_PaddingLeft = 0;
    int m_PaddingTop = 0;
    int m_PaddingRight = 0;
    int m_PaddingBottom = 0;

    // Transformation properties
    float m_Rotate = 0.0f;
    bool m_HasTransformMatrix = false;
    float m_TransformMatrix[6] = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};

    // Backdrop filter cache — avoids per-frame GPU surface reallocation.
    BackdropFilter m_BackdropFilterCache;
    GfxRect        m_BackdropFilterBounds{};
    Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> m_BackdropFilterTarget;
    Microsoft::WRL::ComPtr<ID2D1Bitmap>              m_BackdropFilterBitmap;

    // Tooltip properties
    std::wstring m_ToolTipText;
    std::wstring m_ToolTipTitle;
    std::wstring m_ToolTipIcon;
    int m_ToolTipMaxWidth = 0;
    int m_ToolTipMaxHeight = 0;
    bool m_ToolTipBalloon = false;
    bool m_ToolTipDisabled = false;

    void RenderBackground(ID2D1DeviceContext* context);
    void RenderBackdropFilter(ID2D1DeviceContext* context);
    void RenderBevel(ID2D1DeviceContext* context);
    void ApplyRenderTransform(ID2D1DeviceContext* context, D2D1_MATRIX_3X2_F& originalTransform);
    void RestoreRenderTransform(ID2D1DeviceContext* context, const D2D1_MATRIX_3X2_F& originalTransform);

protected:
    HWND m_OwnerHWND = nullptr;
};

#endif
