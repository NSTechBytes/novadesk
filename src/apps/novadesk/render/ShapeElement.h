/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SHAPE_ELEMENT_H__
#define __NOVADESK_SHAPE_ELEMENT_H__

#include "Element.h"
#include <wrl/client.h>

/**
 * @brief Base class for all geometric shape elements (rect, ellipse, line,
 * etc.).
 *
 * @note Provides common stroke/fill rendering, hit testing, and geometry
 *       creation. Subclasses implement specific shape geometry via
 * CreateGeometry().
 */
class ShapeElement : public Element {
public:
  /**
   * @brief Constructs a shape element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Bounding box width.
   * @param height Bounding box height.
   * @param type Element type (default: ELEMENT_SHAPE).
   */
  ShapeElement(const std::wstring &id, int x, int y, int width, int height,
               ElementType type = ELEMENT_SHAPE);
  virtual ~ShapeElement();

  virtual void Render(ID2D1DeviceContext *context) = 0;
  virtual bool HitTest(int x, int y) override;

  /**
   * @brief Creates the shape's geometry for rendering and hit testing.
   *
   * @param factory Direct2D factory for geometry creation.
   * @param geometry Receives the created geometry.
   *
   * @return True if geometry was created successfully.
   */
  virtual bool
  CreateGeometry(ID2D1Factory *factory,
                 Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const = 0;

  // ============================================================================
  // Stroke Configuration
  // ============================================================================

  /// Sets the stroke width, color, and alpha.
  void SetStroke(float width, COLORREF color, BYTE alpha) {
    m_StrokeWidth = width;
    m_StrokeColor = color;
    m_StrokeAlpha = alpha;
    m_HasStroke = true;
    m_HasStrokeGradient = false;
  }

  /// Sets a gradient stroke.
  void SetStrokeGradient(const GradientInfo &gradient) {
    m_StrokeGradient = gradient;
    m_HasStrokeGradient = true;
    m_HasStroke = true;
  }

  // ============================================================================
  // Fill Configuration
  // ============================================================================

  /// Sets the fill color and alpha.
  void SetFill(COLORREF color, BYTE alpha) {
    m_FillColor = color;
    m_FillAlpha = alpha;
    m_HasFill = true;
    m_HasFillGradient = false;
  }

  /// Sets a gradient fill.
  void SetFillGradient(const GradientInfo &gradient) {
    m_FillGradient = gradient;
    m_HasFillGradient = true;
    m_HasFill = true;
  }

  // ============================================================================
  // Getters
  // ============================================================================

  bool HasStroke() const { return m_HasStroke; }
  float GetStrokeWidth() const { return m_StrokeWidth; }
  COLORREF GetStrokeColor() const { return m_StrokeColor; }
  BYTE GetStrokeAlpha() const { return m_StrokeAlpha; }
  bool HasStrokeGradient() const { return m_HasStrokeGradient; }
  const GradientInfo &GetStrokeGradient() const { return m_StrokeGradient; }

  bool HasFill() const { return m_HasFill; }
  COLORREF GetFillColor() const { return m_FillColor; }
  BYTE GetFillAlpha() const { return m_FillAlpha; }
  bool HasFillGradient() const { return m_HasFillGradient; }
  const GradientInfo &GetFillGradient() const { return m_FillGradient; }

  D2D1_CAP_STYLE GetStrokeStartCap() const { return m_StrokeStartCap; }
  D2D1_CAP_STYLE GetStrokeEndCap() const { return m_StrokeEndCap; }
  D2D1_CAP_STYLE GetStrokeDashCap() const { return m_StrokeDashCap; }
  D2D1_LINE_JOIN GetStrokeLineJoin() const { return m_StrokeLineJoin; }
  float GetStrokeDashOffset() const { return m_StrokeDashOffset; }
  const std::vector<float> &GetStrokeDashes() const { return m_StrokeDashes; }

  // ============================================================================
  // Virtual Getters for Specialized Shapes
  // ============================================================================

  virtual float GetRadiusX() const { return 0; }
  virtual float GetRadiusY() const { return 0; }
  virtual float GetStartX() const { return 0; }
  virtual float GetStartY() const { return 0; }
  virtual float GetEndX() const { return 0; }
  virtual float GetEndY() const { return 0; }
  virtual float GetControlX() const { return 0; }
  virtual float GetControlY() const { return 0; }
  virtual float GetControl2X() const { return 0; }
  virtual float GetControl2Y() const { return 0; }
  virtual float GetStartAngle() const { return 0; }
  virtual float GetEndAngle() const { return 0; }
  virtual bool IsClockwise() const { return true; }
  virtual std::wstring GetPathData() const { return L""; }
  virtual std::wstring GetCurveType() const { return L"quadratic"; }
  GfxRect GetBackgroundBounds() override;

  /// Sets the stroke rendering style.
  void SetStrokeStyle(D2D1_CAP_STYLE start, D2D1_CAP_STYLE end,
                      D2D1_CAP_STYLE dash, D2D1_LINE_JOIN join, float offset,
                      const std::vector<float> &dashes) {
    m_StrokeStartCap = start;
    m_StrokeEndCap = end;
    m_StrokeDashCap = dash;
    m_StrokeLineJoin = join;
    m_StrokeDashOffset = offset;
    m_StrokeDashes = dashes;
    m_UpdateStrokeStyle = true;
  }

  // Virtual setters for specialized shapes (no-op in base class)
  virtual void SetRadii(float rx, float ry) {}
  virtual void SetLinePoints(float x1, float y1, float x2, float y2) {}
  virtual void SetArcParams(float startAngle, float endAngle, bool clockwise) {}
  virtual void SetPathData(const std::wstring &pathData) {}
  virtual void SetCurveParams(float startX, float startY, float controlX,
                              float controlY, float control2X, float control2Y,
                              float endX, float endY,
                              const std::wstring &curveType) {}

  // ============================================================================
  // Geometry Combine Support
  // ============================================================================

  /// Increments the combine consumer count.
  void AddCombineConsumer() { ++m_CombineConsumerCount; }

  /// Decrements the combine consumer count.
  void RemoveCombineConsumer() {
    if (m_CombineConsumerCount > 0)
      --m_CombineConsumerCount;
  }

  /// @return True if this shape has been consumed by a combine operation.
  bool IsConsumed() const { return m_CombineConsumerCount > 0; }

  /// @return The combined transformation matrix for rendering.
  D2D1_MATRIX_3X2_F GetRenderTransformMatrix() const;

protected:
  // ============================================================================
  // Stroke State
  // ============================================================================

  bool m_HasStroke = false;
  float m_StrokeWidth = 1.0f;
  COLORREF m_StrokeColor = RGB(0, 0, 0);
  BYTE m_StrokeAlpha = 255;

  bool m_HasStrokeGradient = false;
  GradientInfo m_StrokeGradient;

  // ============================================================================
  // Fill State
  // ============================================================================

  bool m_HasFill = false;
  COLORREF m_FillColor = RGB(255, 255, 255);
  BYTE m_FillAlpha = 255;

  bool m_HasFillGradient = false;
  GradientInfo m_FillGradient;

  // ============================================================================
  // Stroke Style
  // ============================================================================

  D2D1_CAP_STYLE m_StrokeStartCap = D2D1_CAP_STYLE_FLAT;
  D2D1_CAP_STYLE m_StrokeEndCap = D2D1_CAP_STYLE_FLAT;
  D2D1_CAP_STYLE m_StrokeDashCap = D2D1_CAP_STYLE_FLAT;
  D2D1_LINE_JOIN m_StrokeLineJoin = D2D1_LINE_JOIN_MITER;
  float m_StrokeDashOffset = 0.0f;
  std::vector<float> m_StrokeDashes;

  bool m_UpdateStrokeStyle = false;
  Microsoft::WRL::ComPtr<ID2D1StrokeStyle1> m_StrokeStyle;
  int m_CombineConsumerCount =
      0; ///< Number of combine operations consuming this shape.

  void CreateBrush(ID2D1DeviceContext *context, ID2D1Brush **ppBrush,
                   bool isStroke);
  void UpdateStrokeStyle(ID2D1DeviceContext *context);
  void EnsureStrokeStyle();

  bool TryCreateStrokeBrush(ID2D1DeviceContext *context,
                            Microsoft::WRL::ComPtr<ID2D1Brush> &outBrush);
  bool TryCreateFillBrush(ID2D1DeviceContext *context,
                          Microsoft::WRL::ComPtr<ID2D1Brush> &outBrush);

  /// Hit test within the shape's local coordinate space.
  virtual bool HitTestLocal(const D2D1_POINT_2F &point) = 0;
};

#endif
