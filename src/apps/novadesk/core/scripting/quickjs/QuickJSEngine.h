/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>

namespace QuickJSEngine {

    struct Runtime {
        void* rt = nullptr;
        void* ctx = nullptr;
    };

    bool Initialize(Runtime& runtime);
    void Shutdown(Runtime& runtime);

    // Evaluates JavaScript source code in global context.
    // Returns true on success and false on exception.
    bool Eval(Runtime& runtime, const std::string& source, const std::string& filename = "<input>");

    // Returns and clears the last pending exception as a string.
    std::string GetAndClearLastError(Runtime& runtime);
}
