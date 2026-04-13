/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_HISTOGRAM_ELEMENT_H__
#define __NOVADESK_HISTOGRAM_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"

#include <vector>

class HistogramElement : public Element
{
public:
    HistogramElement(const std::wstring &id, int x, int y, int w, int h);
    virtual ~HistogramElement() {}

    virtual void Render(ID2D1DeviceContext *context) override;
    virtual int GetAutoWidth() override;
    virtual int GetAutoHeight() override;

    void SetData(const std::vector<float> &data) { m_PrimaryData = data; }
    void SetData2(const std::vector<float> &data) { m_SecondaryData = data; }
    const std::vector<float> &GetData() const { return m_PrimaryData; }
    const std::vector<float> &GetData2() const { return m_SecondaryData; }

    void SetAutoScale(bool enable) { m_AutoScale = enable; }
    bool GetAutoScale() const { return m_AutoScale; }

    void SetGraphStartLeft(bool left) { m_GraphStartLeft = left; }
    bool GetGraphStartLeft() const { return m_GraphStartLeft; }

    void SetGraphHorizontalOrientation(bool horizontal) { m_GraphHorizontalOrientation = horizontal; }
    bool GetGraphHorizontalOrientation() const { return m_GraphHorizontalOrientation; }

    void SetFlip(bool flip) { m_Flip = flip; }
    bool GetFlip() const { return m_Flip; }

    void SetPrimaryColor(COLORREF color, BYTE alpha) { m_PrimaryColor = color; m_PrimaryAlpha = alpha; }
    void SetSecondaryColor(COLORREF color, BYTE alpha) { m_SecondaryColor = color; m_SecondaryAlpha = alpha; }
    void SetBothColor(COLORREF color, BYTE alpha) { m_BothColor = color; m_BothAlpha = alpha; }
    COLORREF GetPrimaryColor() const { return m_PrimaryColor; }
    BYTE GetPrimaryAlpha() const { return m_PrimaryAlpha; }
    COLORREF GetSecondaryColor() const { return m_SecondaryColor; }
    BYTE GetSecondaryAlpha() const { return m_SecondaryAlpha; }
    COLORREF GetBothColor() const { return m_BothColor; }
    BYTE GetBothAlpha() const { return m_BothAlpha; }

    void SetPrimaryImageName(const std::wstring &path) { m_PrimaryImage.SetPath(path); }
    void SetSecondaryImageName(const std::wstring &path) { m_SecondaryImage.SetPath(path); }
    void SetBothImageName(const std::wstring &path) { m_BothImage.SetPath(path); }

    const std::wstring &GetPrimaryImageName() const { return m_PrimaryImage.GetPath(); }
    const std::wstring &GetSecondaryImageName() const { return m_SecondaryImage.GetPath(); }
    const std::wstring &GetBothImageName() const { return m_BothImage.GetPath(); }

    GeneralImage &GetPrimaryImage() { return m_PrimaryImage; }
    GeneralImage &GetSecondaryImage() { return m_SecondaryImage; }
    GeneralImage &GetBothImage() { return m_BothImage; }
    const GeneralImage &GetPrimaryImage() const { return m_PrimaryImage; }
    const GeneralImage &GetSecondaryImage() const { return m_SecondaryImage; }
    const GeneralImage &GetBothImage() const { return m_BothImage; }

private:
    bool BuildScale(float &outMin, float &outMax) const;
    float SampleAtFromNewest(const std::vector<float> &series, int sampleIndex) const;
    static float NormalizeValue(float value, float minValue, float maxValue);
    void DrawSpan(
        ID2D1DeviceContext *context,
        const D2D1_RECT_F &contentRect,
        const D2D1_RECT_F &dstRect,
        GeneralImage &image,
        COLORREF color,
        BYTE alpha);

private:
    std::vector<float> m_PrimaryData;
    std::vector<float> m_SecondaryData;

    bool m_AutoScale = false;
    bool m_GraphStartLeft = false;             // false = right
    bool m_GraphHorizontalOrientation = false; // false = vertical
    bool m_Flip = false;

    COLORREF m_PrimaryColor = RGB(0, 128, 0);
    BYTE m_PrimaryAlpha = 255;
    COLORREF m_SecondaryColor = RGB(255, 0, 0);
    BYTE m_SecondaryAlpha = 255;
    COLORREF m_BothColor = RGB(255, 255, 0);
    BYTE m_BothAlpha = 255;

    GeneralImage m_PrimaryImage;
    GeneralImage m_SecondaryImage;
    GeneralImage m_BothImage;
};

#endif
