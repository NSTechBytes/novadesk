/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ElementLayoutBox.h"

#include "Direct2DHelper.h"
#include "../shared/Logging.h"
#include <algorithm>
#include <cmath>
#include <d2d1effects.h>

ElementLayoutBox::ElementLayoutBox(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height, ELEMENT_LAYOUT_BOX)
{
}
bool ElementLayoutBox::HitTestLocal(const D2D1_POINT_2F& point)
{
    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory)
        return false;
    Microsoft::WRL::ComPtr<ID2D1Geometry> geometry;
    if (!CreateGeometry(factory, geometry))
        return false;
    BOOL hit = FALSE;
    if (m_HasFill && m_FillAlpha > 0)
    {
        if (SUCCEEDED(geometry->FillContainsPoint(point, nullptr, &hit)) && hit)
            return true;
    }
    if (m_HasStroke && m_StrokeWidth > 0.0f && m_StrokeAlpha > 0)
    {
        EnsureStrokeStyle();
        hit = FALSE;
        if (SUCCEEDED(geometry->StrokeContainsPoint(point, m_StrokeWidth, m_StrokeStyle, nullptr, &hit)) && hit)
            return true;
    }
    return false;
}
bool ElementLayoutBox::CreateGeometry(ID2D1Factory* factory, Microsoft::WRL::ComPtr<ID2D1Geometry>& geometry) const
{
    if (!factory)
        return false;
    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> rounded;
    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;
    if (FAILED(factory->CreateRoundedRectangleGeometry(rect, rounded.GetAddressOf())))
        return false;
    geometry = rounded;
    return true;
}
void ElementLayoutBox::Render(ID2D1DeviceContext* context)
{
    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);
    Microsoft::WRL::ComPtr<ID2D1Brush> strokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> fillBrush;
    TryCreateStrokeBrush(context, strokeBrush);
    TryCreateFillBrush(context, fillBrush);
    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY;
    for (const auto& shadow : m_BoxShadows)
    {
        if (!shadow.inset)
            RenderSingleShadow(context, rect, shadow);
    }
    if (fillBrush)
    {
        context->FillRoundedRectangle(rect, fillBrush.Get());
    }
    for (const auto& shadow : m_BoxShadows)
    {
        if (shadow.inset)
            RenderSingleShadow(context, rect, shadow);
    }
    if (strokeBrush)
    {
        D2D1_ROUNDED_RECT borderRect = rect;
        float radiusX = m_RadiusX;
        float radiusY = m_RadiusY;
        if (m_BorderPosition == BorderPosition::Outside)
        {
            const float half = m_StrokeWidth * 0.5f;
            borderRect.rect.left -= half;
            borderRect.rect.top -= half;
            borderRect.rect.right += half;
            borderRect.rect.bottom += half;
            if (m_RadiusX > 0.0f)
                radiusX += half;
            if (m_RadiusY > 0.0f)
                radiusY += half;
        }
        else if (m_BorderPosition == BorderPosition::Inside)
        {
            const float half = m_StrokeWidth * 0.5f;
            borderRect.rect.left += half;
            borderRect.rect.top += half;
            borderRect.rect.right -= half;
            borderRect.rect.bottom -= half;
            radiusX -= half;
            radiusY -= half;
            if (radiusX < 0.0f)
                radiusX = 0.0f;
            if (radiusY < 0.0f)
                radiusY = 0.0f;
        }
        borderRect.radiusX = radiusX;
        borderRect.radiusY = radiusY;
        RenderBorderWithStyle(context, borderRect, strokeBrush.Get(), m_StrokeWidth);
    }
    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
