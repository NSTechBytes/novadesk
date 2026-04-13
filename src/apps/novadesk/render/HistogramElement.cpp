/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "HistogramElement.h"
#include "Direct2DHelper.h"
#include <algorithm>

HistogramElement::HistogramElement(const std::wstring &id, int x, int y, int w, int h)
    : Element(ELEMENT_HISTOGRAM, id, x, y, w, h)
{
}

int HistogramElement::GetAutoWidth()
{
    if (!m_PrimaryImage.GetPath().empty())
    {
        return m_PrimaryImage.GetAutoWidth() + m_PaddingLeft + m_PaddingRight;
    }
    return 0;
}

int HistogramElement::GetAutoHeight()
{
    if (!m_PrimaryImage.GetPath().empty())
    {
        return m_PrimaryImage.GetAutoHeight() + m_PaddingTop + m_PaddingBottom;
    }
    return 0;
}

bool HistogramElement::BuildScale(float &outMin, float &outMax) const
{
    if (!m_AutoScale)
    {
        outMin = 0.0f;
        outMax = 100.0f;
        return true;
    }

    bool hasValue = false;
    float minV = 0.0f;
    float maxV = 0.0f;

    auto scanSeries = [&](const std::vector<float> &series)
    {
        for (float v : series)
        {
            if (!hasValue)
            {
                minV = maxV = v;
                hasValue = true;
            }
            else
            {
                minV = (std::min)(minV, v);
                maxV = (std::max)(maxV, v);
            }
        }
    };

    scanSeries(m_PrimaryData);
    scanSeries(m_SecondaryData);

    if (!hasValue)
    {
        outMin = 0.0f;
        outMax = 1.0f;
        return false;
    }

    if (maxV - minV < 0.000001f)
    {
        outMin = minV;
        outMax = minV + 1.0f;
        return true;
    }

    outMin = minV;
    outMax = maxV;
    return true;
}

float HistogramElement::SampleAtFromNewest(const std::vector<float> &series, int sampleIndex) const
{
    if (series.empty() || sampleIndex < 0)
        return 0.0f;
    const int idx = static_cast<int>(series.size()) - 1 - sampleIndex;
    if (idx < 0 || idx >= static_cast<int>(series.size()))
        return 0.0f;
    return series[static_cast<size_t>(idx)];
}

float HistogramElement::NormalizeValue(float value, float minValue, float maxValue)
{
    const float range = maxValue - minValue;
    if (range <= 0.000001f)
        return 0.0f;
    float n = (value - minValue) / range;
    if (n < 0.0f)
        n = 0.0f;
    if (n > 1.0f)
        n = 1.0f;
    return n;
}

