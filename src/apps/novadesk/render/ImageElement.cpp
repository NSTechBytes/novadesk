/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ImageElement.h"
#include "Direct2DHelper.h"
#include <d2d1effects.h>
#include <algorithm>
#include "../shared/Logging.h"

ImageElement::ImageElement(const std::wstring &id, int x, int y, int w, int h,
                           const std::wstring &path)
    : Element(ELEMENT_IMAGE, id, x, y, w, h),
      m_ImagePath(path)
{
}

ImageElement::~ImageElement()
{
}

void ImageElement::SetUseExifOrientation(bool enabled)
{
    if (m_UseExifOrientation == enabled)
        return;

    m_UseExifOrientation = enabled;
    m_D2DBitmap.Reset();
    m_pWICBitmap.Reset();
}

void ImageElement::SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin)
{
    if (w <= 0.0f || h <= 0.0f)
    {
        m_HasImageCrop = false;
        return;
    }

    m_HasImageCrop = true;
    m_ImageCropX = x;
    m_ImageCropY = y;
    m_ImageCropW = w;
    m_ImageCropH = h;
    m_ImageCropOrigin = origin;
}

bool ImageElement::ResolveImageCropRect(float imageWidth, float imageHeight, D2D1_RECT_F &rect) const
{
    if (!m_HasImageCrop || imageWidth <= 0.0f || imageHeight <= 0.0f || m_ImageCropW <= 0.0f || m_ImageCropH <= 0.0f)
    {
        return false;
    }

    float startX = m_ImageCropX;
    float startY = m_ImageCropY;

    switch (m_ImageCropOrigin)
    {
    case IMAGE_CROP_ORIGIN_TOP_RIGHT:
        startX = imageWidth + m_ImageCropX;
        break;
    case IMAGE_CROP_ORIGIN_BOTTOM_RIGHT:
        startX = imageWidth + m_ImageCropX;
        startY = imageHeight + m_ImageCropY;
        break;
    case IMAGE_CROP_ORIGIN_BOTTOM_LEFT:
        startY = imageHeight + m_ImageCropY;
        break;
    case IMAGE_CROP_ORIGIN_CENTER:
        startX = (imageWidth * 0.5f) + m_ImageCropX;
        startY = (imageHeight * 0.5f) + m_ImageCropY;
        break;
    default:
        break;
    }

    float left = startX;
    float top = startY;
    float right = startX + m_ImageCropW;
    float bottom = startY + m_ImageCropH;

    left = (std::max)(0.0f, left);
    top = (std::max)(0.0f, top);
    right = (std::min)(imageWidth, right);
    bottom = (std::min)(imageHeight, bottom);

    if (right <= left || bottom <= top)
    {
        return false;
    }

    rect = D2D1::RectF(left, top, right, bottom);
    return true;
}

