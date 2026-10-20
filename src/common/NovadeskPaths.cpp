/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "NovadeskPaths.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <shlobj.h>

#pragma comment(lib, "shell32.lib")

namespace fs = std::filesystem;

namespace NovadeskPaths {
namespace {

// ============================================================================
// Internal Helpers
// ============================================================================

/**
 * @brief Wraps SHGetKnownFolderPath with consistent CoTaskMemFree handling.
 *
 * @param knownFolderId Folder to resolve.
 *
 * @return Resolved path, or an empty path if the API fails.
 */
fs::path KnownFolder(const GUID &knownFolderId) {
  PWSTR path = nullptr;
  if (FAILED(SHGetKnownFolderPath(knownFolderId, 0, nullptr, &path)) || !path)
    return {};
  fs::path result(path);
  CoTaskMemFree(path);
  return result;
}

}  // anonymous namespace

// ============================================================================
// Public API Implementation
// ============================================================================

fs::path GetDocumentsRoot() {
  const fs::path documents = KnownFolder(FOLDERID_Documents);
  if (documents.empty())
    return {};
  return documents / L"Novadesk";
}

fs::path GetWidgetsDir() {
  const fs::path root = GetDocumentsRoot();
  if (root.empty())
    return {};
  return root / L"Widgets";
}

fs::path GetAddonsDir() {
  const fs::path root = GetDocumentsRoot();
  if (root.empty())
    return {};
  return root / L"Addons";
}

fs::path GetAppDataDir() {
  const fs::path roaming = KnownFolder(FOLDERID_RoamingAppData);
  if (roaming.empty())
    return {};
  return roaming / L"Novadesk";
}

fs::path GetKnownFolder(const GUID &knownFolderId) {
  return KnownFolder(knownFolderId);
}

fs::path GetProgramsDir() {
  return KnownFolder(FOLDERID_Programs);
}

fs::path GetDesktopDir() {
  return KnownFolder(FOLDERID_Desktop);
}

fs::path GetStartupDir() {
  return KnownFolder(FOLDERID_Startup);
}

}  // namespace NovadeskPaths
