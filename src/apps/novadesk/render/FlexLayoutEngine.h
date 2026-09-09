/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_FLEX_LAYOUT_ENGINE_H__
#define __NOVADESK_FLEX_LAYOUT_ENGINE_H__

#include <string>
#include "Element.h"

/**
 * @brief Configuration for CSS Flexbox layout.
 */
struct FlexLayoutConfig {
  std::wstring direction = L"ltr"; ///< Text direction: "ltr" or "rtl".
  std::wstring flexDirection =
      L"row";  ///< Main axis direction: "row", "rowreverse", "column",
               ///< "columnreverse".
  int gap = 0; ///< Gap between items in pixels.
  std::wstring align =
      L"start"; ///< Cross-axis alignment: "normal", "stretch", "center",
                ///< "start", "end", "flexstart", "flexend".
  std::wstring justify =
      L"start";          ///< Main-axis alignment: "start", "center", "end".
  int paddingLeft = 0;   ///< Container left padding.
  int paddingTop = 0;    ///< Container top padding.
  int paddingRight = 0;  ///< Container right padding.
  int paddingBottom = 0; ///< Container bottom padding.
};

/**
 * @brief Implements CSS Flexbox layout algorithm for positioning child
 * elements.
 *
 * @note Static utility class; handles positioning and sizing of child elements
 *       within a flex container according to CSS Flexbox specification rules.
 */
class FlexLayoutEngine {
public:
  /**
   * @brief Applies flexbox layout to a container and its children.
   *
   * @param container The container element to layout.
   * @param config The flexbox configuration (direction, alignment, gaps, etc.).
   */
  static void ApplyLayout(Element *container, const FlexLayoutConfig &config);

private:
  FlexLayoutEngine() = delete; // Static utility class; no instances.
};

#endif // __NOVADESK_FLEX_LAYOUT_ENGINE_H__
