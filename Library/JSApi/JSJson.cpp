/* Copyright (C) 2026 Novadesk Project
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSJson.h"
#include "../json/json.hpp"
#include "../PathUtils.h"
#include "../FileUtils.h"
#include "../Utils.h"
#include "../Logging.h"
#include <fstream>

using json = nlohmann::json;

namespace JSApi {

    // Helper to convert nlohmann::json to Duktape
    void PushJsonToDuk(duk_context* ctx, const json& j) {
        if (j.is_null()) {
            duk_push_null(ctx);
        }
        else if (j.is_boolean()) {
            duk_push_boolean(ctx, j.get<bool>());
        }
        else if (j.is_number()) {
            duk_push_number(ctx, j.get<double>());
        }
        else if (j.is_string()) {
            duk_push_string(ctx, j.get<std::string>().c_str());
        }
        else if (j.is_array()) {
            duk_idx_t arr_idx = duk_push_array(ctx);
            int i = 0;
            for (auto& element : j) {
                PushJsonToDuk(ctx, element);
                duk_put_prop_index(ctx, arr_idx, i++);
            }
        }
        else if (j.is_object()) {
            duk_idx_t obj_idx = duk_push_object(ctx);
            for (auto& el : j.items()) {
                PushJsonToDuk(ctx, el.value());
                duk_put_prop_string(ctx, obj_idx, el.key().c_str());
            }
        }
    }

    duk_ret_t js_read_json(duk_context* ctx) {
        const char* path = duk_get_string(ctx, 0);
        if (!path) return 0;

        std::wstring wpath = Utils::ToWString(path);
        if (PathUtils::IsPathRelative(wpath)) {
            wpath = PathUtils::ResolvePath(wpath, PathUtils::GetParentDir(s_CurrentScriptPath));
        }

        try {
            std::ifstream f(wpath);
            if (!f.is_open()) {
                Logging::Log(LogLevel::Error, L"Failed to open JSON file: %s", wpath.c_str());
                return 0;
            }

            json j;
            f >> j;
            
            PushJsonToDuk(ctx, j);
            return 1;
        }
        catch (json::parse_error& e) {
            Logging::Log(LogLevel::Error, L"JSON parse error in %s: %s", wpath.c_str(), Utils::ToWString(e.what()).c_str());
            return 0;
        }
        catch (...) {
            return 0;
        }
    }

    duk_ret_t js_write_json(duk_context* ctx) {
        const char* path = duk_get_string(ctx, 0);
        if (!path) return 0;

        // Ensure second argument is present
        if (duk_is_undefined(ctx, 1)) return 0;

        std::wstring wpath = Utils::ToWString(path);
        if (PathUtils::IsPathRelative(wpath)) {
            wpath = PathUtils::ResolvePath(wpath, PathUtils::GetParentDir(s_CurrentScriptPath));
        }

        // Encode to JSON string using Duktape
        duk_dup(ctx, 1);
        const char* jsonStr = duk_json_encode(ctx, -1);
        
        std::ofstream out(wpath);
        bool success = out.is_open();
        if (success) {
            out << jsonStr;
            out.close();
        }
        duk_pop(ctx); // Pop encoded string

        duk_push_boolean(ctx, success);
        return 1;
    }

    void BindJsonMethods(duk_context* ctx) {
        // "system" object is already on top of the stack
        
        duk_push_c_function(ctx, js_read_json, 1);
        duk_put_prop_string(ctx, -2, "readJson");
        
        duk_push_c_function(ctx, js_write_json, 2);
        duk_put_prop_string(ctx, -2, "writeJson");
    }
}