void ElementLayoutBox::RenderSingleShadow(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& baseRect, const BoxShadow& shadow)
{
    if (!context || shadow.alpha == 0)
        return;
    const float width = baseRect.rect.right - baseRect.rect.left;
    const float height = baseRect.rect.bottom - baseRect.rect.top;
    const float pad = std::max(2.0f, std::ceil(std::max(0.0f, shadow.blur) + std::fabs(shadow.spread) + 4.0f));
    D2D1_ROUNDED_RECT shadowRect = D2D1::RoundedRect(
        D2D1::RectF(pad, pad, pad + width, pad + height),
        baseRect.radiusX,
        baseRect.radiusY);
    if (shadow.spread != 0.0f)
    {
        shadowRect.rect.left -= shadow.spread;
        shadowRect.rect.top -= shadow.spread;
        shadowRect.rect.right += shadow.spread;
        shadowRect.rect.bottom += shadow.spread;
        shadowRect.radiusX = std::max(0.0f, baseRect.radiusX + shadow.spread);
        shadowRect.radiusY = std::max(0.0f, baseRect.radiusY + shadow.spread);
        const float minExtent = 1.0f;
        if ((shadowRect.rect.right - shadowRect.rect.left) < minExtent)
        {
            const float cx = (shadowRect.rect.left + shadowRect.rect.right) * 0.5f;
            shadowRect.rect.left = cx - (minExtent * 0.5f);
            shadowRect.rect.right = cx + (minExtent * 0.5f);
        }
        if ((shadowRect.rect.bottom - shadowRect.rect.top) < minExtent)
        {
            const float cy = (shadowRect.rect.top + shadowRect.rect.bottom) * 0.5f;
            shadowRect.rect.top = cy - (minExtent * 0.5f);
            shadowRect.rect.bottom = cy + (minExtent * 0.5f);
        }
    }
    Microsoft::WRL::ComPtr<ID2D1CommandList> commandList;
    if (FAILED(context->CreateCommandList(&commandList)))
        return;
    Microsoft::WRL::ComPtr<ID2D1Device> device;
    context->GetDevice(&device);
    if (!device)
        return;
    Microsoft::WRL::ComPtr<ID2D1DeviceContext> recordingContext;
    if (FAILED(device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &recordingContext)))
        return;
    recordingContext->SetTarget(commandList.Get());
    recordingContext->BeginDraw();
    recordingContext->Clear(D2D1::ColorF(0, 0.0f));
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> whiteBrush;
    if (Direct2D::CreateSolidBrush(recordingContext.Get(), RGB(255, 255, 255), 1.0f, &whiteBrush))
    {
        if (shadow.inset)
        {
            float insetThickness = std::max(
                1.0f,
                (shadow.blur * 1.15f) +
                (std::max(0.0f, shadow.spread) * 1.6f) +
                (std::max(std::fabs(shadow.x), std::fabs(shadow.y)) * 0.45f));
            if (shadow.spread < 0.0f)
            {
                insetThickness = std::max(1.0f, insetThickness + shadow.spread);
            }
            recordingContext->DrawRoundedRectangle(shadowRect, whiteBrush.Get(), insetThickness);
        }
        else
        {
            recordingContext->FillRoundedRectangle(shadowRect, whiteBrush.Get());
        }
    }
    recordingContext->EndDraw();
    commandList->Close();
    Microsoft::WRL::ComPtr<ID2D1Effect> shadowEffect;
    Microsoft::WRL::ComPtr<ID2D1Effect> colorMatrixEffect;
    if (FAILED(context->CreateEffect(CLSID_D2D1Shadow, &shadowEffect)) ||
        FAILED(context->CreateEffect(CLSID_D2D1ColorMatrix, &colorMatrixEffect)))
    {
        return;
    }
    shadowEffect->SetInput(0, commandList.Get());
    shadowEffect->SetValue(D2D1_SHADOW_PROP_BLUR_STANDARD_DEVIATION, std::max(0.0f, shadow.blur));
    colorMatrixEffect->SetInputEffect(0, shadowEffect.Get());
    D2D1_COLOR_F c = Direct2D::ColorToD2D(shadow.color, shadow.alpha / 255.0f);
    D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
        0, 0, 0, 0,
        0, 0, 0, 0,
        0, 0, 0, 0,
        c.r * c.a, c.g * c.a, c.b * c.a, c.a,
        0, 0, 0, 0);
    colorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
    const D2D1_POINT_2F shadowOffset = D2D1::Point2F(
        baseRect.rect.left + shadow.x - pad,
        baseRect.rect.top + shadow.y - pad);
    if (shadow.inset)
    {
        Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> clipGeometry;
        ID2D1Factory1* factory = Direct2D::GetFactory();
        if (!factory || FAILED(factory->CreateRoundedRectangleGeometry(baseRect, &clipGeometry)))
            return;
        D2D1_LAYER_PARAMETERS1 layerParams = D2D1::LayerParameters1(
            baseRect.rect,
            clipGeometry.Get(),
            D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
            D2D1::Matrix3x2F::Identity(),
            1.0f,
            nullptr,
            D2D1_LAYER_OPTIONS1_NONE);
        context->PushLayer(layerParams, nullptr);
        context->DrawImage(colorMatrixEffect.Get(), shadowOffset);
        context->PopLayer();
        return;
    }
    context->DrawImage(colorMatrixEffect.Get(), shadowOffset);
}
void ElementLayoutBox::RenderBorderWithStyle(ID2D1DeviceContext* context, const D2D1_ROUNDED_RECT& rect,
    ID2D1Brush* strokeBrush, float strokeWidth)
{
    if (m_BorderStyleTop == BorderStyle::None && m_BorderStyleRight == BorderStyle::None &&
        m_BorderStyleBottom == BorderStyle::None && m_BorderStyleLeft == BorderStyle::None)
        return;
    if (m_BorderStyleTop == BorderStyle::Hidden && m_BorderStyleRight == BorderStyle::Hidden &&
        m_BorderStyleBottom == BorderStyle::Hidden && m_BorderStyleLeft == BorderStyle::Hidden)
        return;
    bool sameStyle = (m_BorderStyleTop == m_BorderStyleRight &&
        m_BorderStyleRight == m_BorderStyleBottom &&
        m_BorderStyleBottom == m_BorderStyleLeft);
    auto getStrokeStyle = [&](BorderStyle style, ID2D1StrokeStyle1** outStyle)
        {
            *outStyle = nullptr;
            ID2D1Factory1* factory = Direct2D::GetFactory();
            if (!factory)
                return;
            D2D1_STROKE_STYLE_PROPERTIES1 props = {};
            props.startCap = m_StrokeStartCap;
            props.endCap = m_StrokeEndCap;
            props.dashCap = m_StrokeDashCap;
            props.lineJoin = m_StrokeLineJoin;
            props.miterLimit = 10.0f;
            props.dashOffset = m_StrokeDashOffset;
            props.transformType = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
            if (style == BorderStyle::Dotted)
            {
                props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
                props.startCap = D2D1_CAP_STYLE_ROUND;
                props.endCap = D2D1_CAP_STYLE_ROUND;
                props.dashCap = D2D1_CAP_STYLE_ROUND;
                float dashes[] = { 0.0f, 2.0f };
                factory->CreateStrokeStyle(props, dashes, 2, outStyle);
            }
            else if (style == BorderStyle::Dashed)
            {
                props.dashStyle = D2D1_DASH_STYLE_CUSTOM;
                float dashes[] = { 3.0f, 1.0f };
                factory->CreateStrokeStyle(props, dashes, 2, outStyle);
            }
            else
            {
                props.dashStyle = D2D1_DASH_STYLE_SOLID;
                factory->CreateStrokeStyle(props, nullptr, 0, outStyle);
            }
        };
    auto drawStyleRect = [&](const D2D1_ROUNDED_RECT& r, BorderStyle style, ID2D1Brush* brush, float width)
        {
            if (style == BorderStyle::None || style == BorderStyle::Hidden)
                return;
            Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> sstyle;
            getStrokeStyle(style, &sstyle);
            if (style == BorderStyle::Double)
            {
                float third = width / 3.0f;
                D2D1_ROUNDED_RECT r1 = r;
                D2D1_ROUNDED_RECT r2 = r;
                r1.rect.left -= third;
                r1.rect.top -= third;
                r1.rect.right += third;
                r1.rect.bottom += third;
                r2.rect.left += third;
                r2.rect.top += third;
                r2.rect.right -= third;
                r2.rect.bottom -= third;
                if (r.radiusX == 0.0f && r.radiusY == 0.0f)
                {
                    context->DrawRectangle(r1.rect, brush, third, sstyle.Get());
                    context->DrawRectangle(r2.rect, brush, third, sstyle.Get());
                }
                else
                {
                    r1.radiusX = r.radiusX + third;
                    r1.radiusY = r.radiusY + third;
                    r2.radiusX = std::max(0.0f, r.radiusX - third);
                    r2.radiusY = std::max(0.0f, r.radiusY - third);
                    context->DrawRoundedRectangle(r1, brush, third, sstyle.Get());
                    context->DrawRoundedRectangle(r2, brush, third, sstyle.Get());
                }
            }
            else if (style == BorderStyle::Groove || style == BorderStyle::Ridge ||
                style == BorderStyle::Inset || style == BorderStyle::Outset)
            {
                COLORREF baseColor = m_StrokeColor;
                BYTE rC = GetRValue(baseColor), gC = GetGValue(baseColor), bC = GetBValue(baseColor);
                COLORREF lightC = RGB(std::min(255, rC + 50), std::min(255, gC + 50), std::min(255, bC + 50));
                COLORREF darkC = RGB(std::max(0, rC - 50), std::max(0, gC - 50), std::max(0, bC - 50));
                Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> lightBrush, darkBrush;
                context->CreateSolidColorBrush(Direct2D::ColorToD2D(lightC, m_StrokeAlpha / 255.0f), &lightBrush);
                context->CreateSolidColorBrush(Direct2D::ColorToD2D(darkC, m_StrokeAlpha / 255.0f), &darkBrush);
                ID2D1Brush* topLeftBrush = nullptr;
                ID2D1Brush* bottomRightBrush = nullptr;
                if (style == BorderStyle::Groove)
                {
                    topLeftBrush = darkBrush.Get();
                    bottomRightBrush = lightBrush.Get();
                }
                else if (style == BorderStyle::Ridge)
                {
                    topLeftBrush = lightBrush.Get();
                    bottomRightBrush = darkBrush.Get();
                }
                else if (style == BorderStyle::Inset)
                {
                    topLeftBrush = darkBrush.Get();
                    bottomRightBrush = lightBrush.Get();
                }
                else if (style == BorderStyle::Outset)
                {
                    topLeftBrush = lightBrush.Get();
                    bottomRightBrush = darkBrush.Get();
                }
                if (m_RadiusX == 0.0f && m_RadiusY == 0.0f)
                {
                    D2D1_ANTIALIAS_MODE oldMode = context->GetAntialiasMode();
                    context->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);
                    float L = r.rect.left, T = r.rect.top;
                    float R = r.rect.right, B = r.rect.bottom;
                    float w = width;
                    auto drawPolygon = [&](const D2D1_POINT_2F* points, int count, ID2D1Brush* pBrush)
                        {
                            if (!pBrush)
                                return;
                            Microsoft::WRL::ComPtr<ID2D1Factory> factory;
                            context->GetFactory(&factory);
                            if (!factory)
                                return;
                            Microsoft::WRL::ComPtr<ID2D1PathGeometry> pathGeom;
                            if (SUCCEEDED(factory->CreatePathGeometry(&pathGeom)))
                            {
                                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                                if (SUCCEEDED(pathGeom->Open(&sink)))
                                {
                                    sink->BeginFigure(points[0], D2D1_FIGURE_BEGIN_FILLED);
                                    sink->AddLines(points + 1, count - 1);
                                    sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                                    sink->Close();
                                    context->FillGeometry(pathGeom.Get(), pBrush);
                                }
                            }
                        };
                    if (style == BorderStyle::Inset || style == BorderStyle::Outset)
                    {
                        ID2D1Brush* tbBrush = topLeftBrush;
                        ID2D1Brush* lrBrush = topLeftBrush;
                        ID2D1Brush* bbBrush = bottomRightBrush;
                        ID2D1Brush* rrBrush = bottomRightBrush;
                        D2D1_POINT_2F topPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(L + w, T + w) };
                        drawPolygon(topPts, 4, tbBrush);
                        D2D1_POINT_2F leftPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(L + w, T + w),
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(L, B) };
                        drawPolygon(leftPts, 4, lrBrush);
                        D2D1_POINT_2F bottomPts[] = {
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(R - w, B - w),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(L, B) };
                        drawPolygon(bottomPts, 4, bbBrush);
                        D2D1_POINT_2F rightPts[] = {
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(R - w, B - w) };
                        drawPolygon(rightPts, 4, rrBrush);
                    }
                    else if (style == BorderStyle::Groove || style == BorderStyle::Ridge)
                    {
                        float h = w * 0.5f;
                        ID2D1Brush* outerTopLeftBrush = (style == BorderStyle::Groove) ? darkBrush.Get() : lightBrush.Get();
                        ID2D1Brush* innerTopLeftBrush = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                        ID2D1Brush* outerBotRightBrush = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                        ID2D1Brush* innerBotRightBrush = (style == BorderStyle::Groove) ? darkBrush.Get() : lightBrush.Get();
                        D2D1_POINT_2F topOuterPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(L + h, T + h) };
                        drawPolygon(topOuterPts, 4, outerTopLeftBrush);
                        D2D1_POINT_2F topInnerPts[] = {
                            D2D1::Point2F(L + h, T + h),
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(L + w, T + w) };
                        drawPolygon(topInnerPts, 4, innerTopLeftBrush);
                        D2D1_POINT_2F leftOuterPts[] = {
                            D2D1::Point2F(L, T),
                            D2D1::Point2F(L + h, T + h),
                            D2D1::Point2F(L + h, B - h),
                            D2D1::Point2F(L, B) };
                        drawPolygon(leftOuterPts, 4, outerTopLeftBrush);
                        D2D1_POINT_2F leftInnerPts[] = {
                            D2D1::Point2F(L + h, T + h),
                            D2D1::Point2F(L + w, T + w),
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(L + h, B - h) };
                        drawPolygon(leftInnerPts, 4, innerTopLeftBrush);
                        D2D1_POINT_2F bottomOuterPts[] = {
                            D2D1::Point2F(L + h, B - h),
                            D2D1::Point2F(R - h, B - h),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(L, B) };
                        drawPolygon(bottomOuterPts, 4, outerBotRightBrush);
                        D2D1_POINT_2F bottomInnerPts[] = {
                            D2D1::Point2F(L + w, B - w),
                            D2D1::Point2F(R - w, B - w),
                            D2D1::Point2F(R - h, B - h),
                            D2D1::Point2F(L + h, B - h) };
                        drawPolygon(bottomInnerPts, 4, innerBotRightBrush);
                        D2D1_POINT_2F rightOuterPts[] = {
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(R, T),
                            D2D1::Point2F(R, B),
                            D2D1::Point2F(R - h, B - h) };
                        drawPolygon(rightOuterPts, 4, outerBotRightBrush);
                        D2D1_POINT_2F rightInnerPts[] = {
                            D2D1::Point2F(R - w, T + w),
                            D2D1::Point2F(R - h, T + h),
                            D2D1::Point2F(R - h, B - h),
                            D2D1::Point2F(R - w, B - w) };
                        drawPolygon(rightInnerPts, 4, innerBotRightBrush);
                    }
                    context->SetAntialiasMode(oldMode);
                }
                else
                {
                    float half = width / 2.0f;
                    float L = r.rect.left, T = r.rect.top;
                    float R = r.rect.right, B = r.rect.bottom;
                    Microsoft::WRL::ComPtr<ID2D1Factory> d2dFactory;
                    context->GetFactory(&d2dFactory);
                    if (d2dFactory)
                    {
                        float pad = width * 4.0f + 100.0f;
                        float dx = R - L;
                        float dy = T - B;
                        float len = sqrtf(dx * dx + dy * dy);
                        D2D1_POINT_2F pTR_ext = D2D1::Point2F(R, T);
                        D2D1_POINT_2F pBL_ext = D2D1::Point2F(L, B);
                        if (len > 0.001f)
                        {
                            float ux = dx / len;
                            float uy = dy / len;
                            pTR_ext = D2D1::Point2F(R + ux * pad, T + uy * pad);
                            pBL_ext = D2D1::Point2F(L - ux * pad, B - uy * pad);
                        }
                        Microsoft::WRL::ComPtr<ID2D1PathGeometry> tlClip;
                        {
                            Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                            if (SUCCEEDED(d2dFactory->CreatePathGeometry(&tlClip)) &&
                                SUCCEEDED(tlClip->Open(&sink)))
                            {
                                sink->BeginFigure(pBL_ext, D2D1_FIGURE_BEGIN_FILLED);
                                sink->AddLine(pTR_ext);
                                sink->AddLine(D2D1::Point2F(pTR_ext.x - pad, pTR_ext.y - pad));
                                sink->AddLine(D2D1::Point2F(pBL_ext.x - pad, pBL_ext.y - pad));
                                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                                sink->Close();
                            }
                        }
                        Microsoft::WRL::ComPtr<ID2D1PathGeometry> brClip;
                        {
                            Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                            if (SUCCEEDED(d2dFactory->CreatePathGeometry(&brClip)) &&
                                SUCCEEDED(brClip->Open(&sink)))
                            {
                                sink->BeginFigure(pBL_ext, D2D1_FIGURE_BEGIN_FILLED);
                                sink->AddLine(pTR_ext);
                                sink->AddLine(D2D1::Point2F(pTR_ext.x + pad, pTR_ext.y + pad));
                                sink->AddLine(D2D1::Point2F(pBL_ext.x + pad, pBL_ext.y + pad));
                                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                                sink->Close();
                            }
                        }
                        auto drawSplitRing = [&](const D2D1_ROUNDED_RECT& rr, float sw,
                            ID2D1Brush* tlBrush, ID2D1Brush* brBrush)
                            {
                                if (tlClip && tlBrush)
                                {
                                    auto lp = D2D1::LayerParameters1(D2D1::InfiniteRect(), tlClip.Get());
                                    context->PushLayer(lp, nullptr);
                                    context->DrawRoundedRectangle(rr, tlBrush, sw, sstyle.Get());
                                    context->PopLayer();
                                }
                                if (brClip && brBrush)
                                {
                                    auto lp = D2D1::LayerParameters1(D2D1::InfiniteRect(), brClip.Get());
                                    context->PushLayer(lp, nullptr);
                                    context->DrawRoundedRectangle(rr, brBrush, sw, sstyle.Get());
                                    context->PopLayer();
                                }
                            };
                        if (style == BorderStyle::Inset || style == BorderStyle::Outset)
                        {
                            drawSplitRing(r, width, topLeftBrush, bottomRightBrush);
                        }
                        else
                        {
                            D2D1_ROUNDED_RECT rOuter = r, rInner = r;
                            rOuter.rect.left -= half / 2;
                            rOuter.rect.top -= half / 2;
                            rOuter.rect.right += half / 2;
                            rOuter.rect.bottom += half / 2;
                            rInner.rect.left += half / 2;
                            rInner.rect.top += half / 2;
                            rInner.rect.right -= half / 2;
                            rInner.rect.bottom -= half / 2;
                            rOuter.radiusX = r.radiusX + half / 2;
                            rOuter.radiusY = r.radiusY + half / 2;
                            rInner.radiusX = std::max(0.0f, r.radiusX - half / 2);
                            rInner.radiusY = std::max(0.0f, r.radiusY - half / 2);
                            ID2D1Brush* outerTl = (style == BorderStyle::Groove) ? darkBrush.Get() : lightBrush.Get();
                            ID2D1Brush* outerBr = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                            ID2D1Brush* innerTl = (style == BorderStyle::Groove) ? lightBrush.Get() : darkBrush.Get();
                            ID2D1Brush* innerBr = (style == BorderStyle::Groove) ? darkBrush.Get() : lightBrush.Get();
                            drawSplitRing(rOuter, half, outerTl, outerBr);
                            drawSplitRing(rInner, half, innerTl, innerBr);
                        }
                    }
                }
            }
            else if (style == BorderStyle::Dotted)
            {
                float L = r.rect.left, T = r.rect.top;
                float R = r.rect.right, B = r.rect.bottom;
                float rx = std::min(r.radiusX, (R - L) * 0.5f);
                float ry = std::min(r.radiusY, (B - T) * 0.5f);
                float L_top = R - L - 2.0f * rx;
                float L_bottom = L_top;
                float L_left = B - T - 2.0f * ry;
                float L_right = L_left;
                float L_arc = (rx > 0.0f || ry > 0.0f)
                    ? 1.570796f * sqrtf((rx * rx + ry * ry) / 2.0f)
                    : 0.0f;
                float perimeter = L_top + L_right + L_bottom + L_left + 4.0f * L_arc;
                if (perimeter > 0.001f)
                {
                    float ideal_spacing = width * 2.0f;
                    int num_dots = static_cast<int>(perimeter / ideal_spacing + 0.5f);
                    if (num_dots < 4)
                        num_dots = 4;
                    float step = perimeter / num_dots;
                    float r_dot = width / 2.0f;
                    auto getPoint = [&](float t, float& x, float& y)
                        {
                            if (t < 0.0f)
                                t = 0.0f;
                            if (t > perimeter)
                                t = perimeter;
                            if (t < L_top)
                            {
                                x = L + rx + t;
                                y = T;
                                return;
                            }
                            t -= L_top;
                            if (t < L_arc)
                            {
                                float theta = -1.570796f + 1.570796f * (t / L_arc);
                                x = R - rx + rx * cosf(theta);
                                y = T + ry + ry * sinf(theta);
                                return;
                            }
                            t -= L_arc;
                            if (t < L_right)
                            {
                                x = R;
                                y = T + ry + t;
                                return;
                            }
                            t -= L_right;
                            if (t < L_arc)
                            {
                                float theta = 0.0f + 1.570796f * (t / L_arc);
                                x = R - rx + rx * cosf(theta);
                                y = B - ry + ry * sinf(theta);
                                return;
                            }
                            t -= L_arc;
                            if (t < L_bottom)
                            {
                                x = R - rx - t;
                                y = B;
                                return;
                            }
                            t -= L_bottom;
                            if (t < L_arc)
                            {
                                float theta = 1.570796f + 1.570796f * (t / L_arc);
                                x = L + rx + rx * cosf(theta);
                                y = B - ry + ry * sinf(theta);
                                return;
                            }
                            t -= L_arc;
                            if (t < L_left)
                            {
                                x = L;
                                y = B - ry - t;
                                return;
                            }
                            t -= L_left;
                            {
                                float theta = 3.1415927f + 1.570796f * (t / L_arc);
                                x = L + rx + rx * cosf(theta);
                                y = T + ry + ry * sinf(theta);
                            }
                        };
                    for (int i = 0; i < num_dots; ++i)
                    {
                        float x = 0.0f, y = 0.0f;
                        getPoint(static_cast<float>(i) * step, x, y);
                        context->FillEllipse(D2D1::Ellipse(D2D1::Point2F(x, y), r_dot, r_dot), brush);
                    }
                }
            }
            else if (style == BorderStyle::Dashed && m_RadiusX == 0.0f && m_RadiusY == 0.0f)
            {
                float left = r.rect.left;
                float top = r.rect.top;
                float right = r.rect.right;
                float bottom = r.rect.bottom;
                float dashLen = width * 3.0f;
                float gap = width * 1.0f;
                float period = dashLen + gap;
                context->FillRectangle(D2D1::RectF(left, top, left + dashLen, top + width), brush);
                context->FillRectangle(D2D1::RectF(left, top, left + width, top + dashLen), brush);
                context->FillRectangle(D2D1::RectF(right - dashLen, top, right, top + width), brush);
                context->FillRectangle(D2D1::RectF(right - width, top, right, top + dashLen), brush);
                context->FillRectangle(D2D1::RectF(right - dashLen, bottom - width, right, bottom), brush);
                context->FillRectangle(D2D1::RectF(right - width, bottom - dashLen, right, bottom), brush);
                context->FillRectangle(D2D1::RectF(left, bottom - width, left + dashLen, bottom), brush);
                context->FillRectangle(D2D1::RectF(left, bottom - dashLen, left + width, bottom), brush);
                auto fillDashes = [&](float runStart, float runEnd, bool isHoriz, float edgeCoord)
                    {
                        float runLen = runEnd - runStart;
                        if (runLen <= 0.0f)
                            return;
                        int n = (runLen >= gap + dashLen)
                            ? static_cast<int>((runLen - gap) / period)
                            : 0;
                        if (n < 1)
                            return;
                        float gapActual = (runLen - static_cast<float>(n) * dashLen) / static_cast<float>(n + 1);
                        for (int i = 0; i < n; ++i)
                        {
                            float s = runStart + gapActual + static_cast<float>(i) * (dashLen + gapActual);
                            float e = s + dashLen;
                            if (isHoriz)
                                context->FillRectangle(D2D1::RectF(s, edgeCoord, e, edgeCoord + width), brush);
                            else
                                context->FillRectangle(D2D1::RectF(edgeCoord, s, edgeCoord + width, e), brush);
                        }
                    };
                fillDashes(left + dashLen, right - dashLen, true, top);
                fillDashes(left + dashLen, right - dashLen, true, bottom - width);
                fillDashes(top + dashLen, bottom - dashLen, false, left);
                fillDashes(top + dashLen, bottom - dashLen, false, right - width);
            }
            else
            {
                if (r.radiusX == 0.0f && r.radiusY == 0.0f)
                {
                    context->DrawRectangle(r.rect, brush, width, sstyle.Get());
                }
                else
                {
                    context->DrawRoundedRectangle(r, brush, width, sstyle.Get());
                }
            }
        };
    if (sameStyle)
    {
        drawStyleRect(rect, m_BorderStyleTop, strokeBrush, strokeWidth);
    }
    else
    {
        static bool s_loggedMixedBorderDebug = false;
        const float L = std::floor(rect.rect.left) + 0.5f;
        const float T = std::floor(rect.rect.top) + 0.5f;
        const float R = std::floor(rect.rect.right) + 0.5f;
        const float B = std::floor(rect.rect.bottom) + 0.5f;
        const float w = strokeWidth;
        const float eps = std::max(0.5f, w * 0.125f);
        if (w <= 0.0f || R <= L || B <= T)
            return;
        ID2D1Factory1* factory = Direct2D::GetFactory();
        if (!factory)
            return;
        auto makePolyClip = [&](const D2D1_POINT_2F* pts) -> Microsoft::WRL::ComPtr<ID2D1Geometry>
            {
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                if (FAILED(factory->CreatePathGeometry(&path)) || FAILED(path->Open(&sink)))
                    return nullptr;
                sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(pts[1]);
                sink->AddLine(pts[2]);
                sink->AddLine(pts[3]);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                Microsoft::WRL::ComPtr<ID2D1Geometry> g = path;
                return g;
            };
        D2D1_POINT_2F topPts[4] = {
            D2D1::Point2F(L - eps, T - eps), D2D1::Point2F(R + eps, T - eps),
            D2D1::Point2F(R - w + eps, T + w + eps), D2D1::Point2F(L + w - eps, T + w + eps) };
        D2D1_POINT_2F rightPts[4] = {
            D2D1::Point2F(R + eps, T - eps), D2D1::Point2F(R + eps, B + eps),
            D2D1::Point2F(R - w - eps, B - w + eps), D2D1::Point2F(R - w - eps, T + w - eps) };
        D2D1_POINT_2F bottomPts[4] = {
            D2D1::Point2F(L - eps, B + eps), D2D1::Point2F(R + eps, B + eps),
            D2D1::Point2F(R - w + eps, B - w - eps), D2D1::Point2F(L + w - eps, B - w - eps) };
        D2D1_POINT_2F leftPts[4] = {
            D2D1::Point2F(L - eps, T - eps), D2D1::Point2F(L - eps, B + eps),
            D2D1::Point2F(L + w + eps, B - w + eps), D2D1::Point2F(L + w + eps, T + w - eps) };
        auto topClip = makePolyClip(topPts);
        auto rightClip = makePolyClip(rightPts);
        auto bottomClip = makePolyClip(bottomPts);
        auto leftClip = makePolyClip(leftPts);
        struct SidePaint
        {
            BorderStyle style;
            Microsoft::WRL::ComPtr<ID2D1Geometry> clip;
            int priority;
            D2D1_POINT_2F p0;
            D2D1_POINT_2F p1;
            int index;
        };
        auto fillPolygon = [&](const D2D1_POINT_2F* pts, int count, ID2D1Brush* brush = nullptr)
            {
                if (count < 3)
                    return;
                if (!brush)
                    brush = strokeBrush;
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                if (FAILED(factory->CreatePathGeometry(&path)) || FAILED(path->Open(&sink)))
                    return;
                sink->BeginFigure(pts[0], D2D1_FIGURE_BEGIN_FILLED);
                for (int i = 1; i < count; ++i)
                    sink->AddLine(pts[i]);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                context->FillGeometry(path.Get(), brush);
            };
        auto drawBlinkSolidSideWithBrush = [&](int side, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2, ID2D1Brush* brush)
            {
                if (x2 <= x1 || y2 <= y1)
                    return;
                if (adjacentWidth1 == 0.0f && adjacentWidth2 == 0.0f)
                {
                    context->FillRectangle(D2D1::RectF(x1, y1, x2, y2), brush);
                    return;
                }
                D2D1_POINT_2F quad[4];
                switch (side)
                {
                case 0:
                    quad[0] = D2D1::Point2F(x1 + std::max(-adjacentWidth1, 0.0f), y1);
                    quad[1] = D2D1::Point2F(x1 + std::max(adjacentWidth1, 0.0f), y2);
                    quad[2] = D2D1::Point2F(x2 - std::max(adjacentWidth2, 0.0f), y2);
                    quad[3] = D2D1::Point2F(x2 - std::max(-adjacentWidth2, 0.0f), y1);
                    break;
                case 2:
                    quad[0] = D2D1::Point2F(x1 + std::max(adjacentWidth1, 0.0f), y1);
                    quad[1] = D2D1::Point2F(x1 + std::max(-adjacentWidth1, 0.0f), y2);
                    quad[2] = D2D1::Point2F(x2 - std::max(-adjacentWidth2, 0.0f), y2);
                    quad[3] = D2D1::Point2F(x2 - std::max(adjacentWidth2, 0.0f), y1);
                    break;
                case 3:
                    quad[0] = D2D1::Point2F(x1, y1 + std::max(-adjacentWidth1, 0.0f));
                    quad[1] = D2D1::Point2F(x1, y2 - std::max(-adjacentWidth2, 0.0f));
                    quad[2] = D2D1::Point2F(x2, y2 - std::max(adjacentWidth2, 0.0f));
                    quad[3] = D2D1::Point2F(x2, y1 + std::max(adjacentWidth1, 0.0f));
                    break;
                case 1:
                default:
                    quad[0] = D2D1::Point2F(x1, y1 + std::max(adjacentWidth1, 0.0f));
                    quad[1] = D2D1::Point2F(x1, y2 - std::max(adjacentWidth2, 0.0f));
                    quad[2] = D2D1::Point2F(x2, y2 - std::max(-adjacentWidth2, 0.0f));
                    quad[3] = D2D1::Point2F(x2, y1 + std::max(-adjacentWidth1, 0.0f));
                    break;
                }
                fillPolygon(quad, 4, brush);
            };
        auto drawBlinkSolidSide = [&](int side, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2)
            {
                drawBlinkSolidSideWithBrush(side, x1, y1, x2, y2, adjacentWidth1, adjacentWidth2, strokeBrush);
            };
        auto drawBlinkDoubleSide = [&](int side, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2)
            {
                const float thickness = (side == 0 || side == 2) ? (y2 - y1) : (x2 - x1);
                const float length = (side == 0 || side == 2) ? (x2 - x1) : (y2 - y1);
                if (thickness <= 0.0f || length <= 0.0f)
                    return;
                const float third = std::max(1.0f, std::floor((thickness + 1.0f) / 3.0f));
                if (adjacentWidth1 == 0.0f && adjacentWidth2 == 0.0f)
                {
                    if (side == 0 || side == 2)
                    {
                        context->FillRectangle(D2D1::RectF(x1, y1, x2, y1 + third), strokeBrush);
                        context->FillRectangle(D2D1::RectF(x1, y2 - third, x2, y2), strokeBrush);
                    }
                    else
                    {
                        context->FillRectangle(D2D1::RectF(x1, y1, x1 + third, y2), strokeBrush);
                        context->FillRectangle(D2D1::RectF(x2 - third, y1, x2, y2), strokeBrush);
                    }
                    return;
                }
                auto bigThird = [](float value)
                    {
                        return value > 0.0f ? std::floor((value + 1.0f) / 3.0f) : std::ceil((value - 1.0f) / 3.0f);
                    };
                const float adjacent1BigThird = bigThird(adjacentWidth1);
                const float adjacent2BigThird = bigThird(adjacentWidth2);
                switch (side)
                {
                case 0:
                    drawBlinkSolidSide(side,
                        x1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1,
                        x2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1 + third,
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2 - third,
                        x2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2,
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                case 3:
                    drawBlinkSolidSide(side,
                        x1,
                        y1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x1 + third,
                        y2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x2 - third,
                        y1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x2,
                        y2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                case 2:
                    drawBlinkSolidSide(side,
                        x1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1,
                        x2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y1 + third,
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2 - third,
                        x2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        y2,
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                case 1:
                default:
                    drawBlinkSolidSide(side,
                        x1,
                        y1 + std::max((adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x1 + third,
                        y2 - std::max((adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    drawBlinkSolidSide(side,
                        x2 - third,
                        y1 + std::max((-adjacentWidth1 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        x2,
                        y2 - std::max((-adjacentWidth2 * 2.0f + 1.0f) / 3.0f, 0.0f),
                        adjacent1BigThird, adjacent2BigThird);
                    break;
                }
            };
        COLORREF baseColor = m_StrokeColor;
        BYTE rC = GetRValue(baseColor), gC = GetGValue(baseColor), bC = GetBValue(baseColor);
        COLORREF lightC = RGB(std::min(255, rC + 50), std::min(255, gC + 50), std::min(255, bC + 50));
        COLORREF darkC = RGB(std::max(0, rC - 50), std::max(0, gC - 50), std::max(0, bC - 50));
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> lightBrush, darkBrush;
        context->CreateSolidColorBrush(Direct2D::ColorToD2D(lightC, m_StrokeAlpha / 255.0f), &lightBrush);
        context->CreateSolidColorBrush(Direct2D::ColorToD2D(darkC, m_StrokeAlpha / 255.0f), &darkBrush);
        auto drawBlink3DSide = [&](int side, BorderStyle style, float x1, float y1, float x2, float y2,
            float adjacentWidth1, float adjacentWidth2)
            {
                const float thickness = (side == 0 || side == 2) ? (y2 - y1) : (x2 - x1);
                if (thickness <= 0.0f)
                    return;
                auto bigHalf = [](float value)
                    {
                        return value > 0.0f ? std::floor((value + 1.0f) / 2.0f) : std::ceil((value - 1.0f) / 2.0f);
                    };
                const float adjacent1BigHalf = bigHalf(adjacentWidth1);
                const float adjacent2BigHalf = bigHalf(adjacentWidth2);
                ID2D1Brush* insetBrush = (side == 0 || side == 3) ? darkBrush.Get() : lightBrush.Get();
                ID2D1Brush* outsetBrush = (side == 0 || side == 3) ? lightBrush.Get() : darkBrush.Get();
                if (style == BorderStyle::Inset || style == BorderStyle::Outset)
                {
                    ID2D1Brush* solidBrush = (style == BorderStyle::Inset) ? insetBrush : outsetBrush;
                    drawBlinkSolidSideWithBrush(side, x1, y1, x2, y2, adjacentWidth1, adjacentWidth2, solidBrush);
                    return;
                }
                ID2D1Brush* firstBrush = (style == BorderStyle::Groove) ? insetBrush : outsetBrush;
                ID2D1Brush* secondBrush = (style == BorderStyle::Groove) ? outsetBrush : insetBrush;
                switch (side)
                {
                case 0:
                    drawBlinkSolidSideWithBrush(0,
                        x1 + std::max(-adjacentWidth1, 0.0f) / 2.0f,
                        y1,
                        x2 - std::max(-adjacentWidth2, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        adjacent1BigHalf, adjacent2BigHalf, firstBrush);
                    drawBlinkSolidSideWithBrush(0,
                        x1 + std::max(adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        x2 - std::max(adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        y2,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, secondBrush);
                    break;
                case 2:
                    drawBlinkSolidSideWithBrush(2,
                        x1 + std::max(adjacentWidth1, 0.0f) / 2.0f,
                        y1,
                        x2 - std::max(adjacentWidth2, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        adjacent1BigHalf, adjacent2BigHalf, secondBrush);
                    drawBlinkSolidSideWithBrush(2,
                        x1 + std::max(-adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        std::floor((y1 + y2 + 1.0f) / 2.0f),
                        x2 - std::max(-adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        y2,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, firstBrush);
                    break;
                case 3:
                    drawBlinkSolidSideWithBrush(3,
                        x1,
                        y1 + std::max(-adjacentWidth1, 0.0f) / 2.0f,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y2 - std::max(-adjacentWidth2, 0.0f) / 2.0f,
                        adjacent1BigHalf, adjacent2BigHalf, firstBrush);
                    drawBlinkSolidSideWithBrush(3,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y1 + std::max(adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        x2,
                        y2 - std::max(adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, secondBrush);
                    break;
                case 1:
                default:
                    drawBlinkSolidSideWithBrush(1,
                        x1,
                        y1 + std::max(adjacentWidth1, 0.0f) / 2.0f,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y2 - std::max(adjacentWidth2, 0.0f) / 2.0f,
                        adjacent1BigHalf, adjacent2BigHalf, secondBrush);
                    drawBlinkSolidSideWithBrush(1,
                        std::floor((x1 + x2 + 1.0f) / 2.0f),
                        y1 + std::max(-adjacentWidth1 + 1.0f, 0.0f) / 2.0f,
                        x2,
                        y2 - std::max(-adjacentWidth2 + 1.0f, 0.0f) / 2.0f,
                        adjacentWidth1 / 2.0f, adjacentWidth2 / 2.0f, firstBrush);
                    break;
                }
            };
        auto stylePriority = [](BorderStyle s) -> int
            {
                if (s == BorderStyle::Dotted || s == BorderStyle::Dashed || s == BorderStyle::Double)
                    return 1;
                if (s == BorderStyle::None || s == BorderStyle::Hidden)
                    return 0;
                return 2;
            };
        const D2D1_POINT_2F topP0 = D2D1::Point2F(L + w * 0.5f, T + w * 0.5f);
        const D2D1_POINT_2F topP1 = D2D1::Point2F(R - w * 0.5f, T + w * 0.5f);
        const D2D1_POINT_2F rightP0 = D2D1::Point2F(R - w * 0.5f, T + w * 0.5f);
        const D2D1_POINT_2F rightP1 = D2D1::Point2F(R - w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F bottomP0 = D2D1::Point2F(R - w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F bottomP1 = D2D1::Point2F(L + w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F leftP0 = D2D1::Point2F(L + w * 0.5f, B - w * 0.5f);
        const D2D1_POINT_2F leftP1 = D2D1::Point2F(L + w * 0.5f, T + w * 0.5f);
        std::vector<SidePaint> sides = {
            {m_BorderStyleTop, topClip, stylePriority(m_BorderStyleTop), topP0, topP1, 0},
            {m_BorderStyleRight, rightClip, stylePriority(m_BorderStyleRight), rightP0, rightP1, 1},
            {m_BorderStyleBottom, bottomClip, stylePriority(m_BorderStyleBottom), bottomP0, bottomP1, 2},
            {m_BorderStyleLeft, leftClip, stylePriority(m_BorderStyleLeft), leftP0, leftP1, 3} };
        std::stable_sort(sides.begin(), sides.end(), [](const SidePaint& a, const SidePaint& b)
            { return a.priority < b.priority; });
        auto isVisibleStyleForSide = [](BorderStyle style)
            {
                return style != BorderStyle::None && style != BorderStyle::Hidden;
            };
        auto is3DStyleForSide = [](BorderStyle style)
            {
                return style == BorderStyle::Groove || style == BorderStyle::Ridge ||
                    style == BorderStyle::Inset || style == BorderStyle::Outset;
            };
        auto styleName = [](BorderStyle style) -> const wchar_t*
            {
                switch (style)
                {
                case BorderStyle::None:
                    return L"none";
                case BorderStyle::Hidden:
                    return L"hidden";
                case BorderStyle::Dotted:
                    return L"dotted";
                case BorderStyle::Dashed:
                    return L"dashed";
                case BorderStyle::Solid:
                    return L"solid";
                case BorderStyle::Double:
                    return L"double";
                case BorderStyle::Groove:
                    return L"groove";
                case BorderStyle::Ridge:
                    return L"ridge";
                case BorderStyle::Inset:
                    return L"inset";
                case BorderStyle::Outset:
                    return L"outset";
                default:
                    return L"unknown";
                }
            };
        if (!s_loggedMixedBorderDebug)
        {
            s_loggedMixedBorderDebug = true;
            Logging::Log(LogLevel::Info,
                L"[LAYOUTBOX_BORDER_DBG] mixed rect=[%.1f,%.1f,%.1f,%.1f] w=%.1f styles top=%s right=%s bottom=%s left=%s",
                L, T, R, B, w,
                styleName(m_BorderStyleTop), styleName(m_BorderStyleRight),
                styleName(m_BorderStyleBottom), styleName(m_BorderStyleLeft));
            Logging::Log(LogLevel::Info,
                L"[LAYOUTBOX_BORDER_DBG] bottom-left join bottom=%s left=%s visibleBottom=%d visibleLeft=%d",
                styleName(m_BorderStyleBottom), styleName(m_BorderStyleLeft),
                isVisibleStyleForSide(m_BorderStyleBottom) ? 1 : 0,
                isVisibleStyleForSide(m_BorderStyleLeft) ? 1 : 0);
            Logging::Log(LogLevel::Info,
                L"[LAYOUTBOX_BORDER_DBG] bottom-left ownership leftIs3D=%d bottomSolidLeftAdjacentWidth=%.1f",
                is3DStyleForSide(m_BorderStyleLeft) ? 1 : 0,
                is3DStyleForSide(m_BorderStyleLeft) ? 0.0f : w);
        }
        for (const auto& side : sides)
        {
            if (!side.clip || side.style == BorderStyle::None || side.style == BorderStyle::Hidden)
                continue;
            const bool useSideClip = side.style == BorderStyle::Dotted || side.style == BorderStyle::Dashed;
            if (useSideClip)
            {
                D2D1_LAYER_PARAMETERS1 params = D2D1::LayerParameters1(
                    D2D1::InfiniteRect(),
                    side.clip.Get(),
                    D2D1_ANTIALIAS_MODE_PER_PRIMITIVE,
                    D2D1::Matrix3x2F::Identity(),
                    1.0f,
                    nullptr,
                    D2D1_LAYER_OPTIONS1_NONE);
                context->PushLayer(params, nullptr);
            }
            if (side.style == BorderStyle::Dotted || side.style == BorderStyle::Dashed)
            {
                const float dx = side.p1.x - side.p0.x;
                const float dy = side.p1.y - side.p0.y;
                const float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.001f)
                {
                    const float ux = dx / len;
                    const float uy = dy / len;
                    if (side.style == BorderStyle::Dotted)
                    {
                        const float targetSpacing = w * 2.0f;
                        int count = std::max(2, static_cast<int>(len / targetSpacing + 0.5f) + 1);
                        for (int i = 0; i < count; ++i)
                        {
                            const float t = len * static_cast<float>(i) / static_cast<float>(count - 1);
                            const D2D1_POINT_2F pt = D2D1::Point2F(
                                side.p0.x + ux * t,
                                side.p0.y + uy * t);
                            context->FillEllipse(D2D1::Ellipse(pt, w * 0.5f, w * 0.5f), strokeBrush);
                        }
                    }
                    else
                    {
                        const float totalLen = len + w;
                        int n = std::max(1, static_cast<int>((totalLen / w + 1.0f) / 4.0f + 0.5f));
                        const float g = totalLen / (4.0f * static_cast<float>(n) - 1.0f);
                        const float d = 3.0f * g;
                        const float period = d + g;
                        const float px = -uy * (w * 0.5f);
                        const float py = ux * (w * 0.5f);
                        for (int i = 0; i < n; ++i)
                        {
                            const float start = -w * 0.5f + static_cast<float>(i) * period;
                            const float s = start;
                            const float e = start + d;
                            const D2D1_POINT_2F a = D2D1::Point2F(side.p0.x + ux * s + px, side.p0.y + uy * s + py);
                            const D2D1_POINT_2F b = D2D1::Point2F(side.p0.x + ux * e + px, side.p0.y + uy * e + py);
                            const D2D1_POINT_2F c = D2D1::Point2F(side.p0.x + ux * e - px, side.p0.y + uy * e - py);
                            const D2D1_POINT_2F d_pt = D2D1::Point2F(side.p0.x + ux * s - px, side.p0.y + uy * s - py);
                            Microsoft::WRL::ComPtr<ID2D1PathGeometry> dashPath;
                            Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                            if (SUCCEEDED(factory->CreatePathGeometry(&dashPath)) &&
                                SUCCEEDED(dashPath->Open(&sink)))
                            {
                                sink->BeginFigure(a, D2D1_FIGURE_BEGIN_FILLED);
                                sink->AddLine(b);
                                sink->AddLine(c);
                                sink->AddLine(d_pt);
                                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                                sink->Close();
                                context->FillGeometry(dashPath.Get(), strokeBrush);
                            }
                        }
                    }
                }
            }
            else if (side.style == BorderStyle::Solid)
            {
                const BorderStyle adjacentStyle1 = side.index == 0 ? m_BorderStyleLeft : side.index == 1 ? m_BorderStyleTop
                    : side.index == 2 ? m_BorderStyleLeft
                    : m_BorderStyleTop;
                const BorderStyle adjacentStyle2 = side.index == 0 ? m_BorderStyleRight : side.index == 1 ? m_BorderStyleBottom
                    : side.index == 2 ? m_BorderStyleRight
                    : m_BorderStyleBottom;
                const float adjacent1 = (side.index == 2 && is3DStyleForSide(adjacentStyle1)) ? 0.0f : (isVisibleStyleForSide(adjacentStyle1) ? w : 0.0f);
                const float adjacent2 = isVisibleStyleForSide(adjacentStyle2) ? w : 0.0f;
                if (side.index == 0)
                    drawBlinkSolidSide(0, L, T, R, T + w, adjacent1, adjacent2);
                else if (side.index == 1)
                    drawBlinkSolidSide(1, R - w, T, R, B, adjacent1, adjacent2);
                else if (side.index == 2)
                    drawBlinkSolidSide(2, L, B - w, R, B, adjacent1, adjacent2);
                else
                    drawBlinkSolidSide(3, L, T, L + w, B, adjacent1, adjacent2);
            }
            else if (side.style == BorderStyle::Double)
            {
                const float adjacent1 = isVisibleStyleForSide(side.index == 0 ? m_BorderStyleLeft : side.index == 1 ? m_BorderStyleTop
                    : side.index == 2 ? m_BorderStyleLeft
                    : m_BorderStyleTop)
                    ? w
                    : 0.0f;
                const float adjacent2 = isVisibleStyleForSide(side.index == 0 ? m_BorderStyleRight : side.index == 1 ? m_BorderStyleBottom
                    : side.index == 2 ? m_BorderStyleRight
                    : m_BorderStyleBottom)
                    ? w
                    : 0.0f;
                if (side.index == 0)
                    drawBlinkDoubleSide(0, L, T, R, T + w, adjacent1, adjacent2);
                else if (side.index == 1)
                    drawBlinkDoubleSide(1, R - w, T, R, B, adjacent1, adjacent2);
                else if (side.index == 2)
                    drawBlinkDoubleSide(2, L, B - w, R, B, adjacent1, adjacent2);
                else
                    drawBlinkDoubleSide(3, L, T, L + w, B, adjacent1, adjacent2);
            }
            else
            {
                const float adjacent1 = isVisibleStyleForSide(side.index == 0 ? m_BorderStyleLeft : side.index == 1 ? m_BorderStyleTop
                    : side.index == 2 ? m_BorderStyleLeft
                    : m_BorderStyleTop)
                    ? w
                    : 0.0f;
                const float adjacent2 = isVisibleStyleForSide(side.index == 0 ? m_BorderStyleRight : side.index == 1 ? m_BorderStyleBottom
                    : side.index == 2 ? m_BorderStyleRight
                    : m_BorderStyleBottom)
                    ? w
                    : 0.0f;
                if (side.index == 0)
                    drawBlink3DSide(0, side.style, L, T, R, T + w, adjacent1, adjacent2);
                else if (side.index == 1)
                    drawBlink3DSide(1, side.style, R - w, T, R, B, adjacent1, adjacent2);
                else if (side.index == 2)
                    drawBlink3DSide(2, side.style, L, B - w, R, B, adjacent1, adjacent2);
                else
                    drawBlink3DSide(3, side.style, L, T, L + w, B, adjacent1, adjacent2);
            }
            if (useSideClip)
                context->PopLayer();
        }
        auto isVisibleStyle = [](BorderStyle s)
            {
                return s != BorderStyle::None && s != BorderStyle::Hidden;
            };
        auto isComplexJoinStyle = [](BorderStyle s)
            {
                return s == BorderStyle::Dotted || s == BorderStyle::Dashed ||
                    s == BorderStyle::Double || s == BorderStyle::Groove ||
                    s == BorderStyle::Ridge || s == BorderStyle::Inset ||
                    s == BorderStyle::Outset;
            };
        auto fillCornerBevel = [&](D2D1_POINT_2F a, D2D1_POINT_2F b, D2D1_POINT_2F c)
            {
                Microsoft::WRL::ComPtr<ID2D1PathGeometry> g;
                Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
                if (FAILED(factory->CreatePathGeometry(&g)) || FAILED(g->Open(&sink)))
                    return;
                sink->BeginFigure(a, D2D1_FIGURE_BEGIN_FILLED);
                sink->AddLine(b);
                sink->AddLine(c);
                sink->EndFigure(D2D1_FIGURE_END_CLOSED);
                sink->Close();
                context->FillGeometry(g.Get(), strokeBrush);
            };
        if (isVisibleStyle(m_BorderStyleTop) && isVisibleStyle(m_BorderStyleLeft) &&
            !isComplexJoinStyle(m_BorderStyleTop) && !isComplexJoinStyle(m_BorderStyleLeft))
            fillCornerBevel(
                D2D1::Point2F(L, T),
                D2D1::Point2F(L + w, T),
                D2D1::Point2F(L, T + w));
        if (isVisibleStyle(m_BorderStyleTop) && isVisibleStyle(m_BorderStyleRight) &&
            !isComplexJoinStyle(m_BorderStyleTop) && !isComplexJoinStyle(m_BorderStyleRight))
            fillCornerBevel(
                D2D1::Point2F(R - w, T),
                D2D1::Point2F(R, T),
                D2D1::Point2F(R, T + w));
        if (isVisibleStyle(m_BorderStyleBottom) && isVisibleStyle(m_BorderStyleLeft) &&
            !isComplexJoinStyle(m_BorderStyleBottom) && !isComplexJoinStyle(m_BorderStyleLeft))
            fillCornerBevel(
                D2D1::Point2F(L, B - w),
                D2D1::Point2F(L + w, B - w),
                D2D1::Point2F(L, B));
        if (isVisibleStyle(m_BorderStyleBottom) && isVisibleStyle(m_BorderStyleRight) &&
            !isComplexJoinStyle(m_BorderStyleBottom) && !isComplexJoinStyle(m_BorderStyleRight))
            fillCornerBevel(
                D2D1::Point2F(R - w, B - w),
                D2D1::Point2F(R, B - w),
                D2D1::Point2F(R, B));
    }
}