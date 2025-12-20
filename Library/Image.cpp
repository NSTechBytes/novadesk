/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Image.h"
#include "Logging.h"
#include <algorithm>

using namespace Gdiplus;

ImageElement::ImageElement(const std::wstring& id, int x, int y, int w, int h,
             const std::wstring& path)
    : Element(ELEMENT_IMAGE, id, x, y, w, h),
      m_ImagePath(path), m_Image(nullptr)
{
    LoadImage();
}

ImageElement::~ImageElement()
{
    if (m_Image)
    {
        delete m_Image;
        m_Image = nullptr;
    }
}

void ImageElement::LoadImage()
{
    if (m_Image)
    {
        delete m_Image;
        m_Image = nullptr;
    }
    
    m_Image = Image::FromFile(m_ImagePath.c_str());
    if (m_Image && m_Image->GetLastStatus() != Ok)
    {
        delete m_Image;
        m_Image = nullptr;
        Logging::Log(LogLevel::Error, L"Failed to load image: %s", m_ImagePath.c_str());
    }
}

void ImageElement::UpdateImage(const std::wstring& path)
{
    m_ImagePath = path;
    LoadImage();
}

void ImageElement::Render(Graphics& graphics)
{
    if (!m_Image) return;
    
    RectF destRect((REAL)m_X, (REAL)m_Y, (REAL)m_Width, (REAL)m_Height);
    graphics.DrawImage(m_Image, destRect);
}