bool ImageElement::ComputeImageLayout(float imageWidth, float imageHeight, ImageLayout &layout)
{
    const int w = GetWidth();
    const int h = GetHeight();
    layout.contentX = m_X + m_PaddingLeft;
    layout.contentY = m_Y + m_PaddingTop;
    layout.contentW = w - m_PaddingLeft - m_PaddingRight;
    layout.contentH = h - m_PaddingTop - m_PaddingBottom;

    if (layout.contentW <= 0 || layout.contentH <= 0 || imageWidth <= 0.0f || imageHeight <= 0.0f)
    {
        return false;
    }

    layout.finalRect = D2D1::RectF(
        (float)layout.contentX,
        (float)layout.contentY,
        (float)(layout.contentX + layout.contentW),
        (float)(layout.contentY + layout.contentH));
    layout.srcRect = D2D1::RectF(0.0f, 0.0f, imageWidth, imageHeight);
    D2D1_RECT_F cropRect;
    if (ResolveImageCropRect(imageWidth, imageHeight, cropRect))
    {
        layout.srcRect = cropRect;
    }

    const float srcWidth = layout.srcRect.right - layout.srcRect.left;
    const float srcHeight = layout.srcRect.bottom - layout.srcRect.top;
    if (srcWidth <= 0.0f || srcHeight <= 0.0f)
    {
        return false;
    }

    if (m_PreserveAspectRatio == IMAGE_ASPECT_PRESERVE)
    {
        const float scaleX = (float)layout.contentW / srcWidth;
        const float scaleY = (float)layout.contentH / srcHeight;
        const float scale = (std::min)(scaleX, scaleY);
        const float finalW = srcWidth * scale;
        const float finalH = srcHeight * scale;

        layout.finalRect.left = layout.contentX + (layout.contentW - finalW) / 2.0f;
        layout.finalRect.top = layout.contentY + (layout.contentH - finalH) / 2.0f;
        layout.finalRect.right = layout.finalRect.left + finalW;
        layout.finalRect.bottom = layout.finalRect.top + finalH;
    }
    else if (m_PreserveAspectRatio == IMAGE_ASPECT_CROP)
    {
        const float scaleX = (float)layout.contentW / srcWidth;
        const float scaleY = (float)layout.contentH / srcHeight;
        const float scale = (std::max)(scaleX, scaleY);
        const float visibleW = layout.contentW / scale;
        const float visibleH = layout.contentH / scale;

        layout.srcRect.left += (srcWidth - visibleW) / 2.0f;
        layout.srcRect.top += (srcHeight - visibleH) / 2.0f;
        layout.srcRect.right = layout.srcRect.left + visibleW;
        layout.srcRect.bottom = layout.srcRect.top + visibleH;
    }

    return true;
}

bool ImageElement::MapPointToImagePixel(float targetX, float targetY, UINT imageWidth, UINT imageHeight, float &pixelX, float &pixelY)
{
    ImageLayout layout;
    if (!ComputeImageLayout((float)imageWidth, (float)imageHeight, layout))
    {
        return false;
    }

    if (m_PreserveAspectRatio == IMAGE_ASPECT_PRESERVE)
    {
        if (targetX < layout.finalRect.left || targetX >= layout.finalRect.right ||
            targetY < layout.finalRect.top || targetY >= layout.finalRect.bottom)
        {
            return false;
        }
    }
    else
    {
        if (targetX < layout.contentX || targetX >= layout.contentX + layout.contentW ||
            targetY < layout.contentY || targetY >= layout.contentY + layout.contentH)
        {
            return false;
        }
    }

    const float srcW = layout.srcRect.right - layout.srcRect.left;
    const float srcH = layout.srcRect.bottom - layout.srcRect.top;
    const float finalW = layout.finalRect.right - layout.finalRect.left;
    const float finalH = layout.finalRect.bottom - layout.finalRect.top;
    if (srcW <= 0.0f || srcH <= 0.0f || finalW <= 0.0f || finalH <= 0.0f)
    {
        return false;
    }

    pixelX = layout.srcRect.left + ((targetX - layout.finalRect.left) / finalW) * srcW;
    pixelY = layout.srcRect.top + ((targetY - layout.finalRect.top) / finalH) * srcH;

    // Match render-time flip so alpha hit testing uses the visible image pixel.
    switch (m_ImageFlip)
    {
    case IMAGE_FLIP_HORIZONTAL:
        pixelX = layout.srcRect.right - (pixelX - layout.srcRect.left) - 0.001f;
        break;
    case IMAGE_FLIP_VERTICAL:
        pixelY = layout.srcRect.bottom - (pixelY - layout.srcRect.top) - 0.001f;
        break;
    case IMAGE_FLIP_BOTH:
        pixelX = layout.srcRect.right - (pixelX - layout.srcRect.left) - 0.001f;
        pixelY = layout.srcRect.bottom - (pixelY - layout.srcRect.top) - 0.001f;
        break;
    default:
        break;
    }

    return pixelX >= 0.0f && pixelX < imageWidth && pixelY >= 0.0f && pixelY < imageHeight;
}

