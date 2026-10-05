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
#include "../../render/ImageElement.h"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <vector>

namespace PropertyParser {

// ── Parse one flat array of setting rows into out ────────────────────────────
static bool ParseSettingsRows(JSContext *ctx, JSValueConst arr,
                              std::vector<WidgetSetting> &out) {
  if (!JS_IsArray(arr))
    return false;

  uint32_t len = 0;
  JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
  if (JS_ToUint32(ctx, &len, lenV) != 0) {
    JS_FreeValue(ctx, lenV);
    return false;
  }
  JS_FreeValue(ctx, lenV);

  bool parsedAny = false;
  for (uint32_t i = 0; i < len; ++i) {
    JSValue itemV = JS_GetPropertyUint32(ctx, arr, i);
    if (!JS_IsObject(itemV)) {
      JS_FreeValue(ctx, itemV);
      continue;
    }

    WidgetSetting setting;
    setting.id = Js::GetStringProp(ctx, itemV, "id");
    std::wstring typeStr = Js::GetStringProp(ctx, itemV, "type");
    std::transform(typeStr.begin(), typeStr.end(), typeStr.begin(), ::towlower);
    if (setting.id.empty() || !ParseWidgetSettingType(typeStr, setting.type)) {
      JS_FreeValue(ctx, itemV);
      continue;
    }

    setting.label = Js::GetStringProp(ctx, itemV, "label");
    if (setting.label.empty())
      setting.label = setting.id;

    JSValue defV = JS_GetPropertyStr(ctx, itemV, "default");
    if (JS_IsString(defV)) {
      setting.defaultValue = Js::GetStringProp(ctx, itemV, "default");
    } else if (JS_IsBool(defV)) {
      setting.defaultValue = JS_ToBool(ctx, defV) > 0 ? L"true" : L"false";
    } else if (JS_IsNumber(defV)) {
      double d = 0.0;
      if (JS_ToFloat64(ctx, &d, defV) == 0)
        setting.defaultValue = WidgetSettingNumberToString(d);
    }
    JS_FreeValue(ctx, defV);

    if (setting.type == WidgetSettingType::Toggle &&
        setting.defaultValue.empty())
      setting.defaultValue = L"false";

    setting.hasMin = Js::GetFloatProp(ctx, itemV, "min", setting.minValue);
    setting.hasMax = Js::GetFloatProp(ctx, itemV, "max", setting.maxValue);

    if (setting.type == WidgetSettingType::Select) {
      JSValue optsV = JS_GetPropertyStr(ctx, itemV, "options");
      if (JS_IsArray(optsV)) {
        uint32_t optLen = 0;
        JSValue optLenV = JS_GetPropertyStr(ctx, optsV, "length");
        if (JS_ToUint32(ctx, &optLen, optLenV) == 0) {
          for (uint32_t j = 0; j < optLen; ++j) {
            JSValue optV = JS_GetPropertyUint32(ctx, optsV, j);
            const char *optStr = JS_ToCString(ctx, optV);
            if (optStr && *optStr)
              setting.options.push_back(Utils::ToWString(optStr));
            if (optStr)
              JS_FreeCString(ctx, optStr);
            JS_FreeValue(ctx, optV);
          }
        }
        JS_FreeValue(ctx, optLenV);
      }
      JS_FreeValue(ctx, optsV);
      if (setting.options.empty()) {
        JS_FreeValue(ctx, itemV);
        continue;
      }
    }

    JSValue bindV = JS_GetPropertyStr(ctx, itemV, "bind");
    if (JS_IsObject(bindV)) {
      setting.binding.elementId = Js::GetStringProp(ctx, bindV, "element");
      setting.binding.property  = Js::GetStringProp(ctx, bindV, "property");
      std::transform(setting.binding.property.begin(),
                     setting.binding.property.end(),
                     setting.binding.property.begin(), ::towlower);
    }
    JS_FreeValue(ctx, bindV);

    out.push_back(std::move(setting));
    parsedAny = true;
    JS_FreeValue(ctx, itemV);
  }

  return parsedAny;
}

// ── Kept for internal use (old flat-array path) ───────────────────────────────
bool ParseSettingsSchema(JSContext *ctx, JSValueConst arr,
                         std::vector<WidgetSetting> &out) {
  return ParseSettingsRows(ctx, arr, out);
}

// ── New entry point: accepts object config OR backward-compat plain array ──────
bool ParseSettingsConfig(JSContext *ctx, JSValueConst val,
                         WidgetSettingsCatalog &catalog) {
  catalog.tabs.clear();
  catalog.schema.clear();
  catalog.panelTitle.clear();
  catalog.showWindowTab = true;
  catalog.about = {};

  // ── Backward compat: plain array → single tab named "Settings" ────────────
  if (JS_IsArray(val)) {
    WidgetSettingsTab tab;
    tab.label = L"Settings";
    if (!ParseSettingsRows(ctx, val, tab.settings))
      return false;
    catalog.tabs.push_back(std::move(tab));
    for (const auto &s : catalog.tabs[0].settings)
      catalog.schema.push_back(s);
    catalog.hasSchema = !catalog.schema.empty();
    return catalog.hasSchema;
  }

  if (!JS_IsObject(val))
    return false;

  // ── panel-level options ────────────────────────────────────────────────────
  {
    std::wstring t = Js::GetStringProp(ctx, val, "title");
    if (!t.empty()) catalog.panelTitle = t;
  }
  {
    JSValue v = JS_GetPropertyStr(ctx, val, "showWindowTab");
    if (!JS_IsUndefined(v) && !JS_IsNull(v)) {
      int b = JS_ToBool(ctx, v);
      if (b >= 0) catalog.showWindowTab = (b != 0);
    }
    JS_FreeValue(ctx, v);
  }

  // ── about object ──────────────────────────────────────────────────────────
  {
    JSValue aboutV = JS_GetPropertyStr(ctx, val, "about");
    if (JS_IsObject(aboutV)) {
      std::wstring nm  = Js::GetStringProp(ctx, aboutV, "name");
      std::wstring ver = Js::GetStringProp(ctx, aboutV, "version");
      std::wstring dsc = Js::GetStringProp(ctx, aboutV, "description");
      if (!nm.empty())  { catalog.about.name    = nm;  catalog.about.hasName        = true; }
      if (!ver.empty()) { catalog.about.version = ver; catalog.about.hasVersion     = true; }
      if (!dsc.empty()) { catalog.about.description = dsc; catalog.about.hasDescription = true; }
    }
    JS_FreeValue(ctx, aboutV);
  }

  // ── tabs array ────────────────────────────────────────────────────────────
  JSValue tabsV = JS_GetPropertyStr(ctx, val, "tabs");
  if (JS_IsArray(tabsV)) {
    uint32_t tabLen = 0;
    JSValue tLenV = JS_GetPropertyStr(ctx, tabsV, "length");
    if (JS_ToUint32(ctx, &tabLen, tLenV) == 0) {
      for (uint32_t ti = 0; ti < tabLen; ++ti) {
        JSValue tabItemV = JS_GetPropertyUint32(ctx, tabsV, ti);
        if (!JS_IsObject(tabItemV)) { JS_FreeValue(ctx, tabItemV); continue; }

        WidgetSettingsTab tab;
        tab.label = Js::GetStringProp(ctx, tabItemV, "label");
        if (tab.label.empty()) tab.label = L"Tab";
        tab.icon  = Js::GetStringProp(ctx, tabItemV, "icon");

        JSValue rowsV = JS_GetPropertyStr(ctx, tabItemV, "settings");
        ParseSettingsRows(ctx, rowsV, tab.settings);
        JS_FreeValue(ctx, rowsV);

        // Merge rows into the flat schema
        for (const auto &s : tab.settings)
          catalog.schema.push_back(s);

        catalog.tabs.push_back(std::move(tab));
        JS_FreeValue(ctx, tabItemV);
      }
    }
    JS_FreeValue(ctx, tLenV);
  }
  JS_FreeValue(ctx, tabsV);

  catalog.hasSchema = !catalog.schema.empty() || !catalog.tabs.empty();
  return catalog.hasSchema || !catalog.panelTitle.empty()
                           || catalog.about.hasName
                           || !catalog.showWindowTab;
}

} // namespace PropertyParser

