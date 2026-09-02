/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SCROLLBAR_RENDERER_H__
#define __NOVADESK_SCROLLBAR_RENDERER_H__

#include <d2d1.h>
#include <d2d1_1.h>
#include <wrl/client.h>
#include "../render/Element.h"

// Forward declaration
class Widget;

/**
 * @brief Renders scrollbar tracks, thumbs, and arrow buttons for scrollable containers.
 *
 * @note Static utility class; renders CSS-style scrollbars with customizable
 *       appearance (width, colors, radius, hover/active states).
 */
class ScrollbarRenderer {
public:
  /**
   * @brief Renders the scrollbars for a container element.
   *
   * @param widget The widget instance owning this container.
   * @param container The scrollable container element.
   * @param context Direct2D render target.
   * @param bounds Container bounds rectangle.
   */
  static void DrawScrollbars(const Widget &widget, Element *container,
                             ID2D1RenderTarget *context, const GfxRect &bounds);

  /**
   * @brief Renders a crisp vector arrow triangle for scrollbar buttons.
   *
   * @param context Direct2D render target.
   * @param p1 First vertex of the triangle.
   * @param p2 Second vertex of the triangle.
   * @param p3 Third vertex of the triangle.
   * @param brush Fill brush for the triangle.
   */
  static void DrawArrowTriangle(ID2D1RenderTarget *context, D2D1_POINT_2F p1,
                                D2D1_POINT_2F p2, D2D1_POINT_2F p3,
                                ID2D1Brush *brush);

private:
  ScrollbarRenderer() = delete; // Static utility class; no instances.
};

#endif // __NOVADESK_SCROLLBAR_RENDERER_H__
