/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WidgetAnimationHelper.h"

#include <algorithm>
#include <cmath>

#include "AnimationEasing.h"
#include "ColorUtil.h"
#include "Settings.h"
#include "TextElement.h"
#include "Widget.h"

namespace {
void ApplyTextAnimationTarget(TextElement *text,
                              const Widget::AnimationTarget &target) {
  if (!text)
    return;

  if (target.hasFontSize)
    text->SetFontSize(static_cast<int>(std::lround(target.fontSize)));
  if (target.hasFontWeight)
    text->SetFontWeight(static_cast<int>(std::lround(target.fontWeight)));
  if (target.hasLetterSpacing)
    text->SetLetterSpacing(target.letterSpacing);
  if (target.hasFontColor) {
    COLORREF color = 0;
    BYTE alpha = 255;
    ColorUtil::FromFloatRGBA(target.fontColorR, target.fontColorG,
                             target.fontColorB, target.fontAlpha, color, alpha);
    text->SetFontColor(color, alpha);
  }
}

Widget::AnimationTarget
SampleTransformAnimationTarget(const Widget::AnimationTarget &from,
                               const Widget::AnimationTarget &to, float p) {
  Widget::AnimationTarget out{};
  auto lerp = [p](float a, float b) { return a + (b - a) * p; };

  if (to.hasX) {
    out.hasX = true;
    out.x = lerp(from.x, to.x);
  }
  if (to.hasY) {
    out.hasY = true;
    out.y = lerp(from.y, to.y);
  }
  if (to.hasWidth) {
    out.hasWidth = true;
    out.width = lerp(from.width, to.width);
  }
  if (to.hasHeight) {
    out.hasHeight = true;
    out.height = lerp(from.height, to.height);
  }
  if (to.hasRotate) {
    out.hasRotate = true;
    out.rotate = lerp(from.rotate, to.rotate);
  }
  return out;
}

Widget::AnimationTarget
SampleAnimationTarget(const Widget::AnimationTarget &from,
                      const Widget::AnimationTarget &to, float p) {
  Widget::AnimationTarget out = SampleTransformAnimationTarget(from, to, p);
  auto lerp = [p](float a, float b) { return a + (b - a) * p; };

  if (to.hasFontSize) {
    out.hasFontSize = true;
    out.fontSize = lerp(from.fontSize, to.fontSize);
  }
  if (to.hasFontWeight) {
    out.hasFontWeight = true;
    out.fontWeight = lerp(from.fontWeight, to.fontWeight);
  }
  if (to.hasLetterSpacing) {
    out.hasLetterSpacing = true;
    out.letterSpacing = lerp(from.letterSpacing, to.letterSpacing);
  }
  if (to.hasFontColor) {
    out.hasFontColor = true;
    out.fontColorR = lerp(from.fontColorR, to.fontColorR);
    out.fontColorG = lerp(from.fontColorG, to.fontColorG);
    out.fontColorB = lerp(from.fontColorB, to.fontColorB);
    out.fontAlpha = lerp(from.fontAlpha, to.fontAlpha);
  }
  return out;
}

void MergeAnimationTarget(Widget::AnimationTarget &base,
                          const Widget::AnimationTarget &patch) {
  if (patch.hasX) {
    base.hasX = true;
    base.x = patch.x;
  }
  if (patch.hasY) {
    base.hasY = true;
    base.y = patch.y;
  }
  if (patch.hasWidth) {
    base.hasWidth = true;
    base.width = patch.width;
  }
  if (patch.hasHeight) {
    base.hasHeight = true;
    base.height = patch.height;
  }
  if (patch.hasRotate) {
    base.hasRotate = true;
    base.rotate = patch.rotate;
  }
  if (patch.hasFontSize) {
    base.hasFontSize = true;
    base.fontSize = patch.fontSize;
  }
  if (patch.hasFontWeight) {
    base.hasFontWeight = true;
    base.fontWeight = patch.fontWeight;
  }
  if (patch.hasLetterSpacing) {
    base.hasLetterSpacing = true;
    base.letterSpacing = patch.letterSpacing;
  }
  if (patch.hasFontColor) {
    base.hasFontColor = true;
    base.fontColorR = patch.fontColorR;
    base.fontColorG = patch.fontColorG;
    base.fontColorB = patch.fontColorB;
    base.fontAlpha = patch.fontAlpha;
  }
}

Widget::AnimationTarget CaptureElementAnimationState(Element *element) {
  Widget::AnimationTarget state{};
  if (!element)
    return state;

  state.hasX = true;
  state.x = static_cast<float>(element->GetX());
  state.hasY = true;
  state.y = static_cast<float>(element->GetY());
  state.hasWidth = true;
  state.width = static_cast<float>(element->GetWidth());
  state.hasHeight = true;
  state.height = static_cast<float>(element->GetHeight());
  state.hasRotate = true;
  state.rotate = element->GetRotate();

  if (element->GetType() == ELEMENT_TEXT) {
    TextElement *text = static_cast<TextElement *>(element);
    state.hasFontSize = true;
    state.fontSize = static_cast<float>(text->GetFontSize());
    state.hasFontWeight = true;
    state.fontWeight = static_cast<float>(text->GetFontWeight());
    state.hasLetterSpacing = true;
    state.letterSpacing = text->GetLetterSpacing();
    state.hasFontColor = true;
    const COLORREF color = text->GetFontColor();
    state.fontColorR = static_cast<float>(GetRValue(color));
    state.fontColorG = static_cast<float>(GetGValue(color));
    state.fontColorB = static_cast<float>(GetBValue(color));
    state.fontAlpha = static_cast<float>(text->GetFontAlpha());
  }
  return state;
}

void ResolveKeyframeStops(
    Element *element, const std::vector<Widget::AnimationKeyframe> &keyframes,
    std::vector<float> &offsets, std::vector<std::wstring> &easings,
    std::vector<Widget::AnimationTarget> &resolved) {
  offsets.clear();
  easings.clear();
  resolved.clear();
  if (keyframes.empty())
    return;

  Widget::AnimationTarget carry = CaptureElementAnimationState(element);
  Widget::AnimationTarget unionMask{};

  for (const Widget::AnimationKeyframe &kf : keyframes)
    MergeAnimationTarget(unionMask, kf.values);

  for (const Widget::AnimationKeyframe &kf : keyframes) {
    MergeAnimationTarget(carry, kf.values);
    Widget::AnimationTarget stop{};
    if (unionMask.hasX) {
      stop.hasX = true;
      stop.x = carry.x;
    }
    if (unionMask.hasY) {
      stop.hasY = true;
      stop.y = carry.y;
    }
    if (unionMask.hasWidth) {
      stop.hasWidth = true;
      stop.width = carry.width;
    }
    if (unionMask.hasHeight) {
      stop.hasHeight = true;
      stop.height = carry.height;
    }
    if (unionMask.hasRotate) {
      stop.hasRotate = true;
      stop.rotate = carry.rotate;
    }
    if (unionMask.hasFontSize) {
      stop.hasFontSize = true;
      stop.fontSize = carry.fontSize;
    }
    if (unionMask.hasFontWeight) {
      stop.hasFontWeight = true;
      stop.fontWeight = carry.fontWeight;
    }
    if (unionMask.hasLetterSpacing) {
      stop.hasLetterSpacing = true;
      stop.letterSpacing = carry.letterSpacing;
    }
    if (unionMask.hasFontColor) {
      stop.hasFontColor = true;
      stop.fontColorR = carry.fontColorR;
      stop.fontColorG = carry.fontColorG;
      stop.fontColorB = carry.fontColorB;
      stop.fontAlpha = carry.fontAlpha;
    }

    offsets.push_back(kf.offset);
    easings.push_back(kf.easing);
    resolved.push_back(stop);
  }
}

void ApplyAnimationTargetToElement(Element *element,
                                   const Widget::AnimationTarget &target) {
  if (!element)
    return;

  int x = element->GetX();
  int y = element->GetY();
  int w = element->GetWidth();
  int h = element->GetHeight();
  if (target.hasX)
    x = static_cast<int>(std::lround(target.x));
  if (target.hasY)
    y = static_cast<int>(std::lround(target.y));
  if (target.hasWidth)
    w = static_cast<int>(std::lround(target.width));
  if (target.hasHeight)
    h = static_cast<int>(std::lround(target.height));
  element->SetPosition(x, y);
  element->SetSize(w, h);
  if (target.hasRotate)
    element->SetRotate(target.rotate);
  if (element->GetType() == ELEMENT_TEXT)
    ApplyTextAnimationTarget(static_cast<TextElement *>(element), target);
}

Widget::AnimationTarget
SampleKeyframeTrack(const std::vector<float> &offsets,
                    const std::vector<std::wstring> &easings,
                    const std::vector<Widget::AnimationTarget> &stops, float t,
                    const std::wstring &defaultEasing) {
  if (offsets.empty() || stops.empty())
    return Widget::AnimationTarget{};

  if (t <= offsets.front())
    return stops.front();
  if (t >= offsets.back())
    return stops.back();

  for (size_t i = 0; i + 1 < offsets.size(); ++i) {
    if (t < offsets[i + 1]) {
      const float span = offsets[i + 1] - offsets[i];
      float u = span > 0.0f ? (t - offsets[i]) / span : 0.0f;
      if (u < 0.0f)
        u = 0.0f;
      if (u > 1.0f)
        u = 1.0f;
      std::wstring segmentEasing = defaultEasing;
      if (i + 1 < easings.size() && !easings[i + 1].empty())
        segmentEasing = easings[i + 1];
      const float p = AnimationEasing::Evaluate(u, segmentEasing);
      return SampleAnimationTarget(stops[i], stops[i + 1], p);
    }
  }
  return stops.back();
}

// ── Window Animation Helpers ─────────────────────────────────────────────

Widget::WindowAnimationTarget
SampleWindowAnimationTarget(const Widget::WindowAnimationTarget &from,
                            const Widget::WindowAnimationTarget &to, float p) {
  Widget::WindowAnimationTarget out{};
  auto lerp = [p](float a, float b) { return a + (b - a) * p; };

  if (to.hasX) {
    out.hasX = true;
    out.x = lerp(from.x, to.x);
  }
  if (to.hasY) {
    out.hasY = true;
    out.y = lerp(from.y, to.y);
  }
  if (to.hasWidth) {
    out.hasWidth = true;
    out.width = lerp(from.width, to.width);
  }
  if (to.hasHeight) {
    out.hasHeight = true;
    out.height = lerp(from.height, to.height);
  }
  if (to.hasOpacity) {
    out.hasOpacity = true;
    out.opacity = lerp(from.opacity, to.opacity);
  }
  if (to.hasBackgroundColor) {
    out.hasBackgroundColor = true;
    out.bgColorR = lerp(from.bgColorR, to.bgColorR);
    out.bgColorG = lerp(from.bgColorG, to.bgColorG);
    out.bgColorB = lerp(from.bgColorB, to.bgColorB);
    out.bgAlpha = lerp(from.bgAlpha, to.bgAlpha);
  }
  return out;
}

void MergeWindowAnimationTarget(Widget::WindowAnimationTarget &base,
                                const Widget::WindowAnimationTarget &patch) {
  if (patch.hasX) {
    base.hasX = true;
    base.x = patch.x;
  }
  if (patch.hasY) {
    base.hasY = true;
    base.y = patch.y;
  }
  if (patch.hasWidth) {
    base.hasWidth = true;
    base.width = patch.width;
  }
  if (patch.hasHeight) {
    base.hasHeight = true;
    base.height = patch.height;
  }
  if (patch.hasOpacity) {
    base.hasOpacity = true;
    base.opacity = patch.opacity;
  }
  if (patch.hasBackgroundColor) {
    base.hasBackgroundColor = true;
    base.bgColorR = patch.bgColorR;
    base.bgColorG = patch.bgColorG;
    base.bgColorB = patch.bgColorB;
    base.bgAlpha = patch.bgAlpha;
  }
}

Widget::WindowAnimationTarget SampleWindowKeyframeTrack(
    const std::vector<float> &offsets, const std::vector<std::wstring> &easings,
    const std::vector<Widget::WindowAnimationTarget> &stops, float t,
    const std::wstring &defaultEasing) {
  if (offsets.empty() || stops.empty())
    return Widget::WindowAnimationTarget{};

  if (t <= offsets.front())
    return stops.front();
  if (t >= offsets.back())
    return stops.back();

  for (size_t i = 0; i + 1 < offsets.size(); ++i) {
    if (t < offsets[i + 1]) {
      const float span = offsets[i + 1] - offsets[i];
      float u = span > 0.0f ? (t - offsets[i]) / span : 0.0f;
      if (u < 0.0f)
        u = 0.0f;
      if (u > 1.0f)
        u = 1.0f;
      std::wstring segmentEasing = defaultEasing;
      if (i + 1 < easings.size() && !easings[i + 1].empty())
        segmentEasing = easings[i + 1];
      const float p = AnimationEasing::Evaluate(u, segmentEasing);
      return SampleWindowAnimationTarget(stops[i], stops[i + 1], p);
    }
  }
  return stops.back();
}
} // namespace

