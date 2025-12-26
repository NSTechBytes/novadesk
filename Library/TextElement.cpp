/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "TextElement.h"
#include "Logging.h"

using namespace Gdiplus;

TextElement::TextElement(const std::wstring& id, int x, int y, int w, int h,
     const std::wstring& text, const std::wstring& fontFace,
     int fontSize, COLORREF fontColor, BYTE alpha,
     bool bold, bool italic, TextAlignment textAlign,
     TextClipString clip, int clipW, int clipH)
    : Element(ELEMENT_TEXT, id, x, y, w, h),
      m_Text(text), m_FontFace(fontFace), m_FontSize(fontSize),
      m_FontColor(fontColor), m_Alpha(alpha), m_Bold(bold), m_Italic(italic),
      m_TextAlign(textAlign), m_ClipString(clip), m_ClipStringW(clipW), m_ClipStringH(clipH)
{
}

void TextElement::Render(Graphics& graphics)
{
    // Draw background first
    RenderBackground(graphics);

    // Draw bevel second
    RenderBevel(graphics);

    // Set antialiasing mode
    if (m_AntiAlias) {
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
    } else {
        graphics.SetTextRenderingHint(Gdiplus::TextRenderingHintSingleBitPerPixel);
    }

    // Create font
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    
    Font font(m_FontFace.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);
    
    // Create brush with color and alpha
    Color textColor(m_Alpha, GetRValue(m_FontColor), GetGValue(m_FontColor), GetBValue(m_FontColor));
    SolidBrush brush(textColor);
    
    // Set up string format for alignment
    StringFormat format;
    switch (m_TextAlign)
    {
    case TEXT_ALIGN_LEFT_TOP:
    case TEXT_ALIGN_LEFT_CENTER:
    case TEXT_ALIGN_LEFT_BOTTOM:
        format.SetAlignment(StringAlignmentNear);
        break;
    case TEXT_ALIGN_CENTER_TOP:
    case TEXT_ALIGN_CENTER_CENTER:
    case TEXT_ALIGN_CENTER_BOTTOM:
        format.SetAlignment(StringAlignmentCenter);
        break;
    case TEXT_ALIGN_RIGHT_TOP:
    case TEXT_ALIGN_RIGHT_CENTER:
    case TEXT_ALIGN_RIGHT_BOTTOM:
        format.SetAlignment(StringAlignmentFar);
        break;
    }

    switch (m_TextAlign)
    {
    case TEXT_ALIGN_LEFT_TOP:
    case TEXT_ALIGN_CENTER_TOP:
    case TEXT_ALIGN_RIGHT_TOP:
        format.SetLineAlignment(StringAlignmentNear);
        break;
    case TEXT_ALIGN_LEFT_CENTER:
    case TEXT_ALIGN_CENTER_CENTER:
    case TEXT_ALIGN_RIGHT_CENTER:
        format.SetLineAlignment(StringAlignmentCenter);
        break;
    case TEXT_ALIGN_LEFT_BOTTOM:
    case TEXT_ALIGN_CENTER_BOTTOM:
    case TEXT_ALIGN_RIGHT_BOTTOM:
        format.SetLineAlignment(StringAlignmentFar);
        break;
    }

    // Set trimming/clipping
    if (m_ClipString == TEXT_CLIP_ELLIPSIS)
    {
        format.SetTrimming(StringTrimmingEllipsisCharacter);
    }
    else if (m_ClipString == TEXT_CLIP_ON)
    {
        format.SetTrimming(StringTrimmingCharacter);
    }
    
    // Apply rotation if specified
    if (m_Rotate != 0.0f)
    {
        REAL centerX = m_X + GetWidth() / 2.0f;
        REAL centerY = m_Y + GetHeight() / 2.0f;
        graphics.TranslateTransform(centerX, centerY);
        graphics.RotateTransform(m_Rotate);
        graphics.TranslateTransform(-centerX, -centerY);
    }
    
    // Apply padding to layout rectangle
    int layoutX = m_X + m_PaddingLeft;
    int layoutY = m_Y + m_PaddingTop;
    int layoutW = GetWidth() - m_PaddingLeft - m_PaddingRight;
    int layoutH = GetHeight() - m_PaddingTop - m_PaddingBottom;
    
    // Ensure positive dimensions
    if (layoutW < 0) layoutW = 0;
    if (layoutH < 0) layoutH = 0;
    
    // Draw text
    RectF layoutRect((REAL)layoutX, (REAL)layoutY, (REAL)layoutW, (REAL)layoutH);
    graphics.DrawString(m_Text.c_str(), -1, &font, layoutRect, &format, &brush);
    
    // Reset transform if rotated
    if (m_Rotate != 0.0f)
    {
        graphics.ResetTransform();
    }
}

