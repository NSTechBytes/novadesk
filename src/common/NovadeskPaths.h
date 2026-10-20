/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <filesystem>
#include <string>
#include <guiddef.h>

/**
 * @brief Resolves the well-known folder locations used across the Novadesk app ecosystem.
 *
 * Centralizes the Documents\Novadesk, %APPDATA%\Novadesk, and shortcut-folder
 * lookups that were duplicated in nwm, manage_novadesk, ndpkg_installer, and
 * installer_stub.
 */
namespace NovadeskPaths {

/**
 * @brief Gets %USERPROFILE%\Documents\Novadesk.
 *
 * @return Path to the per-user Novadesk documents root, or an empty path on failure.
 */
std::filesystem::path GetDocumentsRoot();

/**
 * @brief Gets %USERPROFILE%\Documents\Novadesk\Widgets.
 *
 * Non-portable widget install location; portable installs live next to the exe.
 *
 * @return Path to the widgets folder, or an empty path on failure.
 */
std::filesystem::path GetWidgetsDir();

/**
 * @brief Gets %USERPROFILE%\Documents\Novadesk\Addons.
 *
 * @return Path to the addons folder, or an empty path on failure.
 */
std::filesystem::path GetAddonsDir();

/**
 * @brief Gets %APPDATA%\Novadesk.
 *
 * @return Path to the per-user app data folder, or an empty path on failure.
 */
std::filesystem::path GetAppDataDir();

/**
 * @brief Gets a shell-known folder (Start Menu Programs, Desktop, Startup...).
 *
 * @param knownFolderId One of FOLDERID_Programs, FOLDERID_Desktop, FOLDERID_Startup, etc.
 *
 * @return Path to the folder, or an empty path on failure.
 */
std::filesystem::path GetKnownFolder(const GUID &knownFolderId);

/**
 * @brief Gets %APPDATA%\Microsoft\Windows\Start Menu\Programs.
 *
 * @return Path to the Start Menu programs folder, or an empty path on failure.
 */
std::filesystem::path GetProgramsDir();

/**
 * @brief Gets the current user's Desktop folder.
 *
 * @return Path to the desktop, or an empty path on failure.
 */
std::filesystem::path GetDesktopDir();

/**
 * @brief Gets the current user's Startup folder.
 *
 * @return Path to the startup folder, or an empty path on failure.
 */
std::filesystem::path GetStartupDir();

}  // namespace NovadeskPaths
