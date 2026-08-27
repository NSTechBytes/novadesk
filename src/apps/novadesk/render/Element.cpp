/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Element.h"
#include "../shared/Logging.h"
#include "Direct2DHelper.h"
#include <algorithm>
#include <cmath>
#include <d2d1effects.h>
#include <d2d1effects_2.h>

Element::Element(ElementType type, const std::wstring& id, int x, int y, int width, int height)
    : m_Type(type), m_Id(id), m_X(x), m_Y(y)
{
    m_Width = (width > 0) ? width : 0;
    m_Height = (height > 0) ? height : 0;
    m_WDefined = (width > 0);
    m_HDefined = (height > 0);
    m_ToolTipDisabled = false;
}

/*
** Get the width of the element.
*/
int Element::GetWidth() { 
    int w = m_WDefined ? m_Width : GetAutoWidth();
    return w + m_PaddingLeft + m_PaddingRight;
}

/*
** Get the height of the element.
*/
int Element::GetHeight() { 
    int h = m_HDefined ? m_Height : GetAutoHeight();
    return h + m_PaddingTop + m_PaddingBottom;
}

/*
** Get the bounding box of the element.
*/
GfxRect Element::GetBounds() {
    if (!m_Show) {
        return GfxRect(m_X, m_Y, 0, 0);
    }
    return GfxRect(m_X, m_Y, GetWidth(), GetHeight());
}

GfxRect Element::GetBackgroundBounds() {
    return GetBounds();
}

/*
** Check if a point is within the element's bounds.
*/
bool Element::HitTest(int x, int y) {
    if (!m_Show) return false;
    if (!m_HasTransformMatrix && m_Rotate == 0.0f) {
        GfxRect bounds = GetBounds();
        return (x >= bounds.X && x < bounds.X + bounds.Width &&
                y >= bounds.Y && y < bounds.Y + bounds.Height);
    }

    GfxRect bounds = GetBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;

    D2D1::Matrix3x2F matrix;

    if (m_HasTransformMatrix) {
        matrix = D2D1::Matrix3x2F(
            m_TransformMatrix[0], m_TransformMatrix[1],
            m_TransformMatrix[2], m_TransformMatrix[3],
            m_TransformMatrix[4], m_TransformMatrix[5]
        );
    } else {
        matrix = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
    }
    
    // If inversion fails (degenerate matrix), fallback to standard bounds
    if (!matrix.Invert()) {
        return (x >= bounds.X && x < bounds.X + bounds.Width &&
                y >= bounds.Y && y < bounds.Y + bounds.Height);
    }

    D2D1_POINT_2F p = D2D1::Point2F((float)x, (float)y);
    D2D1_POINT_2F transformed = matrix.TransformPoint(p); 
    
    return (transformed.x >= bounds.X && transformed.x < bounds.X + bounds.Width &&
            transformed.y >= bounds.Y && transformed.y < bounds.Y + bounds.Height);
}

/*
** Check if the element has an action associated with it.
*/
bool Element::HasAction(UINT message, WPARAM wParam) const {
    switch (message)
    {
    case WM_LBUTTONUP:     return m_OnLeftMouseUpCallbackId != -1;
    case WM_LBUTTONDOWN:   return m_OnLeftMouseDownCallbackId != -1;
    case WM_LBUTTONDBLCLK: return m_OnLeftDoubleClickCallbackId != -1;
    case WM_RBUTTONUP:     return m_OnRightMouseUpCallbackId != -1;
    case WM_RBUTTONDOWN:   return m_OnRightMouseDownCallbackId != -1;
    case WM_RBUTTONDBLCLK: return m_OnRightDoubleClickCallbackId != -1;
    case WM_MBUTTONUP:     return m_OnMiddleMouseUpCallbackId != -1;
    case WM_MBUTTONDOWN:   return m_OnMiddleMouseDownCallbackId != -1;
    case WM_MBUTTONDBLCLK: return m_OnMiddleDoubleClickCallbackId != -1;
    case WM_XBUTTONUP:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return m_OnX1MouseUpCallbackId != -1;
        else return m_OnX2MouseUpCallbackId != -1;
    case WM_XBUTTONDOWN:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return m_OnX1MouseDownCallbackId != -1;
        else return m_OnX2MouseDownCallbackId != -1;
    case WM_XBUTTONDBLCLK:
        if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) return m_OnX1DoubleClickCallbackId != -1;
        else return m_OnX2DoubleClickCallbackId != -1;
    case WM_MOUSEWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return m_OnScrollUpCallbackId != -1;
        else return m_OnScrollDownCallbackId != -1;
    case WM_MOUSEHWHEEL:
        if (GET_WHEEL_DELTA_WPARAM(wParam) > 0) return m_OnScrollRightCallbackId != -1;
        else return m_OnScrollLeftCallbackId != -1;
    case WM_MOUSEMOVE:
        return m_OnMouseOverCallbackId != -1 || m_OnMouseLeaveCallbackId != -1;
    }
    return false;
}

