/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

namespace BoxBorder {

/**
 * @brief Border position relative to the element's edge.
 */
enum class Position {
  Outside, ///< Border is drawn outside the element bounds.
  Center,  ///< Border is centered on the element edge.
  Inside   ///< Border is drawn inside the element bounds.
};

/**
 * @brief CSS-style border styles.
 */
enum class Style {
  None,   ///< No border rendered.
  Hidden, ///< Border hidden but still takes space.
  Solid,  ///< Solid line border.
  Inset,  ///< Inset 3D border (like CSS inset).
  Outset, ///< Outset 3D border (like CSS outset).
  Groove, ///< Groove 3D border (like CSS groove).
  Ridge,  ///< Ridge 3D border (like CSS ridge).
  Dotted, ///< Dotted line border.
  Dashed, ///< Dashed line border.
  Double  ///< Double line border.
};

} // namespace BoxBorder
