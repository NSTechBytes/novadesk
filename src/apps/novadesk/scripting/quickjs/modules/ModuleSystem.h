/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "quickjs.h"

/**
 * @brief Module loader system for QuickJS JavaScript runtime.
 *
 * @note Handles ES module resolution, normalization, and loading with
 *       support for UI script import restrictions.
 */
namespace novadesk::scripting::quickjs {

/// Enables or disables debug logging for module operations.
void SetModuleSystemDebug(bool debug);

/// Enables or disables UI script import restrictions.
void SetUiScriptImportRestricted(bool restricted);

/**
 * @brief Normalizes a module name relative to the base module.
 *
 * @param ctx The QuickJS context.
 * @param baseName The base module name for resolution.
 * @param name The module name to normalize.
 * @param opaque Opaque pointer (unused).
 *
 * @return Normalized module name string (caller must free).
 */
char *ModuleNormalizeName(JSContext *ctx, const char *baseName,
                          const char *name, void *opaque);

/**
 * @brief Loads a module by name.
 *
 * @param ctx The QuickJS context.
 * @param moduleName The module name to load.
 * @param opaque Opaque pointer (unused).
 *
 * @return The loaded module definition, or nullptr if not found.
 */
JSModuleDef *ModuleLoader(JSContext *ctx, const char *moduleName, void *opaque);

} // namespace novadesk::scripting::quickjs
