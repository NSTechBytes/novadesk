/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "quickjs.h"

/**
 * @brief Filesystem module for QuickJS JavaScript runtime.
 *
 * @note Provides read/write file operations, directory listing, and
 *       path manipulation from JavaScript.
 */
namespace novadesk::scripting::quickjs {

/**
 * @brief Ensures the filesystem module is registered and returns it.
 *
 * @param ctx The QuickJS context.
 * @param moduleName Module name to register.
 *
 * @return The module definition, or nullptr on failure.
 */
JSModuleDef *EnsureFsModule(JSContext *ctx, const char *moduleName);

} // namespace novadesk::scripting::quickjs
