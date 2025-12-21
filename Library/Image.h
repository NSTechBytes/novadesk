/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __COSMOS_IMAGE_ELEMENT_H__
#define __COSMOS_IMAGE_ELEMENT_H__

#include "Element.h"

class ImageElement : public Element
{
public:
    ImageElement(const std::wstring& id, int x, int y, int w, int h,
                 const std::wstring& path);
                 
    virtual ~ImageElement();

    virtual void Render(Gdiplus::Graphics& graphics) override;
    
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;
    
    // Returns true if image loaded successfully
    bool IsLoaded() const { return m_Image != nullptr && m_Image->GetLastStatus() == Gdiplus::Ok; }
    
    void UpdateImage(const std::wstring& path);

    virtual bool HitTest(int x, int y) override;

private:
    std::wstring m_ImagePath;
    Gdiplus::Bitmap* m_Image;
    
    void LoadImage();
};

#endif