Widget::WindowAnimationTarget
WidgetAnimationHelper::CaptureWindowAnimationState(const Widget &widget) {
  Widget::WindowAnimationTarget state{};
  state.hasX = true;
  state.x = static_cast<float>(widget.m_Options.x);
  state.hasY = true;
  state.y = static_cast<float>(widget.m_Options.y);
  state.hasWidth = true;
  state.width = static_cast<float>(widget.m_Options.width);
  state.hasHeight = true;
  state.height = static_cast<float>(widget.m_Options.height);
  state.hasOpacity = true;
  state.opacity = widget.m_Options.windowOpacity / 255.0f;
  state.hasBackgroundColor = true;
  state.bgColorR = static_cast<float>(GetRValue(widget.m_Options.color));
  state.bgColorG = static_cast<float>(GetGValue(widget.m_Options.color));
  state.bgColorB = static_cast<float>(GetBValue(widget.m_Options.color));
  state.bgAlpha = static_cast<float>(widget.m_Options.bgAlpha);
  return state;
}

void WidgetAnimationHelper::ResolveWindowKeyframeStops(
    const Widget &widget,
    const std::vector<Widget::WindowAnimationKeyframe> &keyframes,
    std::vector<float> &offsets, std::vector<std::wstring> &easings,
    std::vector<Widget::WindowAnimationTarget> &resolved) {
  offsets.clear();
  easings.clear();
  resolved.clear();
  if (keyframes.empty())
    return;

  Widget::WindowAnimationTarget carry = CaptureWindowAnimationState(widget);
  Widget::WindowAnimationTarget unionMask{};

  for (const Widget::WindowAnimationKeyframe &kf : keyframes)
    MergeWindowAnimationTarget(unionMask, kf.values);

  for (const Widget::WindowAnimationKeyframe &kf : keyframes) {
    MergeWindowAnimationTarget(carry, kf.values);
    Widget::WindowAnimationTarget stop{};
    if (unionMask.hasX) {
      stop.hasX = true;
      stop.x = carry.x;
    }
    if (unionMask.hasY) {
      stop.hasY = true;
      stop.y = carry.y;
    }
    if (unionMask.hasWidth) {
      stop.hasWidth = true;
      stop.width = carry.width;
    }
    if (unionMask.hasHeight) {
      stop.hasHeight = true;
      stop.height = carry.height;
    }
    if (unionMask.hasOpacity) {
      stop.hasOpacity = true;
      stop.opacity = carry.opacity;
    }
    if (unionMask.hasBackgroundColor) {
      stop.hasBackgroundColor = true;
      stop.bgColorR = carry.bgColorR;
      stop.bgColorG = carry.bgColorG;
      stop.bgColorB = carry.bgColorB;
      stop.bgAlpha = carry.bgAlpha;
    }

    offsets.push_back(kf.offset);
    easings.push_back(kf.easing);
    resolved.push_back(stop);
  }
}

