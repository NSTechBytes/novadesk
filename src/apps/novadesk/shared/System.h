/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>
#include <cstddef>
#include <vector>
#include <string>
#include <cstdint>

namespace novadesk::shared::system {

/**
 * @brief Represents a rectangular region on a display monitor.
 */
struct DisplayRect {
  int left = 0;   ///< Left edge coordinate.
  int top = 0;    ///< Top edge coordinate.
  int right = 0;  ///< Right edge coordinate.
  int bottom = 0; ///< Bottom edge coordinate.
};

/**
 * @brief Information about a single display monitor.
 */
struct DisplayMonitorInfo {
  int id = 0;               ///< Monitor index.
  bool active = false;      ///< True if this is the primary/active monitor.
  std::wstring deviceName;  ///< Device name (e.g., "\\.\DISPLAY1").
  std::wstring monitorName; ///< Friendly monitor name from EDID.
  DisplayRect work;         ///< Work area (excluding taskbar).
  DisplayRect screen;       ///< Full screen bounds.
};

/**
 * @brief Aggregate display metrics for all connected monitors.
 */
struct DisplayMetrics {
  int virtualTop = 0;                       ///< Top of virtual screen.
  int virtualLeft = 0;                      ///< Left of virtual screen.
  int virtualHeight = 0;                    ///< Height of virtual screen.
  int virtualWidth = 0;                     ///< Width of virtual screen.
  int primaryIndex = 0;                     ///< Index of the primary monitor.
  std::vector<DisplayMonitorInfo> monitors; ///< List of connected monitors.
};

/**
 * @brief System power and battery status information.
 */
struct PowerStatus {
  int acline = 0;        ///< AC power status (0=battery, 1=plugged in).
  int status = 0;        ///< Battery charge status flags.
  int status2 = 0;       ///< Extended battery status.
  double lifetime = 0.0; ///< Estimated battery lifetime in seconds.
  int percent = 0;       ///< Battery charge percentage.
  double mhz = 0.0;      ///< Current CPU frequency in MHz.
  double hz = 0.0;       ///< Current CPU frequency in Hz.
};

/**
 * @brief CPU usage statistics.
 */
struct CpuStats {
  double usage = 0.0; ///< CPU usage percentage (0-100).
};

/**
 * @brief System memory statistics.
 */
struct MemoryStats {
  double total = 0.0;     ///< Total physical memory in bytes.
  double available = 0.0; ///< Available physical memory in bytes.
  double used = 0.0;      ///< Used physical memory in bytes.
  int percent = 0;        ///< Memory usage percentage.
};

/**
 * @brief Network traffic statistics.
 */
struct NetworkStats {
  double netIn = 0.0;    ///< Current download speed (bytes/sec).
  double netOut = 0.0;   ///< Current upload speed (bytes/sec).
  double totalIn = 0.0;  ///< Total bytes downloaded since boot.
  double totalOut = 0.0; ///< Total bytes uploaded since boot.
};

/**
 * @brief Disk space statistics for a volume.
 */
struct DiskStats {
  double total = 0.0;     ///< Total disk space in bytes.
  double available = 0.0; ///< Free disk space in bytes.
  double used = 0.0;      ///< Used disk space in bytes.
  int percent = 0;        ///< Disk usage percentage.
};

/**
 * @brief Disk I/O throughput statistics.
 */
struct DiskIoStats {
  double readSpeed = 0.0;  ///< Current read speed (bytes/sec).
  double writeSpeed = 0.0; ///< Current write speed (bytes/sec).
};

/**
 * @brief Recycle bin statistics.
 */
struct RecycleBinStats {
  double count = 0.0; ///< Number of items in recycle bin.
  double size = 0.0;  ///< Total size of items in bytes.
};

/**
 * @brief Registry value types.
 */
enum class RegistryValueType {
  None = 0, ///< No value or unknown type.
  String,   ///< String (REG_SZ) value.
  Number    ///< Numeric (REG_DWORD/QWORD) value.
};

/**
 * @brief A registry value with type information.
 */
struct RegistryValue {
  RegistryValueType type = RegistryValueType::None; ///< Value type.
  std::wstring stringValue; ///< String value (if type is String).
  double numberValue = 0.0; ///< Numeric value (if type is Number).
};

/**
 * @brief Copies text to the system clipboard.
 *
 * @param text The text to copy.
 *
 * @return True if successful.
 */
bool ClipboardSetText(const std::wstring &text);

/**
 * @brief Retrieves text from the system clipboard.
 *
 * @param outText Receives the clipboard text.
 *
 * @return True if clipboard contains text.
 */
bool ClipboardGetText(std::wstring &outText);

/**
 * @brief Sets the desktop wallpaper.
 *
 * @param imagePath Path to the image file.
 * @param style Wallpaper style ("fill", "fit", "stretch", "tile", "center").
 *
 * @return True if successful.
 */
bool SetWallpaper(const std::wstring &imagePath,
                  const std::wstring &style = L"fill");

/**
 * @brief Gets the path of the current desktop wallpaper.
 *
 * @param outPath Receives the wallpaper path.
 *
 * @return True if successful.
 */
bool GetCurrentWallpaperPath(std::wstring &outPath);

/// @return True if power status was retrieved successfully.
bool GetPowerStatus(PowerStatus &outStatus);

/// @return True if CPU stats were retrieved successfully.
bool GetCpuStats(CpuStats &outStats);

/// @return True if memory stats were retrieved successfully.
bool GetMemoryStats(MemoryStats &outStats);

/// @return True if network stats were retrieved successfully.
bool GetNetworkStats(NetworkStats &outStats);

/**
 * @brief Gets disk space statistics for a volume.
 *
 * @param path Path on the target volume (e.g., "C:\").
 * @param outStats Receives the disk statistics.
 *
 * @return True if successful.
 */
bool GetDiskStats(const std::wstring &path, DiskStats &outStats);

/// @return True if disk I/O stats were retrieved successfully.
bool GetDiskIoStats(DiskIoStats &outStats);

/// Shuts down disk I/O monitoring (frees resources).
void ShutdownDiskIoStats();

/// @return True if recycle bin stats were retrieved successfully.
bool GetRecycleBinStats(RecycleBinStats &outStats);

/// Opens the recycle bin folder.
bool OpenRecycleBin();

/**
 * @brief Empties the recycle bin.
 *
 * @param silent If true, suppresses confirmation dialog.
 *
 * @return True if successful.
 */
bool EmptyRecycleBin(bool silent);

/**
 * @brief Formats the current time as a string.
 *
 * @param format strftime-style format string.
 * @param localeName Locale name for formatting (e.g., "en-US").
 *
 * @return Formatted time string.
 */
std::string FormatCurrentTime(const std::string &format,
                              const std::string &localeName);

/// @return Current Unix timestamp in seconds.
double CurrentUnixTimestamp();

/**
 * @brief Formats a Unix timestamp as a string.
 *
 * @param timestamp Unix timestamp in seconds.
 * @param format strftime-style format string.
 * @param localeName Locale name for formatting.
 *
 * @return Formatted timestamp string.
 */
std::string FormatTimestamp(double timestamp, const std::string &format,
                            const std::string &localeName);

/**
 * @brief Parses a timestamp string into a Unix timestamp.
 *
 * @param text The timestamp string to parse.
 * @param format Expected format of the timestamp.
 * @param localeName Locale name for parsing.
 * @param outTimestamp Receives the parsed Unix timestamp.
 *
 * @return True if parsing succeeded.
 */
bool ParseTimestamp(const std::string &text, const std::string &format,
                    const std::string &localeName, double &outTimestamp);

/// @return True if daylight saving time is currently in effect.
bool IsDaylightSavingTimeNow();

/// @return Display metrics for all connected monitors.
DisplayMetrics GetDisplayMetrics();

/**
 * @brief Sets the system audio volume.
 *
 * @param volumePercent Volume level (0-100).
 *
 * @return True if successful.
 */
bool AudioSetVolume(int volumePercent);

/// @return Current system audio volume (0-100).
int AudioGetVolume();

/**
 * @brief Plays a sound file.
 *
 * @param path Path to the sound file.
 * @param loop If true, loops the sound continuously.
 *
 * @return True if playback started successfully.
 */
bool AudioPlaySound(const std::wstring &path, bool loop);

/// Stops any currently playing sound.
void AudioStopSound();

/**
 * @brief Reads a text file as a JSON string.
 *
 * @param path Path to the file.
 * @param outText Receives the file content.
 *
 * @return True if the file was read successfully.
 */
bool JsonReadTextFile(const std::wstring &path, std::string &outText);

/**
 * @brief Writes a string to a text file.
 *
 * @param path Path to the file.
 * @param text Content to write.
 *
 * @return True if the file was written successfully.
 */
bool JsonWriteTextFile(const std::wstring &path, const std::string &text);

/**
 * @brief Applies a JSON merge patch to a file.
 *
 * @param path Path to the JSON file.
 * @param patchText The merge patch to apply.
 *
 * @return True if the patch was applied successfully.
 */
bool JsonMergePatchFile(const std::wstring &path, const std::string &patchText);

/**
 * @brief Gets an environment variable by name.
 *
 * @param name The environment variable name.
 *
 * @return The variable value, or empty string if not found.
 */
std::wstring GetEnv(const std::wstring &name);

/// @return All environment variables as name-value pairs.
std::vector<std::pair<std::wstring, std::wstring>> GetAllEnv();

/**
 * @brief Executes a process.
 *
 * @param target Path to the executable.
 * @param parameters Command-line parameters.
 * @param workingDir Working directory for the process.
 * @param show Window show command (e.g., SW_SHOWNORMAL).
 *
 * @return True if the process was started successfully.
 */
bool Execute(const std::wstring &target, const std::wstring &parameters = L"",
             const std::wstring &workingDir = L"", int show = SW_SHOWNORMAL);

/// Maximum response size for WebFetch to prevent unbounded memory use.
constexpr size_t kWebFetchMaxResponseBytes = 64u * 1024u * 1024u;

/**
 * @brief Fetches content from a URL (HTTP/HTTPS/file).
 *
 * @param url The URL to fetch.
 * @param outData Receives the response body.
 *
 * @return True if the fetch succeeded.
 *
 * @warning Responses larger than kWebFetchMaxResponseBytes are rejected.
 */
bool WebFetch(const std::wstring &url, std::string &outData);

/**
 * @brief Reads a registry value.
 *
 * @param fullPath Full registry path (e.g., "HKLM\SOFTWARE\...").
 * @param valueName Name of the value to read.
 * @param outValue Receives the registry value.
 *
 * @return True if the value was read successfully.
 */
bool RegistryReadData(const std::wstring &fullPath,
                      const std::wstring &valueName, RegistryValue &outValue);

/**
 * @brief Writes a string value to the registry.
 *
 * @param fullPath Full registry path.
 * @param valueName Name of the value to write.
 * @param value String value to write.
 *
 * @return True if successful.
 */
bool RegistryWriteString(const std::wstring &fullPath,
                         const std::wstring &valueName,
                         const std::wstring &value);

/**
 * @brief Writes a numeric value to the registry.
 *
 * @param fullPath Full registry path.
 * @param valueName Name of the value to write.
 * @param value Numeric value to write.
 *
 * @return True if successful.
 */
bool RegistryWriteNumber(const std::wstring &fullPath,
                         const std::wstring &valueName, double value);

/**
 * @brief System uptime statistics.
 */
struct UptimeStats {
  double seconds = 0.0; ///< Total uptime in seconds.
  int days = 0;         ///< Uptime days.
  int hours = 0;        ///< Remaining hours.
  int minutes = 0;      ///< Remaining minutes.
  int secs = 0;         ///< Remaining seconds.
};

/**
 * @brief Gets the system uptime.
 *
 * @param outStats Receives the uptime statistics.
 *
 * @return True if successful.
 */
bool GetSystemUptime(UptimeStats &outStats);

/**
 * @brief Formats uptime as a string.
 *
 * @param stats The uptime statistics to format.
 * @param format Format string with placeholders (e.g., "{d}d {h}h {m}m").
 *
 * @return Formatted uptime string.
 */
std::string FormatUptime(const UptimeStats &stats, const std::string &format);

/**
 * @brief Configuration options for a message box dialog.
 */
struct MessageBoxOptions {
  std::wstring title;    ///< Dialog title.
  std::wstring message;  ///< Message content.
  std::wstring type;     ///< Icon type: "info", "warning", "error", "question".
  std::wstring buttons;  ///< Button layout: "ok", "ok-cancel", "yes-no", etc.
  HWND parent = nullptr; ///< Optional owner window for modal behavior.
};

/**
 * @brief Displays a message box dialog and returns the user's response.
 *
 * @param opts Configuration for the message box.
 *
 * @return The button label: "ok", "cancel", "yes", "no", "retry", "abort",
 * "ignore".
 */
std::string ShowMessageBox(const MessageBoxOptions &opts);

/**
 * @brief File type filter for open/save dialogs.
 */
struct FileFilter {
  std::wstring name; ///< Display name (e.g., "Images").
  std::vector<std::wstring>
      extensions; ///< Extensions (e.g., ["jpg", "png"] or ["*"]).
};

/**
 * @brief Configuration for an open file dialog.
 */
struct OpenFileDialogOptions {
  std::wstring title;              ///< Dialog title.
  std::wstring defaultPath;        ///< Initial directory.
  std::wstring buttonLabel;        ///< Custom button text.
  std::vector<FileFilter> filters; ///< File type filters.
  bool multiSelections = false;    ///< Allow selecting multiple files.
  bool openDirectory = false;      ///< Enable directory selection mode.
  bool showHiddenFiles = false;    ///< Show hidden files.
  HWND parent = nullptr;           ///< Optional owner window.
};

/**
 * @brief Result of an open file dialog.
 */
struct OpenFileDialogResult {
  bool canceled = true;                ///< True if user canceled.
  std::vector<std::wstring> filePaths; ///< Selected file paths.
};

/**
 * @brief Displays an open file dialog.
 *
 * @param opts Configuration for the dialog.
 *
 * @return The dialog result with selected files.
 */
OpenFileDialogResult ShowOpenFileDialog(const OpenFileDialogOptions &opts);

/**
 * @brief Configuration for a save file dialog.
 */
struct SaveFileDialogOptions {
  std::wstring title;              ///< Dialog title.
  std::wstring defaultPath;        ///< Initial directory/filename.
  std::wstring buttonLabel;        ///< Custom button text.
  std::wstring defaultExtension;   ///< Default file extension.
  std::vector<FileFilter> filters; ///< File type filters.
  bool showHiddenFiles = false;    ///< Show hidden files.
  HWND parent = nullptr;           ///< Optional owner window.
};

/**
 * @brief Result of a save file dialog.
 */
struct SaveFileDialogResult {
  bool canceled = true;  ///< True if user canceled.
  std::wstring filePath; ///< Selected save path.
};

/**
 * @brief Displays a save file dialog.
 *
 * @param opts Configuration for the dialog.
 *
 * @return The dialog result with the selected path.
 */
SaveFileDialogResult ShowSaveFileDialog(const SaveFileDialogOptions &opts);

} // namespace novadesk::shared::system