void ImageElement::EnsureBitmap(ID2D1DeviceContext *context)
{
    if (m_pLastTarget != (ID2D1RenderTarget *)context)
    {
        m_D2DBitmap.Reset();
        // WIC bitmap is CPU side, doesn't strictly need reset on target change,
        // but we'll re-verify it if we are reloading anyway.
        m_pLastTarget = (ID2D1RenderTarget *)context;
    }

    if (!m_D2DBitmap)
    {
        const bool ok = Direct2D::LoadBitmapFromFile(context, m_ImagePath, m_D2DBitmap.ReleaseAndGetAddressOf(), m_pWICBitmap.ReleaseAndGetAddressOf(), m_UseExifOrientation);
        if (!ok)
        {
            Logging::Log(LogLevel::Error, L"[novadesk] failed to load image bitmap: %s", m_ImagePath.c_str());
        }
    }
}

void ImageElement::UpdateImage(const std::wstring &path)
{
    m_ImagePath = path;
    m_D2DBitmap.Reset();
    m_pWICBitmap.Reset();
    const bool ok = Direct2D::LoadWICBitmapFromFile(m_ImagePath, m_pWICBitmap.ReleaseAndGetAddressOf(), m_UseExifOrientation);
    if (!ok)
    {
        Logging::Log(LogLevel::Error, L"[novadesk] failed to preload WIC image: %s", m_ImagePath.c_str());
    }
}