/*
** Check if the element has any interactive mouse action.
*/
bool Element::HasMouseAction() const {
    return m_OnLeftMouseUpCallbackId != -1 ||
           m_OnLeftMouseDownCallbackId != -1 ||
           m_OnLeftDoubleClickCallbackId != -1 ||
           m_OnRightMouseUpCallbackId != -1 ||
           m_OnRightMouseDownCallbackId != -1 ||
           m_OnRightDoubleClickCallbackId != -1 ||
           m_OnMiddleMouseUpCallbackId != -1 ||
           m_OnMiddleMouseDownCallbackId != -1 ||
           m_OnMiddleDoubleClickCallbackId != -1 ||
           m_OnX1MouseUpCallbackId != -1 ||
           m_OnX1MouseDownCallbackId != -1 ||
           m_OnX1DoubleClickCallbackId != -1 ||
           m_OnX2MouseUpCallbackId != -1 ||
           m_OnX2MouseDownCallbackId != -1 ||
           m_OnX2DoubleClickCallbackId != -1 ||
           m_OnScrollUpCallbackId != -1 ||
           m_OnScrollDownCallbackId != -1 ||
           m_OnScrollLeftCallbackId != -1 ||
           m_OnScrollRightCallbackId != -1 ||
           m_OnMouseOverCallbackId != -1 ||
           m_OnMouseLeaveCallbackId != -1 ||
           m_OnDragStartCallbackId != -1 ||
           m_OnDragCallbackId != -1 ||
           m_OnDragEndCallbackId != -1;
}

bool Element::HasDragAction() const
{
    return m_OnDragStartCallbackId != -1 ||
           m_OnDragCallbackId != -1 ||
           m_OnDragEndCallbackId != -1;
}

/*
** Set the padding for the element.
*/
void Element::SetPadding(int left, int top, int right, int bottom) {
    // Logging::Log(LogLevel::Debug, L"[PADDING] Element::SetPadding on '%s': L=%d, T=%d, R=%d, B=%d", 
    //    m_Id.c_str(), left, top, right, bottom);
    m_PaddingLeft = left;
    m_PaddingTop = top;
    m_PaddingRight = right;
    m_PaddingBottom = bottom;
}

void Element::RemoveContainerItem(Element* item)
{
    m_ContainerItems.erase(std::remove(m_ContainerItems.begin(), m_ContainerItems.end(), item), m_ContainerItems.end());
}

void Element::ClearContainerItems()
{
    m_ContainerItems.clear();
}

/*
** Render the background of the element.
*/
void Element::RenderBackground(ID2D1DeviceContext* context) {
    RenderBackdropFilter(context);
    if (!m_HasSolidColor) return;

    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

    GfxRect bounds = GetBackgroundBounds();
    D2D1_RECT_F rect = D2D1::RectF((float)bounds.X, (float)bounds.Y, (float)(bounds.X + bounds.Width), (float)(bounds.Y + bounds.Height));
    
    if (rect.right <= rect.left || rect.bottom <= rect.top) return;

    Microsoft::WRL::ComPtr<ID2D1Brush> brush;
    Direct2D::CreateBrushFromGradientOrColor(
        context,
        rect,
        &m_SolidGradient,
        m_SolidColor,
        m_SolidAlpha / 255.0f,
        brush.GetAddressOf()
    );

    if (brush) {
        if (m_CornerRadius > 0) {
            D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(rect, (float)m_CornerRadius, (float)m_CornerRadius);
            context->FillRoundedRectangle(roundedRect, brush.Get());
        } else {
            context->FillRectangle(rect, brush.Get());
        }
    }
}

