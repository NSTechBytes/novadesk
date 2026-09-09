/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <algorithm>

#include "BoxBorderPaint.h"
#include "ShapeElement.h"

/**
 * @brief A layout container element with CSS-style box model support.
 *
 * @note Supports box shadows, borders, flexbox layout, and list markers.
 *       Inherits from ShapeElement for background/border rendering.
 */
class ElementLayoutBox : public ShapeElement {
public:
  /**
   * @brief Display type for the layout box.
   */
  enum class DisplayType { Flex, None, ListItem };

  /**
   * @brief CSS-style box shadow configuration.
   */
  struct BoxShadow {
    float x = 0.0f;                ///< Horizontal offset.
    float y = 0.0f;                ///< Vertical offset.
    float blur = 0.0f;             ///< Blur radius.
    float spread = 0.0f;           ///< Spread radius.
    COLORREF color = RGB(0, 0, 0); ///< Shadow color.
    BYTE alpha = 255;              ///< Shadow opacity.
    bool inset = false;            ///< True for inset shadow.
  };

  /**
   * @brief List marker style types (CSS list-style-type).
   */
  enum class ListStyleType {
    Disc,       ///< • (filled circle)
    Circle,     ///< ○ (hollow circle)
    Square,     ///< ■ (filled square)
    UpperRoman, ///< I, II, III, IV, V, etc.
    LowerRoman, ///< i, ii, iii, iv, v, etc.
    Decimal,    ///< 1, 2, 3, 4, 5, etc.
    LowerAlpha, ///< a, b, c, d, e, etc.
    UpperAlpha, ///< A, B, C, D, E, etc.
    None        ///< No marker.
  };

  /**
   * @brief Configuration for list item markers.
   */
  struct ListMarker {
    ListStyleType type = ListStyleType::Disc; ///< Marker style.
    COLORREF color = RGB(0, 0, 0);            ///< Marker color.
    BYTE alpha = 255;                         ///< Marker opacity.
    float size = 6.0f;                        ///< Marker size.
    float offsetX = -20.0f; ///< Distance from content (negative = left).
  };

  using BorderStyle = BoxBorder::Style;

  /**
   * @brief Constructs a layout box element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels.
   * @param height Height in pixels.
   */
  ElementLayoutBox(const std::wstring &id, int x, int y, int width, int height);
  ~ElementLayoutBox() override = default;

  void Render(ID2D1DeviceContext *context) override;
  bool HitTestLocal(const D2D1_POINT_2F &point) override;
  bool CreateGeometry(
      ID2D1Factory *factory,
      Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const override;
  GfxRect GetBackgroundBounds() override;

  int GetAutoWidth() override;
  int GetAutoHeight() override;

  /// Sets the corner radii for the layout box.
  void SetRadii(float rx, float ry) override {
    m_RadiusX = rx;
    m_RadiusY = ry;
  }
  float GetRadiusX() const override { return m_RadiusX; }
  float GetRadiusY() const override { return m_RadiusY; }

  /// Sets the box shadow effects.
  void SetBoxShadows(const std::vector<BoxShadow> &shadows) {
    m_BoxShadows = shadows;
  }
  const std::vector<BoxShadow> &GetBoxShadows() const { return m_BoxShadows; }

  /// Sets per-side border styles.
  void SetBorderStyle(BorderStyle top, BorderStyle right, BorderStyle bottom,
                      BorderStyle left) {
    m_BorderStyleTop = top;
    m_BorderStyleRight = right;
    m_BorderStyleBottom = bottom;
    m_BorderStyleLeft = left;
  }
  BorderStyle GetBorderStyleTop() const { return m_BorderStyleTop; }
  BorderStyle GetBorderStyleRight() const { return m_BorderStyleRight; }
  BorderStyle GetBorderStyleBottom() const { return m_BorderStyleBottom; }
  BorderStyle GetBorderStyleLeft() const { return m_BorderStyleLeft; }

  /// Sets the display type (Flex, None, ListItem).
  void SetDisplayType(DisplayType display) { m_DisplayType = display; }
  DisplayType GetDisplayType() const { return m_DisplayType; }

  // ============================================================================
  // List Item Marker
  // ============================================================================

  /// Sets the list marker configuration.
  void SetListMarker(const ListMarker &marker) { m_ListMarker = marker; }
  const ListMarker &GetListMarker() const { return m_ListMarker; }
  void SetListStyleType(ListStyleType type) { m_ListMarker.type = type; }
  ListStyleType GetListStyleType() const { return m_ListMarker.type; }

  // ============================================================================
  // Layout Configuration
  // ============================================================================

  /// Sets the flex direction for child layout.
  void SetLayoutDirection(const std::wstring &flexDir) {
    m_FlexDirection = flexDir;
  }

  /// Sets the gap between child elements.
  void SetLayoutGap(int gap) { m_LayoutGap = gap; }
  const std::wstring &GetLayoutDirection() const { return m_FlexDirection; }
  int GetLayoutGap() const { return m_LayoutGap; }

private:
  void RenderSingleShadow(ID2D1DeviceContext *context,
                          const D2D1_ROUNDED_RECT &baseRect,
                          const BoxShadow &shadow);
  void RenderListMarker(ID2D1DeviceContext *context);
  void RenderTextMarker(ID2D1DeviceContext *context, const std::wstring &text,
                        float markerCenterX, float markerCenterY,
                        float markerSize, ID2D1SolidColorBrush *brush);
  BoxBorderPaintParams BuildBorderPaintParams() const;

  float m_RadiusX = 0.0f;              ///< Corner radius X.
  float m_RadiusY = 0.0f;              ///< Corner radius Y.
  std::vector<BoxShadow> m_BoxShadows; ///< Box shadow effects.
  BorderStyle m_BorderStyleTop = BorderStyle::Solid;
  BorderStyle m_BorderStyleRight = BorderStyle::Solid;
  BorderStyle m_BorderStyleBottom = BorderStyle::Solid;
  BorderStyle m_BorderStyleLeft = BorderStyle::Solid;
  DisplayType m_DisplayType = DisplayType::Flex; ///< Display mode.
  ListMarker m_ListMarker;               ///< List item marker configuration.
  std::wstring m_FlexDirection = L"row"; ///< Flex direction.
  int m_LayoutGap = 0;                   ///< Gap between children.
};
