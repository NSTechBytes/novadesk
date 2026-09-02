/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>
#include <vector>

namespace PathUtils {

/**
 * @brief Decomposed components of a file system path.
 */
struct PathParts {
  std::wstring root; ///< Root/drive letter (e.g., "C:\").
  std::wstring dir;  ///< Directory path excluding filename.
  std::wstring base; ///< Filename without extension.
  std::wstring ext;  ///< File extension including the dot.
  std::wstring name; ///< Full filename (base + ext).
};

/**
 * @brief Gets the full path of the currently running executable.
 *
 * @return Absolute path to the .exe file.
 */
std::wstring GetExePath();

/**
 * @brief Gets the directory containing the currently running executable.
 *
 * @return Directory path with trailing backslash.
 */
std::wstring GetExeDir();

/**
 * @brief Gets the path to the widgets directory.
 *
 * @return Path to the widgets folder.
 */
std::wstring GetWidgetsDir();

/**
 * @brief Gets the path to the addons directory.
 *
 * @return Path to the addons folder.
 */
std::wstring GetAddonsDir();

/**
 * @brief Detects whether the application is running in portable mode.
 *
 * @return True if running from a portable (non-installed) location.
 */
bool IsPortableEnvironment();

/**
 * @brief Gets the application data path for Novadesk.
 *
 * @return Path to the AppData directory for Novadesk.
 */
std::wstring GetAppDataPath();

/**
 * @brief Gets the product name from version information.
 *
 * @return The product name string.
 */
std::wstring GetProductName();

/**
 * @brief Gets the parent directory of a given path.
 *
 * @param path The path to get the parent of.
 *
 * @return Parent directory path.
 */
std::wstring GetParentDir(const std::wstring &path);

/**
 * @brief Checks if a path is relative (not rooted).
 *
 * @param path The path to check.
 *
 * @return True if the path is relative.
 */
bool IsPathRelative(const std::wstring &path);

/**
 * @brief Joins path components with the platform separator.
 *
 * @param parts Vector of path components to join.
 *
 * @return Combined path string.
 */
std::wstring Join(const std::vector<std::wstring> &parts);

/**
 * @brief Extracts the filename from a path, optionally removing an extension.
 *
 * @param path The full path.
 * @param ext Optional extension to strip (e.g., ".cpp").
 *
 * @return The basename of the path.
 */
std::wstring Basename(const std::wstring &path, const std::wstring &ext = L"");

/**
 * @brief Gets the directory portion of a path.
 *
 * @param path The full path.
 *
 * @return Directory path without trailing separator.
 */
std::wstring Dirname(const std::wstring &path);

/**
 * @brief Gets the file extension including the dot.
 *
 * @param path The full path.
 *
 * @return Extension string (e.g., ".txt"), or empty if none.
 */
std::wstring Extname(const std::wstring &path);

/**
 * @brief Checks if a path is absolute.
 *
 * @param path The path to check.
 *
 * @return True if the path is absolute.
 */
bool IsAbsolute(const std::wstring &path);

/**
 * @brief Computes a relative path from one location to another.
 *
 * @param from Source directory path.
 * @param to Target path.
 *
 * @return Relative path string.
 */
std::wstring Relative(const std::wstring &from, const std::wstring &to);

/**
 * @brief Parses a path into its component parts.
 *
 * @param path The path to parse.
 *
 * @return Decomposed PathParts structure.
 */
PathParts Parse(const std::wstring &path);

/**
 * @brief Constructs a path from decomposed parts.
 *
 * @param parts The decomposed path components.
 *
 * @return Reconstructed path string.
 */
std::wstring Format(const PathParts &parts);

/**
 * @brief Normalizes a path by resolving redundant separators and "."
 * components.
 *
 * @param path The path to normalize.
 *
 * @return Normalized path string.
 */
std::wstring NormalizePath(const std::wstring &path);

/**
 * @brief Resolves a path against a base directory if it is relative.
 *
 * @param path The path to resolve.
 * @param baseDir Optional base directory for resolution.
 *
 * @return Absolute resolved path.
 */
std::wstring ResolvePath(const std::wstring &path,
                         const std::wstring &baseDir = L"");

/**
 * @brief Computes the base directory for a script file.
 *
 * @param scriptPath Path to the script file.
 * @param defaultBaseDir Default base directory if computation fails.
 *
 * @return Base directory for resolving relative imports.
 */
std::wstring GetScriptBaseDir(const std::wstring &scriptPath,
                              const std::wstring &defaultBaseDir);

/**
 * @brief Checks if a path is a URL (starts with http:// or https://).
 *
 * @param path The path to check.
 *
 * @return True if the path is a URL.
 */
bool IsURL(const std::wstring &path);

/**
 * @brief Gets the parent directory of a URL.
 *
 * @param url The URL to process.
 *
 * @return URL of the parent directory.
 */
std::wstring GetUrlParentDir(const std::wstring &url);

/**
 * @brief Resolves a relative URL against a base URL.
 *
 * @param path The path or URL to resolve.
 * @param baseUrl The base URL for resolution.
 *
 * @return Resolved absolute URL.
 */
std::wstring ResolveUrl(const std::wstring &path, const std::wstring &baseUrl);
} // namespace PathUtils
