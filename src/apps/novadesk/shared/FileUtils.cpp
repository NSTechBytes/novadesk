/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FileUtils.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include "Logging.h"
#include "PathUtils.h"
#include "System.h"
#include "Utils.h"
#include <vector>

namespace FileUtils {

// ============================================================================
// File Reading Operations
// ============================================================================

/*
** Read the entire content of a file into a string.
** Supports UTF-16 file paths (std::wstring).
** Returns an empty string if the file cannot be opened.
*/
std::string ReadFileContent(const std::wstring &path) {
  std::ifstream file(std::filesystem::path(path), std::ios::binary);
  if (!file.is_open())
    return "";

  // Get file size
  file.seekg(0, std::ios::end);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  if (size <= 0)
    return "";

  std::vector<char> buffer((size_t)size);
  if (!file.read(buffer.data(), size))
    return "";

  // NOTE: Check for UTF-16 LE BOM (0xFF 0xFE). Windows scripts may be encoded
  // in UTF-16. If detected, decode the wide string and convert to UTF-8.
  if (size >= 2 && (unsigned char)buffer[0] == 0xFF &&
      (unsigned char)buffer[1] == 0xFE) {
    std::wstring wstr((wchar_t *)(buffer.data() + 2), (size - 2) / 2);
    return Utils::ToString(wstr);
  }

  // NOTE: Check for UTF-8 BOM (0xEF 0xBB 0xBF). Skip BOM bytes if present
  // to prevent BOM artifacts in parsed content (e.g., JSON parsing errors).
  if (size >= 3 && (unsigned char)buffer[0] == 0xEF &&
      (unsigned char)buffer[1] == 0xBB && (unsigned char)buffer[2] == 0xBF) {
    return std::string(buffer.data() + 3, (size_t)size - 3);
  }

  return std::string(buffer.data(), (size_t)size);
}

std::string ReadFileOrUrlContent(const std::wstring &pathOrUrl) {
  // NOTE: Distinguish between file paths and URLs. If URL detected, fetch via
  // WebFetch() which handles HTTPS/redirects. Otherwise, read from local disk.
  if (PathUtils::IsURL(pathOrUrl)) {
    std::string data;
    if (!novadesk::shared::system::WebFetch(pathOrUrl, data)) {
      Logging::Log(LogLevel::Error, L"Failed to fetch script from URL: %s",
                   pathOrUrl.c_str());
      return "";
    }
    return data;
  }

  return ReadFileContent(pathOrUrl);
}
} // namespace FileUtils
