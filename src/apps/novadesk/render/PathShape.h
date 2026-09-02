/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_PATH_SHAPE_H__
#define __NOVADESK_PATH_SHAPE_H__

#include "ShapeElement.h"
#include <string>

/**
 * @brief Renders a shape from SVG-style path data string.
 *
 * @note Supports path data parsing, geometry combination with other shapes,
 *       and automatic bounds calculation from path control points.
 */
class PathShape : public ShapeElement {
public:
  /**
   * @brief Defines a geometry combine operation with another shape.
   */
  struct CombineOp {
    std::wstring id; ///< ID of the shape to combine with.
    D2D1_COMBINE_MODE mode = D2D1_COMBINE_MODE_UNION; ///< Combine mode.
    bool consume = false; ///< If true, the combined shape is removed after use.
  };

  /**
   * @brief Constructs a path shape from path data.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Bounding box width.
   * @param height Bounding box height.
   */
  PathShape(const std::wstring &id, int x, int y, int width, int height);
  virtual ~PathShape();

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;
  virtual GfxRect GetBounds() override;
  virtual bool HitTestLocal(const D2D1_POINT_2F &point) override;
  virtual bool CreateGeometry(
      ID2D1Factory *factory,
      Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;

  /// Sets the SVG-style path data string.
  virtual void SetPathData(const std::wstring &pathData) override;
  virtual std::wstring GetPathData() const override { return m_PathData; }

  /// Sets a pre-computed combined geometry.
  void SetCombinedGeometry(Microsoft::WRL::ComPtr<ID2D1Geometry> geometry,
                           const D2D1_RECT_F &bounds);

  /// Clears any pre-computed combined geometry.
  void ClearCombinedGeometry();

  /// @return True if a combined geometry is set.
  bool HasCombinedGeometry() const { return m_HasCombinedGeometry; }

  /// Sets the combine data for combining with other shapes.
  void SetCombineData(const std::wstring &baseId,
                      const std::vector<CombineOp> &ops, bool consumeBase);

  /// Gets the combine data configuration.
  void GetCombineData(std::wstring &baseId, std::vector<CombineOp> &ops,
                      bool &consumeBase) const;

  /// @return True if this shape is configured as a combine target.
  bool IsCombineShape() const { return m_IsCombineShape; }

private:
  std::wstring m_PathData; ///< SVG-style path data string.

  bool m_HasPathBounds = false; ///< True if path bounds have been computed.
  float m_PathMinX = 0.0f;     ///< Path bounds minimum X.
  float m_PathMinY = 0.0f;     ///< Path bounds minimum Y.
  float m_PathMaxX = 0.0f;     ///< Path bounds maximum X.
  float m_PathMaxY = 0.0f;     ///< Path bounds maximum Y.

  bool m_IsCombineShape = false;      ///< True if this is a combine target.
  std::wstring m_CombineBaseId;       ///< Base shape ID for combine.
  std::vector<CombineOp> m_CombineOps; ///< Combine operations.
  bool m_CombineConsumeBase = false;   ///< Consume base shape after combine.

  bool m_HasCombinedGeometry = false; ///< True if combined geometry is set.
  Microsoft::WRL::ComPtr<ID2D1Geometry> m_CombinedGeometry; ///< Pre-computed geometry.
  GfxRect m_CombinedBounds; ///< Bounds of the combined geometry.

  void CreatePathGeometry(ID2D1Factory *factory,
                          ID2D1PathGeometry **ppGeometry) const;
  void UpdatePathBounds();
};

#endif
