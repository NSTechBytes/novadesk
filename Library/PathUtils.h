/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>

namespace PathUtils {
    /*
    ** Get the full path to the current executable.
    */
    std::wstring GetExePath();

    /*
    ** Get the directory containing the current executable (including trailing backslash).
    */
    std::wstring GetExeDir();

    /*
    ** Get the standard Widgets directory (including trailing backslash).
    */
    std::wstring GetWidgetsDir();

    /*
    ** Check if a path is relative.
    */
    bool IsPathRelative(const std::wstring& path);

    /*
    ** Resolve a path. If relative, it's resolved against baseDir.
    ** If baseDir is empty, it's resolved against the executable directory.
    */
    std::wstring ResolvePath(const std::wstring& path, const std::wstring& baseDir = L"");
}
