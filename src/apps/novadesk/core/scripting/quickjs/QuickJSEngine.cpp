/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "QuickJSEngine.h"

extern "C" {
    int nd_qjs_initialize(void** outRt, void** outCtx);
    void nd_qjs_shutdown(void** rt, void** ctx);
    int nd_qjs_eval(void* ctx, const char* source, size_t len, const char* filename);
    char* nd_qjs_get_last_error(void* ctx);
    void nd_qjs_free_error(char* err);
}

namespace QuickJSEngine {

    bool Initialize(Runtime& runtime) {
        if (runtime.rt || runtime.ctx) {
            return false;
        }
        return nd_qjs_initialize(&runtime.rt, &runtime.ctx) != 0;
    }

    void Shutdown(Runtime& runtime) {
        nd_qjs_shutdown(&runtime.rt, &runtime.ctx);
    }

    bool Eval(Runtime& runtime, const std::string& source, const std::string& filename) {
        if (!runtime.ctx) {
            return false;
        }
        return nd_qjs_eval(runtime.ctx, source.c_str(), source.size(), filename.c_str()) != 0;
    }

    std::string GetAndClearLastError(Runtime& runtime) {
        if (!runtime.ctx) {
            return "QuickJS context is not initialized.";
        }
        char* err = nd_qjs_get_last_error(runtime.ctx);
        std::string out = err ? err : "Unknown QuickJS exception";
        nd_qjs_free_error(err);
        return out;
    }
}