void WidgetAnimationHelper::ApplyWindowAnimationTarget(
    Widget &widget, const Widget::WindowAnimationTarget &target) {
  bool posChanged = false;
  int x = widget.m_Options.x;
  int y = widget.m_Options.y;
  int w = widget.m_Options.width;
  int h = widget.m_Options.height;

  if (target.hasX) {
    x = static_cast<int>(std::lround(target.x));
    posChanged = true;
  }
  if (target.hasY) {
    y = static_cast<int>(std::lround(target.y));
    posChanged = true;
  }
  if (target.hasWidth) {
    w = static_cast<int>(std::lround(target.width));
    posChanged = true;
  }
  if (target.hasHeight) {
    h = static_cast<int>(std::lround(target.height));
    posChanged = true;
  }

  if (posChanged) {
    widget.m_Options.x = x;
    widget.m_Options.y = y;
    widget.m_Options.width = w;
    widget.m_Options.height = h;
    if (target.hasWidth && w > 0)
      widget.m_Options.m_WDefined = true;
    if (target.hasHeight && h > 0)
      widget.m_Options.m_HDefined = true;

    if (widget.m_hWnd) {
      SetWindowPos(widget.m_hWnd, NULL, x, y, w, h,
                   SWP_NOZORDER | SWP_NOACTIVATE);
    }
  }

  if (target.hasOpacity) {
    BYTE op = static_cast<BYTE>(std::clamp(
        static_cast<long>(std::lround(target.opacity * 255.0f)), 0L, 255L));
    widget.m_Options.windowOpacity = op;
  }

  if (target.hasBackgroundColor) {
    BYTE r = static_cast<BYTE>(
        std::clamp(static_cast<long>(std::lround(target.bgColorR)), 0L, 255L));
    BYTE g = static_cast<BYTE>(
        std::clamp(static_cast<long>(std::lround(target.bgColorG)), 0L, 255L));
    BYTE b = static_cast<BYTE>(
        std::clamp(static_cast<long>(std::lround(target.bgColorB)), 0L, 255L));
    widget.m_Options.color = RGB(r, g, b);
    widget.m_Options.bgAlpha = static_cast<BYTE>(
        std::clamp(static_cast<long>(std::lround(target.bgAlpha)), 0L, 255L));
  }
}