/*
** Render the bevel of the element.
*/
void Element::RenderBevel(ID2D1DeviceContext* context) {
    if (m_BevelType == 0 || m_BevelWidth <= 0) return;

    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

    const float pad = 2.0f;
    GfxRect bounds = GetBounds();
    D2D1_RECT_F rect = D2D1::RectF(
        (float)bounds.X - pad,
        (float)bounds.Y - pad,
        (float)(bounds.X + bounds.Width) + pad,
        (float)(bounds.Y + bounds.Height) + pad
    );
    
    Microsoft::WRL::ComPtr<ID2D1Brush> highlightBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> shadowBrush;
    Direct2D::CreateBrushFromGradientOrColor(
        context,
        rect,
        &m_BevelGradient,
        m_BevelColor,
        m_BevelAlpha / 255.0f,
        highlightBrush.GetAddressOf());
    Direct2D::CreateBrushFromGradientOrColor(
        context,
        rect,
        &m_BevelGradient2,
        m_BevelColor2,
        m_BevelAlpha2 / 255.0f,
        shadowBrush.GetAddressOf());

    float offset = m_BevelWidth / 2.0f;
    
    switch (m_BevelType) {
    case 1: // Raised
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.top + offset), highlightBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.left + offset, rect.bottom - offset), highlightBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.right - offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), shadowBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.bottom - offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), shadowBrush.Get(), (float)m_BevelWidth);
        break;
        
    case 2: // Sunken
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.top + offset), shadowBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.left + offset, rect.bottom - offset), shadowBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.right - offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), highlightBrush.Get(), (float)m_BevelWidth);
        context->DrawLine(D2D1::Point2F(rect.left + offset, rect.bottom - offset), D2D1::Point2F(rect.right - offset, rect.bottom - offset), highlightBrush.Get(), (float)m_BevelWidth);
        break;
        
    case 3: // Emboss
        {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> midBrush;
            Direct2D::CreateSolidBrush(context, m_BevelColor, (m_BevelAlpha / 2.0f) / 255.0f, midBrush.GetAddressOf());
            context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.right - offset, rect.top + offset), highlightBrush.Get(), (float)m_BevelWidth);
            context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset), D2D1::Point2F(rect.left + offset, rect.bottom - offset), midBrush.Get(), (float)m_BevelWidth);
        }
        break;
        
    case 4: // Pillow
        if (m_BevelGradient.type != GRADIENT_NONE && !m_BevelGradient.stops.empty())
        {
            for (int i = 0; i < m_BevelWidth; i++) {
                D2D1_RECT_F r = D2D1::RectF(rect.left + i, rect.top + i, rect.right - i, rect.bottom - i);
                context->DrawRectangle(r, highlightBrush.Get(), 1.0f);
            }
        }
        else
        {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fadeBrush;
            Direct2D::CreateSolidBrush(context, m_BevelColor, 1.0f, fadeBrush.GetAddressOf());
            if (fadeBrush)
            {
                for (int i = 0; i < m_BevelWidth; i++) {
                    float alpha = (m_BevelAlpha / 255.0f) * (1.0f - (float)i / m_BevelWidth);
                    fadeBrush->SetOpacity(alpha);
                    D2D1_RECT_F r = D2D1::RectF(rect.left + i, rect.top + i, rect.right - i, rect.bottom - i);
                    context->DrawRectangle(r, fadeBrush.Get(), 1.0f);
                }
            }
        }
        break;
    }
}

void Element::ApplyRenderTransform(ID2D1DeviceContext* context, D2D1_MATRIX_3X2_F& originalTransform) {
    if (!context) return;
    context->GetTransform(&originalTransform);

    if (!m_HasTransformMatrix && m_Rotate == 0.0f) {
        return;
    }

    if (m_HasTransformMatrix) {
        D2D1::Matrix3x2F matrix = D2D1::Matrix3x2F(
            m_TransformMatrix[0], m_TransformMatrix[1],
            m_TransformMatrix[2], m_TransformMatrix[3],
            m_TransformMatrix[4], m_TransformMatrix[5]
        );
        context->SetTransform(matrix * originalTransform);
        return;
    }

    GfxRect bounds = GetBounds();
    float centerX = bounds.X + bounds.Width / 2.0f;
    float centerY = bounds.Y + bounds.Height / 2.0f;
    context->SetTransform(D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY)) * originalTransform);
}

void Element::RestoreRenderTransform(ID2D1DeviceContext* context, const D2D1_MATRIX_3X2_F& originalTransform) {
    if (!context) return;
    context->SetTransform(originalTransform);
}
bool BackdropFilter::IsActive() const
{
    return blur > 0.0f || brightness != 1.0f || contrast != 1.0f ||
        grayscale > 0.0f || saturate != 1.0f || sepia > 0.0f ||
        hueRotate != 0.0f || invert > 0.0f || opacity != 1.0f;
}

