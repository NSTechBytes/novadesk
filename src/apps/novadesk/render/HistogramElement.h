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

/**
 * @brief Renders a histogram chart with primary and secondary data series.
 *
 * @note Supports horizontal/vertical orientation, auto-ranging, and
 *       gradient fills. Data is sampled from the newest value.
 */
class HistogramElement : public Element {
public:
  /**
   * @brief Constructs a histogram element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param w Width in pixels.
   * @param h Height in pixels.
   */
  HistogramElement(const std::wstring &id, int x, int y, int w, int h);
  virtual ~HistogramElement() {}

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual bool HitTest(int x, int y) override;
  virtual int GetAutoWidth() override { return 0; }
  virtual int GetAutoHeight() override { return 0; }

  /// Sets the primary data series.
  void SetData(const std::vector<float> &data) { m_PrimaryData = data; }

  /// Sets the secondary data series.
  void SetData2(const std::vector<float> &data) { m_SecondaryData = data; }

  const std::vector<float> &GetData() const { return m_PrimaryData; }
  const std::vector<float> &GetData2() const { return m_SecondaryData; }

  /// Enables auto-ranging based on data min/max.
  void SetAutoRange(bool enable) { m_AutoRange = enable; }
  bool GetAutoRange() const { return m_AutoRange; }

  /// Sets the graph direction (left-to-right or right-to-left).
  void SetGraphStartLeft(bool left) { m_GraphStartLeft = left; }
  bool GetGraphStartLeft() const { return m_GraphStartLeft; }

  /// Sets horizontal or vertical orientation.
  void SetGraphHorizontalOrientation(bool horizontal) {
    m_GraphHorizontalOrientation = horizontal;
  }
  bool GetGraphHorizontalOrientation() const {
    return m_GraphHorizontalOrientation;
  }

  /// Flips the graph axis.
  void SetFlip(bool flip) { m_Flip = flip; }
  bool GetFlip() const { return m_Flip; }

  /// Sets the primary data series color.
  void SetPrimaryColor(COLORREF color, BYTE alpha) {
    m_PrimaryColor = color;
    m_PrimaryAlpha = alpha;
  }

  /// Sets the secondary data series color.
  void SetSecondaryColor(COLORREF color, BYTE alpha) {
    m_SecondaryColor = color;
    m_SecondaryAlpha = alpha;
  }

  /// Sets the color for overlapping regions.
  void SetBothColor(COLORREF color, BYTE alpha) {
    m_BothColor = color;
    m_BothAlpha = alpha;
  }

  void SetPrimaryGradient(const GradientInfo &gradient) {
    m_PrimaryGradient = gradient;
  }
  void SetSecondaryGradient(const GradientInfo &gradient) {
    m_SecondaryGradient = gradient;
  }
  void SetBothGradient(const GradientInfo &gradient) {
    m_BothGradient = gradient;
  }
  COLORREF GetPrimaryColor() const { return m_PrimaryColor; }
  BYTE GetPrimaryAlpha() const { return m_PrimaryAlpha; }
  COLORREF GetSecondaryColor() const { return m_SecondaryColor; }
  BYTE GetSecondaryAlpha() const { return m_SecondaryAlpha; }
  COLORREF GetBothColor() const { return m_BothColor; }
  BYTE GetBothAlpha() const { return m_BothAlpha; }
  const GradientInfo &GetPrimaryGradient() const { return m_PrimaryGradient; }
  const GradientInfo &GetSecondaryGradient() const {
    return m_SecondaryGradient;
  }
  const GradientInfo &GetBothGradient() const { return m_BothGradient; }

private:
  bool BuildAutoRange(float &outMin, float &outMax) const;
  float SampleAtFromNewest(const std::vector<float> &series,
                           int sampleIndex) const;
  static float NormalizeValue(float value, float minValue, float maxValue);
  void DrawSpan(ID2D1DeviceContext *context, const D2D1_RECT_F &dstRect,
                const D2D1_RECT_F &gradientRect, const GradientInfo *gradient,
                COLORREF color, BYTE alpha);

private:
  std::vector<float> m_PrimaryData;   ///< Primary data series.
  std::vector<float> m_SecondaryData; ///< Secondary data series.

  bool m_AutoRange = false;              ///< Auto-range from data min/max.
  bool m_GraphStartLeft = false;         ///< Direction: false = right-to-left.
  bool m_GraphHorizontalOrientation = false; ///< Orientation: false = vertical.
  bool m_Flip = false;                   ///< Flip the axis.

  COLORREF m_PrimaryColor = RGB(0, 128, 0);   ///< Primary series color.
  BYTE m_PrimaryAlpha = 255;
  GradientInfo m_PrimaryGradient;
  COLORREF m_SecondaryColor = RGB(255, 0, 0); ///< Secondary series color.
  BYTE m_SecondaryAlpha = 255;
  GradientInfo m_SecondaryGradient;
  COLORREF m_BothColor = RGB(255, 255, 0);    ///< Overlap color.
  BYTE m_BothAlpha = 255;
  GradientInfo m_BothGradient;
};

#endif
