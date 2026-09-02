/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <d2d1.h>
#include <vector>
#include "Element.h"

struct duk_hthread;
using duk_context = duk_hthread;

namespace Utils {

/**
 * @brief Converts a UTF-8 encoded string to a wide character string.
 *
 * @param str The UTF-8 input string.
 *
 * @return Wide character string, or empty wstring if input is empty.
 *
 * @note Uses Windows MultiByteToWideChar for conversion.
 */
std::wstring ToWString(const std::string &str);

/**
 * @brief Converts a wide character string to a UTF-8 encoded string.
 *
 * @param wstr The wide character input string.
 *
 * @return UTF-8 encoded string, or empty string if input is empty.
 */
std::string ToString(const std::wstring &wstr);

/**
 * @brief Trims whitespace and converts to uppercase.
 *
 * @param s The input string to trim and uppercase.
 *
 * @return Trimmed and uppercased string.
 */
std::wstring TrimUpper(const std::wstring &s);

/**
 * @brief Splits a comma-separated string, respecting nested parentheses.
 *
 * @param s The input string to split.
 * @param parts Vector to receive the split parts.
 *
 * @return True if parsing succeeded; false if parentheses are unbalanced.
 */
bool TrySplitByComma(const std::wstring &s, std::vector<std::wstring> &parts);

/**
 * @brief Splits a comma-separated string into parts.
 *
 * @param s The input string to split.
 *
 * @return Vector of trimmed parts. Returns empty vector on parse failure.
 */
std::vector<std::wstring> SplitByComma(const std::wstring &s);

/**
 * @brief Extracts a file's icon and saves it as an ICO file.
 *
 * @param filePath Path to the source file.
 * @param outIcoPath Output path for the .ico file.
 * @param size Desired icon size in pixels (clamped to 48; max 256).
 *
 * @return True if the icon was extracted and saved successfully.
 *
 * @note Falls back to system shell icons if PrivateExtractIcons fails.
 */
bool ExtractFileIconToIco(const std::wstring &filePath,
                          const std::wstring &outIcoPath, int size = 48);
} // namespace Utils
