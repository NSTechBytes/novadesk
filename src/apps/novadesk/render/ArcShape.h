/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ARC_SHAPE_H__
#define __NOVADESK_ARC_SHAPE_H__

#include "ShapeElement.h"

/**
 * @brief Renders an arc shape defined by radii and start/end angles.
 *
 * @note Supports clockwise and counter-clockwise arc directions with
 *       configurable elliptical radii.
 */
class ArcShape : public ShapeElement {
public:
  /**
   * @brief Constructs an arc shape.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Bounding box width.
   * @param height Bounding box height.
   */
  ArcShape(const std::wstring &id, int x, int y, int width, int height);
  virtual ~ArcShape();

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;
  virtual bool HitTestLocal(const D2D1_POINT_2F &point) override;
  virtual bool CreateGeometry(
      ID2D1Factory *factory,
      Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;

  /// Sets the elliptical radii for the arc.
  virtual void SetRadii(float rx, float ry) override {
    m_RadiusX = rx;
    m_RadiusY = ry;
  }

  /// Configures the arc angle parameters.
  virtual void SetArcParams(float startAngle, float endAngle,
                            bool clockwise) override {
    m_StartAngle = startAngle;
    m_EndAngle = endAngle;
    m_Clockwise = clockwise;
  }

  virtual float GetRadiusX() const override { return m_RadiusX; }
  virtual float GetRadiusY() const override { return m_RadiusY; }
  virtual float GetStartAngle() const override { return m_StartAngle; }
  virtual float GetEndAngle() const override { return m_EndAngle; }
  virtual bool IsClockwise() const override { return m_Clockwise; }

private:
  float m_RadiusX = 0.0f;   ///< Horizontal radius of the ellipse.
  float m_RadiusY = 0.0f;   ///< Vertical radius of the ellipse.
  float m_StartAngle = 0.0f; ///< Arc start angle in degrees.
  float m_EndAngle = 90.0f;  ///< Arc end angle in degrees.
  bool m_Clockwise = true;   ///< Arc sweep direction.

  D2D1_POINT_2F CheckPoint(float angle, float rx, float ry, float cx,
                           float cy) const;
  bool CreateArcGeometry(ID2D1Factory *factory, ID2D1PathGeometry **outGeometry,
                         float &outRx, float &outRy, float &outCx,
                         float &outCy) const;
};

#endif
