/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "EllipseShape.h"

EllipseShape::EllipseShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

EllipseShape::~EllipseShape()
{
}

void EllipseShape::Render(ID2D1DeviceContext* context)
{
    ID2D1Brush* pStrokeBrush = nullptr;
    if (m_HasStroke && m_StrokeWidth > 0) {
        CreateBrush(context, &pStrokeBrush, true);
    }

    ID2D1Brush* pFillBrush = nullptr;
    if (m_HasFill) {
        CreateBrush(context, &pFillBrush, false);
    }

    D2D1_ELLIPSE ellipse;
    ellipse.point = D2D1::Point2F(m_X + m_Width/2.0f, m_Y + m_Height/2.0f);
    
    float rx = (m_Width > 0) ? m_Width / 2.0f : m_RadiusX;
    float ry = (m_Height > 0) ? m_Height / 2.0f : m_RadiusY;

    ellipse.radiusX = rx;
    ellipse.radiusY = ry;

    if (pFillBrush) {
        context->FillEllipse(ellipse, pFillBrush);
    }
    if (pStrokeBrush) {
        context->DrawEllipse(ellipse, pStrokeBrush, m_StrokeWidth);
    }

    if (pStrokeBrush) pStrokeBrush->Release();
    if (pFillBrush) pFillBrush->Release();
}