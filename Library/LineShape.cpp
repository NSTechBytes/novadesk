/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "LineShape.h"

LineShape::LineShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

LineShape::~LineShape()
{
}

void LineShape::Render(ID2D1DeviceContext* context)
{
    ID2D1Brush* pStrokeBrush = nullptr;
    if (m_HasStroke && m_StrokeWidth > 0) {
        CreateBrush(context, &pStrokeBrush, true);
    }

    D2D1_POINT_2F start = D2D1::Point2F(m_StartX, m_StartY);
    D2D1_POINT_2F end = D2D1::Point2F(m_EndX, m_EndY);
    
    if (pStrokeBrush) {
        UpdateStrokeStyle(context);
        context->DrawLine(start, end, pStrokeBrush, m_StrokeWidth, m_StrokeStyle);
    }

    if (pStrokeBrush) pStrokeBrush->Release();
}