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
// String helpers (ToWString, ToString, TrimUpper, TrySplitByComma,
// SplitByComma) have been moved to src/common/StringUtils.h.
// This include keeps existing callers working without changes.
#include "../../../common/StringUtils.h"

namespace Utils {

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

/**
 * @brief Parses an HTTP date header (RFC 7231 IMF-fixdate).
 *
 * @param value The header value, e.g. "Tue, 29 Sep 2026 03:40:57 GMT".
 *
 * @return Seconds since the Unix epoch, or -1 if the value cannot be parsed.
 */
long long ParseHttpDate(const std::wstring &value);

/**
 * @brief Checks a Cache-Control header value for a directive.
 *
 * @param cacheControl The raw header value (comma-separated directives).
 * @param name The directive token to look for, e.g. L"no-store".
 *
 * @return True if the directive is present (case-insensitive, parameters
 *         like max-age=5 do not match a bare directive name).
 */
bool HasCacheControlDirective(const std::wstring &cacheControl,
                              const std::wstring &name);

/**
 * @brief Extracts the max-age=N value from a Cache-Control header.
 *
 * @param cacheControl The raw header value.
 *
 * @return The lifetime in seconds, or -1 when the directive is absent or
 *         malformed.
 */
long long ParseCacheControlMaxAge(const std::wstring &cacheControl);
} // namespace Utils