void HistogramElement::DrawSpan(
    ID2D1DeviceContext *context,
    const D2D1_RECT_F &contentRect,
    const D2D1_RECT_F &dstRect,
    GeneralImage &image,
    COLORREF color,
    BYTE alpha)
{
    if (!context)
        return;
    if (dstRect.right <= dstRect.left || dstRect.bottom <= dstRect.top)
        return;

    ID2D1Bitmap *bitmap = nullptr;
    if (!image.GetPath().empty())
    {
        image.EnsureBitmap(context);
        bitmap = image.GetBitmap();
    }
    if (!bitmap)
    {
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        Direct2D::CreateSolidBrush(context, color, alpha / 255.0f, brush.GetAddressOf());
        if (brush)
        {
            context->FillRectangle(dstRect, brush.Get());
        }
        return;
    }

    Microsoft::WRL::ComPtr<ID2D1Image> finalImage;
    if (!image.BuildProcessedImage(context, finalImage) || !finalImage)
    {
        return;
    }

    const float srcLeft = dstRect.left - contentRect.left;
    const float srcTop = dstRect.top - contentRect.top;
    const float srcRight = dstRect.right - contentRect.left;
    const float srcBottom = dstRect.bottom - contentRect.top;
    const D2D1_RECT_F srcRect = D2D1::RectF(srcLeft, srcTop, srcRight, srcBottom);

    const D2D1_BITMAP_INTERPOLATION_MODE interp = m_AntiAlias
                                                      ? D2D1_BITMAP_INTERPOLATION_MODE_LINEAR
                                                      : D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR;
    const float opacity = image.GetImageAlpha() / 255.0f;

    if (finalImage.Get() == static_cast<ID2D1Image *>(bitmap))
    {
        context->DrawBitmap(bitmap, &dstRect, opacity, interp, &srcRect);
    }
    else
    {
        D2D1_MATRIX_3X2_F effectTransform;
        context->GetTransform(&effectTransform);
        context->SetTransform(D2D1::Matrix3x2F::Translation(dstRect.left, dstRect.top) * effectTransform);
        context->DrawImage(
            finalImage.Get(),
            nullptr,
            &srcRect,
            m_AntiAlias ? D2D1_INTERPOLATION_MODE_LINEAR : D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
        context->SetTransform(effectTransform);
    }
}

void HistogramElement::Render(ID2D1DeviceContext *context)
{
    if (!m_Show || !context)
        return;

    context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE : D2D1_ANTIALIAS_MODE_ALIASED);

    const int width = GetWidth();
    const int height = GetHeight();
    if (width <= 0 || height <= 0)
        return;

    D2D1_MATRIX_3X2_F originalTransform;
    ApplyRenderTransform(context, originalTransform);

    RenderBackground(context);

    const float left = static_cast<float>(m_X + m_PaddingLeft);
    const float top = static_cast<float>(m_Y + m_PaddingTop);
    const float right = left + static_cast<float>(width);
    const float bottom = top + static_cast<float>(height);
    const D2D1_RECT_F contentRect = D2D1::RectF(left, top, right, bottom);

    float minValue = 0.0f;
    float maxValue = 100.0f;
    BuildScale(minValue, maxValue);

    const bool hasSecondary = !m_SecondaryData.empty();
    const int samples = m_GraphHorizontalOrientation ? height : width;

    for (int i = 0; i < samples; ++i)
    {
        const float primaryValue = SampleAtFromNewest(m_PrimaryData, i);
        const float primaryN = NormalizeValue(primaryValue, minValue, maxValue);
        const int primarySize = m_GraphHorizontalOrientation
                                    ? static_cast<int>(primaryN * static_cast<float>(width))
                                    : static_cast<int>(primaryN * static_cast<float>(height));

        int secondarySize = 0;
        if (hasSecondary)
        {
            const float secondaryValue = SampleAtFromNewest(m_SecondaryData, i);
            const float secondaryN = NormalizeValue(secondaryValue, minValue, maxValue);
            secondarySize = m_GraphHorizontalOrientation
                                ? static_cast<int>(secondaryN * static_cast<float>(width))
                                : static_cast<int>(secondaryN * static_cast<float>(height));
        }

        const int bothSize = hasSecondary ? (std::min)(primarySize, secondarySize) : 0;

        if (m_GraphHorizontalOrientation)
        {
            const int sampleY = m_Flip ? (height - 1 - i) : i;
            const float y = top + static_cast<float>(sampleY);

            if (hasSecondary && bothSize > 0)
            {
                D2D1_RECT_F bothRect = m_GraphStartLeft
                                           ? D2D1::RectF(left, y, left + static_cast<float>(bothSize), y + 1.0f)
                                           : D2D1::RectF(right - static_cast<float>(bothSize), y, right, y + 1.0f);
                DrawSpan(context, contentRect, bothRect, m_BothImage, m_BothColor, m_BothAlpha);
            }

            if (hasSecondary)
            {
                const bool secondaryLarger = secondarySize > primarySize;
                const int larger = secondaryLarger ? secondarySize : primarySize;
                if (larger > bothSize)
                {
                    D2D1_RECT_F restRect = m_GraphStartLeft
                                               ? D2D1::RectF(left + static_cast<float>(bothSize), y, left + static_cast<float>(larger), y + 1.0f)
                                               : D2D1::RectF(right - static_cast<float>(larger), y, right - static_cast<float>(bothSize), y + 1.0f);
                    if (secondaryLarger)
                    {
                        DrawSpan(context, contentRect, restRect, m_SecondaryImage, m_SecondaryColor, m_SecondaryAlpha);
                    }
                    else
                    {
                        DrawSpan(context, contentRect, restRect, m_PrimaryImage, m_PrimaryColor, m_PrimaryAlpha);
                    }
                }
            }
            else if (primarySize > 0)
            {
                D2D1_RECT_F primaryRect = m_GraphStartLeft
                                              ? D2D1::RectF(left, y, left + static_cast<float>(primarySize), y + 1.0f)
                                              : D2D1::RectF(right - static_cast<float>(primarySize), y, right, y + 1.0f);
                DrawSpan(context, contentRect, primaryRect, m_PrimaryImage, m_PrimaryColor, m_PrimaryAlpha);
            }
        }
        else
        {
            const int sampleX = m_GraphStartLeft ? i : (width - 1 - i);
            const float x = left + static_cast<float>(sampleX);

            if (hasSecondary && bothSize > 0)
            {
                D2D1_RECT_F bothRect = m_Flip
                                           ? D2D1::RectF(x, top, x + 1.0f, top + static_cast<float>(bothSize))
                                           : D2D1::RectF(x, bottom - static_cast<float>(bothSize), x + 1.0f, bottom);
                DrawSpan(context, contentRect, bothRect, m_BothImage, m_BothColor, m_BothAlpha);
            }

            if (hasSecondary)
            {
                const bool secondaryLarger = secondarySize > primarySize;
                const int larger = secondaryLarger ? secondarySize : primarySize;
                if (larger > bothSize)
                {
                    D2D1_RECT_F restRect = m_Flip
                                               ? D2D1::RectF(x, top + static_cast<float>(bothSize), x + 1.0f, top + static_cast<float>(larger))
                                               : D2D1::RectF(x, bottom - static_cast<float>(larger), x + 1.0f, bottom - static_cast<float>(bothSize));
                    if (secondaryLarger)
                    {
                        DrawSpan(context, contentRect, restRect, m_SecondaryImage, m_SecondaryColor, m_SecondaryAlpha);
                    }
                    else
                    {
                        DrawSpan(context, contentRect, restRect, m_PrimaryImage, m_PrimaryColor, m_PrimaryAlpha);
                    }
                }
            }
            else if (primarySize > 0)
            {
                D2D1_RECT_F primaryRect = m_Flip
                                              ? D2D1::RectF(x, top, x + 1.0f, top + static_cast<float>(primarySize))
                                              : D2D1::RectF(x, bottom - static_cast<float>(primarySize), x + 1.0f, bottom);
                DrawSpan(context, contentRect, primaryRect, m_PrimaryImage, m_PrimaryColor, m_PrimaryAlpha);
            }
        }
    }

    RenderBevel(context);
    RestoreRenderTransform(context, originalTransform);
}
