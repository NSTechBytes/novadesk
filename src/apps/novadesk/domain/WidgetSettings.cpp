/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WidgetSettings.h"

#include "Widget.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "../scripting/quickjs/parser/PropertyParser.h"
#include "../shared/ColorUtil.h"
#include "../shared/Settings.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cwchar>
#include <cwctype>
#include <sstream>

const WidgetSetting *WidgetSettingsCatalog::Find(const std::wstring &id) const {
  for (const WidgetSetting &setting : schema) {
    if (setting.id == id)
      return &setting;
  }
  return nullptr;
}

std::wstring WidgetSettingsCatalog::ValueOrDefault(
    const std::wstring &id) const {
  auto it = values.find(id);
  if (it != values.end())
    return it->second;
  const WidgetSetting *setting = Find(id);
  return setting ? setting->defaultValue : std::wstring();
}

bool ParseWidgetSettingType(const std::wstring &name, WidgetSettingType &out) {
  std::wstring lower = name;
  std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
  if (lower == L"color") {
    out = WidgetSettingType::Color;
    return true;
  }
  if (lower == L"number") {
    out = WidgetSettingType::Number;
    return true;
  }
  if (lower == L"text") {
    out = WidgetSettingType::Text;
    return true;
  }
  if (lower == L"toggle") {
    out = WidgetSettingType::Toggle;
    return true;
  }
  if (lower == L"select") {
    out = WidgetSettingType::Select;
    return true;
  }
  return false;
}

std::wstring WidgetSettingNumberToString(double value) {
  if (std::isfinite(value) && value == std::floor(value) &&
      std::fabs(value) < 1e15) {
    return std::to_wstring(static_cast<long long>(value));
  }
  std::wostringstream ss;
  ss << value;
  return ss.str();
}

std::wstring WidgetSettingColorToHex(COLORREF color) {
  wchar_t buf[16];
  swprintf(buf, 16, L"#%02X%02X%02X", GetRValue(color), GetGValue(color),
           GetBValue(color));
  return buf;
}

namespace {

bool ParseSettingNumber(const std::wstring &value, double &out) {
  if (value.empty())
    return false;
  wchar_t *end = nullptr;
  const double d = wcstod(value.c_str(), &end);
  if (end == value.c_str())
    return false;
  out = d;
  return true;
}

bool ParseSettingBool(const std::wstring &value) {
  return value == L"true" || value == L"1" || value == L"on" || value == L"yes";
}

double ClampToSetting(const WidgetSetting &setting, double value) {
  if (setting.hasMin && value < setting.minValue)
    value = setting.minValue;
  if (setting.hasMax && value > setting.maxValue)
    value = setting.maxValue;
  return value;
}

} // namespace

