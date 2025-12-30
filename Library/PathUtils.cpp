/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PathUtils.h"
#include <Windows.h>
#include <shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

namespace PathUtils {

    /*
    ** Get the full path to the current executable.
    */

    std::wstring GetExePath() {
        wchar_t path[MAX_PATH];
        GetModuleFileNameW(NULL, path, MAX_PATH);
        return std::wstring(path);
    }

    /*
    ** Get the directory of the current executable.
    */
    std::wstring GetExeDir() {
        std::wstring fullPath = GetExePath();
        size_t lastBackslash = fullPath.find_last_of(L"\\");
        if (lastBackslash != std::wstring::npos) {
            return fullPath.substr(0, lastBackslash + 1);
        }
        return L"";
    }

    /*
    ** Normalize a path by converting slashes and collapsing redundant ones.
    */
    std::wstring NormalizePath(const std::wstring& path) {
        if (path.empty()) return L"";
        std::wstring res = path;
        for (auto& c : res) if (c == L'/') c = L'\\';
        
        // Collapse redundant backslashes, but keep leading \\ for UNC
        size_t start = (res.length() >= 2 && res[0] == L'\\' && res[1] == L'\\') ? 2 : 1;
        for (size_t i = start; i < res.length(); ++i) {
            if (res[i] == L'\\' && res[i - 1] == L'\\') {
                res.erase(i, 1);
                --i;
            }
        }
        return res;
    }

    /*
    ** Get the directory containing the widgets.
    */
    std::wstring GetWidgetsDir() {
        return GetExeDir() + L"Widgets\\";
    }

    /*
    ** Get the parent directory of a path.
    */
    std::wstring GetParentDir(const std::wstring& path) {
        if (path.empty()) return L"";
        
        std::wstring norm = NormalizePath(path);
        
        // Remove trailing backslash if it's not the root
        while (norm.length() > 3 && norm.back() == L'\\') {
            norm.pop_back();
        }

        size_t lastBackslash = norm.find_last_of(L"\\");
        if (lastBackslash != std::wstring::npos) {
            return norm.substr(0, lastBackslash + 1);
        }
        return L"";
    }

    /*
    ** Check if a path is relative.
    */
    bool IsPathRelative(const std::wstring& path) {
        return PathIsRelativeW(path.c_str()) != FALSE;
    }

    /*
    ** Resolve a path to an absolute path.
    */
    std::wstring ResolvePath(const std::wstring& path, const std::wstring& baseDir) {
        if (path.empty()) return L"";
        
        std::wstring normPath = NormalizePath(path);

        if (!IsPathRelative(normPath)) {
            return normPath;
        }

        std::wstring finalBase = baseDir.empty() ? GetExeDir() : baseDir;
        std::wstring normBase = NormalizePath(finalBase);
        
        // Ensure normBase ends with a single backslash
        if (!normBase.empty() && normBase.back() != L'\\') {
            normBase += L'\\';
        }

        std::wstring combined = normBase + normPath;

        wchar_t fullPath[MAX_PATH];
        if (GetFullPathNameW(combined.c_str(), MAX_PATH, fullPath, NULL)) {
            return std::wstring(fullPath);
        }

        return combined;
    }
}
