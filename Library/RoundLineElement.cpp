/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "RoundLineElement.h"
#include "Direct2DHelper.h"
#include <cmath>

RoundLineElement::RoundLineElement(const std::wstring& id, int x, int y, int w, int h, float value)
    : Element(ELEMENT_ROUNDLINE, id, x, y, w, h), m_Value(value)
{
}

void RoundLineElement::Render(ID2D1DeviceContext* context)
{
    RenderBackground(context);

    float centerX = (float)m_X + (float)m_Width / 2.0f;
    float centerY = (float)m_Y + (float)m_Height / 2.0f;
    float radius = (float)m_Radius;

    if (radius <= 0) {
        radius = (min((float)m_Width, (float)m_Height) - (float)m_Thickness) / 2.0f;
    }

    if (radius <= 0) return;

    float startAngle = m_StartAngle - 90.0f; 
    float sweepAngle = m_TotalAngle * m_Value;

    auto drawArc = [&](float sAngle, float swAngle, ID2D1Brush* brush, float thickness, float endThick, RoundLineCap sCap, RoundLineCap eCap, const std::vector<float>& dashes) {
        if (abs(swAngle) < 0.001f) return;

        Microsoft::WRL::ComPtr<ID2D1PathGeometry> pPathGeometry;
        Direct2D::GetFactory()->CreatePathGeometry(&pPathGeometry);

        Microsoft::WRL::ComPtr<ID2D1GeometrySink> pSink;
        pPathGeometry->Open(&pSink);

        float sAngleRad = sAngle * 3.14159265f / 180.0f;
        D2D1_POINT_2F sPoint = D2D1::Point2F(centerX + radius * cos(sAngleRad), centerY + radius * sin(sAngleRad));

        pSink->BeginFigure(sPoint, D2D1_FIGURE_BEGIN_HOLLOW);

        if (abs(swAngle) >= 360.0f) {
            pSink->AddArc(D2D1::ArcSegment(
                D2D1::Point2F(centerX + radius * cos(sAngleRad + 3.14159265f), centerY + radius * sin(sAngleRad + 3.14159265f)),
                D2D1::SizeF(radius, radius), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
            pSink->AddArc(D2D1::ArcSegment(sPoint, D2D1::SizeF(radius, radius), 0.0f, D2D1_SWEEP_DIRECTION_CLOCKWISE, D2D1_ARC_SIZE_SMALL));
        } else {
            float actualSwAngle = m_Clockwise ? swAngle : -swAngle;
            float eAngleRad = (sAngle + actualSwAngle) * 3.14159265f / 180.0f;
            D2D1_POINT_2F ePoint = D2D1::Point2F(centerX + radius * cos(eAngleRad), centerY + radius * sin(eAngleRad));

            pSink->AddArc(D2D1::ArcSegment(
                ePoint, D2D1::SizeF(radius, radius), 0.0f,
                actualSwAngle > 0 ? D2D1_SWEEP_DIRECTION_CLOCKWISE : D2D1_SWEEP_DIRECTION_COUNTER_CLOCKWISE,
                abs(actualSwAngle) > 180.0f ? D2D1_ARC_SIZE_LARGE : D2D1_ARC_SIZE_SMALL));
        }

        pSink->EndFigure(D2D1_FIGURE_END_OPEN);
        pSink->Close();

        D2D1_STROKE_STYLE_PROPERTIES strokeProps = D2D1::StrokeStyleProperties();
        strokeProps.startCap = sCap == ROUNDLINE_CAP_ROUND ? D2D1_CAP_STYLE_ROUND : D2D1_CAP_STYLE_FLAT;
        strokeProps.endCap = eCap == ROUNDLINE_CAP_ROUND ? D2D1_CAP_STYLE_ROUND : D2D1_CAP_STYLE_FLAT;

        Microsoft::WRL::ComPtr<ID2D1StrokeStyle> pStrokeStyle;
        if (!dashes.empty()) {
            Direct2D::GetFactory()->CreateStrokeStyle(strokeProps, dashes.data(), (UINT32)dashes.size(), &pStrokeStyle);
        } else {
            Direct2D::GetFactory()->CreateStrokeStyle(strokeProps, nullptr, 0, &pStrokeStyle);
        }

        if (endThick != -1 && endThick != thickness) {
            // Tapering implementation (Approximation using multiple segments if needed, here we just use average for now or complex path)
            // For now, let's just draw twice with different thickness or simple average
            context->DrawGeometry(pPathGeometry.Get(), brush, (thickness + endThick) / 2.0f, pStrokeStyle.Get());
        } else {
            context->DrawGeometry(pPathGeometry.Get(), brush, (float)thickness, pStrokeStyle.Get());
        }
    };

    if (m_HasLineColorBg || m_LineGradientBg.type != GRADIENT_NONE) {
        Microsoft::WRL::ComPtr<ID2D1Brush> pBgBrush;
        bool bgBrushCreated = false;
        if (m_LineGradientBg.type != GRADIENT_NONE) {
             D2D1_RECT_F rect = D2D1::RectF((float)m_X, (float)m_Y, (float)m_X + m_Width, (float)m_Y + m_Height);
             bgBrushCreated = Direct2D::CreateGradientBrush(context, rect, m_LineGradientBg, &pBgBrush);
        }
        if (!bgBrushCreated) {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pSolidBg;
            Direct2D::CreateSolidBrush(context, m_LineColorBg, m_LineAlphaBg / 255.0f, &pSolidBg);
            pBgBrush = pSolidBg;
        }
        drawArc(startAngle, m_TotalAngle, pBgBrush.Get(), (float)m_Thickness, -1, m_StartCap, m_EndCap, {});
    }

    Microsoft::WRL::ComPtr<ID2D1Brush> pBrush;
    bool brushCreated = false;
    if (m_LineGradient.type != GRADIENT_NONE) {
        D2D1_RECT_F rect = D2D1::RectF((float)m_X, (float)m_Y, (float)m_X + m_Width, (float)m_Y + m_Height);
        brushCreated = Direct2D::CreateGradientBrush(context, rect, m_LineGradient, &pBrush);
    }
    
    if (!brushCreated) {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pSolidBrush;
        Direct2D::CreateSolidBrush(context, m_LineColor, m_LineAlpha / 255.0f, &pSolidBrush);
        pBrush = pSolidBrush;
    }

    if (pBrush) {
        drawArc(startAngle, sweepAngle, pBrush.Get(), (float)m_Thickness, (float)m_EndThickness, m_StartCap, m_EndCap, m_DashArray);
    }

    if (m_Ticks > 0) {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> pTickBrush;
        Direct2D::CreateSolidBrush(context, m_LineColor, m_LineAlpha / 255.0f, &pTickBrush);
        float tickAngleStep = m_TotalAngle / (float)m_Ticks;
        float tickLen = (float)m_Thickness * 1.5f;
        for (int i = 0; i <= m_Ticks; i++) {
            float angle = startAngle + i * tickAngleStep;
            if (!m_Clockwise) angle = startAngle - i * tickAngleStep;
            float rad = angle * 3.14159265f / 180.0f;
            float innerR = radius - tickLen / 2.0f;
            float outerR = radius + tickLen / 2.0f;
            context->DrawLine(
                D2D1::Point2F(centerX + innerR * cos(rad), centerY + innerR * sin(rad)),
                D2D1::Point2F(centerX + outerR * cos(rad), centerY + outerR * sin(rad)),
                pTickBrush.Get(), 2.0f
            );
        }
    }

    RenderBevel(context);
}
