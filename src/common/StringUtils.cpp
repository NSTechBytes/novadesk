/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StringUtils.h"
#include <Windows.h>
#include <algorithm>
#include <cwctype>

namespace Utils {

std::wstring ToWString(const std::string &str) {
  if (str.empty())
    return std::wstring();

  // Determine required buffer size for wide character conversion
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

std::string ToString(const std::wstring &wstr) {
  if (wstr.empty())
    return std::string();

  // Determine required buffer size for UTF-8 conversion
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                        NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                      size_needed, NULL, NULL);
  return strTo;
}

std::wstring TrimUpper(const std::wstring &s) {
  // Trim leading whitespace
  size_t a = 0;
  while (a < s.size() && iswspace(s[a]))
    ++a;

  // Trim trailing whitespace
  size_t b = s.size();
  while (b > a && iswspace(s[b - 1]))
    --b;

  std::wstring out = s.substr(a, b - a);

  // Convert to uppercase
  for (auto &ch : out)
    ch = towupper(ch);
  return out;
}

bool TrySplitByComma(const std::wstring &s, std::vector<std::wstring> &parts) {
  parts.clear();
  int depth = 0;
  size_t last = 0;

  for (size_t i = 0; i < s.length(); i++) {
    if (s[i] == L'(') {
      depth++;
    } else if (s[i] == L')') {
      // Unbalanced parentheses detected
      if (depth == 0) {
        parts.clear();
        return false;
      }
      depth--;
    } else if (s[i] == L',' && depth == 0) {
      // Split at top-level comma
      parts.push_back(s.substr(last, i - last));
      last = i + 1;
    }
  }

  // Check for unbalanced parentheses
  if (depth != 0) {
    parts.clear();
    return false;
  }

  parts.push_back(s.substr(last));

  // Trim whitespace from each part
  for (auto &p : parts) {
    p.erase(0, p.find_first_not_of(L' '));
    p.erase(p.find_last_not_of(L' ') + 1);
  }
  return true;
}

std::vector<std::wstring> SplitByComma(const std::wstring &s) {
  std::vector<std::wstring> parts;
  TrySplitByComma(s, parts);
  return parts;
}

} // namespace Utils
