/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once

#include "ShapeElement.h"

class ElementLayoutBox : public ShapeElement
{
public:
    struct BoxShadow
    {
        float x = 0.0f;
        float y = 0.0f;
        float blur = 0.0f;
        float spread = 0.0f;
        COLORREF color = RGB(0, 0, 0);
        BYTE alpha = 255;
        bool inset = false;
    };

    enum class BorderPosition
    {
        Outside,
        Center,
        Inside
    };

    ElementLayoutBox(const std::wstring &id, int x, int y, int width, int height);
    ~ElementLayoutBox() override = default;

    void Render(ID2D1DeviceContext *context) override;
    bool HitTestLocal(const D2D1_POINT_2F &point) override;
    bool CreateGeometry(ID2D1Factory *factory, Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;

    void SetRadii(float rx, float ry) override { m_RadiusX = rx; m_RadiusY = ry; }
    float GetRadiusX() const override { return m_RadiusX; }
    float GetRadiusY() const override { return m_RadiusY; }
    void SetBorderPosition(BorderPosition position) { m_BorderPosition = position; }
    BorderPosition GetBorderPosition() const { return m_BorderPosition; }
    void SetBoxShadows(const std::vector<BoxShadow> &shadows) { m_BoxShadows = shadows; }
    const std::vector<BoxShadow> &GetBoxShadows() const { return m_BoxShadows; }

private:
    void RenderSingleShadow(ID2D1DeviceContext *context, const D2D1_ROUNDED_RECT &baseRect, const BoxShadow &shadow);

    float m_RadiusX = 0.0f;
    float m_RadiusY = 0.0f;
    BorderPosition m_BorderPosition = BorderPosition::Outside;
    std::vector<BoxShadow> m_BoxShadows;
};