namespace novadesk::scripting::quickjs::parser {
void ParseWidgetWindowOptions(JSContext *ctx, JSValueConst options,
                              WidgetWindowOptions &out) {
  if (!JS_IsObject(options)) {
    return;
  }

  std::wstring id = PropertyParser::Js::GetStringProp(ctx, options, "id");
  if (!id.empty())
    out.id = id;

  int32_t v = 0;
  JSValue widthVal = JS_GetPropertyStr(ctx, options, "width");
  if (!JS_IsUndefined(widthVal) && !JS_IsNull(widthVal) &&
      JS_ToInt32(ctx, &v, widthVal) == 0) {
    out.width = static_cast<int>(v);
    out.hasWidth = true;
  }
  JS_FreeValue(ctx, widthVal);

  JSValue heightVal = JS_GetPropertyStr(ctx, options, "height");
  if (!JS_IsUndefined(heightVal) && !JS_IsNull(heightVal) &&
      JS_ToInt32(ctx, &v, heightVal) == 0) {
    out.height = static_cast<int>(v);
    out.hasHeight = true;
  }
  JS_FreeValue(ctx, heightVal);

  JSValue minWidthVal = JS_GetPropertyStr(ctx, options, "minWidth");
  if (!JS_IsUndefined(minWidthVal) && !JS_IsNull(minWidthVal) &&
      JS_ToInt32(ctx, &v, minWidthVal) == 0) {
    out.minWidth = static_cast<int>(v);
    out.hasMinWidth = true;
  }
  JS_FreeValue(ctx, minWidthVal);

  JSValue minHeightVal = JS_GetPropertyStr(ctx, options, "minHeight");
  if (!JS_IsUndefined(minHeightVal) && !JS_IsNull(minHeightVal) &&
      JS_ToInt32(ctx, &v, minHeightVal) == 0) {
    out.minHeight = static_cast<int>(v);
    out.hasMinHeight = true;
  }
  JS_FreeValue(ctx, minHeightVal);

  JSValue xVal = JS_GetPropertyStr(ctx, options, "x");
  if (!JS_IsUndefined(xVal) && JS_ToInt32(ctx, &v, xVal) == 0) {
    out.x = static_cast<int>(v);
    out.hasX = true;
  }
  JS_FreeValue(ctx, xVal);

  JSValue yVal = JS_GetPropertyStr(ctx, options, "y");
  if (!JS_IsUndefined(yVal) && JS_ToInt32(ctx, &v, yVal) == 0) {
    out.y = static_cast<int>(v);
    out.hasY = true;
  }
  JS_FreeValue(ctx, yVal);

  out.hasDraggable =
      PropertyParser::Js::GetBoolProp(ctx, options, "draggable", out.draggable);
  out.hasResizable =
      PropertyParser::Js::GetBoolProp(ctx, options, "resizable", out.resizable);
  out.hasClickThrough = PropertyParser::Js::GetBoolProp(
      ctx, options, "clickThrough", out.clickThrough);
  out.hasKeepOnScreen = PropertyParser::Js::GetBoolProp(
      ctx, options, "keepOnScreen", out.keepOnScreen);
  out.hasSnapEdges =
      PropertyParser::Js::GetBoolProp(ctx, options, "snapEdges", out.snapEdges);
  out.hasShowInToolbar = PropertyParser::Js::GetBoolProp(
      ctx, options, "showInToolbar", out.showInToolbar);

  JSValue toolbarIconVal = JS_GetPropertyStr(ctx, options, "toolbarIcon");
  if (!JS_IsUndefined(toolbarIconVal) && !JS_IsNull(toolbarIconVal)) {
    const char *s = JS_ToCString(ctx, toolbarIconVal);
    if (s) {
      out.toolbarIcon = Utils::ToWString(s);
      out.hasToolbarIcon = true;
      JS_FreeCString(ctx, s);
    }
  }
  JS_FreeValue(ctx, toolbarIconVal);

  JSValue toolbarTitleVal = JS_GetPropertyStr(ctx, options, "toolbarTitle");
  if (!JS_IsUndefined(toolbarTitleVal) && !JS_IsNull(toolbarTitleVal)) {
    const char *s = JS_ToCString(ctx, toolbarTitleVal);
    if (s) {
      out.toolbarTitle = Utils::ToWString(s);
      out.hasToolbarTitle = true;
      JS_FreeCString(ctx, s);
    }
  }
  JS_FreeValue(ctx, toolbarTitleVal);

  out.hasShow = PropertyParser::Js::GetBoolProp(ctx, options, "show", out.show);

  std::wstring bg =
      PropertyParser::Js::GetStringProp(ctx, options, "backgroundColor");
  if (!bg.empty()) {
    out.backgroundColor = bg;
    bool hasBg = false;
    PropertyParser::Js::ParseGradientOrColor(bg, out.color, out.bgAlpha,
                                             out.bgGradient, hasBg);
    out.hasBackgroundColor = true;
  }

  JSValue backgroundImageV = JS_GetPropertyStr(ctx, options, "backgroundImage");
  if (!JS_IsUndefined(backgroundImageV) && !JS_IsNull(backgroundImageV)) {
    const char *value = JS_ToCString(ctx, backgroundImageV);
    if (value) {
      out.backgroundImage = Utils::ToWString(value);
      out.hasBackgroundImage = true;
      JS_FreeCString(ctx, value);
    }
  }
  JS_FreeValue(ctx, backgroundImageV);

  JSValue backgroundImageFallbackV =
      JS_GetPropertyStr(ctx, options, "backgroundImageFallback");
  if (!JS_IsUndefined(backgroundImageFallbackV) &&
      !JS_IsNull(backgroundImageFallbackV)) {
    const char *value = JS_ToCString(ctx, backgroundImageFallbackV);
    if (value) {
      out.backgroundImageFallback = Utils::ToWString(value);
      out.hasBackgroundImageFallback = true;
      JS_FreeCString(ctx, value);
    }
  }
  JS_FreeValue(ctx, backgroundImageFallbackV);

  JSValue backgroundImageFallbackAspectV =
      JS_GetPropertyStr(ctx, options, "backgroundImageFallbackAspectRatio");
  if (JS_IsUndefined(backgroundImageFallbackAspectV) || JS_IsNull(backgroundImageFallbackAspectV)) {
    JS_FreeValue(ctx, backgroundImageFallbackAspectV);
    backgroundImageFallbackAspectV =
        JS_GetPropertyStr(ctx, options, "backgroundImageFallbackSize");
  }
  if (!JS_IsUndefined(backgroundImageFallbackAspectV) &&
      !JS_IsNull(backgroundImageFallbackAspectV)) {
    const char *value = JS_ToCString(ctx, backgroundImageFallbackAspectV);
    if (value) {
      std::wstring aspect = Utils::ToWString(value);
      std::transform(aspect.begin(), aspect.end(), aspect.begin(), ::towlower);
      if (aspect == L"preserve" || aspect == L"fit" || aspect == L"contain") {
        out.backgroundImageFallbackAspectRatio = IMAGE_ASPECT_PRESERVE;
        out.hasBackgroundImageFallbackAspectRatio = true;
      } else if (aspect == L"crop" || aspect == L"cover") {
        out.backgroundImageFallbackAspectRatio = IMAGE_ASPECT_CROP;
        out.hasBackgroundImageFallbackAspectRatio = true;
      } else if (aspect == L"stretch") {
        out.backgroundImageFallbackAspectRatio = IMAGE_ASPECT_STRETCH;
        out.hasBackgroundImageFallbackAspectRatio = true;
      }
      JS_FreeCString(ctx, value);
    }
  }
  JS_FreeValue(ctx, backgroundImageFallbackAspectV);

  JSValue backgroundImageSizeV =
      JS_GetPropertyStr(ctx, options, "backgroundImageSize");
  if (JS_IsString(backgroundImageSizeV)) {
    const char *value = JS_ToCString(ctx, backgroundImageSizeV);
    if (value) {
      std::wstring backgroundImageSize = Utils::ToWString(value);
      std::transform(backgroundImageSize.begin(), backgroundImageSize.end(),
                     backgroundImageSize.begin(), ::towlower);
      if (backgroundImageSize == L"cover" ||
          backgroundImageSize == L"contain" ||
          backgroundImageSize == L"stretch") {
        out.backgroundImageSize = backgroundImageSize;
        out.backgroundImageSizeIsExplicit = false;
        out.hasBackgroundImageSize = true;
      }
      JS_FreeCString(ctx, value);
    }
  } else if (JS_IsObject(backgroundImageSizeV)) {
    JSValue widthV = JS_GetPropertyStr(ctx, backgroundImageSizeV, "width");
    JSValue heightV = JS_GetPropertyStr(ctx, backgroundImageSizeV, "height");
    double width = 0.0, height = 0.0;
    const bool widthProvided = !JS_IsUndefined(widthV);
    const bool heightProvided = !JS_IsUndefined(heightV);
    const bool hasWidth = widthProvided && JS_IsNumber(widthV) &&
                          JS_ToFloat64(ctx, &width, widthV) == 0 &&
                          std::isfinite(width) && width > 0.0;
    const bool hasHeight = heightProvided && JS_IsNumber(heightV) &&
                           JS_ToFloat64(ctx, &height, heightV) == 0 &&
                           std::isfinite(height) && height > 0.0;
    JS_FreeValue(ctx, widthV);
    JS_FreeValue(ctx, heightV);
    if ((!widthProvided || hasWidth) && (!heightProvided || hasHeight) &&
        (hasWidth || hasHeight)) {
      out.backgroundImageSizeIsExplicit = true;
      out.backgroundImageSizeWidth = static_cast<float>(width);
      out.backgroundImageSizeHeight = static_cast<float>(height);
      out.backgroundImageSizeHasWidth = hasWidth;
      out.backgroundImageSizeHasHeight = hasHeight;
      out.hasBackgroundImageSize = true;
    }
  }
  JS_FreeValue(ctx, backgroundImageSizeV);
  JSValue backgroundImagePositionV =
      JS_GetPropertyStr(ctx, options, "backgroundImagePosition");
  if (JS_IsString(backgroundImagePositionV)) {
    const char *value = JS_ToCString(ctx, backgroundImagePositionV);
    if (value) {
      std::wstring backgroundImagePosition = Utils::ToWString(value);
      std::transform(backgroundImagePosition.begin(),
                     backgroundImagePosition.end(),
                     backgroundImagePosition.begin(), ::towlower);
      static const std::vector<std::wstring> positions = {
          L"top-left", L"top",         L"top-right", L"left",        L"center",
          L"right",    L"bottom-left", L"bottom",    L"bottom-right"};
      if (std::find(positions.begin(), positions.end(),
                    backgroundImagePosition) != positions.end()) {
        out.backgroundImagePosition = backgroundImagePosition;
        out.backgroundImagePositionIsExplicit = false;
        out.hasBackgroundImagePosition = true;
      }
      JS_FreeCString(ctx, value);
    }
  } else if (JS_IsObject(backgroundImagePositionV)) {
    JSValue xV = JS_GetPropertyStr(ctx, backgroundImagePositionV, "x");
    JSValue yV = JS_GetPropertyStr(ctx, backgroundImagePositionV, "y");
    double x = 0.0, y = 0.0;
    const bool validX =
        JS_IsNumber(xV) && JS_ToFloat64(ctx, &x, xV) == 0 && std::isfinite(x);
    const bool validY =
        JS_IsNumber(yV) && JS_ToFloat64(ctx, &y, yV) == 0 && std::isfinite(y);
    JS_FreeValue(ctx, xV);
    JS_FreeValue(ctx, yV);
    if (validX && validY) {
      out.backgroundImagePositionIsExplicit = true;
      out.backgroundImagePositionX = static_cast<float>(x);
      out.backgroundImagePositionY = static_cast<float>(y);
      out.hasBackgroundImagePosition = true;
    }
  }
  JS_FreeValue(ctx, backgroundImagePositionV);

  JSValue opacityVal = JS_GetPropertyStr(ctx, options, "opacity");
  if (!JS_IsUndefined(opacityVal) && !JS_IsNull(opacityVal)) {
    if (JS_IsString(opacityVal)) {
      const char *s = JS_ToCString(ctx, opacityVal);
      if (s) {
        std::wstring ws = Utils::ToWString(s);
        JS_FreeCString(ctx, s);
        ws.erase(std::remove_if(ws.begin(), ws.end(), iswspace), ws.end());
        if (!ws.empty() && ws.back() == L'%') {
          ws.pop_back();
          try {
            float pct = std::stof(ws);
            pct = std::max(0.0f, std::min(100.0f, pct));
            out.windowOpacity = static_cast<BYTE>((pct / 100.0f) * 255.0f);
            out.hasWindowOpacity = true;
          } catch (...) {
          }
        } else {
          try {
            float val = std::stof(ws);
            if (val <= 1.0f)
              val *= 255.0f;
            val = std::max(0.0f, std::min(255.0f, val));
            out.windowOpacity = static_cast<BYTE>(val);
            out.hasWindowOpacity = true;
          } catch (...) {
          }
        }
      }
    } else {
      double d = 1.0;
      if (JS_ToFloat64(ctx, &d, opacityVal) == 0) {
        if (d <= 1.0)
          d *= 255.0;
        d = std::max(0.0, std::min(255.0, d));
        out.windowOpacity = static_cast<BYTE>(d);
        out.hasWindowOpacity = true;
      }
    }
  }
  JS_FreeValue(ctx, opacityVal);

  std::wstring zPosStr =
      PropertyParser::Js::GetStringProp(ctx, options, "zPos");
  std::transform(zPosStr.begin(), zPosStr.end(), zPosStr.begin(), ::towlower);
  if (zPosStr == L"ondesktop") {
    out.zPos = -2;
    out.hasZPos = true;
  } else if (zPosStr == L"onbottom") {
    out.zPos = -1;
    out.hasZPos = true;
  } else if (zPosStr == L"normal") {
    out.zPos = 0;
    out.hasZPos = true;
  } else if (zPosStr == L"ontop") {
    out.zPos = 1;
    out.hasZPos = true;
  } else if (zPosStr == L"ontopmost") {
    out.zPos = 2;
    out.hasZPos = true;
  }

  std::wstring scriptPath =
      PropertyParser::Js::GetStringProp(ctx, options, "script");
  if (!scriptPath.empty()) {
    out.scriptPath = scriptPath;
    out.hasScriptPath = true;
  }

  JSValue settingsVal = JS_GetPropertyStr(ctx, options, "settings");
  if (JS_IsArray(settingsVal) &&
      PropertyParser::ParseSettingsSchema(ctx, settingsVal, out.settings)) {
    out.hasSettings = true;
  }
  JS_FreeValue(ctx, settingsVal);
}

void ParseWidgetWindowSize(JSContext *ctx, JSValueConst options, int &width,
                           int &height) {
  WidgetWindowOptions parsed;
  ParseWidgetWindowOptions(ctx, options, parsed);
  width = parsed.width;
  height = parsed.height;
}
} // namespace novadesk::scripting::quickjs::parser
