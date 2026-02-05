/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PathShape.h"
#include "Utils.h"
#include "Direct2DHelper.h"
#include <cmath>

#define NANOSVG_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4244)
#include "nanosvg/nanosvg.h"
#pragma warning(pop)

#include <vector>
#include <string>

PathShape::PathShape(const std::wstring& id, int x, int y, int width, int height)
    : ShapeElement(id, x, y, width, height)
{
}

PathShape::~PathShape()
{
}

void PathShape::SetPathData(const std::wstring& pathData)
{
    m_PathData = pathData;
    UpdatePathBounds();
}

int PathShape::GetAutoWidth()
{
    if (!m_HasPathBounds) return 0;
    return (int)ceilf(m_PathMaxX - m_PathMinX);
}

int PathShape::GetAutoHeight()
{
    if (!m_HasPathBounds) return 0;
    return (int)ceilf(m_PathMaxY - m_PathMinY);
}

GfxRect PathShape::GetBounds()
{
    if (!m_HasPathBounds) return ShapeElement::GetBounds();

    float pad = (m_StrokeWidth > 0.0f) ? (m_StrokeWidth / 2.0f) : 0.0f;
    int x = (int)floorf(m_X + m_PathMinX - pad);
    int y = (int)floorf(m_Y + m_PathMinY - pad);
    int w = (int)ceilf((m_PathMaxX - m_PathMinX) + pad * 2.0f);
    int h = (int)ceilf((m_PathMaxY - m_PathMinY) + pad * 2.0f);
    return GfxRect(x, y, w, h);
}

bool PathShape::HitTestLocal(const D2D1_POINT_2F& point)
{
    ID2D1Factory1* factory = Direct2D::GetFactory();
    if (!factory) return false;

    ID2D1PathGeometry* geometry = nullptr;
    CreatePathGeometry(factory, &geometry);
    if (!geometry) return false;

    BOOL hit = FALSE;
    if (m_HasFill && m_FillAlpha > 0) {
        if (SUCCEEDED(geometry->FillContainsPoint(point, nullptr, &hit)) && hit) {
            geometry->Release();
            return true;
        }
    }

    if (m_HasStroke && m_StrokeWidth > 0.0f && m_StrokeAlpha > 0) {
        EnsureStrokeStyle();
        hit = FALSE;
        if (SUCCEEDED(geometry->StrokeContainsPoint(point, m_StrokeWidth, m_StrokeStyle, nullptr, &hit)) && hit) {
            geometry->Release();
            return true;
        }
    }

    geometry->Release();
    return false;
}

void PathShape::UpdatePathBounds()
{
    m_HasPathBounds = false;
    m_PathMinX = 0.0f;
    m_PathMinY = 0.0f;
    m_PathMaxX = 0.0f;
    m_PathMaxY = 0.0f;

    if (m_PathData.empty()) return;

    std::string pathDataStr = Utils::ToString(m_PathData);
    std::string svgContent = "<svg><path d=\"" + pathDataStr + "\" /></svg>";

    std::vector<char> inputBuffer(svgContent.begin(), svgContent.end());
    inputBuffer.push_back('\0');

    NSVGimage* image = nsvgParse(inputBuffer.data(), "px", 96.0f);
    if (!image) return;

    bool anyPoint = false;

    for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {
            if (path->npts < 2) continue;
            float* pts = &path->pts[0];
            for (int i = 0; i < path->npts * 2; i += 2) {
                float x = pts[i];
                float y = pts[i + 1];
                if (!anyPoint) {
                    m_PathMinX = m_PathMaxX = x;
                    m_PathMinY = m_PathMaxY = y;
                    anyPoint = true;
                } else {
                    if (x < m_PathMinX) m_PathMinX = x;
                    if (y < m_PathMinY) m_PathMinY = y;
                    if (x > m_PathMaxX) m_PathMaxX = x;
                    if (y > m_PathMaxY) m_PathMaxY = y;
                }
            }
        }
    }

    nsvgDelete(image);
    m_HasPathBounds = anyPoint;
}

void PathShape::CreatePathGeometry(ID2D1Factory* factory, ID2D1PathGeometry** ppGeometry)
{
    if (m_PathData.empty()) return;

    std::string pathDataStr = Utils::ToString(m_PathData);

    std::string svgContent = "<svg><path d=\"" + pathDataStr + "\" /></svg>";

    std::vector<char> inputBuffer(svgContent.begin(), svgContent.end());
    inputBuffer.push_back('\0');

    NSVGimage* image = nsvgParse(inputBuffer.data(), "px", 96.0f);
    if (!image) return;

    factory->CreatePathGeometry(ppGeometry);
    if (!*ppGeometry) {
        nsvgDelete(image);
        return;
    }

    ID2D1GeometrySink* pSink = nullptr;
    (*ppGeometry)->Open(&pSink);
    if (!pSink) {
        (*ppGeometry)->Release();
        *ppGeometry = nullptr;
        nsvgDelete(image);
        return;
    }

    pSink->SetFillMode(D2D1_FILL_MODE_WINDING);

    for (NSVGshape* shape = image->shapes; shape != NULL; shape = shape->next) {
        for (NSVGpath* path = shape->paths; path != NULL; path = path->next) {

            if (path->npts < 2) continue;

            float* p = &path->pts[0];
            D2D1_POINT_2F startPt = D2D1::Point2F(m_X + p[0], m_Y + p[1]);

            pSink->BeginFigure(startPt, m_HasFill ? D2D1_FIGURE_BEGIN_FILLED : D2D1_FIGURE_BEGIN_HOLLOW);

            for (int i = 0; i < path->npts - 1; i += 3) {
                float* pt = &path->pts[i * 2];

                D2D1_POINT_2F cp1 = D2D1::Point2F(m_X + pt[2], m_Y + pt[3]);
                D2D1_POINT_2F cp2 = D2D1::Point2F(m_X + pt[4], m_Y + pt[5]);
                D2D1_POINT_2F end = D2D1::Point2F(m_X + pt[6], m_Y + pt[7]);

                pSink->AddBezier(D2D1::BezierSegment(cp1, cp2, end));
            }

            pSink->EndFigure(path->closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
        }
    }

    pSink->Close();
    pSink->Release();
    nsvgDelete(image);
}

void PathShape::Render(ID2D1DeviceContext* context)
{
    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    Microsoft::WRL::ComPtr<ID2D1Brush> pStrokeBrush;
    Microsoft::WRL::ComPtr<ID2D1Brush> pFillBrush;
    TryCreateStrokeBrush(context, pStrokeBrush);
    TryCreateFillBrush(context, pFillBrush);

    if (!pStrokeBrush && !pFillBrush) {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    ID2D1Factory* pFactory = nullptr;
    context->GetFactory(&pFactory);
    if (!pFactory) {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    ID2D1PathGeometry* pGeometry = nullptr;
    CreatePathGeometry(pFactory, &pGeometry);

    if (pGeometry) {
        if (pFillBrush) {
            context->FillGeometry(pGeometry, pFillBrush.Get());
        }
        if (pStrokeBrush) {
            UpdateStrokeStyle(context);
            context->DrawGeometry(pGeometry, pStrokeBrush.Get(), m_StrokeWidth, m_StrokeStyle);
        }
        pGeometry->Release();
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}

