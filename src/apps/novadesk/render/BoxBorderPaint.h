/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "BoxBorderTypes.h"
#include <d2d1_1.h>

/**
 * @brief Parameters for rendering a CSS-style box border.
 */
struct BoxBorderPaintParams {
  BoxBorder::Position position =
      BoxBorder::Position::Inside; ///< Border position relative to element.
  BoxBorder::Style styleTop = BoxBorder::Style::Solid; ///< Top border style.
  BoxBorder::Style styleRight =
      BoxBorder::Style::Solid; ///< Right border style.
  BoxBorder::Style styleBottom =
      BoxBorder::Style::Solid; ///< Bottom border style.
  BoxBorder::Style styleLeft = BoxBorder::Style::Solid; ///< Left border style.
  float elementRadiusX = 0.0f;         ///< Element corner radius X.
  float elementRadiusY = 0.0f;         ///< Element corner radius Y.
  float strokeWidth = 0.0f;            ///< Border stroke width in pixels.
  COLORREF strokeColor = RGB(0, 0, 0); ///< Border color.
  BYTE strokeAlpha = 255;              ///< Border opacity.
  D2D1_CAP_STYLE strokeStartCap = D2D1_CAP_STYLE_FLAT;  ///< Start cap style.
  D2D1_CAP_STYLE strokeEndCap = D2D1_CAP_STYLE_FLAT;    ///< End cap style.
  D2D1_CAP_STYLE strokeDashCap = D2D1_CAP_STYLE_FLAT;   ///< Dash cap style.
  D2D1_LINE_JOIN strokeLineJoin = D2D1_LINE_JOIN_MITER; ///< Line join style.
  float strokeDashOffset = 0.0f; ///< Dash pattern offset.
};

/**
 * @brief Utility class for rendering CSS-style box borders using Direct2D.
 *
 * @note Supports inside, center, and outside border positions with
 *       per-side styles (solid, dashed, dotted, etc.).
 */
class BoxBorderPaint {
public:
  /**
   * @brief Builds the border geometry rectangle adjusted for border position.
   *
   * @param elementRect The element's rounded rectangle.
   * @param params Border rendering parameters.
   *
   * @return Adjusted rounded rectangle for border rendering.
   */
  static D2D1_ROUNDED_RECT
  BuildBorderGeometryRect(const D2D1_ROUNDED_RECT &elementRect,
                          const BoxBorderPaintParams &params);

  /**
   * @brief Renders a border using the specified geometry and brush.
   *
   * @param context The Direct2D device context.
   * @param borderRect The border geometry.
   * @param params Border rendering parameters.
   * @param strokeBrush The brush for border stroke.
   */
  static void Paint(ID2D1DeviceContext *context,
                    const D2D1_ROUNDED_RECT &borderRect,
                    const BoxBorderPaintParams &params,
                    ID2D1Brush *strokeBrush);

  /**
   * @brief Renders a border for an element with automatic geometry adjustment.
   *
   * @param context The Direct2D device context.
   * @param elementRect The element's rounded rectangle.
   * @param params Border rendering parameters.
   * @param strokeBrush The brush for border stroke.
   */
  static void PaintForElement(ID2D1DeviceContext *context,
                              const D2D1_ROUNDED_RECT &elementRect,
                              const BoxBorderPaintParams &params,
                              ID2D1Brush *strokeBrush);
};
