/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_PATH_SHAPE_H__
#define __NOVADESK_PATH_SHAPE_H__

#include "ShapeElement.h"
#include <string>

class PathShape : public ShapeElement
{
public:
    PathShape(const std::wstring& id, int x, int y, int width, int height);
    virtual ~PathShape();

    virtual void Render(ID2D1DeviceContext* context) override;
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    virtual GfxRect GetBounds() override;
    virtual bool HitTestLocal(const D2D1_POINT_2F& point) override;
    
    virtual void SetPathData(const std::wstring& pathData) override;

private:
    std::wstring m_PathData;
    bool m_HasPathBounds = false;
    float m_PathMinX = 0.0f;
    float m_PathMinY = 0.0f;
    float m_PathMaxX = 0.0f;
    float m_PathMaxY = 0.0f;

    void CreatePathGeometry(ID2D1Factory* factory, ID2D1PathGeometry** ppGeometry);
    void UpdatePathBounds();
};

#endif
