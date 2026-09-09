/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>

/**
 * @brief Evaluates CSS-style easing functions for animations.
 *
 * @note Supports standard easing names: "linear", "ease", "ease-in",
 *       "ease-out", "ease-in-out", and cubic-bezier custom curves.
 */
namespace AnimationEasing {

/**
 * @brief Evaluates an easing function at a given time.
 *
 * @param t Normalized time (0.0 to 1.0).
 * @param easing Easing function name or cubic-bezier parameters.
 *
 * @return The eased value (typically 0.0 to 1.0, may overshoot for
 * elastic/bounce).
 */
float Evaluate(float t, const std::wstring &easing);

} // namespace AnimationEasing
