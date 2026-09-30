/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../render/SliderElement.h"

#include <algorithm>

namespace PropertyParser {

// helpers
namespace {
void ParseColorAlpha(JSContext *ctx, JSValueConst obj, const char *key,
                     COLORREF &color, BYTE &alpha) {
  std::wstring s = Js::GetStringProp(ctx, obj, key);
  if (!s.empty())
    ColorUtil::ParseRGBA(s, color, alpha);
}

void ParseFloatProp(JSContext *ctx, JSValueConst obj, const char *key,
                    float &out) {
  float v = 0.0f;
  if (Js::GetFloatProp(ctx, obj, key, v))
    out = v;
}

void ParseDoubleProp(JSContext *ctx, JSValueConst obj, const char *key,
                     double &out) {
  float v = 0.0f;
  if (Js::GetFloatProp(ctx, obj, key, v))
    out = static_cast<double>(v);
}
} // namespace

void ParseSliderOptions(JSContext *ctx, JSValueConst obj, SliderOptions &o,
                        const std::wstring &base) {
  ParseElementOptions(ctx, obj, o, base);

  // value / range
  ParseDoubleProp(ctx, obj, "value", o.value);
  ParseDoubleProp(ctx, obj, "minValue", o.minValue);
  ParseDoubleProp(ctx, obj, "maxValue", o.maxValue);
  ParseDoubleProp(ctx, obj, "step", o.step);

  // direction: "row"/"column" (flex vocabulary) with orientation aliases.
  {
    std::wstring dir = Js::GetStringProp(ctx, obj, "direction");
    if (dir.empty())
      dir = Js::GetStringProp(ctx, obj, "orientation");
    if (!dir.empty()) {
      std::transform(dir.begin(), dir.end(), dir.begin(), ::towlower);
      o.vertical = dir == L"column" || dir == L"vertical";
    }
  }

  // track styling
  ParseFloatProp(ctx, obj, "trackThickness", o.trackThickness);
  ParseFloatProp(ctx, obj, "trackBorderRadius", o.trackBorderRadius);
  ParseColorAlpha(ctx, obj, "trackColor", o.trackColor, o.trackAlpha);
  ParseColorAlpha(ctx, obj, "fillColor", o.fillColor, o.fillAlpha);
  ParseFloatProp(ctx, obj, "sliderOpacity", o.sliderOpacity);

  // thumb styling
  ParseFloatProp(ctx, obj, "thumbSize", o.thumbSize);
  ParseColorAlpha(ctx, obj, "thumbColor", o.thumbColor, o.thumbAlpha);
  ParseFloatProp(ctx, obj, "thumbBorderWidth", o.thumbBorderWidth);
  ParseColorAlpha(ctx, obj, "thumbBorderColor", o.thumbBorderColor,
                  o.thumbBorderAlpha);
  {
    std::wstring hover = Js::GetStringProp(ctx, obj, "hoverThumbColor");
    if (!hover.empty()) {
      ColorUtil::ParseRGBA(hover, o.hoverThumbColor, o.hoverThumbAlpha);
      o.hasHoverThumbColor = true;
    }
  }
  {
    std::wstring pressed = Js::GetStringProp(ctx, obj, "pressedThumbColor");
    if (!pressed.empty()) {
      ColorUtil::ParseRGBA(pressed, o.pressedThumbColor, o.pressedThumbAlpha);
      o.hasPressedThumbColor = true;
    }
  }

  // disabled state
  Js::GetBoolProp(ctx, obj, "disabled", o.disabled);
  ParseColorAlpha(ctx, obj, "disabledTrackColor", o.disabledTrackColor,
                  o.disabledTrackAlpha);
  ParseColorAlpha(ctx, obj, "disabledFillColor", o.disabledFillColor,
                  o.disabledFillAlpha);
  ParseColorAlpha(ctx, obj, "disabledThumbColor", o.disabledThumbColor,
                  o.disabledThumbAlpha);

  // callbacks
  Js::GetEventCallbackProp(ctx, obj, "onChange", o.onChangeCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onInput", o.onInputCallbackId);
}

void ApplySliderOptions(SliderElement *e, const SliderOptions &o) {
  ApplyElementOptions(e, o);

  e->SetRange(o.minValue, o.maxValue);
  e->m_Step = o.step;
  e->m_Orientation =
      o.vertical ? SliderElement::Orientation::Vertical
                 : SliderElement::Orientation::Horizontal;

  e->m_TrackThickness = o.trackThickness;
  e->m_TrackBorderRadius = o.trackBorderRadius;
  e->m_TrackColor = o.trackColor;
  e->m_TrackAlpha = o.trackAlpha;
  e->m_FillColor = o.fillColor;
  e->m_FillAlpha = o.fillAlpha;
  e->m_SliderOpacity = o.sliderOpacity;

  e->m_ThumbSize = o.thumbSize;
  e->m_ThumbColor = o.thumbColor;
  e->m_ThumbAlpha = o.thumbAlpha;
  e->m_ThumbBorderWidth = o.thumbBorderWidth;
  e->m_ThumbBorderColor = o.thumbBorderColor;
  e->m_ThumbBorderAlpha = o.thumbBorderAlpha;
  e->m_HasHoverThumbColor = o.hasHoverThumbColor;
  e->m_HoverThumbColor = o.hoverThumbColor;
  e->m_HoverThumbAlpha = o.hoverThumbAlpha;
  e->m_HasPressedThumbColor = o.hasPressedThumbColor;
  e->m_PressedThumbColor = o.pressedThumbColor;
  e->m_PressedThumbAlpha = o.pressedThumbAlpha;

  e->m_Disabled = o.disabled;
  e->m_DisabledTrackColor = o.disabledTrackColor;
  e->m_DisabledTrackAlpha = o.disabledTrackAlpha;
  e->m_DisabledFillColor = o.disabledFillColor;
  e->m_DisabledFillAlpha = o.disabledFillAlpha;
  e->m_DisabledThumbColor = o.disabledThumbColor;
  e->m_DisabledThumbAlpha = o.disabledThumbAlpha;

  e->m_OnChangeCallbackId = o.onChangeCallbackId;
  e->m_OnInputCallbackId = o.onInputCallbackId;

  // An interactive slider shows the hand cursor even without a JS callback.
  if (e->GetMouseEventCursorName().empty()) {
    e->SetMouseEventCursor(true);
    e->SetMouseEventCursorName(L"hand");
  }

  const double wanted = e->SnapAndClamp(o.value);
  if (e->GetValue() != wanted)
    e->SetValue(wanted);
}

// ─────────────────────────────────────────────────────────────────────────────
void PreFillSliderOptions(SliderOptions &o, SliderElement *e) {
  PreFillElementOptions(o, e);

  o.value = e->GetValue();
  o.minValue = e->m_MinValue;
  o.maxValue = e->m_MaxValue;
  o.step = e->m_Step;
  o.vertical = e->IsVertical();

  o.trackThickness = e->m_TrackThickness;
  o.trackBorderRadius = e->m_TrackBorderRadius;
  o.trackColor = e->m_TrackColor;
  o.trackAlpha = e->m_TrackAlpha;
  o.fillColor = e->m_FillColor;
  o.fillAlpha = e->m_FillAlpha;
  o.sliderOpacity = e->m_SliderOpacity;

  o.thumbSize = e->m_ThumbSize;
  o.thumbColor = e->m_ThumbColor;
  o.thumbAlpha = e->m_ThumbAlpha;
  o.thumbBorderWidth = e->m_ThumbBorderWidth;
  o.thumbBorderColor = e->m_ThumbBorderColor;
  o.thumbBorderAlpha = e->m_ThumbBorderAlpha;
  o.hasHoverThumbColor = e->m_HasHoverThumbColor;
  o.hoverThumbColor = e->m_HoverThumbColor;
  o.hoverThumbAlpha = e->m_HoverThumbAlpha;
  o.hasPressedThumbColor = e->m_HasPressedThumbColor;
  o.pressedThumbColor = e->m_PressedThumbColor;
  o.pressedThumbAlpha = e->m_PressedThumbAlpha;

  o.disabled = e->m_Disabled;
  o.disabledTrackColor = e->m_DisabledTrackColor;
  o.disabledTrackAlpha = e->m_DisabledTrackAlpha;
  o.disabledFillColor = e->m_DisabledFillColor;
  o.disabledFillAlpha = e->m_DisabledFillAlpha;
  o.disabledThumbColor = e->m_DisabledThumbColor;
  o.disabledThumbAlpha = e->m_DisabledThumbAlpha;

  o.onChangeCallbackId = e->m_OnChangeCallbackId;
  o.onInputCallbackId = e->m_OnInputCallbackId;
}

} // namespace PropertyParser
