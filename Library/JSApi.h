#pragma once
#include "duktape/duktape.h"

namespace JSApi {
    // Initialize JavaScript API and register all functions
    void InitializeJavaScriptAPI(duk_context* ctx);

    // Load and execute the main JavaScript file (index.js)
    bool LoadAndExecuteScript(duk_context* ctx);

    // Reload scripts (cleanup and reload)
    void ReloadScripts(duk_context* ctx);

    // JavaScript API functions
    duk_ret_t js_log(duk_context* ctx);
    duk_ret_t js_error(duk_context* ctx);
    duk_ret_t js_debug(duk_context* ctx);
    duk_ret_t js_create_widget_window(duk_context* ctx);
}
