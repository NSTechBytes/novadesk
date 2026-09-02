/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ELLIPSE_SHAPE_H__
#define __NOVADESK_ELLIPSE_SHAPE_H__

#include "ShapeElement.h"

/**
 * @brief Renders an ellipse shape defined by horizontal and vertical radii.
 *
 * @note The ellipse is inscribed within the element's bounding box.
 *       Radii define the elliptical shape independent of the bounding box.
 */
class EllipseShape : public ShapeElement {
public:
  /**
   * @brief Constructs an ellipse shape.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Bounding box width.
   * @param height Bounding box height.
   */
  EllipseShape(const std::wstring &id, int x, int y, int width, int height);
  virtual ~EllipseShape();

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;

  /// Sets the horizontal and vertical radii.
  virtual void SetRadii(float rx, float ry) override {
    m_RadiusX = rx;
    m_RadiusY = ry;
  }

  virtual float GetRadiusX() const override { return m_RadiusX; }
  virtual float GetRadiusY() const override { return m_RadiusY; }
  virtual bool HitTestLocal(const D2D1_POINT_2F &point) override;
  virtual bool CreateGeometry(
      ID2D1Factory *factory,
      Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;

private:
  float m_RadiusX = 0.0f; ///< Horizontal radius.
  float m_RadiusY = 0.0f; ///< Vertical radius.
};

#endif
