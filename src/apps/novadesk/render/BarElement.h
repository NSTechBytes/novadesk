/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_BAR_ELEMENT_H__
#define __NOVADESK_BAR_ELEMENT_H__

#include "Element.h"

/**
 * @brief Orientation of the bar gauge.
 */
enum BarOrientation {
  BAR_HORIZONTAL = 0, ///< Bar fills from left to right.
  BAR_VERTICAL = 1    ///< Bar fills from bottom to top.
};

/**
 * @brief Horizontal or vertical bar gauge element.
 *
 * @note Value ranges from 0.0 (empty) to 1.0 (full). Supports gradient
 *       fills and custom bar colors.
 */
class BarElement : public Element {
public:
  /**
   * @brief Constructs a bar element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param w Width in pixels.
   * @param h Height in pixels.
   * @param value Initial value (0.0-1.0).
   * @param orientation Horizontal or vertical orientation.
   */
  BarElement(const std::wstring &id, int x, int y, int w, int h, float value,
             BarOrientation orientation);
  virtual ~BarElement() {}

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual bool HitTest(int x, int y) override;
  virtual int GetAutoWidth() override { return 0; }
  virtual int GetAutoHeight() override { return 0; }

  /// @return Current bar value (0.0-1.0).
  float GetValue() const { return m_Value; }

  /// @return The bar orientation.
  BarOrientation GetOrientation() const { return m_Orientation; }

  /// Sets the bar value (0.0-1.0).
  void SetValue(float value) { m_Value = value; }

  /// Sets the bar orientation.
  void SetOrientation(BarOrientation orientation) {
    m_Orientation = orientation;
  }

  /// Sets the corner radius of the bar fill.
  void SetBarCornerRadius(int radius) { m_BarCornerRadius = radius; }
  int GetBarCornerRadius() const { return m_BarCornerRadius; }

  /// @return The bar gradient configuration.
  const GradientInfo &GetBarGradient() const { return m_BarGradient; }

  /// Sets the bar fill color and opacity.
  void SetBarColor(COLORREF color, BYTE alpha) {
    m_BarColor = color;
    m_BarAlpha = alpha;
    m_HasBarColor = true;
  }

  /// Sets a gradient fill for the bar.
  void SetBarGradient(const GradientInfo &gradient) {
    m_BarGradient = gradient;
  }

  /// @return True if a custom bar color is set.
  bool HasBarColor() const { return m_HasBarColor; }
  COLORREF GetBarColor() const { return m_BarColor; }
  BYTE GetBarAlpha() const { return m_BarAlpha; }

private:
  float m_Value;                ///< Current value (0.0-1.0).
  BarOrientation m_Orientation; ///< Bar orientation.

  int m_BarCornerRadius = 0; ///< Corner radius of the bar fill.

  bool m_HasBarColor = false;
  COLORREF m_BarColor = RGB(0, 255, 0); ///< Default bar color: green.
  BYTE m_BarAlpha = 255;

  GradientInfo m_BarGradient; ///< Gradient fill for the bar.
};

#endif
