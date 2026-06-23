/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_INPUT_BOX_ELEMENT_H__
#define __NOVADESK_INPUT_BOX_ELEMENT_H__

#include "Element.h"
#include "TextElement.h" // for TextAlignment
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wrl/client.h>
#include <string>

// A custom text input field rendered entirely with Direct2D/DirectWrite inside
// the Widget paint loop. Because it lives in the same m_Elements list as every
// other element, it composites between/over siblings in insertion order
// (CSS-like document flow) without any dedicated z-index infrastructure.
class InputBoxElement : public Element
{
public:
    InputBoxElement(const std::wstring &id, int x, int y, int width, int height);
    virtual ~InputBoxElement() {}

    virtual void Render(ID2D1DeviceContext *context) override;

    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    virtual GfxRect GetBounds() override;
    virtual bool HitTest(int x, int y) override;

    // Text content
    void SetText(const std::wstring &text);
    const std::wstring &GetText() const { return m_Text; }

    // Typography
    void SetFontFace(const std::wstring &font) { m_FontFace = font; }
    void SetFontSize(int size) { m_FontSize = size; }
    void SetFontColor(COLORREF color, BYTE alpha)
    {
        m_FontColor = color;
        m_FontAlpha = alpha;
    }
    void SetFontWeight(int weight) { m_FontWeight = weight; }
    void SetItalic(bool italic) { m_Italic = italic; }
    void SetFontPath(const std::wstring &path) { m_FontPath = path; }
    void SetTextAlign(TextAlignment align) { m_TextAlign = align; }

    const std::wstring &GetFontFace() const { return m_FontFace; }
    int GetFontSize() const { return m_FontSize; }
    COLORREF GetFontColor() const { return m_FontColor; }
    BYTE GetFontAlpha() const { return m_FontAlpha; }
    int GetFontWeight() const { return m_FontWeight; }
    bool IsItalic() const { return m_Italic; }
    TextAlignment GetTextAlign() const { return m_TextAlign; }

    // Placeholder
    void SetPlaceholder(const std::wstring &placeholder) { m_Placeholder = placeholder; }
    const std::wstring &GetPlaceholder() const { return m_Placeholder; }
    void SetPlaceholderColor(COLORREF color, BYTE alpha)
    {
        m_PlaceholderColor = color;
        m_PlaceholderAlpha = alpha;
    }

    // Caret / selection colors
    void SetCaretColor(COLORREF color, BYTE alpha)
    {
        m_CaretColor = color;
        m_CaretAlpha = alpha;
    }
    void SetSelectionColor(COLORREF color, BYTE alpha)
    {
        m_SelectionColor = color;
        m_SelectionAlpha = alpha;
    }

    // Border (solid only)
    void SetBorderColor(COLORREF color, BYTE alpha)
    {
        m_BorderColor = color;
        m_BorderAlpha = alpha;
    }
    void SetBorderWidth(float width) { m_BorderWidth = width; }
    void SetBorderRadius(float radius) { m_BorderRadius = radius; }
    COLORREF GetBorderColor() const { return m_BorderColor; }
    BYTE GetBorderAlpha() const { return m_BorderAlpha; }
    float GetBorderWidth() const { return m_BorderWidth; }
    float GetBorderRadius() const { return m_BorderRadius; }

    // Password masking (reserved for future; off by default)
    void SetPasswordMode(bool enabled) { m_Password = enabled; }
    bool IsPasswordMode() const { return m_Password; }

    // Max length (reserved; 0 = unlimited)
    void SetMaxLength(int len) { m_MaxLength = len; }
    int GetMaxLength() const { return m_MaxLength; }

    // Focus
    bool IsFocused() const { return m_Focused; }
    void SetFocus(bool focused);

    // Caret visibility toggles on a timer; the Widget calls this each repaint.
    void UpdateBlink();

    // Editing mutations (called by Widget keyboard routing)
    // Returns true if the content changed (so Widget can fire onChange + redraw).
    bool HandleChar(wchar_t ch);
    bool HandleKeyDown(WPARAM vk, bool shift, bool control);
    void HandleMouseDown(int x, int y, bool shift);
    void HandleMouseMove(int x, int y);
    void HandleMouseUp();

    // Caret / selection state
    bool HasSelection() const { return m_SelectionStart != m_SelectionEnd; }
    void SelectAll();
    void ClearSelection();
    std::wstring GetSelectedText() const;
    void DeleteSelection();
    void ReplaceSelection(const std::wstring &text);

    // Callback IDs (Widget fills these in from JS)
    int m_OnTextChangeCallbackId = -1;
    int m_OnEnterCallbackId = -1;
    int m_OnFocusCallbackId = -1;
    int m_OnBlurCallbackId = -1;

private:
    // Build/retrieve a DirectWrite text layout for the currently rendered text.
    // textOverride lets callers build a layout for the placeholder text.
    Microsoft::WRL::ComPtr<IDWriteTextLayout> CreateTextLayout(
        ID2D1DeviceContext *context, const std::wstring &text, float layoutW, float layoutH) const;

    // Resolve the local content rect (inside padding) used for drawing text.
    D2D1_RECT_F GetContentRect() const;

    // Map a caret index to an x offset (in content-local coordinates).
    float CaretIndexToX(UINT32 index) const;
    // Map a point (element-local) to a caret index.
    UINT32 PointToCaretIndex(int x, int y) const;

    // Normalize selection so start <= end.
    void NormalizeSelection(UINT32 &outStart, UINT32 &outEnd) const;

    // Recompute the horizontal scroll offset so the caret stays visible
    // when the text is wider than the content area.
    void EnsureCaretVisible();

    // Measure the rendered text width (in DIPs), accounting for password masking.
    float MeasureTextWidth() const;

    std::wstring m_Text;
    std::wstring m_Placeholder;

    std::wstring m_FontFace = L"Segoe UI";
    int m_FontSize = 14;
    COLORREF m_FontColor = RGB(240, 240, 240);
    BYTE m_FontAlpha = 255;
    int m_FontWeight = 400;
    bool m_Italic = false;
    TextAlignment m_TextAlign = TEXT_ALIGN_LEFT_CENTER;
    std::wstring m_FontPath;

    COLORREF m_PlaceholderColor = RGB(150, 150, 150);
    BYTE m_PlaceholderAlpha = 255;
    COLORREF m_CaretColor = RGB(255, 255, 255);
    BYTE m_CaretAlpha = 255;
    COLORREF m_SelectionColor = RGB(135, 206, 235);
    BYTE m_SelectionAlpha = 128;

    // Border (solid)
    float m_BorderWidth = 0.0f;
    float m_BorderRadius = 0.0f;
    COLORREF m_BorderColor = RGB(0, 0, 0);
    BYTE m_BorderAlpha = 255;

    bool m_Password = false;
    int m_MaxLength = 0;

    bool m_Focused = false;

    // Caret / selection state
    UINT32 m_CaretPos = 0;        // Caret position (0..text.length())
    UINT32 m_SelectionAnchor = 0; // Anchor for shift-click / drag selection
    UINT32 m_SelectionStart = 0;  // Normalized selection start
    UINT32 m_SelectionEnd = 0;    // Normalized selection end
    bool m_IsDragging = false;

    // Blink state
    DWORD m_LastBlinkTick = 0;
    bool m_CaretVisible = true;

    // Horizontal scroll offset (positive = text shifted left). Applied so that
    // the caret remains visible when the text overflows the content area.
    float m_ScrollOffset = 0.0f;
};

#endif
