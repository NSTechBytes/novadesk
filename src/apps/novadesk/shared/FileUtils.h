/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>

namespace FileUtils {

/**
 * @brief Reads the entire content of a file as a string.
 *
 * @param path Path to the file to read.
 *
 * @return File content as a string; empty if file doesn't exist or read fails.
 */
std::string ReadFileContent(const std::wstring &path);

/**
 * @brief Reads content from a file path or URL.
 *
 * @param pathOrUrl Local file path or HTTP/HTTPS URL.
 *
 * @return Content as a string; empty if read fails.
 */
std::string ReadFileOrUrlContent(const std::wstring &pathOrUrl);
} // namespace FileUtils
