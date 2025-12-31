/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSIPC.h"
#include "../Logging.h"
#include "../Utils.h"
#include <cstring>

namespace JSApi {

    // Helper to get the correct listeners object based on context.
    // We'll use "main_ipc_listeners" and "ui_ipc_listeners" in the global stash.
    static const char* GetCurrentContextListenersKey(duk_context* ctx) {
        // If we are currently executing a widget script, 'original_win' will be in the stash.
        duk_push_global_stash(ctx);
        if (duk_has_prop_string(ctx, -1, "original_win")) {
            duk_pop(ctx);
            return "ui_ipc_listeners";
        }
        duk_pop(ctx);

        // Also check if the global 'win' object is currently a widget.
        // This helps identify asynchronous callbacks if the engine leaves 'win' breadcrumbs.
        duk_get_global_string(ctx, "win");
        if (duk_is_object(ctx, -1) && duk_has_prop_string(ctx, -1, "\xFF" "widgetPtr")) {
            duk_pop(ctx);
            return "ui_ipc_listeners";
        }
        duk_pop(ctx);

        return "main_ipc_listeners";
    }

    static const char* GetTargetContextListenersKey(duk_context* ctx) {
        const char* current = GetCurrentContextListenersKey(ctx);
        return (strcmp(current, "ui_ipc_listeners") == 0) ? "main_ipc_listeners" : "ui_ipc_listeners";
    }

    duk_ret_t js_ipc_on(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_function(ctx, 1)) return DUK_RET_TYPE_ERROR;
        const char* channel = duk_get_string(ctx, 0);
        const char* key = GetCurrentContextListenersKey(ctx);

        duk_push_global_stash(ctx);
        
        // Ensure the base listeners object (e.g., ui_ipc_listeners) exists
        if (!duk_get_prop_string(ctx, -1, key)) {
            duk_pop(ctx);
            duk_push_object(ctx);
            duk_dup(ctx, -1);
            duk_put_prop_string(ctx, -3, key);
        }

        // Get or create the array for this specific channel
        if (!duk_get_prop_string(ctx, -1, channel)) {
            duk_pop(ctx);
            duk_push_array(ctx);
            duk_dup(ctx, -1);
            duk_put_prop_string(ctx, -3, channel);
        }
        
        // Add the callback to the array
        duk_dup(ctx, 1);
        duk_put_prop_index(ctx, -2, (duk_uarridx_t)duk_get_length(ctx, -2));
        
        duk_pop_3(ctx); // array, listeners object, stash
        return 0;
    }

    duk_ret_t js_ipc_send(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        const char* channel = duk_get_string(ctx, 0);
        const char* targetKey = GetTargetContextListenersKey(ctx);

        duk_push_global_stash(ctx);
        if (duk_get_prop_string(ctx, -1, targetKey)) {
            if (duk_get_prop_string(ctx, -1, channel)) {
                if (duk_is_array(ctx, -1)) {
                    // Broadcast to all listeners in the array
                    duk_size_t len = duk_get_length(ctx, -1); // Length of array
                    for (duk_size_t i = 0; i < len; i++) {
                        duk_get_prop_index(ctx, -1, (duk_uarridx_t)i);
                        if (duk_is_function(ctx, -1)) {
                            duk_dup(ctx, 1); // Pass data arg
                            if (duk_pcall(ctx, 1) != 0) {
                                Logging::Log(LogLevel::Error, L"IPC Broadcast Error (%S) index %d: %S", channel, (int)i, duk_safe_to_string(ctx, -1));
                            }
                        }
                        duk_pop(ctx); // result
                    }
                } else if (duk_is_function(ctx, -1)) {
                    // Fallback for single functions (migration safety)
                    duk_dup(ctx, 1);
                    if (duk_pcall(ctx, 1) != 0) {
                        Logging::Log(LogLevel::Error, L"IPC Single Error (%S): %S", channel, duk_safe_to_string(ctx, -1));
                    }
                }
            }
            duk_pop(ctx); // channel prop
        }
        duk_pop_2(ctx); // targetKey obj, stash
        return 0;
    }

    void BindIPCMethods(duk_context* ctx) {
        duk_push_object(ctx);
        duk_push_c_function(ctx, js_ipc_on, 2);
        duk_put_prop_string(ctx, -2, "on");
        duk_push_c_function(ctx, js_ipc_send, 2);
        duk_put_prop_string(ctx, -2, "send");
        duk_put_global_string(ctx, "ipc");
    }
}
