/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include "quickjs.h"

/**
 * @brief System module for QuickJS JavaScript runtime.
 *
 * @note Provides system information, clipboard, audio, and web fetch
 *       operations from JavaScript.
 */
namespace novadesk::scripting::quickjs {

/**
 * @brief Ensures the system module is registered and returns it.
 *
 * @param ctx The QuickJS context.
 * @param moduleName Module name to register.
 *
 * @return The module definition, or nullptr on failure.
 */
JSModuleDef *EnsureSystemModule(JSContext *ctx, const char *moduleName);

/**
 * @brief Dispatches a web fetch result to the JavaScript engine.
 *
 * @param payload Pointer to WebFetchResultPayload (allocated with new).
 */
void DispatchWebFetchResult(void *payload);

/// Clears all pending web fetch requests.
void ClearWebFetchRequests(JSContext *ctx = nullptr);

/// Clears web fetch requests for a specific script.
void ClearWebFetchRequestsForScript(const std::wstring &scriptPath);

} // namespace novadesk::scripting::quickjs
