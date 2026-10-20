/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "AppPaths.h"

#include <Windows.h>

namespace fs = std::filesystem;

namespace AppPaths {

fs::path GetExePath() {
  wchar_t buffer[MAX_PATH] = {};
  // NOTE: Truncation is theoretically possible for paths > MAX_PATH; all
  // Novadesk install locations are well under that limit.
  const DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
  if (len == 0 || len >= MAX_PATH)
    return {};
  return fs::path(buffer);
}

fs::path GetExeDir() {
  const fs::path exePath = GetExePath();
  if (exePath.empty())
    return {};
  return exePath.parent_path();
}

}  // namespace AppPaths
