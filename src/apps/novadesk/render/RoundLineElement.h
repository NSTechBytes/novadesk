/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ROUNDLINE_ELEMENT_H__
#define __NOVADESK_ROUNDLINE_ELEMENT_H__

#include "Element.h"

/**
 * @brief Line cap style for round line endpoints.
 */
enum RoundLineCap { 
  ROUNDLINE_CAP_FLAT = 0,  ///< Flat/square line cap.
  ROUNDLINE_CAP_ROUND = 1  ///< Rounded line cap.
};

/**
 * @brief Circular arc gauge element that fills based on a value.
 *
 * @note Renders a circular arc (partial or full circle) that fills
 *       clockwise or counter-clockwise based on the value (0.0-1.0).
 *       Supports dash patterns, tick marks, and gradient fills.
 */
class RoundLineElement : public Element {
public:
  /**
   * @brief Constructs a round line gauge element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param w Width in pixels.
   * @param h Height in pixels.
   * @param value Initial value (0.0-1.0).
   */
  RoundLineElement(const std::wstring &id, int x, int y, int w, int h,
                   float value);
  virtual ~RoundLineElement() {}

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual bool HitTest(int x, int y) override;
  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;

  /// @return Current value (0.0-1.0).
  float GetValue() const { return m_Value; }

  /// Sets the value (0.0-1.0).
  void SetValue(float value) { m_Value = value; }

  /// @return The gauge radius in pixels.
  int GetRadius() const { return m_Radius; }

  /// Sets the gauge radius in pixels.
  void SetRadius(int radius) { m_Radius = radius; }

  /// @return The line thickness in pixels.
  int GetThickness() const { return m_Thickness; }

  /// Sets the line thickness in pixels.
  void SetThickness(int thickness) { m_Thickness = thickness; }

  /// @return The starting angle in degrees.
  float GetStartAngle() const { return m_StartAngle; }

  /// Sets the starting angle in degrees.
  void SetStartAngle(float angle) { m_StartAngle = angle; }

  /// @return The total arc angle in degrees.
  float GetTotalAngle() const { return m_TotalAngle; }

  /// Sets the total arc angle in degrees (default: 360 for full circle).
  void SetTotalAngle(float angle) { m_TotalAngle = angle; }

  /// @return True if the arc sweeps clockwise.
  bool IsClockwise() const { return m_Clockwise; }

  /// Sets the sweep direction.
  void SetClockwise(bool clockwise) { m_Clockwise = clockwise; }

  /// Sets both start and end cap styles.
  RoundLineCap GetCapType() const { return m_StartCap; }
  void SetCapType(RoundLineCap cap) { m_StartCap = m_EndCap = cap; }

  /// Sets the start cap style.
  RoundLineCap GetStartCap() const { return m_StartCap; }
  void SetStartCap(RoundLineCap cap) { m_StartCap = cap; }

  /// Sets the end cap style.
  RoundLineCap GetEndCap() const { return m_EndCap; }
  void SetEndCap(RoundLineCap cap) { m_EndCap = cap; }

  /// Sets the dash pattern array.
  const std::vector<float> &GetDashArray() const { return m_DashArray; }
  void SetDashArray(const std::vector<float> &dashArray) {
    m_DashArray = dashArray;
  }

  /// Sets the end thickness (for tapered arcs).
  int GetEndThickness() const { return m_EndThickness; }
  void SetEndThickness(int thickness) { m_EndThickness = thickness; }

  /// Sets the number of tick marks.
  int GetTicks() const { return m_Ticks; }
  void SetTicks(int ticks) { m_Ticks = ticks; }

  /// Sets the foreground arc color.
  void SetLineColor(COLORREF color, BYTE alpha) {
    m_LineColor = color;
    m_LineAlpha = alpha;
    m_HasLineColor = true;
  }

  /// Sets the background arc color.
  void SetLineColorBg(COLORREF color, BYTE alpha) {
    m_LineColorBg = color;
    m_LineAlphaBg = alpha;
    m_HasLineColorBg = true;
  }

  /// Sets the background arc gradient.
  void SetLineGradientBg(const GradientInfo &gradient) {
    m_LineGradientBg = gradient;
  }

  /// Sets the foreground arc gradient.
  void SetLineGradient(const GradientInfo &gradient) {
    m_LineGradient = gradient;
  }

  const GradientInfo &GetLineGradient() const { return m_LineGradient; }
  const GradientInfo &GetLineGradientBg() const { return m_LineGradientBg; }

  bool HasLineColor() const { return m_HasLineColor; }
  COLORREF GetLineColor() const { return m_LineColor; }
  BYTE GetLineAlpha() const { return m_LineAlpha; }

  bool HasLineColorBg() const { return m_HasLineColorBg; }
  COLORREF GetLineColorBg() const { return m_LineColorBg; }
  BYTE GetLineAlphaBg() const { return m_LineAlphaBg; }

private:
  float m_Value;              ///< Current value (0.0-1.0).
  int m_Radius = 0;          ///< Gauge radius in pixels.
  int m_Thickness = 2;       ///< Line thickness in pixels.
  int m_EndThickness = -1;   ///< End thickness (-1 = same as start).
  float m_StartAngle = 0.0f; ///< Starting angle in degrees.
  float m_TotalAngle = 360.0f; ///< Total arc angle in degrees.
  bool m_Clockwise = true;   ///< Sweep direction.

  RoundLineCap m_StartCap = ROUNDLINE_CAP_FLAT; ///< Start cap style.
  RoundLineCap m_EndCap = ROUNDLINE_CAP_FLAT;   ///< End cap style.
  std::vector<float> m_DashArray; ///< Dash pattern.
  int m_Ticks = 0;            ///< Number of tick marks.

  bool m_HasLineColor = false;
  COLORREF m_LineColor = RGB(0, 255, 0); ///< Foreground color.
  BYTE m_LineAlpha = 255;

  bool m_HasLineColorBg = false;
  COLORREF m_LineColorBg = RGB(50, 50, 50); ///< Background color.
  BYTE m_LineAlphaBg = 255;

  GradientInfo m_LineGradient;    ///< Foreground gradient.
  GradientInfo m_LineGradientBg;  ///< Background gradient.
};

#endif
