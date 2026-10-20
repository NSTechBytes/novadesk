/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <filesystem>

/**
 * @brief Resolves paths relative to the currently running executable.
 *
 * Replaces the GetModuleFileNameW boilerplate that was copy-pasted across
 * nwm, installer_stub, ndpkg_installer, and manage_novadesk.
 */
namespace AppPaths {

/**
 * @brief Gets the full path of the currently running executable.
 *
 * @return Absolute path to the .exe file, or an empty path on failure.
 */
std::filesystem::path GetExePath();

/**
 * @brief Gets the directory containing the currently running executable.
 *
 * @return Directory path without a trailing separator, or an empty path on failure.
 */
std::filesystem::path GetExeDir();

}  // namespace AppPaths
