/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../render/CheckBoxElement.h"

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

void ParseCheckBoxOptions(JSContext *ctx, JSValueConst obj, CheckBoxOptions &o,
                          const std::wstring &base) {
  ParseElementOptions(ctx, obj, o, base);

  // checked accepts a bool or the string "indeterminate".
  std::wstring checkedStr = Js::GetStringProp(ctx, obj, "checked");
  if (!checkedStr.empty()) {
    std::transform(checkedStr.begin(), checkedStr.end(), checkedStr.begin(),
                   ::towlower);
    if (checkedStr == L"indeterminate") {
      o.checked = true;
      o.indeterminate = true;
    } else {
      o.checked = checkedStr != L"false" && checkedStr != L"0";
      o.indeterminate = false;
    }
  } else {
    Js::GetBoolProp(ctx, obj, "checked", o.checked);
    if (!o.checked) {
      std::wstring state = Js::GetStringProp(ctx, obj, "state");
      if (!state.empty()) {
        std::transform(state.begin(), state.end(), state.begin(), ::towlower);
        if (state == L"indeterminate") {
          o.checked = true;
          o.indeterminate = true;
        } else if (state == L"checked") {
          o.checked = true;
          o.indeterminate = false;
        } else if (state == L"unchecked") {
          o.checked = false;
          o.indeterminate = false;
        }
      }
    }
  }

  // box styling
  Js::GetBoolProp(ctx, obj, "triState", o.triState);
  ParseFloatProp(ctx, obj, "boxSize", o.boxSize);
  ParseFloatProp(ctx, obj, "borderRadius", o.borderRadius);
  ParseColorAlpha(ctx, obj, "uncheckedBorderColor", o.uncheckedBorderColor,
                  o.uncheckedBorderAlpha);
  ParseFloatProp(ctx, obj, "uncheckedBorderWidth", o.uncheckedBorderWidth);
  ParseColorAlpha(ctx, obj, "checkedColor", o.checkedColor, o.checkedAlpha);
  ParseFloatProp(ctx, obj, "boxOpacity", o.boxOpacity);

  // mark styling
  ParseColorAlpha(ctx, obj, "checkColor", o.checkColor, o.checkAlpha);
  ParseFloatProp(ctx, obj, "checkThickness", o.checkThickness);

  // disabled & hover states
  Js::GetBoolProp(ctx, obj, "disabled", o.disabled);
  ParseColorAlpha(ctx, obj, "disabledBoxColor", o.disabledBoxColor,
                  o.disabledBoxAlpha);
  ParseColorAlpha(ctx, obj, "disabledCheckColor", o.disabledCheckColor,
                  o.disabledCheckAlpha);
  {
    std::wstring hover = Js::GetStringProp(ctx, obj, "hoverBorderColor");
    if (!hover.empty()) {
      ColorUtil::ParseRGBA(hover, o.hoverBorderColor, o.hoverBorderAlpha);
      o.hasHoverBorderColor = true;
    }
  }

  // callbacks
  Js::GetEventCallbackProp(ctx, obj, "onChange", o.onChangeCallbackId);
}

void ApplyCheckBoxOptions(CheckBoxElement *e, const CheckBoxOptions &o) {
  ApplyElementOptions(e, o);

  e->m_TriState = o.triState;
  e->m_BoxSize = o.boxSize;
  e->m_BorderRadius = o.borderRadius;
  e->m_UncheckedBorderColor = o.uncheckedBorderColor;
  e->m_UncheckedBorderAlpha = o.uncheckedBorderAlpha;
  e->m_UncheckedBorderWidth = o.uncheckedBorderWidth;
  e->m_CheckedColor = o.checkedColor;
  e->m_CheckedAlpha = o.checkedAlpha;
  e->m_BoxOpacity = o.boxOpacity;

  e->m_CheckColor = o.checkColor;
  e->m_CheckAlpha = o.checkAlpha;
  e->m_CheckThickness = o.checkThickness;

  e->m_Disabled = o.disabled;
  e->m_DisabledBoxColor = o.disabledBoxColor;
  e->m_DisabledBoxAlpha = o.disabledBoxAlpha;
  e->m_DisabledCheckColor = o.disabledCheckColor;
  e->m_DisabledCheckAlpha = o.disabledCheckAlpha;
  e->m_HasHoverBorderColor = o.hasHoverBorderColor;
  e->m_HoverBorderColor = o.hoverBorderColor;
  e->m_HoverBorderAlpha = o.hoverBorderAlpha;

  e->m_OnChangeCallbackId = o.onChangeCallbackId;

  // An interactive check box shows the hand cursor even without a JS callback.
  if (e->GetMouseEventCursorName().empty()) {
    e->SetMouseEventCursor(true);
    e->SetMouseEventCursorName(L"hand");
  }

  const CheckBoxElement::State wanted =
      o.indeterminate ? CheckBoxElement::State::Indeterminate
                      : (o.checked ? CheckBoxElement::State::Checked
                                   : CheckBoxElement::State::Unchecked);
  e->SetState(wanted);
}

// ─────────────────────────────────────────────────────────────────────────────
void PreFillCheckBoxOptions(CheckBoxOptions &o, CheckBoxElement *e) {
  PreFillElementOptions(o, e);

  o.checked = e->IsChecked();
  o.indeterminate =
      e->GetState() == CheckBoxElement::State::Indeterminate;
  o.triState = e->m_TriState;

  o.boxSize = e->m_BoxSize;
  o.borderRadius = e->m_BorderRadius;
  o.uncheckedBorderColor = e->m_UncheckedBorderColor;
  o.uncheckedBorderAlpha = e->m_UncheckedBorderAlpha;
  o.uncheckedBorderWidth = e->m_UncheckedBorderWidth;
  o.checkedColor = e->m_CheckedColor;
  o.checkedAlpha = e->m_CheckedAlpha;
  o.boxOpacity = e->m_BoxOpacity;

  o.checkColor = e->m_CheckColor;
  o.checkAlpha = e->m_CheckAlpha;
  o.checkThickness = e->m_CheckThickness;

  o.disabled = e->m_Disabled;
  o.disabledBoxColor = e->m_DisabledBoxColor;
  o.disabledBoxAlpha = e->m_DisabledBoxAlpha;
  o.disabledCheckColor = e->m_DisabledCheckColor;
  o.disabledCheckAlpha = e->m_DisabledCheckAlpha;
  o.hasHoverBorderColor = e->m_HasHoverBorderColor;
  o.hoverBorderColor = e->m_HoverBorderColor;
  o.hoverBorderAlpha = e->m_HoverBorderAlpha;

  o.onChangeCallbackId = e->m_OnChangeCallbackId;
}

} // namespace PropertyParser