bool ApplySettingToWidget(Widget *widget, const WidgetSetting &setting,
                          const std::wstring &value) {
  if (!widget || !Widget::IsValid(widget))
    return false;
  if (setting.binding.elementId.empty() || setting.binding.property.empty())
    return false;

  Element *element = widget->FindElementById(setting.binding.elementId);
  if (!element)
    return false;

  const std::wstring &prop = setting.binding.property;

  if (element->GetType() == ELEMENT_TOGGLE_SWITCH && prop == L"checked") {
    static_cast<ToggleSwitchElement *>(element)->SetChecked(
        ParseSettingBool(value), /*animate=*/false);
    return true;
  }

  if (element->GetType() == ELEMENT_CHECK_BOX && prop == L"checked") {
    static_cast<CheckBoxElement *>(element)->SetChecked(
        ParseSettingBool(value), /*animate=*/false);
    return true;
  }

  if (element->GetType() == ELEMENT_SLIDER && prop == L"value") {
    double d = 0.0;
    if (!ParseSettingNumber(value, d))
      return false;
    d = ClampToSetting(setting, d);
    static_cast<SliderElement *>(element)->SetValue(d);
    return true;
  }

  if (element->GetType() == ELEMENT_TEXT) {
    TextElement *text = static_cast<TextElement *>(element);
    PropertyParser::TextOptions opts;
    PropertyParser::PreFillTextOptions(opts, text);
    if (prop == L"text") {
      opts.text = value;
    } else if (prop == L"fontcolor") {
      COLORREF color = 0;
      BYTE alpha = 255;
      if (!ColorUtil::ParseRGBA(value, color, alpha))
        return false;
      opts.fontColor = color;
    } else if (prop == L"fontsize") {
      double d = 0.0;
      if (!ParseSettingNumber(value, d))
        return false;
      d = ClampToSetting(setting, d);
      if (d < 1.0)
        return false;
      opts.fontSize = static_cast<int>(std::lround(d));
    } else if (prop == L"fontface") {
      if (value.empty())
        return false;
      opts.fontFace = value;
    } else {
      return false;
    }
    PropertyParser::ApplyTextOptions(text, opts);
    return true;
  }

  if ((prop == L"image" || prop == L"path") &&
      element->GetType() == ELEMENT_IMAGE) {
    ImageElement *image = static_cast<ImageElement *>(element);
    PropertyParser::ImageOptions opts;
    PropertyParser::PreFillImageOptions(opts, image);
    opts.path = value;
    PropertyParser::ApplyImageOptions(image, opts);
    return true;
  }

  PropertyParser::ElementOptions opts;
  PropertyParser::PreFillElementOptions(opts, element);
  if (prop == L"x" || prop == L"y") {
    double d = 0.0;
    if (!ParseSettingNumber(value, d))
      return false;
    if (prop == L"x")
      opts.x = static_cast<int>(std::lround(d));
    else
      opts.y = static_cast<int>(std::lround(d));
  } else if (prop == L"width" || prop == L"height") {
    double d = 0.0;
    if (!ParseSettingNumber(value, d))
      return false;
    d = ClampToSetting(setting, d);
    if (d < 0.0)
      d = 0.0;
    if (prop == L"width")
      opts.width = static_cast<int>(std::lround(d));
    else
      opts.height = static_cast<int>(std::lround(d));
  } else if (prop == L"color") {
    COLORREF color = 0;
    BYTE alpha = 255;
    if (!ColorUtil::ParseRGBA(value, color, alpha))
      return false;
    if (!opts.hasSolidColor)
      opts.solidAlpha = 255;
    opts.hasSolidColor = true;
    opts.solidColor = color;
  } else if (prop == L"cornerradius") {
    double d = 0.0;
    if (!ParseSettingNumber(value, d))
      return false;
    if (d < 0.0)
      d = 0.0;
    opts.solidColorRadius = static_cast<int>(std::lround(d));
  } else if (prop == L"show") {
    opts.show = ParseSettingBool(value);
  } else {
    return false;
  }

  PropertyParser::ApplyElementOptions(element, opts);
  return true;
}

void ApplyAllSettings(Widget *widget) {
  if (!widget || !Widget::IsValid(widget))
    return;
  WidgetSettingsCatalog &catalog = widget->GetSettings();
  if (!catalog.hasSchema)
    return;

  widget->BeginUpdate();
  for (const WidgetSetting &setting : catalog.schema) {
    const std::wstring value = catalog.ValueOrDefault(setting.id);
    if (value.empty())
      continue;
    ApplySettingToWidget(widget, setting, value);
  }
  widget->EndUpdate();
}

bool CommitWidgetSetting(Widget *widget, const std::wstring &settingId,
                         const std::wstring &rawValue) {
  if (!widget || !Widget::IsValid(widget))
    return false;

  WidgetSettingsCatalog &catalog = widget->GetSettings();
  const WidgetSetting *setting = catalog.Find(settingId);
  if (!setting)
    return false;

  std::wstring value = rawValue;
  double d = 0.0;
  if (setting->type == WidgetSettingType::Number &&
      ParseSettingNumber(value, d)) {
    d = ClampToSetting(*setting, d);
    value = WidgetSettingNumberToString(d);
  }

  catalog.values[setting->id] = value;
  ApplySettingToWidget(widget, *setting, value);
  widget->Redraw();
  Settings::SaveWidgetSettingValues(widget->GetOptions().id, catalog);
  JSEngine::TriggerWidgetSettingChange(widget, setting->id, value);
  return true;
}
