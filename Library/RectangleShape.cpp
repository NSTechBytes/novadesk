/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "RectangleShape.h"

RectangleShape::RectangleShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

RectangleShape::~RectangleShape()
{
}

void RectangleShape::Render(ID2D1DeviceContext* context)
{
    ID2D1Brush* pStrokeBrush = nullptr;
    if (m_HasStroke && m_StrokeWidth > 0) {
        CreateBrush(context, &pStrokeBrush, true);
    }

    ID2D1Brush* pFillBrush = nullptr;
    if (m_HasFill) {
        CreateBrush(context, &pFillBrush, false);
    }

    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF((float)m_X, (float)m_Y, (float)(m_X + m_Width), (float)(m_Y + m_Height));
    rect.radiusX = m_RadiusX;
    rect.radiusY = m_RadiusY; 

    if (pFillBrush) {
        context->FillRoundedRectangle(rect, pFillBrush);
    }
    if (pStrokeBrush) {
        UpdateStrokeStyle(context);
        context->DrawRoundedRectangle(rect, pStrokeBrush, m_StrokeWidth, m_StrokeStyle);
    }

    if (pStrokeBrush) pStrokeBrush->Release();
    if (pFillBrush) pFillBrush->Release();
}