int TextElement::GetAutoWidth()
{
    HDC hdc = GetDC(NULL);
    Graphics graphics(hdc);
    
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    Font font(m_FontFace.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);

    RectF boundingBox;
    graphics.MeasureString(m_Text.c_str(), -1, &font, PointF(0, 0), &boundingBox);
    
    ReleaseDC(NULL, hdc);
    
    int width = (int)ceil(boundingBox.Width) + m_PaddingLeft + m_PaddingRight;
    if (!m_WDefined && m_ClipString != TEXT_CLIP_NONE && m_ClipStringW != -1)
    {
        if (width > m_ClipStringW) return m_ClipStringW;
    }
    return width;
}

int TextElement::GetAutoHeight()
{
    HDC hdc = GetDC(NULL);
    Graphics graphics(hdc);
    
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    Font font(m_FontFace.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);

    RectF boundingBox;
    graphics.MeasureString(m_Text.c_str(), -1, &font, PointF(0, 0), &boundingBox);
    
    ReleaseDC(NULL, hdc);
    
    int height = (int)ceil(boundingBox.Height) + m_PaddingTop + m_PaddingBottom;
    if (!m_HDefined && m_ClipString != TEXT_CLIP_NONE && m_ClipStringH != -1)
    {
        if (height > m_ClipStringH) return m_ClipStringH;
    }
    return height;
}

bool TextElement::HitTest(int x, int y)
{
    // Bounding box check first (Element's layout rect)
    if (!Element::HitTest(x, y)) return false;

    HDC hdc = GetDC(NULL);
    Graphics graphics(hdc);
    
    INT fontStyle = FontStyleRegular;
    if (m_Bold) fontStyle |= FontStyleBold;
    if (m_Italic) fontStyle |= FontStyleItalic;
    Font font(m_FontFace.c_str(), (REAL)m_FontSize, fontStyle, UnitPixel);

    StringFormat format;
    switch (m_TextAlign) {
    case TEXT_ALIGN_LEFT_TOP: case TEXT_ALIGN_LEFT_CENTER: case TEXT_ALIGN_LEFT_BOTTOM:
        format.SetAlignment(StringAlignmentNear); break;
    case TEXT_ALIGN_CENTER_TOP: case TEXT_ALIGN_CENTER_CENTER: case TEXT_ALIGN_CENTER_BOTTOM:
        format.SetAlignment(StringAlignmentCenter); break;
    case TEXT_ALIGN_RIGHT_TOP: case TEXT_ALIGN_RIGHT_CENTER: case TEXT_ALIGN_RIGHT_BOTTOM:
        format.SetAlignment(StringAlignmentFar); break;
    }
    switch (m_TextAlign) {
    case TEXT_ALIGN_LEFT_TOP: case TEXT_ALIGN_CENTER_TOP: case TEXT_ALIGN_RIGHT_TOP:
        format.SetLineAlignment(StringAlignmentNear); break;
    case TEXT_ALIGN_LEFT_CENTER: case TEXT_ALIGN_CENTER_CENTER: case TEXT_ALIGN_RIGHT_CENTER:
        format.SetLineAlignment(StringAlignmentCenter); break;
    case TEXT_ALIGN_LEFT_BOTTOM: case TEXT_ALIGN_CENTER_BOTTOM: case TEXT_ALIGN_RIGHT_BOTTOM:
        format.SetLineAlignment(StringAlignmentFar); break;
    }

    int pW = GetWidth() - m_PaddingLeft - m_PaddingRight;
    int pH = GetHeight() - m_PaddingTop - m_PaddingBottom;
    if (pW < 0) pW = 0;
    if (pH < 0) pH = 0;

    RectF layoutRect((REAL)m_PaddingLeft, (REAL)m_PaddingTop, (REAL)pW, (REAL)pH);
    RectF boundingBox;
    graphics.MeasureString(m_Text.c_str(), -1, &font, layoutRect, &format, &boundingBox);
    
    ReleaseDC(NULL, hdc);

    boundingBox.X += m_X;
    boundingBox.Y += m_Y;

    return (x >= boundingBox.X && x < boundingBox.X + boundingBox.Width &&
            y >= boundingBox.Y && y < boundingBox.Y + boundingBox.Height);
}