void WidgetAnimationHelper::StartElementAnimation(
    Widget &widget, const std::wstring &id, const Widget::AnimationTarget &to,
    const Widget::AnimationTarget &from, int durationMs,
    const std::wstring &easing, int iterationCount) {
  if (id.empty())
    return;
  Element *element = widget.FindElementById(id);
  if (!element)
    return;

  if (from.HasTextProps() || to.HasTextProps())
    return;

  if (!to.HasTransformProps())
    return;

  Widget::ElementAnimation anim{};
  anim.id = id;
  anim.easing = easing.empty() ? L"linear" : easing;
  anim.durationMs = durationMs <= 0 ? 250 : durationMs;
  anim.iterationCount = iterationCount;
  anim.startTick = GetTickCount();
  anim.useKeyframes = false;
  anim.to = to;

  Widget::AnimationTarget base = CaptureElementAnimationState(element);
  MergeAnimationTarget(base, from);
  anim.from = base;

  RemoveAnimationsForElement(widget, id);
  widget.m_Animations.push_back(anim);

  ApplyAnimationTargetToElement(element, anim.from);
  widget.Redraw();

  if (widget.m_hWnd)
    SetTimer(widget.m_hWnd, kTimerId, 16, nullptr);
}

void WidgetAnimationHelper::StartElementKeyframeAnimation(
    Widget &widget, const std::wstring &id,
    const std::vector<Widget::AnimationKeyframe> &keyframes, int durationMs,
    const std::wstring &easing, int iterationCount) {
  if (id.empty())
    return;
  Element *element = widget.FindElementById(id);
  if (!element)
    return;

  if (keyframes.size() < 2)
    return;

  Widget::ElementAnimation anim{};
  anim.id = id;
  anim.easing = easing.empty() ? L"linear" : easing;
  anim.durationMs = durationMs <= 0 ? 250 : durationMs;
  anim.iterationCount = iterationCount;
  anim.startTick = GetTickCount();
  anim.useKeyframes = true;

  ResolveKeyframeStops(element, keyframes, anim.keyframeOffsets,
                       anim.keyframeEasings, anim.resolvedStops);
  if (anim.resolvedStops.size() < 2)
    return;

  RemoveAnimationsForElement(widget, id);
  widget.m_Animations.push_back(anim);

  ApplyAnimationTargetToElement(element, anim.resolvedStops.front());
  widget.Redraw();

  if (widget.m_hWnd)
    SetTimer(widget.m_hWnd, kTimerId, 16, nullptr);
}

