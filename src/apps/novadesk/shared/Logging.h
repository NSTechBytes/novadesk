/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_LOGGING_H__
#define __NOVADESK_LOGGING_H__

#include <windows.h>
#include <string>

/**
 * @brief Severity levels for log messages.
 */
enum class LogLevel { 
  Debug = 0, ///< Verbose debug output for development.
  Info = 1,  ///< General informational messages.
  Warn = 2,  ///< Warning messages for recoverable issues.
  Error = 3  ///< Error messages for failures requiring attention.
};

/**
 * @brief Provides file and console logging for Novadesk.
 *
 * @note Thread-safe for concurrent logging from multiple threads.
 */
class Logging {
public:
  /**
   * @brief Logs a formatted message at the specified level.
   *
   * @param level The severity level of the message.
   * @param format printf-style format string.
   * @param ... Variable arguments matching the format string.
   */
  static void Log(LogLevel level, const wchar_t *format, ...);

  /**
   * @brief Enables or disables console logging output.
   *
   * @param enable True to enable console output; false to disable.
   */
  static void SetConsoleLogging(bool enable);

  /**
   * @brief Enables file logging to the specified path.
   *
   * @param filePath Path to the log file.
   * @param clearFile If true, truncates existing log file on open.
   */
  static void SetFileLogging(const std::wstring &filePath,
                             bool clearFile = false);

  /**
   * @brief Sets the minimum log level to output.
   *
   * @param minLevel Messages below this level are discarded.
   */
  static void SetLogLevel(LogLevel minLevel);

private:
  static bool s_ConsoleEnabled;    ///< Whether console logging is active.
  static bool s_FileEnabled;       ///< Whether file logging is active.
  static std::wstring s_LogFilePath; ///< Path to the log file.
  static LogLevel s_MinLevel;      ///< Minimum severity level for output.
};

#endif