void Element::RenderBackdropFilter(ID2D1DeviceContext* context)
{
    if (!context || !m_BackdropFilter.IsActive()) return;
    const GfxRect bounds = GetBackgroundBounds();
    const float pad = std::ceil(m_BackdropFilter.blur * 3.0f);
    const D2D1_SIZE_U canvas = context->GetPixelSize();
    if (!canvas.width || !canvas.height) return;
    const LONG l = (std::max)(0L, static_cast<LONG>(std::floor(bounds.X - pad)));
    const LONG t = (std::max)(0L, static_cast<LONG>(std::floor(bounds.Y - pad)));
    const LONG r = (std::min)(static_cast<LONG>(canvas.width), static_cast<LONG>(std::ceil(bounds.X + bounds.Width + pad)));
    const LONG b = (std::min)(static_cast<LONG>(canvas.height), static_cast<LONG>(std::ceil(bounds.Y + bounds.Height + pad)));
    if (r <= l || b <= t) return;

    // --- Cache validation --------------------------------------------------
    // Rebuild the render-target + bitmap only when the captured region changes.
    // Rebuild the effect chain only when the filter parameters change.
    const GfxRect srcRect(l, t, r - l, b - t);
    const bool targetValid = m_BackdropFilterTarget &&
                             m_BackdropFilterBounds.X == srcRect.X &&
                             m_BackdropFilterBounds.Y == srcRect.Y &&
                             m_BackdropFilterBounds.Width == srcRect.Width &&
                             m_BackdropFilterBounds.Height == srcRect.Height;
    if (!targetValid)
    {
        if (FAILED(context->CreateCompatibleRenderTarget(D2D1::SizeF(srcRect.Width, srcRect.Height), &m_BackdropFilterTarget)))
            return;
        if (FAILED(m_BackdropFilterTarget->GetBitmap(&m_BackdropFilterBitmap)))
            return;
        m_BackdropFilterBounds = srcRect;
        // Force effect-chain rebuild since we got a new bitmap.
        m_BackdropFilterCache = BackdropFilter{};
    }

    // Copy the current content behind the element into the cached bitmap.
    // This is the unavoidable per-frame GPU readback.
    const D2D1_RECT_U source = D2D1::RectU(l, t, r, b);
    if (FAILED(m_BackdropFilterBitmap->CopyFromRenderTarget(nullptr, context, &source))) return;

    // --- Effect chain (cached when filter params unchanged) -----------------
    ID2D1Image* image = m_BackdropFilterBitmap.Get();
    std::vector<Microsoft::WRL::ComPtr<ID2D1Effect>> effects;
    std::vector<Microsoft::WRL::ComPtr<ID2D1Image>> outputs;
    if (m_BackdropFilterCache.blur != m_BackdropFilter.blur ||
        m_BackdropFilterCache.saturate != m_BackdropFilter.saturate ||
        m_BackdropFilterCache.hueRotate != m_BackdropFilter.hueRotate ||
        m_BackdropFilterCache.opacity != m_BackdropFilter.opacity)
    {
        auto add = [&](REFCLSID id, auto set) {
            Microsoft::WRL::ComPtr<ID2D1Effect> effect; Microsoft::WRL::ComPtr<ID2D1Image> output;
            if (FAILED(context->CreateEffect(id, &effect))) return false;
            effect->SetInput(0, image); set(effect.Get()); effect->GetOutput(&output);
            image = output.Get(); effects.push_back(effect); outputs.push_back(output); return true;
        };
        if (m_BackdropFilter.blur > 0 && !add(CLSID_D2D1GaussianBlur, [&](ID2D1Effect* e) { e->SetValue(D2D1_GAUSSIANBLUR_PROP_STANDARD_DEVIATION, m_BackdropFilter.blur); e->SetValue(D2D1_GAUSSIANBLUR_PROP_BORDER_MODE, D2D1_BORDER_MODE_HARD); })) return;
        if (m_BackdropFilter.saturate != 1 && !add(CLSID_D2D1Saturation, [&](ID2D1Effect* e) { e->SetValue(D2D1_SATURATION_PROP_SATURATION, m_BackdropFilter.saturate); })) return;
        if (m_BackdropFilter.hueRotate != 0 && !add(CLSID_D2D1HueRotation, [&](ID2D1Effect* e) { e->SetValue(D2D1_HUEROTATION_PROP_ANGLE, m_BackdropFilter.hueRotate); })) return;
        if (m_BackdropFilter.opacity != 1 && !add(CLSID_D2D1Opacity, [&](ID2D1Effect* e) { e->SetValue(D2D1_OPACITY_PROP_OPACITY, m_BackdropFilter.opacity); })) return;
        m_BackdropFilterCache = m_BackdropFilter;
    }

    const D2D1_RECT_F clip = D2D1::RectF((FLOAT)bounds.X, (FLOAT)bounds.Y, (FLOAT)(bounds.X + bounds.Width), (FLOAT)(bounds.Y + bounds.Height));
    context->PushAxisAlignedClip(clip, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
    const D2D1_POINT_2F offset = D2D1::Point2F((FLOAT)l, (FLOAT)t);
    context->DrawImage(image, &offset, nullptr, D2D1_INTERPOLATION_MODE_LINEAR, D2D1_COMPOSITE_MODE_SOURCE_OVER);
    context->PopAxisAlignedClip();
}
