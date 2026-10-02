/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../render/ToggleSwitchElement.h"

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

void ParseIntProp(JSContext *ctx, JSValueConst obj, const char *key, int &out) {
  int v = 0;
  if (Js::GetIntProp(ctx, obj, key, v))
    out = v;
}
} // namespace

void ParseToggleSwitchOptions(JSContext *ctx, JSValueConst obj,
                              ToggleSwitchOptions &o,
                              const std::wstring &base) {
  ParseElementOptions(ctx, obj, o, base);

  Js::GetBoolProp(ctx, obj, "checked", o.checked);

  // track styling
  ParseColorAlpha(ctx, obj, "onColor", o.onColor, o.onAlpha);
  ParseColorAlpha(ctx, obj, "offColor", o.offColor, o.offAlpha);
  ParseFloatProp(ctx, obj, "borderWidth", o.borderWidth);
  ParseColorAlpha(ctx, obj, "borderColor", o.borderColor, o.borderAlpha);
  ParseFloatProp(ctx, obj, "borderRadius", o.borderRadius);
  ParseFloatProp(ctx, obj, "opacity", o.opacity);

  // knob styling
  ParseColorAlpha(ctx, obj, "knobColor", o.knobColor, o.knobAlpha);
  ParseFloatProp(ctx, obj, "knobBorderWidth", o.knobBorderWidth);
  ParseColorAlpha(ctx, obj, "knobBorderColor", o.knobBorderColor,
                  o.knobBorderAlpha);
  ParseFloatProp(ctx, obj, "knobSize", o.knobSize);
  ParseFloatProp(ctx, obj, "knobPadding", o.knobPadding);

  // disabled & hover states
  Js::GetBoolProp(ctx, obj, "disabled", o.disabled);
  ParseColorAlpha(ctx, obj, "disabledTrackColor", o.disabledTrackColor,
                  o.disabledTrackAlpha);
  ParseColorAlpha(ctx, obj, "disabledKnobColor", o.disabledKnobColor,
                  o.disabledKnobAlpha);
  {
    std::wstring hover = Js::GetStringProp(ctx, obj, "hoverTrackColor");
    if (!hover.empty()) {
      ColorUtil::ParseRGBA(hover, o.hoverTrackColor, o.hoverTrackAlpha);
      o.hasHoverTrackColor = true;
    }
  }

  // labels
  o.onText = Js::GetStringProp(ctx, obj, "onText");
  o.offText = Js::GetStringProp(ctx, obj, "offText");
  {
    std::wstring face = Js::GetStringProp(ctx, obj, "labelFontFace");
    if (!face.empty())
      o.labelFontFace = face;
  }
  ParseIntProp(ctx, obj, "labelFontSize", o.labelFontSize);
  if (o.labelFontSize < 1)
    o.labelFontSize = 1;
  ParseIntProp(ctx, obj, "labelFontWeight", o.labelFontWeight);
  ParseColorAlpha(ctx, obj, "labelFontColor", o.labelFontColor,
                  o.labelFontAlpha);

  // callbacks
  Js::GetEventCallbackProp(ctx, obj, "onChange", o.onChangeCallbackId);
}

void ApplyToggleSwitchOptions(ToggleSwitchElement *e,
                              const ToggleSwitchOptions &o) {
  ApplyElementOptions(e, o);

  e->m_OnColor = o.onColor;
  e->m_OnAlpha = o.onAlpha;
  e->m_OffColor = o.offColor;
  e->m_OffAlpha = o.offAlpha;
  e->m_BorderWidth = o.borderWidth;
  e->m_BorderColor = o.borderColor;
  e->m_BorderAlpha = o.borderAlpha;
  e->m_BorderRadius = o.borderRadius;
  e->m_Opacity = o.opacity;

  e->m_KnobColor = o.knobColor;
  e->m_KnobAlpha = o.knobAlpha;
  e->m_KnobBorderWidth = o.knobBorderWidth;
  e->m_KnobBorderColor = o.knobBorderColor;
  e->m_KnobBorderAlpha = o.knobBorderAlpha;
  e->m_KnobSize = o.knobSize;
  e->m_KnobPadding = o.knobPadding;

  e->m_Disabled = o.disabled;
  e->m_DisabledTrackColor = o.disabledTrackColor;
  e->m_DisabledTrackAlpha = o.disabledTrackAlpha;
  e->m_DisabledKnobColor = o.disabledKnobColor;
  e->m_DisabledKnobAlpha = o.disabledKnobAlpha;
  e->m_HasHoverTrackColor = o.hasHoverTrackColor;
  e->m_HoverTrackColor = o.hoverTrackColor;
  e->m_HoverTrackAlpha = o.hoverTrackAlpha;

  e->m_OnText = o.onText;
  e->m_OffText = o.offText;
  e->m_LabelFontFace = o.labelFontFace;
  e->m_LabelFontSize = o.labelFontSize;
  e->m_LabelFontWeight = o.labelFontWeight;
  e->m_LabelFontColor = o.labelFontColor;
  e->m_LabelFontAlpha = o.labelFontAlpha;

  e->m_OnChangeCallbackId = o.onChangeCallbackId;

  // An interactive switch shows the hand cursor even without a JS callback.
  if (e->GetMouseEventCursorName().empty()) {
    e->SetMouseEventCursor(true);
    e->SetMouseEventCursorName(L"hand");
  }

  e->SetChecked(o.checked);
}

// ─────────────────────────────────────────────────────────────────────────────
void PreFillToggleSwitchOptions(ToggleSwitchOptions &o,
                                ToggleSwitchElement *e) {
  PreFillElementOptions(o, e);

  o.checked = e->IsChecked();

  o.onColor = e->m_OnColor;
  o.onAlpha = e->m_OnAlpha;
  o.offColor = e->m_OffColor;
  o.offAlpha = e->m_OffAlpha;
  o.borderWidth = e->m_BorderWidth;
  o.borderColor = e->m_BorderColor;
  o.borderAlpha = e->m_BorderAlpha;
  o.borderRadius = e->m_BorderRadius;
  o.opacity = e->m_Opacity;

  o.knobColor = e->m_KnobColor;
  o.knobAlpha = e->m_KnobAlpha;
  o.knobBorderWidth = e->m_KnobBorderWidth;
  o.knobBorderColor = e->m_KnobBorderColor;
  o.knobBorderAlpha = e->m_KnobBorderAlpha;
  o.knobSize = e->m_KnobSize;
  o.knobPadding = e->m_KnobPadding;

  o.disabled = e->m_Disabled;
  o.disabledTrackColor = e->m_DisabledTrackColor;
  o.disabledTrackAlpha = e->m_DisabledTrackAlpha;
  o.disabledKnobColor = e->m_DisabledKnobColor;
  o.disabledKnobAlpha = e->m_DisabledKnobAlpha;
  o.hasHoverTrackColor = e->m_HasHoverTrackColor;
  o.hoverTrackColor = e->m_HoverTrackColor;
  o.hoverTrackAlpha = e->m_HoverTrackAlpha;

  o.onText = e->m_OnText;
  o.offText = e->m_OffText;
  o.labelFontFace = e->m_LabelFontFace;
  o.labelFontSize = e->m_LabelFontSize;
  o.labelFontWeight = e->m_LabelFontWeight;
  o.labelFontColor = e->m_LabelFontColor;
  o.labelFontAlpha = e->m_LabelFontAlpha;

  o.onChangeCallbackId = e->m_OnChangeCallbackId;
}

} // namespace PropertyParser