void WidgetAnimationHelper::StartWindowAnimation(
    Widget &widget, const Widget::WindowAnimationTarget &to,
    const Widget::WindowAnimationTarget &from, int durationMs,
    const std::wstring &easing, int iterationCount) {
  if (!to.HasAnyProps())
    return;

  Widget::WindowAnimation anim{};
  anim.easing = easing.empty() ? L"linear" : easing;
  anim.durationMs = durationMs <= 0 ? 250 : durationMs;
  anim.iterationCount = iterationCount;
  anim.startTick = GetTickCount();
  anim.useKeyframes = false;
  anim.to = to;

  Widget::WindowAnimationTarget base = CaptureWindowAnimationState(widget);
  MergeWindowAnimationTarget(base, from);
  anim.from = base;

  widget.m_WindowAnimations.clear();
  widget.m_WindowAnimations.push_back(anim);

  ApplyWindowAnimationTarget(widget, anim.from);
  widget.Redraw();

  if (widget.m_hWnd)
    SetTimer(widget.m_hWnd, kTimerId, 16, nullptr);
}

void WidgetAnimationHelper::StartWindowKeyframeAnimation(
    Widget &widget,
    const std::vector<Widget::WindowAnimationKeyframe> &keyframes,
    int durationMs, const std::wstring &easing, int iterationCount) {
  if (keyframes.size() < 2)
    return;

  Widget::WindowAnimation anim{};
  anim.easing = easing.empty() ? L"linear" : easing;
  anim.durationMs = durationMs <= 0 ? 250 : durationMs;
  anim.iterationCount = iterationCount;
  anim.startTick = GetTickCount();
  anim.useKeyframes = true;

  ResolveWindowKeyframeStops(widget, keyframes, anim.keyframeOffsets,
                             anim.keyframeEasings, anim.resolvedStops);
  if (anim.resolvedStops.size() < 2)
    return;

  widget.m_WindowAnimations.clear();
  widget.m_WindowAnimations.push_back(anim);

  ApplyWindowAnimationTarget(widget, anim.resolvedStops.front());
  widget.Redraw();

  if (widget.m_hWnd)
    SetTimer(widget.m_hWnd, kTimerId, 16, nullptr);
}

