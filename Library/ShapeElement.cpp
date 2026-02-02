/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ShapeElement.h"
#include "Utils.h"
#include "Logging.h"
#include "Direct2DHelper.h"
#include <d2d1effects.h>

ShapeElement::ShapeElement(const std::wstring& id, int x, int y, int width, int height)
    : Element(ELEMENT_SHAPE, id, x, y, width, height)
{
}

ShapeElement::~ShapeElement()
{
    if (m_StrokeStyle) {
        m_StrokeStyle->Release();
        m_StrokeStyle = nullptr;
    }
}

void ShapeElement::CreateBrush(ID2D1DeviceContext* context, ID2D1Brush** ppBrush, bool isStroke)
{
    bool hasGradient = isStroke ? m_HasStrokeGradient : m_HasFillGradient;

    D2D1_RECT_F rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));

    bool success = false;
    if (hasGradient) {
        const GradientInfo& gradInfo = isStroke ? m_StrokeGradient : m_FillGradient;
        success = Direct2D::CreateGradientBrush(context, rect, gradInfo, ppBrush);
    }

    if (!success) {

        COLORREF c = isStroke ? m_StrokeColor : m_FillColor;
        BYTE a = isStroke ? m_StrokeAlpha : m_FillAlpha;

        ID2D1SolidColorBrush* solidBrush = nullptr;
        if (Direct2D::CreateSolidBrush(context, c, a / 255.0f, &solidBrush)) {
            *ppBrush = solidBrush;
        }
    }
}

void ShapeElement::UpdateStrokeStyle(ID2D1DeviceContext* context)
{
    if (m_UpdateStrokeStyle || !m_StrokeStyle) {
        if (m_StrokeStyle) {
            m_StrokeStyle->Release();
            m_StrokeStyle = nullptr;
        }

        ID2D1Factory* factory = nullptr;
        context->GetFactory(&factory);
        ID2D1Factory1* factory1 = nullptr;

        if (factory) {
            factory->QueryInterface(&factory1);
            factory->Release();
        }

        if (factory1) {
            D2D1_STROKE_STYLE_PROPERTIES1 props = D2D1::StrokeStyleProperties1(
                m_StrokeStartCap,
                m_StrokeEndCap,
                m_StrokeDashCap,
                m_StrokeLineJoin,
                10.0f,

                (m_StrokeDashes.empty() ? D2D1_DASH_STYLE_SOLID : D2D1_DASH_STYLE_CUSTOM),
                m_StrokeDashOffset
            );

            factory1->CreateStrokeStyle(
                props,
                m_StrokeDashes.data(),
                (UINT32)m_StrokeDashes.size(),
                &m_StrokeStyle
            );

            factory1->Release();
        }
        m_UpdateStrokeStyle = false;
    }
}
