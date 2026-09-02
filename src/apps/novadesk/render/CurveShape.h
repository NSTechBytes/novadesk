/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_CURVE_SHAPE_H__
#define __NOVADESK_CURVE_SHAPE_H__

#include "ShapeElement.h"

/**
 * @brief Renders a quadratic or cubic Bezier curve shape.
 *
 * @note Supports both quadratic (single control point) and cubic
 *       (two control points) Bezier curves.
 */
class CurveShape : public ShapeElement {
public:
  /**
   * @brief Constructs a curve shape.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Bounding box width.
   * @param height Bounding box height.
   */
  CurveShape(const std::wstring &id, int x, int y, int width, int height);
  virtual ~CurveShape();

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual bool HitTestLocal(const D2D1_POINT_2F &point) override;
  virtual GfxRect GetBounds() override;
  virtual bool CreateGeometry(
      ID2D1Factory *factory,
      Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;

  /// Configures the curve control points and type.
  virtual void SetCurveParams(float startX, float startY, float controlX,
                              float controlY, float control2X, float control2Y,
                              float endX, float endY,
                              const std::wstring &curveType) override {
    m_StartX = startX;
    m_StartY = startY;
    m_ControlX = controlX;
    m_ControlY = controlY;
    m_Control2X = control2X;
    m_Control2Y = control2Y;
    m_EndX = endX;
    m_EndY = endY;
    m_IsCubic = (_wcsicmp(curveType.c_str(), L"cubic") == 0);
  }

  virtual float GetStartX() const override { return m_StartX; }
  virtual float GetStartY() const override { return m_StartY; }
  virtual float GetEndX() const override { return m_EndX; }
  virtual float GetEndY() const override { return m_EndY; }
  virtual float GetControlX() const override { return m_ControlX; }
  virtual float GetControlY() const override { return m_ControlY; }
  virtual float GetControl2X() const override { return m_Control2X; }
  virtual float GetControl2Y() const override { return m_Control2Y; }

  /// @return "cubic" or "quadratic" depending on curve type.
  virtual std::wstring GetCurveType() const override {
    return m_IsCubic ? L"cubic" : L"quadratic";
  }

private:
  float m_StartX = 0.0f;    ///< Start point X.
  float m_StartY = 0.0f;    ///< Start point Y.
  float m_ControlX = 0.0f;  ///< First control point X.
  float m_ControlY = 0.0f;  ///< First control point Y.
  float m_Control2X = 0.0f; ///< Second control point X (cubic only).
  float m_Control2Y = 0.0f; ///< Second control point Y (cubic only).
  float m_EndX = 0.0f;      ///< End point X.
  float m_EndY = 0.0f;      ///< End point Y.
  bool m_IsCubic = false;   ///< True for cubic, false for quadratic.

  bool CreateCurveGeometry(ID2D1Factory *factory,
                           ID2D1PathGeometry **outGeometry) const;
};

#endif
