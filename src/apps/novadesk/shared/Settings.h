/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SETTINGS_H__
#define __NOVADESK_SETTINGS_H__

#include <string>
#include <map>
#include "../domain/Widget.h"
#include "../../../third_party/json/json.hpp"

using json = nlohmann::json;

/**
 * @brief Manages persistent application and widget settings.
 *
 * @note Settings are debounced and saved automatically. Use Flush() to
 *       force immediate write to disk.
 */
class Settings {
public:
  /**
   * @brief Initializes the settings system and loads saved data.
   *
   * @note Must be called once at application startup.
   */
  static void Initialize();

  /**
   * @brief Saves widget configuration to persistent storage.
   *
   * @param id The widget's unique identifier.
   * @param options The widget options to persist.
   */
  static void SaveWidget(const std::wstring &id, const WidgetOptions &options);

  /**
   * @brief Loads widget configuration from persistent storage.
   *
   * @param id The widget's unique identifier.
   * @param outOptions Receives the loaded widget options.
   *
   * @return True if the widget configuration was found and loaded.
   */
  static bool LoadWidget(const std::wstring &id, WidgetOptions &outOptions);

  /**
   * @brief Applies global settings to the application.
   *
   * @note Called during initialization and when settings change.
   */
  static void ApplyGlobalSettings();

  /**
   * @brief Saves all pending settings changes to disk.
   *
   * @note Debounced; multiple calls within a short window are coalesced.
   */
  static void Save();

  /**
   * @brief Forces immediate write of pending settings to disk.
   *
   * @note Bypasses debounce; call before application shutdown.
   */
  static void Flush();

  /**
   * @return The full path to the settings file.
   */
  static std::wstring GetSettingsPath();

  /**
   * @return The full path to the log file.
   */
  static std::wstring GetLogPath();

  /**
   * @return True if this is the first application run (no settings file found).
   */
  static bool IsFirstRun();

  /**
   * @brief Sets a boolean global setting by key.
   *
   * @param key The setting key (e.g., "show_sidebar").
   * @param value The boolean value to store.
   */
  static void SetGlobalBool(const std::string &key, bool value);

  /**
   * @brief Gets a boolean global setting by key.
   *
   * @param key The setting key.
   * @param defaultValue Value to return if the key doesn't exist.
   *
   * @return The stored value, or defaultValue if not found.
   */
  static bool GetGlobalBool(const std::string &key, bool defaultValue);

private:
  static void Load();          ///< Loads settings from disk.
  static json s_Data;          ///< In-memory settings data.
  static bool s_Dirty;         ///< True if unsaved changes exist.
  static bool s_IsFirstRun;    ///< True if no settings file was found on init.
  static DWORD s_LastSaveTick; ///< Tick count of last save (for debounce).
};

#endif
