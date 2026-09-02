/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_LINE_ELEMENT_H__
#define __NOVADESK_LINE_ELEMENT_H__

#include "Element.h"

#include <vector>

/**
 * @brief Multi-series line graph element for real-time data visualization.
 *
 * @note Supports multiple data series with individual colors/gradients,
 *       horizontal guide lines, auto-ranging, and configurable line width.
 */
class LineElement : public Element {
public:
  /**
   * @brief Constructs a line graph element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param w Width in pixels.
   * @param h Height in pixels.
   */
  LineElement(const std::wstring &id, int x, int y, int w, int h);
  virtual ~LineElement() {}

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual bool HitTest(int x, int y) override;
  virtual int GetAutoWidth() override { return 0; }
  virtual int GetAutoHeight() override { return 0; }

  /// Sets the number of data series.
  void SetLineCount(int count);
  int GetLineCount() const { return m_LineCount; }

  /// Sets the data sets for all series.
  void SetDataSets(const std::vector<std::vector<float>> &dataSets);
  const std::vector<std::vector<float>> &GetDataSets() const {
    return m_DataSets;
  }

  /// Sets per-series colors and alpha values.
  void SetLineColors(const std::vector<COLORREF> &colors,
                     const std::vector<BYTE> &alphas);
  const std::vector<COLORREF> &GetLineColors() const { return m_LineColors; }
  const std::vector<BYTE> &GetLineAlphas() const { return m_LineAlphas; }

  /// Sets per-series gradient fills.
  void SetLineGradients(const std::vector<GradientInfo> &gradients);
  const std::vector<GradientInfo> &GetLineGradients() const {
    return m_LineGradients;
  }

  /// Sets per-series scale reference values.
  void SetScaleValues(const std::vector<float> &scaleValues);
  const std::vector<float> &GetScaleValues() const { return m_ScaleValues; }

  /// Sets the line stroke width in pixels.
  void SetLineWidth(float width);
  float GetLineWidth() const { return m_LineWidth; }

  /// Sets the maximum number of data points to display.
  void SetMaxPoints(int maxPoints);
  int GetMaxPoints() const { return m_MaxPoints; }

  /// Enables or disables horizontal guide lines.
  void SetHorizontalLines(bool enabled) { m_HorizontalLines = enabled; }
  bool GetHorizontalLines() const { return m_HorizontalLines; }

  /// Sets the horizontal guide line color.
  void SetHorizontalLineColor(COLORREF color, BYTE alpha) {
    m_HorizontalLineColor = color;
    m_HorizontalLineAlpha = alpha;
  }
  COLORREF GetHorizontalLineColor() const { return m_HorizontalLineColor; }
  BYTE GetHorizontalLineAlpha() const { return m_HorizontalLineAlpha; }

  /// Sets the horizontal guide line gradient.
  void SetHorizontalLineGradient(const GradientInfo &gradient) {
    m_HorizontalLineGradient = gradient;
  }
  const GradientInfo &GetHorizontalLineGradient() const {
    return m_HorizontalLineGradient;
  }

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

  /// Sets the stroke transform type for rendering quality.
  void SetStrokeTransformType(D2D1_STROKE_TRANSFORM_TYPE type) {
    m_StrokeType = type;
  }
  D2D1_STROKE_TRANSFORM_TYPE GetStrokeTransformType() const {
    return m_StrokeType;
  }

  /// Enables auto-ranging based on data min/max.
  void SetAutoRange(bool enabled) { m_AutoRange = enabled; }
  bool GetAutoRange() const { return m_AutoRange; }

  /// Sets the manual scale range.
  void SetScaleRange(float minValue, float maxValue);
  float GetScaleMin() const { return m_ScaleMin; }
  float GetScaleMax() const { return m_ScaleMax; }

private:
  void EnsureStorage();
  bool BuildAutoRange(float &outMin, float &outMax) const;
  bool MapPoint(int dataIndex, int pointIndex, int totalPoints,
                int capacityPoints, float minValue, float maxValue,
                D2D1_POINT_2F &outPoint);
  static float DistancePointToSegment(const D2D1_POINT_2F &p,
                                      const D2D1_POINT_2F &a,
                                      const D2D1_POINT_2F &b);

private:
  int m_LineCount = 1; ///< Number of data series.
  std::vector<std::vector<float>> m_DataSets; ///< Per-series data points.
  std::vector<COLORREF> m_LineColors;  ///< Per-series colors.
  std::vector<BYTE> m_LineAlphas;      ///< Per-series alpha values.
  std::vector<GradientInfo> m_LineGradients; ///< Per-series gradients.
  std::vector<float> m_ScaleValues;    ///< Per-series scale reference.
  float m_LineWidth = 1.0f;            ///< Line stroke width.
  int m_MaxPoints = 0;                 ///< Maximum visible points.
  bool m_HorizontalLines = false;      ///< Show horizontal guide lines.
  COLORREF m_HorizontalLineColor = RGB(0, 0, 0);
  BYTE m_HorizontalLineAlpha = 255;
  GradientInfo m_HorizontalLineGradient;
  bool m_GraphStartLeft = false;       ///< Direction: false = right-to-left.
  bool m_GraphHorizontalOrientation = false; ///< Orientation: false = vertical.
  bool m_Flip = false;                 ///< Flip the axis.
  D2D1_STROKE_TRANSFORM_TYPE m_StrokeType = D2D1_STROKE_TRANSFORM_TYPE_NORMAL;
  bool m_AutoRange = false;            ///< Auto-range from data min/max.
  float m_ScaleMin = 0.0f;            ///< Manual scale minimum.
  float m_ScaleMax = 100.0f;          ///< Manual scale maximum.
};

#endif
