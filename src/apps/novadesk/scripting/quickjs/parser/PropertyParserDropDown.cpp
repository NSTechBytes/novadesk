/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../shared/Utils.h"
#include "../../../render/DropDownElement.h"

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

/// Reads one option entry: a plain string, or { label, value }.
bool ReadDropDownOption(JSContext *ctx, JSValueConst itemV,
                        DropDownOption &out) {
  if (JS_IsString(itemV)) {
    const char *s = JS_ToCString(ctx, itemV);
    if (s) {
      out.label = Utils::ToWString(s);
      out.value = out.label;
      JS_FreeCString(ctx, s);
      return true;
    }
    return false;
  }

  if (!JS_IsObject(itemV) || JS_IsArray(itemV))
    return false;

  out.label = Js::GetStringProp(ctx, itemV, "label");
  if (out.label.empty())
    out.label = Js::GetStringProp(ctx, itemV, "text");
  if (out.label.empty())
    return false;

  // GetStringProp coerces through JS_ToCString, so a numeric `value` arrives
  // written the way the script wrote it ("1", "1.5").
  out.value = Js::GetStringProp(ctx, itemV, "value");
  if (out.value.empty())
    out.value = out.label;
  return true;
}
} // namespace

void ParseDropDownOptions(JSContext *ctx, JSValueConst obj,
                          DropDownOptions &o, const std::wstring &base) {
  ParseElementOptions(ctx, obj, o, base);

  // Choices: an array of strings or { label, value } objects.
  JSValue arrV = JS_GetPropertyStr(ctx, obj, "options");
  if (JS_IsArray(arrV)) {
    o.hasOptions = true;
    o.options.clear();
    uint32_t len = 0;
    JSValue lenV = JS_GetPropertyStr(ctx, arrV, "length");
    if (JS_ToUint32(ctx, &len, lenV) == 0) {
      o.options.reserve(len);
      for (uint32_t i = 0; i < len; ++i) {
        JSValue itemV = JS_GetPropertyUint32(ctx, arrV, i);
        DropDownOption option;
        if (ReadDropDownOption(ctx, itemV, option))
          o.options.push_back(std::move(option));
        JS_FreeValue(ctx, itemV);
      }
    }
    JS_FreeValue(ctx, lenV);
  }
  JS_FreeValue(ctx, arrV);

  ParseIntProp(ctx, obj, "selectedIndex", o.selectedIndex);
  {
    std::wstring wanted = Js::GetStringProp(ctx, obj, "selectedValue");
    if (!wanted.empty()) {
      o.selectedValue = wanted;
      o.hasSelectedValue = true;
    } else {
      // Absent key must not look like an explicit "" selection.
      JSValue v = JS_GetPropertyStr(ctx, obj, "selectedValue");
      if (JS_IsString(v)) {
        o.selectedValue.clear();
        o.hasSelectedValue = true;
      }
      JS_FreeValue(ctx, v);
    }
  }
  o.placeholder = Js::GetStringProp(ctx, obj, "placeholder");

  // Box styling.
  ParseFloatProp(ctx, obj, "dropdownOpacity", o.boxOpacity);
  ParseColorAlpha(ctx, obj, "backgroundColor", o.backgroundColor,
                  o.backgroundAlpha);
  ParseFloatProp(ctx, obj, "borderWidth", o.borderWidth);
  ParseColorAlpha(ctx, obj, "borderColor", o.borderColor, o.borderAlpha);
  ParseFloatProp(ctx, obj, "borderRadius", o.borderRadius);
  ParseFloatProp(ctx, obj, "paddingLeft", o.paddingLeft);
  ParseFloatProp(ctx, obj, "paddingRight", o.paddingRight);

  // Chevron.
  ParseColorAlpha(ctx, obj, "chevronColor", o.chevronColor, o.chevronAlpha);
  ParseFloatProp(ctx, obj, "chevronSize", o.chevronSize);
  ParseFloatProp(ctx, obj, "chevronGap", o.chevronGap);

  // Text.
  {
    std::wstring face = Js::GetStringProp(ctx, obj, "fontFace");
    if (!face.empty())
      o.fontFace = face;
  }
  ParseIntProp(ctx, obj, "fontSize", o.fontSize);
  ParseIntProp(ctx, obj, "fontWeight", o.fontWeight);
  ParseColorAlpha(ctx, obj, "fontColor", o.fontColor, o.fontAlpha);
  ParseColorAlpha(ctx, obj, "placeholderColor", o.placeholderColor,
                  o.placeholderAlpha);
  {
    int cap = 0;
    if (Js::GetIntProp(ctx, obj, "maxDisplayLength", cap) && cap >= 0)
      o.maxDisplayLength = static_cast<UINT>(cap);
  }

  // States.
  Js::GetBoolProp(ctx, obj, "disabled", o.disabled);
  ParseColorAlpha(ctx, obj, "hoverBorderColor", o.hoverBorderColor,
                  o.hoverBorderAlpha);
  ParseColorAlpha(ctx, obj, "openBorderColor", o.openBorderColor,
                  o.openBorderAlpha);
  ParseColorAlpha(ctx, obj, "disabledBackgroundColor",
                  o.disabledBackgroundColor, o.disabledBackgroundAlpha);
  ParseColorAlpha(ctx, obj, "disabledBorderColor", o.disabledBorderColor,
                  o.disabledBorderAlpha);
  ParseColorAlpha(ctx, obj, "disabledTextColor", o.disabledTextColor,
                  o.disabledTextAlpha);

  // Popup styling.
  ParseColorAlpha(ctx, obj, "popupBackground", o.popupBackground,
                  o.popupBackgroundAlpha);
  ParseColorAlpha(ctx, obj, "popupBorderColor", o.popupBorderColor,
                  o.popupBorderAlpha);
  ParseColorAlpha(ctx, obj, "popupHoverColor", o.popupHoverColor,
                  o.popupHoverAlpha);
  ParseColorAlpha(ctx, obj, "popupSelectedColor", o.popupSelectedColor,
                  o.popupSelectedAlpha);
  ParseColorAlpha(ctx, obj, "popupTextColor", o.popupTextColor,
                  o.popupTextAlpha);
  ParseColorAlpha(ctx, obj, "popupCheckColor", o.popupCheckColor,
                  o.popupCheckAlpha);
  ParseIntProp(ctx, obj, "popupItemHeight", o.popupItemHeight);
  ParseIntProp(ctx, obj, "popupMaxVisibleItems", o.popupMaxVisibleItems);
  ParseIntProp(ctx, obj, "popupPadding", o.popupPadding);

  // Callbacks.
  Js::GetEventCallbackProp(ctx, obj, "onChange", o.onChangeCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onOpen", o.onOpenCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onClose", o.onCloseCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onCancel", o.onCancelCallbackId);
}

void ApplyDropDownOptions(DropDownElement *e, const DropDownOptions &o) {
  ApplyElementOptions(e, o);

  if (o.hasOptions)
    e->SetOptions(o.options);

  e->m_Placeholder = o.placeholder;
  e->m_MaxDisplayLength = o.maxDisplayLength;

  e->m_DropdownOpacity = o.boxOpacity;
  e->m_BackgroundColor = o.backgroundColor;
  e->m_BackgroundAlpha = o.backgroundAlpha;
  e->m_BorderWidth = o.borderWidth;
  e->m_BorderColor = o.borderColor;
  e->m_BorderAlpha = o.borderAlpha;
  e->m_BorderRadius = o.borderRadius;
  e->m_PaddingLeft = o.paddingLeft;
  e->m_PaddingRight = o.paddingRight;

  e->m_ChevronColor = o.chevronColor;
  e->m_ChevronAlpha = o.chevronAlpha;
  e->m_ChevronSize = o.chevronSize;
  e->m_ChevronGap = o.chevronGap;

  e->m_FontFace = o.fontFace;
  e->m_FontSize = o.fontSize;
  e->m_FontWeight = o.fontWeight;
  e->m_FontColor = o.fontColor;
  e->m_FontAlpha = o.fontAlpha;
  e->m_PlaceholderColor = o.placeholderColor;
  e->m_PlaceholderAlpha = o.placeholderAlpha;

  e->m_Disabled = o.disabled;
  e->m_HoverBorderColor = o.hoverBorderColor;
  e->m_HoverBorderAlpha = o.hoverBorderAlpha;
  e->m_OpenBorderColor = o.openBorderColor;
  e->m_OpenBorderAlpha = o.openBorderAlpha;
  e->m_DisabledBackgroundColor = o.disabledBackgroundColor;
  e->m_DisabledBackgroundAlpha = o.disabledBackgroundAlpha;
  e->m_DisabledBorderColor = o.disabledBorderColor;
  e->m_DisabledBorderAlpha = o.disabledBorderAlpha;
  e->m_DisabledTextColor = o.disabledTextColor;
  e->m_DisabledTextAlpha = o.disabledTextAlpha;

  e->m_PopupBackground = o.popupBackground;
  e->m_PopupBackgroundAlpha = o.popupBackgroundAlpha;
  e->m_PopupBorderColor = o.popupBorderColor;
  e->m_PopupBorderAlpha = o.popupBorderAlpha;
  e->m_PopupHoverColor = o.popupHoverColor;
  e->m_PopupHoverAlpha = o.popupHoverAlpha;
  e->m_PopupSelectedColor = o.popupSelectedColor;
  e->m_PopupSelectedAlpha = o.popupSelectedAlpha;
  e->m_PopupTextColor = o.popupTextColor;
  e->m_PopupTextAlpha = o.popupTextAlpha;
  e->m_PopupCheckColor = o.popupCheckColor;
  e->m_PopupCheckAlpha = o.popupCheckAlpha;
  e->m_PopupItemHeight = o.popupItemHeight;
  e->m_PopupMaxVisibleItems = o.popupMaxVisibleItems;
  e->m_PopupPadding = o.popupPadding;

  e->m_OnChangeCallbackId = o.onChangeCallbackId;
  e->m_OnOpenCallbackId = o.onOpenCallbackId;
  e->m_OnCloseCallbackId = o.onCloseCallbackId;
  e->m_OnCancelCallbackId = o.onCancelCallbackId;

  // An interactive drop-down shows the hand cursor even without a JS callback.
  if (e->GetMouseEventCursorName().empty()) {
    e->SetMouseEventCursor(true);
    e->SetMouseEventCursorName(L"hand");
  }

  // Commit the choice only when it actually differs, so re-applying the same
  // properties never looks like a user selection. An explicit `selectedValue`
  // outranks `selectedIndex`; an unresolvable value leaves the index alone.
  int wanted = o.selectedIndex;
  if (o.hasSelectedValue) {
    const int byValue = e->IndexForValue(o.selectedValue);
    if (byValue >= 0)
      wanted = byValue;
    else
      wanted = e->GetSelectedIndex();
  }
  if (wanted != e->GetSelectedIndex())
    e->SetSelectedIndex(wanted);
}

// ─────────────────────────────────────────────────────────────────────────────
void PreFillDropDownOptions(DropDownOptions &o, DropDownElement *e) {
  PreFillElementOptions(o, e);

  o.options = e->Options();
  o.hasOptions = true;
  o.selectedIndex = e->GetSelectedIndex();
  o.selectedValue = e->SelectedValue();
  o.hasSelectedValue = !o.selectedValue.empty();
  o.placeholder = e->m_Placeholder;
  o.maxDisplayLength = e->m_MaxDisplayLength;

  o.boxOpacity = e->m_DropdownOpacity;
  o.backgroundColor = e->m_BackgroundColor;
  o.backgroundAlpha = e->m_BackgroundAlpha;
  o.borderWidth = e->m_BorderWidth;
  o.borderColor = e->m_BorderColor;
  o.borderAlpha = e->m_BorderAlpha;
  o.borderRadius = e->m_BorderRadius;
  o.paddingLeft = e->m_PaddingLeft;
  o.paddingRight = e->m_PaddingRight;

  o.chevronColor = e->m_ChevronColor;
  o.chevronAlpha = e->m_ChevronAlpha;
  o.chevronSize = e->m_ChevronSize;
  o.chevronGap = e->m_ChevronGap;

  o.fontFace = e->m_FontFace;
  o.fontSize = e->m_FontSize;
  o.fontWeight = e->m_FontWeight;
  o.fontColor = e->m_FontColor;
  o.fontAlpha = e->m_FontAlpha;
  o.placeholderColor = e->m_PlaceholderColor;
  o.placeholderAlpha = e->m_PlaceholderAlpha;

  o.disabled = e->m_Disabled;
  o.hoverBorderColor = e->m_HoverBorderColor;
  o.hoverBorderAlpha = e->m_HoverBorderAlpha;
  o.openBorderColor = e->m_OpenBorderColor;
  o.openBorderAlpha = e->m_OpenBorderAlpha;
  o.disabledBackgroundColor = e->m_DisabledBackgroundColor;
  o.disabledBackgroundAlpha = e->m_DisabledBackgroundAlpha;
  o.disabledBorderColor = e->m_DisabledBorderColor;
  o.disabledBorderAlpha = e->m_DisabledBorderAlpha;
  o.disabledTextColor = e->m_DisabledTextColor;
  o.disabledTextAlpha = e->m_DisabledTextAlpha;

  o.popupBackground = e->m_PopupBackground;
  o.popupBackgroundAlpha = e->m_PopupBackgroundAlpha;
  o.popupBorderColor = e->m_PopupBorderColor;
  o.popupBorderAlpha = e->m_PopupBorderAlpha;
  o.popupHoverColor = e->m_PopupHoverColor;
  o.popupHoverAlpha = e->m_PopupHoverAlpha;
  o.popupSelectedColor = e->m_PopupSelectedColor;
  o.popupSelectedAlpha = e->m_PopupSelectedAlpha;
  o.popupTextColor = e->m_PopupTextColor;
  o.popupTextAlpha = e->m_PopupTextAlpha;
  o.popupCheckColor = e->m_PopupCheckColor;
  o.popupCheckAlpha = e->m_PopupCheckAlpha;
  o.popupItemHeight = e->m_PopupItemHeight;
  o.popupMaxVisibleItems = e->m_PopupMaxVisibleItems;
  o.popupPadding = e->m_PopupPadding;

  o.onChangeCallbackId = e->m_OnChangeCallbackId;
  o.onOpenCallbackId = e->m_OnOpenCallbackId;
  o.onCloseCallbackId = e->m_OnCloseCallbackId;
  o.onCancelCallbackId = e->m_OnCancelCallbackId;
}

} // namespace PropertyParser