void ImageElement::Render(ID2D1DeviceContext *context)
{
    int w = GetWidth();
    int h = GetHeight();
    if (w <= 0 || h <= 0)
        return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    // Draw background and bevel inside the transform
    RenderBackground(context);
    RenderBevel(context);

    EnsureBitmap(context);
    if (!m_D2DBitmap)
    {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    D2D1_SIZE_F imgSize = m_D2DBitmap->GetSize();
    ImageLayout layout;
    if (!ComputeImageLayout(imgSize.width, imgSize.height, layout))
    {
        RestoreRenderTransform(context, originalTransform);
        return;
    }

    D2D1_MATRIX_3X2_F imageBaseTransform;
    bool hasImageFlipTransform = false;
    if (m_ImageFlip != IMAGE_FLIP_NONE)
    {
        float scaleX = 1.0f;
        float scaleY = 1.0f;
        if (m_ImageFlip == IMAGE_FLIP_HORIZONTAL || m_ImageFlip == IMAGE_FLIP_BOTH)
            scaleX = -1.0f;
        if (m_ImageFlip == IMAGE_FLIP_VERTICAL || m_ImageFlip == IMAGE_FLIP_BOTH)
            scaleY = -1.0f;

        const float centerX = (layout.finalRect.left + layout.finalRect.right) * 0.5f;
        const float centerY = (layout.finalRect.top + layout.finalRect.bottom) * 0.5f;
        const D2D1_MATRIX_3X2_F flipTransform =
            D2D1::Matrix3x2F::Translation(-centerX, -centerY) *
            D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
            D2D1::Matrix3x2F::Translation(centerX, centerY);

        context->GetTransform(&imageBaseTransform);
        context->SetTransform(flipTransform * imageBaseTransform);
        hasImageFlipTransform = true;
    }

    // Rotation already applied above

    float opacity = m_ImageAlpha / 255.0f;
    D2D1_BITMAP_INTERPOLATION_MODE interp = m_AntiAlias ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;

    Microsoft::WRL::ComPtr<ID2D1Image> pFinalImage = m_D2DBitmap.Get();

    if (m_Grayscale || m_HasImageTint || m_HasColorMatrix || m_ImageAlpha < 255)
    {
        Microsoft::WRL::ComPtr<ID2D1Image> pCurrentInput = (ID2D1Image *)m_D2DBitmap.Get();

        // Stage 1: Greyscale
        if (m_Grayscale)
        {
            Microsoft::WRL::ComPtr<ID2D1Effect> pGreyscaleEffect;
            if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, pGreyscaleEffect.GetAddressOf())))
            {
                D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
                    0.299f, 0.299f, 0.299f, 0.0f,
                    0.587f, 0.587f, 0.587f, 0.0f,
                    0.114f, 0.114f, 0.114f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
                    0.0f, 0.0f, 0.0f, 0.0f);
                pGreyscaleEffect->SetInput(0, pCurrentInput.Get());
                pGreyscaleEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
                pGreyscaleEffect->GetOutput(&pCurrentInput);
            }
        }

        // Stage 2: ColorMatrix, Tint, or ImageAlpha
        // We always run this stage if any of these are set, or if we need to apply ImageAlpha
        if (m_HasColorMatrix || m_HasImageTint || m_ImageAlpha < 255)
        {
            Microsoft::WRL::ComPtr<ID2D1Effect> pColorMatrixEffect;
            if (SUCCEEDED(context->CreateEffect(CLSID_D2D1ColorMatrix, pColorMatrixEffect.GetAddressOf())))
            {
                D2D1_MATRIX_5X4_F matrix = D2D1::Matrix5x4F(
                    1.0f, 0.0f, 0.0f, 0.0f,
                    0.0f, 1.0f, 0.0f, 0.0f,
                    0.0f, 0.0f, 1.0f, 0.0f,
                    0.0f, 0.0f, 0.0f, 1.0f,
                    0.0f, 0.0f, 0.0f, 0.0f);

                if (m_HasColorMatrix)
                {
                    memcpy(&matrix, m_ColorMatrix, sizeof(float) * 20);
                }
                else
                {
                    if (m_HasImageTint)
                    {
                        matrix.m[0][0] = GetRValue(m_ImageTint) / 255.0f;
                        matrix.m[1][1] = GetGValue(m_ImageTint) / 255.0f;
                        matrix.m[2][2] = GetBValue(m_ImageTint) / 255.0f;
                        matrix.m[3][3] = m_ImageTintAlpha / 255.0f;
                    }

                    // Multiply by ImageAlpha for Tint or Identity
                    matrix.m[3][3] *= (m_ImageAlpha / 255.0f);
                }

                pColorMatrixEffect->SetInput(0, pCurrentInput.Get());
                pColorMatrixEffect->SetValue(D2D1_COLORMATRIX_PROP_COLOR_MATRIX, matrix);
                pColorMatrixEffect->GetOutput(&pCurrentInput);
            }
        }

        pFinalImage = pCurrentInput;
    }

    if (m_Tile)
    {
        Microsoft::WRL::ComPtr<ID2D1ImageBrush> pBrush;
        D2D1_IMAGE_BRUSH_PROPERTIES brushProps = D2D1::ImageBrushProperties(
            layout.srcRect,
            D2D1_EXTEND_MODE_WRAP,
            D2D1_EXTEND_MODE_WRAP,
            D2D1_INTERPOLATION_MODE_LINEAR);
        context->CreateImageBrush(pFinalImage.Get(), brushProps, pBrush.GetAddressOf());
        if (pBrush)
        {
            pBrush->SetTransform(D2D1::Matrix3x2F::Translation(layout.finalRect.left - layout.srcRect.left, layout.finalRect.top - layout.srcRect.top));
            pBrush->SetOpacity(opacity);
            context->FillRectangle(layout.finalRect, pBrush.Get());
        }
    }
    else
    {
        if (pFinalImage.Get() == (ID2D1Image *)m_D2DBitmap.Get())
        {
            context->DrawBitmap(m_D2DBitmap.Get(), &layout.finalRect, opacity, interp, &layout.srcRect);
        }
        else
        {
            // For effects, we use DrawImage. Offset by finalRect top-left and scale.
            // DrawImage doesn't take a destination rect directly, it draws at offset.
            // We need to apply scaling to the transform if finalRect size != srcRect size.
            float scaleX = (layout.finalRect.right - layout.finalRect.left) / (layout.srcRect.right - layout.srcRect.left);
            float scaleY = (layout.finalRect.bottom - layout.finalRect.top) / (layout.srcRect.bottom - layout.srcRect.top);

            D2D1_MATRIX_3X2_F effectTransform;
            context->GetTransform(&effectTransform);

            // Adjust transform for scale and source rect offset
            context->SetTransform(
                D2D1::Matrix3x2F::Translation(-layout.srcRect.left, -layout.srcRect.top) *
                D2D1::Matrix3x2F::Scale(scaleX, scaleY) *
                D2D1::Matrix3x2F::Translation(layout.finalRect.left, layout.finalRect.top) *
                effectTransform);

            context->DrawImage(pFinalImage.Get(), nullptr, nullptr, m_AntiAlias ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
            context->SetTransform(effectTransform);
        }
    }

    if (hasImageFlipTransform)
    {
        context->SetTransform(imageBaseTransform);
    }

    RestoreRenderTransform(context, originalTransform);
}

