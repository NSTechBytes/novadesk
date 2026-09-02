/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>
#include "quickjs.h"

/**
 * @brief Core Novadesk module for QuickJS JavaScript runtime.
 *
 * @note Provides application-level API, addon management, and command-line
 *       argument access from JavaScript.
 */
namespace novadesk::scripting::quickjs {

/**
 * @brief Sets the application command-line arguments.
 *
 * @param argv The raw command-line arguments.
 *
 * @note Must be called once at startup before the JS engine runs.
 */
void SetAppArgv(const std::vector<std::wstring> &argv);

/// Enables or disables debug logging for module operations.
void SetModuleDebug(bool debug);

/**
 * @brief Ensures the Novadesk module is registered and returns it.
 *
 * @param ctx The QuickJS context.
 * @param moduleName Module name to register.
 *
 * @return The module definition, or nullptr on failure.
 */
JSModuleDef *EnsureNovadeskModule(JSContext *ctx, const char *moduleName);

/// Unloads all registered addons.
void UnloadAllAddons();

} // namespace novadesk::scripting::quickjs
