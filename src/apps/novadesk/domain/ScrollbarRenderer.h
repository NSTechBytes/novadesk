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

class ScrollbarRenderer {
public:
  /**
   * Render the scrollbars (tracks, thumbs, and arrow buttons) for a container
   * element.
   * @param widget The widget instance owning this container
   * @param container The scrollable container element
   * @param context Direct2D render target
   * @param bounds Container bounds
   */
  static void DrawScrollbars(const Widget &widget, Element *container,
                             ID2D1RenderTarget *context, const GfxRect &bounds);

  /**
   * Helper to render a crisp vector arrow triangle.
   */
  static void DrawArrowTriangle(ID2D1RenderTarget *context, D2D1_POINT_2F p1,
                                D2D1_POINT_2F p2, D2D1_POINT_2F p3,
                                ID2D1Brush *brush);

private:
  ScrollbarRenderer() = delete;
};

#endif // __NOVADESK_SCROLLBAR_RENDERER_H__