int ImageElement::GetAutoWidth()
{
    if (m_pWICBitmap)
    {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        float autoW = (float)w;
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect((float)w, (float)h, cropRect))
        {
            autoW = cropRect.right - cropRect.left;
        }
        return (int)autoW + m_PaddingLeft + m_PaddingRight;
    }
    if (!m_D2DBitmap)
        return 0;
    D2D1_SIZE_F size = m_D2DBitmap->GetSize();
    float autoW = size.width;
    D2D1_RECT_F cropRect;
    if (ResolveImageCropRect(size.width, size.height, cropRect))
    {
        autoW = cropRect.right - cropRect.left;
    }
    return (int)autoW + m_PaddingLeft + m_PaddingRight;
}

int ImageElement::GetAutoHeight()
{
    if (m_pWICBitmap)
    {
        UINT w = 0, h = 0;
        m_pWICBitmap->GetSize(&w, &h);
        float autoH = (float)h;
        D2D1_RECT_F cropRect;
        if (ResolveImageCropRect((float)w, (float)h, cropRect))
        {
            autoH = cropRect.bottom - cropRect.top;
        }
        return (int)autoH + m_PaddingTop + m_PaddingBottom;
    }
    if (!m_D2DBitmap)
        return 0;
    D2D1_SIZE_F size = m_D2DBitmap->GetSize();
    float autoH = size.height;
    D2D1_RECT_F cropRect;
    if (ResolveImageCropRect(size.width, size.height, cropRect))
    {
        autoH = cropRect.bottom - cropRect.top;
    }
    return (int)autoH + m_PaddingTop + m_PaddingBottom;
}

bool ImageElement::HitTest(int x, int y)
{
    // Bounding box and transform check
    if (!Element::HitTest(x, y))
        return false;

    // Map the point to image local space
    float targetX = (float)x;
    float targetY = (float)y;

    if (m_HasTransformMatrix || m_Rotate != 0.0f)
    {
        GfxRect bounds = GetBounds();
        float centerX = bounds.X + bounds.Width / 2.0f;
        float centerY = bounds.Y + bounds.Height / 2.0f;

        D2D1::Matrix3x2F matrix;
        if (m_HasTransformMatrix)
        {
            matrix = D2D1::Matrix3x2F(
                m_TransformMatrix[0], m_TransformMatrix[1],
                m_TransformMatrix[2], m_TransformMatrix[3],
                m_TransformMatrix[4], m_TransformMatrix[5]);
        }
        else
        {
            matrix = D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
        }

        if (matrix.Invert())
        {
            D2D1_POINT_2F transformed = matrix.TransformPoint(D2D1::Point2F((float)x, (float)y));
            targetX = transformed.x;
            targetY = transformed.y;
        }
    }

    if (m_HasSolidColor && m_SolidAlpha > 0)
        return true;
    if (!m_pWICBitmap)
        return false;

    UINT imgW, imgH;
    m_pWICBitmap->GetSize(&imgW, &imgH);
    if (imgW == 0 || imgH == 0)
        return false;

    float targetPixelX = 0.0f;
    float targetPixelY = 0.0f;
    if (!MapPointToImagePixel(targetX, targetY, imgW, imgH, targetPixelX, targetPixelY))
        return false;

    // Get pixel alpha
    WICRect rect = {(INT)targetPixelX, (INT)targetPixelY, 1, 1};
    BYTE pixel[4]; // BGRA
    HRESULT hr = m_pWICBitmap->CopyPixels(&rect, 4, 4, pixel);
    if (FAILED(hr))
        return false;

    return pixel[3] > 0; // Alpha channel
}
