/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>

#include "Widget.h"

/**
 * @brief Manages element and window animations for widgets.
 *
 * @note Static utility class for CSS-style keyframe and tween animations.
 *       Animations are stepped via a timer and apply interpolated values
 *       to element/window properties each frame.
 */
class WidgetAnimationHelper {
public:
  static constexpr int kTimerId = 6; ///< Timer ID for animation stepping.

  /**
   * @brief Starts a tween animation on an element.
   *
   * @param widget The widget owning the element.
   * @param id Element ID to animate.
   * @param to Target animation values.
   * @param from Starting animation values.
   * @param durationMs Animation duration in milliseconds.
   * @param easing Easing function name.
   * @param iterationCount Number of iterations (-1 for infinite).
   */
  static void StartElementAnimation(Widget &widget, const std::wstring &id,
                                    const Widget::AnimationTarget &to,
                                    const Widget::AnimationTarget &from,
                                    int durationMs, const std::wstring &easing,
                                    int iterationCount);

  /**
   * @brief Starts a keyframe animation on an element.
   *
   * @param widget The widget owning the element.
   * @param id Element ID to animate.
   * @param keyframes Vector of keyframe definitions.
   * @param durationMs Total animation duration in milliseconds.
   * @param easing Default easing function name.
   * @param iterationCount Number of iterations (-1 for infinite).
   */
  static void StartElementKeyframeAnimation(
      Widget &widget, const std::wstring &id,
      const std::vector<Widget::AnimationKeyframe> &keyframes, int durationMs,
      const std::wstring &easing, int iterationCount);

  /**
   * @brief Starts a tween animation on the window itself.
   *
   * @param widget The widget to animate.
   * @param to Target window animation values.
   * @param from Starting window animation values.
   * @param durationMs Animation duration in milliseconds.
   * @param easing Easing function name.
   * @param iterationCount Number of iterations (-1 for infinite).
   */
  static void StartWindowAnimation(Widget &widget,
                                   const Widget::WindowAnimationTarget &to,
                                   const Widget::WindowAnimationTarget &from,
                                   int durationMs, const std::wstring &easing,
                                   int iterationCount);

  /**
   * @brief Starts a keyframe animation on the window.
   *
   * @param widget The widget to animate.
   * @param keyframes Vector of window keyframe definitions.
   * @param durationMs Total animation duration in milliseconds.
   * @param easing Default easing function name.
   * @param iterationCount Number of iterations (-1 for infinite).
   */
  static void StartWindowKeyframeAnimation(
      Widget &widget,
      const std::vector<Widget::WindowAnimationKeyframe> &keyframes,
      int durationMs, const std::wstring &easing, int iterationCount);

  /// Steps all active animations for a widget (called each timer tick).
  static void StepAnimations(Widget &widget);

  /// Removes all animations for a specific element.
  static void RemoveAnimationsForElement(Widget &widget,
                                         const std::wstring &id);

  /// Clears all element animations.
  static void ClearAllAnimations(Widget &widget);

  /// Stops all window animations.
  static void StopWindowAnimations(Widget &widget);

private:
  WidgetAnimationHelper() = delete; // Static utility class; no instances.

  /// Captures the current window state as an animation target.
  static Widget::WindowAnimationTarget
  CaptureWindowAnimationState(const Widget &widget);

  /// Resolves keyframe stops into interpolated targets.
  static void ResolveWindowKeyframeStops(
      const Widget &widget,
      const std::vector<Widget::WindowAnimationKeyframe> &keyframes,
      std::vector<float> &offsets, std::vector<std::wstring> &easings,
      std::vector<Widget::WindowAnimationTarget> &resolved);

  /// Applies an animation target to the window.
  static void
  ApplyWindowAnimationTarget(Widget &widget,
                             const Widget::WindowAnimationTarget &target);
};
