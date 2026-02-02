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
    
    virtual void SetPathData(const std::wstring& pathData) override { m_PathData = pathData; }

private:
    std::wstring m_PathData;
    
    void CreatePathGeometry(ID2D1Factory* factory, ID2D1PathGeometry** ppGeometry);
};

#endif
