/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_IMAGE_ELEMENT_H__
#define __NOVADESK_IMAGE_ELEMENT_H__

#include "Element.h"
#include <wrl/client.h>
#include <d2d1.h>

enum ImageAspectRatio
{
    IMAGE_ASPECT_STRETCH,  // 0
    IMAGE_ASPECT_PRESERVE, // 1
    IMAGE_ASPECT_CROP      // 2
};

enum ImageFlipMode
{
    IMAGE_FLIP_NONE = 0,
    IMAGE_FLIP_HORIZONTAL,
    IMAGE_FLIP_VERTICAL,
    IMAGE_FLIP_BOTH
};

enum ImageCropOrigin
{
    IMAGE_CROP_ORIGIN_TOP_LEFT = 0,
    IMAGE_CROP_ORIGIN_TOP_RIGHT = 1,
    IMAGE_CROP_ORIGIN_BOTTOM_RIGHT = 2,
    IMAGE_CROP_ORIGIN_BOTTOM_LEFT = 3,
    IMAGE_CROP_ORIGIN_CENTER = 4
};

class ImageElement : public Element
{
public:
    ImageElement(const std::wstring &id, int x, int y, int w, int h,
                 const std::wstring &path);

    virtual ~ImageElement();

    virtual void Render(ID2D1DeviceContext *context) override;

    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;

    // Returns true if image loaded successfully
    bool IsLoaded() const { return m_D2DBitmap != nullptr; }

    void UpdateImage(const std::wstring &path);

    virtual bool HitTest(int x, int y) override;

    void SetPreserveAspectRatio(ImageAspectRatio mode) { m_PreserveAspectRatio = mode; }
    void SetImageTint(COLORREF color, BYTE alpha)
    {
        m_ImageTint = color;
        m_ImageTintAlpha = alpha;
        m_HasImageTint = (alpha > 0);
    }

    void SetImageAlpha(BYTE alpha) { m_ImageAlpha = alpha; }
    void SetGrayscale(bool enable) { m_Grayscale = enable; }
    void SetColorMatrix(const float *matrix)
    {
        if (matrix)
        {
            memcpy(m_ColorMatrix, matrix, sizeof(float) * 20);
            m_HasColorMatrix = true;
        }
        else
        {
            m_HasColorMatrix = false;
        }
    }

    void SetTile(bool tile) { m_Tile = tile; }
    void SetImageFlip(ImageFlipMode flip) { m_ImageFlip = flip; }
    void SetUseExifOrientation(bool enabled);
    void SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin);
    void ClearImageCrop() { m_HasImageCrop = false; }
    void SetScaleMargins(float left, float top, float right, float bottom);
    void ClearScaleMargins() { m_HasScaleMargins = false; }

    const std::wstring &GetImagePath() const { return m_ImagePath; }
    ImageAspectRatio GetPreserveAspectRatio() const { return m_PreserveAspectRatio; }
    bool HasImageTint() const { return m_HasImageTint; }
    COLORREF GetImageTint() const { return m_ImageTint; }
    BYTE GetImageTintAlpha() const { return m_ImageTintAlpha; }
    BYTE GetImageAlpha() const { return m_ImageAlpha; }
    bool IsGrayscale() const { return m_Grayscale; }
    bool IsTile() const { return m_Tile; }
    ImageFlipMode GetImageFlip() const { return m_ImageFlip; }
    bool GetUseExifOrientation() const { return m_UseExifOrientation; }
    bool HasImageCrop() const { return m_HasImageCrop; }
    float GetImageCropX() const { return m_ImageCropX; }
    float GetImageCropY() const { return m_ImageCropY; }
    float GetImageCropW() const { return m_ImageCropW; }
    float GetImageCropH() const { return m_ImageCropH; }
    ImageCropOrigin GetImageCropOrigin() const { return m_ImageCropOrigin; }
    bool HasScaleMargins() const { return m_HasScaleMargins; }
    float GetScaleMarginLeft() const { return m_ScaleMarginLeft; }
    float GetScaleMarginTop() const { return m_ScaleMarginTop; }
    float GetScaleMarginRight() const { return m_ScaleMarginRight; }
    float GetScaleMarginBottom() const { return m_ScaleMarginBottom; }
    bool HasColorMatrix() const { return m_HasColorMatrix; }
    const float *GetColorMatrix() const { return (const float *)m_ColorMatrix; }

private:
    struct ImageLayout
    {
        int contentX = 0;
        int contentY = 0;
        int contentW = 0;
        int contentH = 0;
        D2D1_RECT_F finalRect = D2D1::RectF(0, 0, 0, 0);
        D2D1_RECT_F srcRect = D2D1::RectF(0, 0, 0, 0);
    };

    std::wstring m_ImagePath;
    Microsoft::WRL::ComPtr<ID2D1Bitmap> m_D2DBitmap;
    Microsoft::WRL::ComPtr<IWICBitmap> m_pWICBitmap;
    ImageAspectRatio m_PreserveAspectRatio = IMAGE_ASPECT_STRETCH;
    bool m_HasImageTint = false;
    COLORREF m_ImageTint = 0;
    BYTE m_ImageTintAlpha = 255;
    BYTE m_ImageAlpha = 255;
    bool m_Grayscale = false;
    bool m_HasColorMatrix = false;
    float m_ColorMatrix[20]; // D2D ColorMatrix effect uses 5x4
    bool m_Tile = false;
    ImageFlipMode m_ImageFlip = IMAGE_FLIP_NONE;
    bool m_UseExifOrientation = false;
    bool m_HasImageCrop = false;
    float m_ImageCropX = 0.0f;
    float m_ImageCropY = 0.0f;
    float m_ImageCropW = 0.0f;
    float m_ImageCropH = 0.0f;
    ImageCropOrigin m_ImageCropOrigin = IMAGE_CROP_ORIGIN_TOP_LEFT;
    bool m_HasScaleMargins = false;
    float m_ScaleMarginLeft = 0.0f;
    float m_ScaleMarginTop = 0.0f;
    float m_ScaleMarginRight = 0.0f;
    float m_ScaleMarginBottom = 0.0f;

    // Cache management
    ID2D1RenderTarget *m_pLastTarget = nullptr;

    void EnsureBitmap(ID2D1DeviceContext *context);
    bool ResolveImageCropRect(float imageWidth, float imageHeight, D2D1_RECT_F &rect) const;
    bool ComputeImageLayout(float imageWidth, float imageHeight, ImageLayout &layout);
    bool MapPointToImagePixel(float targetX, float targetY, UINT imageWidth, UINT imageHeight, float &pixelX, float &pixelY);
};

#endif