void WidgetAnimationHelper::StopWindowAnimations(Widget &widget) {
  if (!widget.m_WindowAnimations.empty()) {
    widget.m_WindowAnimations.clear();
    Settings::SaveWidget(widget.m_Options.id, widget.m_Options);
  }
  if (widget.m_Animations.empty() && widget.m_hWnd)
    KillTimer(widget.m_hWnd, kTimerId);
}

void WidgetAnimationHelper::StepAnimations(Widget &widget) {
  if (widget.m_Animations.empty() && widget.m_WindowAnimations.empty()) {
    if (widget.m_hWnd)
      KillTimer(widget.m_hWnd, kTimerId);
    return;
  }

  const DWORD now = GetTickCount();
  bool changed = false;

  // 1. Step element animations
  for (auto it = widget.m_Animations.begin();
       it != widget.m_Animations.end();) {
    Element *element = widget.FindElementById(it->id);
    if (!element) {
      it = widget.m_Animations.erase(it);
      continue;
    }

    const DWORD elapsed = now - it->startTick;
    float t = static_cast<float>(elapsed) / static_cast<float>(it->durationMs);
    if (t < 0.0f)
      t = 0.0f;
    if (t > 1.0f)
      t = 1.0f;

    Widget::AnimationTarget sampled{};
    if (it->useKeyframes)
      sampled = SampleKeyframeTrack(it->keyframeOffsets, it->keyframeEasings,
                                    it->resolvedStops, t, it->easing);
    else {
      const float p = AnimationEasing::Evaluate(t, it->easing);
      sampled = SampleTransformAnimationTarget(it->from, it->to, p);
    }

    ApplyAnimationTargetToElement(element, sampled);
    changed = true;

    if (t >= 1.0f) {
      const bool infinite = it->iterationCount < 0;
      if (infinite) {
        it->startTick = now;
        ++it;
      } else {
        it->completedIterations += 1;
        if (it->completedIterations >= it->iterationCount)
          it = widget.m_Animations.erase(it);
        else {
          it->startTick = now;
          ++it;
        }
      }
    } else
      ++it;
  }

  // 2. Step window animations
  bool windowAnimFinished = false;
  for (auto it = widget.m_WindowAnimations.begin();
       it != widget.m_WindowAnimations.end();) {
    const DWORD elapsed = now - it->startTick;
    float t = static_cast<float>(elapsed) / static_cast<float>(it->durationMs);
    if (t < 0.0f)
      t = 0.0f;
    if (t > 1.0f)
      t = 1.0f;

    Widget::WindowAnimationTarget sampled{};
    if (it->useKeyframes)
      sampled =
          SampleWindowKeyframeTrack(it->keyframeOffsets, it->keyframeEasings,
                                    it->resolvedStops, t, it->easing);
    else {
      const float p = AnimationEasing::Evaluate(t, it->easing);
      sampled = SampleWindowAnimationTarget(it->from, it->to, p);
    }

    ApplyWindowAnimationTarget(widget, sampled);
    changed = true;

    if (t >= 1.0f) {
      const bool infinite = it->iterationCount < 0;
      if (infinite) {
        it->startTick = now;
        ++it;
      } else {
        it->completedIterations += 1;
        if (it->completedIterations >= it->iterationCount) {
          it = widget.m_WindowAnimations.erase(it);
          windowAnimFinished = true;
        } else {
          it->startTick = now;
          ++it;
        }
      }
    } else
      ++it;
  }

  if (windowAnimFinished && widget.m_WindowAnimations.empty()) {
    Settings::SaveWidget(widget.m_Options.id, widget.m_Options);
  }

  if (changed)
    widget.Redraw();

  if (widget.m_Animations.empty() && widget.m_WindowAnimations.empty() &&
      widget.m_hWnd)
    KillTimer(widget.m_hWnd, kTimerId);
}

void WidgetAnimationHelper::RemoveAnimationsForElement(Widget &widget,
                                                       const std::wstring &id) {
  widget.m_Animations.erase(
      std::remove_if(
          widget.m_Animations.begin(), widget.m_Animations.end(),
          [&](const Widget::ElementAnimation &a) { return a.id == id; }),
      widget.m_Animations.end());
}

void WidgetAnimationHelper::ClearAllAnimations(Widget &widget) {
  widget.m_Animations.clear();
  widget.m_WindowAnimations.clear();
}
