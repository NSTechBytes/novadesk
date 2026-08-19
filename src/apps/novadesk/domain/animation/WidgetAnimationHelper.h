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

class WidgetAnimationHelper
{
public:
    static constexpr int kTimerId = 6;

    static void StartElementAnimation(
        Widget &widget,
        const std::wstring &id,
        const Widget::AnimationTarget &to,
        const Widget::AnimationTarget &from,
        int durationMs,
        const std::wstring &easing,
        int iterationCount);

    static void StartElementKeyframeAnimation(
        Widget &widget,
        const std::wstring &id,
        const std::vector<Widget::AnimationKeyframe> &keyframes,
        int durationMs,
        const std::wstring &easing,
        int iterationCount);

    static void StartWindowAnimation(
        Widget &widget,
        const Widget::WindowAnimationTarget &to,
        const Widget::WindowAnimationTarget &from,
        int durationMs,
        const std::wstring &easing,
        int iterationCount);

    static void StartWindowKeyframeAnimation(
        Widget &widget,
        const std::vector<Widget::WindowAnimationKeyframe> &keyframes,
        int durationMs,
        const std::wstring &easing,
        int iterationCount);

    static void StepAnimations(Widget &widget);
    static void RemoveAnimationsForElement(Widget &widget, const std::wstring &id);
    static void ClearAllAnimations(Widget &widget);
    static void StopWindowAnimations(Widget &widget);

private:
    WidgetAnimationHelper() = delete;

    static Widget::WindowAnimationTarget CaptureWindowAnimationState(const Widget &widget);
    static void ResolveWindowKeyframeStops(
        const Widget &widget,
        const std::vector<Widget::WindowAnimationKeyframe> &keyframes,
        std::vector<float> &offsets,
        std::vector<std::wstring> &easings,
        std::vector<Widget::WindowAnimationTarget> &resolved);
    static void ApplyWindowAnimationTarget(Widget &widget, const Widget::WindowAnimationTarget &target);
};
