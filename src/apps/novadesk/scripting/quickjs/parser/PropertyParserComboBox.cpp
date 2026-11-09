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
#include "../../../render/ComboBoxElement.h"

#include <algorithm>

namespace PropertyParser {

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

  out.value = Js::GetStringProp(ctx, itemV, "value");
  if (out.value.empty())
    out.value = out.label;
  return true;
}
} // namespace

void ParseComboBoxOptions(JSContext *ctx, JSValueConst obj,
                          ComboBoxOptions &o, const std::wstring &base) {
  ParseElementOptions(ctx, obj, o, base);

  // Options array
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

  // Value / text
  {
    std::wstring t = Js::GetStringProp(ctx, obj, "text");
    if (!t.empty()) {
      o.text = t;
    }
  }
  {
    std::wstring val = Js::GetStringProp(ctx, obj, "value");
    if (!val.empty()) {
      o.value = val;
      o.hasValue = true;
      if (o.text.empty())
        o.text = val;
    }
  }
  o.placeholder = Js::GetStringProp(ctx, obj, "placeholder");
  ParseIntProp(ctx, obj, "selectedIndex", o.selectedIndex);

  // Flags
  Js::GetBoolProp(ctx, obj, "editable", o.editable);
  Js::GetBoolProp(ctx, obj, "showChevron", o.showChevron);
  Js::GetBoolProp(ctx, obj, "openOnType", o.openOnType);
  Js::GetBoolProp(ctx, obj, "filterOnType", o.filterOnType);
  Js::GetBoolProp(ctx, obj, "autoComplete", o.autoComplete);
  Js::GetBoolProp(ctx, obj, "disabled", o.disabled);
  Js::GetBoolProp(ctx, obj, "rightToLeft", o.rightToLeft);

  // Box styling
  ParseColorAlpha(ctx, obj, "backgroundColor", o.backgroundColor,
                  o.backgroundAlpha);
  ParseFloatProp(ctx, obj, "borderWidth", o.borderWidth);
  ParseColorAlpha(ctx, obj, "borderColor", o.borderColor, o.borderAlpha);
  ParseFloatProp(ctx, obj, "borderRadius", o.borderRadius);
  ParseFloatProp(ctx, obj, "paddingLeft", o.paddingLeft);
  ParseFloatProp(ctx, obj, "paddingRight", o.paddingRight);

  // Chevron
  ParseColorAlpha(ctx, obj, "chevronColor", o.chevronColor, o.chevronAlpha);
  ParseColorAlpha(ctx, obj, "chevronHoverColor", o.chevronHoverColor,
                  o.chevronHoverAlpha);
  ParseFloatProp(ctx, obj, "chevronSize", o.chevronSize);
  ParseFloatProp(ctx, obj, "chevronAreaWidth", o.chevronAreaWidth);
  ParseFloatProp(ctx, obj, "chevronGap", o.chevronGap);

  // Typography
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

  // States
  ParseColorAlpha(ctx, obj, "hoverBorderColor", o.hoverBorderColor,
                  o.hoverBorderAlpha);
  ParseColorAlpha(ctx, obj, "openBorderColor", o.openBorderColor,
                  o.openBorderAlpha);
  ParseColorAlpha(ctx, obj, "borderFocusColor", o.borderFocusColor,
                  o.borderFocusAlpha);
  ParseColorAlpha(ctx, obj, "disabledBackgroundColor",
                  o.disabledBackgroundColor, o.disabledBackgroundAlpha);
  ParseColorAlpha(ctx, obj, "disabledBorderColor", o.disabledBorderColor,
                  o.disabledBorderAlpha);
  ParseColorAlpha(ctx, obj, "disabledTextColor", o.disabledTextColor,
                  o.disabledTextAlpha);

  // Popup styling
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
  ParseIntProp(ctx, obj, "popupItemHeight", o.popupItemHeight);
  ParseIntProp(ctx, obj, "popupMaxVisibleItems", o.popupMaxVisibleItems);
  ParseIntProp(ctx, obj, "popupPadding", o.popupPadding);
  ParseIntProp(ctx, obj, "popupMinWidth", o.popupMinWidth);

  // Callbacks
  Js::GetEventCallbackProp(ctx, obj, "onChange", o.onChangeCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onSelect", o.onSelectCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onTextChange", o.onTextChangeCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onOpen", o.onOpenCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onClose", o.onCloseCallbackId);
  Js::GetEventCallbackProp(ctx, obj, "onEnter", o.onEnterCallbackId);
}

