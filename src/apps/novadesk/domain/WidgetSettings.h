/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_WIDGETSETTINGS_H__
#define __NOVADESK_WIDGETSETTINGS_H__

#include <windows.h>

#include <string>
#include <unordered_map>
#include <vector>

class Widget;

/**
 * @brief User-facing settings a widget script can declare for the built-in
 *        settings panel.
 *
 * @note Values are stored canonically as strings: colors as "#RRGGBB",
 *       booleans as "true"/"false", numbers via standard decimal text.
 */

/// @brief Kind of control shown for a setting in the settings panel.
enum class WidgetSettingType { Color, Number, Text, Toggle, Select };

/// @brief Binds a setting to an element property applied by the engine.
/// @note Supported properties: text elements - "text", "fontcolor",
///       "fontsize", "fontface"; image elements - "image"/"path"; any element
///       - "x", "y", "width", "height", "color", "cornerradius", "show";
///       toggleSwitch and checkBox elements - "checked"; slider elements -
///       "value" (Number settings drive the thumb directly).
struct WidgetSettingBinding {
  std::wstring elementId; ///< Target element ID.
  std::wstring property;  ///< Target property name.
};

/// @brief One declared setting.
struct WidgetSetting {
  std::wstring id;
  std::wstring label;
  WidgetSettingType type = WidgetSettingType::Text;
  std::wstring defaultValue; ///< Canonical string value.
  float minValue = 0.0f;
  bool hasMin = false;
  float maxValue = 0.0f;
  bool hasMax = false;
  std::vector<std::wstring> options; ///< Select choices.
  WidgetSettingBinding binding;
};

/// @brief Schema plus current values for one widget.
struct WidgetSettingsCatalog {
  std::vector<WidgetSetting> schema;
  std::unordered_map<std::wstring, std::wstring> values;
  bool hasSchema = false;

  /// @return The setting with this ID, or nullptr.
  const WidgetSetting *Find(const std::wstring &id) const;
  /// @return The stored value for a setting, or its schema default.
  std::wstring ValueOrDefault(const std::wstring &id) const;
};

/// @brief Parses a setting type name ("color", "number", ...), case-insensitive.
bool ParseWidgetSettingType(const std::wstring &name, WidgetSettingType &out);

/// @brief Converts a number to its canonical setting-value string.
std::wstring WidgetSettingNumberToString(double value);

/// @brief Converts a COLORREF to the canonical "#RRGGBB" setting value.
std::wstring WidgetSettingColorToHex(COLORREF color);

/// @brief Applies one setting value to its bound element property.
/// @return True if the element and property were found and updated.
bool ApplySettingToWidget(Widget *widget, const WidgetSetting &setting,
                          const std::wstring &value);

/// @brief Applies every declared setting (stored value or default) to the
///        widget in one batched redraw.
void ApplyAllSettings(Widget *widget);

/// @brief Normalizes, stores, applies and persists one setting value, then
///        fires the "settingchange" widget event.
/// @return True if the setting exists and was committed.
bool CommitWidgetSetting(Widget *widget, const std::wstring &settingId,
                         const std::wstring &rawValue);

#endif // __NOVADESK_WIDGETSETTINGS_H__