void ApplyComboBoxOptions(ComboBoxElement *e, const ComboBoxOptions &o) {
  ApplyElementOptions(e, o);

  if (o.hasOptions)
    e->SetOptions(o.options);

  if (!o.placeholder.empty())
    e->SetPlaceholder(o.placeholder);

  e->m_Editable = o.editable;
  e->m_ShowChevron = o.showChevron;
  e->m_OpenOnType = o.openOnType;
  e->m_FilterOnType = o.filterOnType;
  e->m_AutoComplete = o.autoComplete;
  e->m_Disabled = o.disabled;
  e->m_RightToLeft = o.rightToLeft;

  e->SetFillColor(o.backgroundColor, o.backgroundAlpha);
  e->SetBorderWidth(o.borderWidth);
  e->SetBorderColor(o.borderColor, o.borderAlpha);
  e->SetBorderRadius(o.borderRadius);
  e->SetPadding(static_cast<int>(o.paddingLeft), e->GetPaddingTop(),
                static_cast<int>(o.paddingRight), e->GetPaddingBottom());

  e->m_ChevronColor = o.chevronColor;
  e->m_ChevronAlpha = o.chevronAlpha;
  e->m_ChevronHoverColor = o.chevronHoverColor;
  e->m_ChevronHoverAlpha = o.chevronHoverAlpha;
  e->m_ChevronSize = o.chevronSize;
  e->m_ChevronAreaWidth = o.chevronAreaWidth;
  e->m_ChevronGap = o.chevronGap;

  e->SetFontFace(o.fontFace);
  e->SetFontSize(o.fontSize);
  e->SetFontWeight(o.fontWeight);
  e->SetFontColor(o.fontColor, o.fontAlpha);
  e->SetPlaceholderColor(o.placeholderColor, o.placeholderAlpha);

  e->m_HoverBorderColor = o.hoverBorderColor;
  e->m_HoverBorderAlpha = o.hoverBorderAlpha;
  e->m_OpenBorderColor = o.openBorderColor;
  e->m_OpenBorderAlpha = o.openBorderAlpha;
  e->SetBorderFocusColor(o.borderFocusColor, o.borderFocusAlpha);

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
  e->m_PopupItemHeight = o.popupItemHeight;
  e->m_PopupMaxVisibleItems = o.popupMaxVisibleItems;
  e->m_PopupPadding = o.popupPadding;
  e->m_PopupMinWidth = (std::max)(0, o.popupMinWidth);

  e->m_OnChangeCallbackId = o.onChangeCallbackId;
  e->m_OnSelectCallbackId = o.onSelectCallbackId;
  e->m_OnTextChangeCallbackId = o.onTextChangeCallbackId;
  e->m_OnOpenCallbackId = o.onOpenCallbackId;
  e->m_OnCloseCallbackId = o.onCloseCallbackId;
  e->m_OnEnterCallbackId = o.onEnterCallbackId;

  // Set initial value or text
  if (o.hasValue) {
    const int idx = e->IndexForValue(o.value);
    if (idx >= 0) {
      e->SelectOption(idx);
    } else {
      e->SetText(o.value);
    }
  } else if (o.selectedIndex >= 0) {
    e->SelectOption(o.selectedIndex);
  } else if (!o.text.empty()) {
    e->SetText(o.text);
  }
}

void PreFillComboBoxOptions(ComboBoxOptions &o, ComboBoxElement *e) {
  if (!e)
    return;
  PreFillElementOptions(o, e);

  o.options = e->Options();
  o.hasOptions = true;
  o.text = e->GetText();
  o.value = e->SelectedValue();
  o.hasValue = false;
  o.selectedIndex = -1;
  o.placeholder = e->GetPlaceholder();

  o.editable = e->m_Editable;
  o.showChevron = e->m_ShowChevron;
  o.openOnType = e->m_OpenOnType;
  o.filterOnType = e->m_FilterOnType;
  o.autoComplete = e->m_AutoComplete;
  o.disabled = e->m_Disabled;
  o.rightToLeft = e->m_RightToLeft;

  o.backgroundColor = e->GetFillColor();
  o.backgroundAlpha = e->GetFillAlpha();
  o.borderWidth = e->GetBorderWidth();
  o.borderColor = e->GetBorderColor();
  o.borderAlpha = e->GetBorderAlpha();
  o.borderRadius = e->GetBorderRadius();
  o.paddingLeft = static_cast<float>(e->GetPaddingLeft());
  o.paddingRight = static_cast<float>(e->GetPaddingRight());

  o.chevronColor = e->m_ChevronColor;
  o.chevronAlpha = e->m_ChevronAlpha;
  o.chevronHoverColor = e->m_ChevronHoverColor;
  o.chevronHoverAlpha = e->m_ChevronHoverAlpha;
  o.chevronSize = e->m_ChevronSize;
  o.chevronAreaWidth = e->m_ChevronAreaWidth;
  o.chevronGap = e->m_ChevronGap;

  o.fontFace = e->GetFontFace();
  o.fontSize = e->GetFontSize();
  o.fontWeight = e->GetFontWeight();
  o.fontColor = e->GetFontColor();
  o.fontAlpha = e->GetFontAlpha();
  o.placeholderColor = e->GetPlaceholderColor();
  o.placeholderAlpha = e->GetPlaceholderAlpha();

  o.hoverBorderColor = e->m_HoverBorderColor;
  o.hoverBorderAlpha = e->m_HoverBorderAlpha;
  o.openBorderColor = e->m_OpenBorderColor;
  o.openBorderAlpha = e->m_OpenBorderAlpha;
  o.borderFocusColor = e->GetBorderFocusColor();
  o.borderFocusAlpha = e->GetBorderFocusAlpha();

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
  o.popupItemHeight = e->m_PopupItemHeight;
  o.popupMaxVisibleItems = e->m_PopupMaxVisibleItems;
  o.popupPadding = e->m_PopupPadding;
  o.popupMinWidth = e->m_PopupMinWidth;

  o.onChangeCallbackId = e->m_OnChangeCallbackId;
  o.onSelectCallbackId = e->m_OnSelectCallbackId;
  o.onTextChangeCallbackId = e->m_OnTextChangeCallbackId;
  o.onOpenCallbackId = e->m_OnOpenCallbackId;
  o.onCloseCallbackId = e->m_OnCloseCallbackId;
  o.onEnterCallbackId = e->m_OnEnterCallbackId;
}

} // namespace PropertyParser
