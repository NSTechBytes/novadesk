/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "CursorManager.h"
#include "Element.h"
#include "PathUtils.h"

#include <algorithm>
#include <cwctype>
#include <filesystem>

CursorManager::~CursorManager()
{
    for (auto& kv : m_CustomCursorCache)
    {
        if (kv.second) {
            DestroyCursor(kv.second);
        }
    }
    m_CustomCursorCache.clear();
}

HCURSOR CursorManager::LoadCustomCursorFile(const std::wstring& fullPath)
{
    auto it = m_CustomCursorCache.find(fullPath);
    if (it != m_CustomCursorCache.end()) {
        return it->second;
    }

    HCURSOR cursor = reinterpret_cast<HCURSOR>(
        LoadImageW(nullptr, fullPath.c_str(), IMAGE_CURSOR, 0, 0, LR_LOADFROMFILE)
    );
    if (cursor) {
        m_CustomCursorCache[fullPath] = cursor;
    }
    return cursor;
}

HCURSOR CursorManager::GetCursorForElement(Element* element)
{
    if (!element || !element->GetMouseEventCursor()) {
        return nullptr;
    }

    std::wstring name = element->GetMouseEventCursorName();
    std::transform(name.begin(), name.end(), name.begin(), ::towlower);

    const struct CursorMapItem { const wchar_t* name; WORD id; } cursorMap[] = {
        { L"hand", 32649 },      // OCR_HAND
        { L"text", 32513 },      // OCR_IBEAM
        { L"help", 32651 },      // OCR_HELP
        { L"busy", 32650 },      // OCR_APPSTARTING
        { L"cross", 32515 },     // OCR_CROSS
        { L"pen", 32515 },       // OCR_CROSS
        { L"no", 32648 },        // OCR_NO
        { L"size_all", 32646 },  // OCR_SIZEALL
        { L"size_nesw", 32643 }, // OCR_SIZENESW
        { L"size_ns", 32645 },   // OCR_SIZENS
        { L"size_nwse", 32642 }, // OCR_SIZENWSE
        { L"size_we", 32644 },   // OCR_SIZEWE
        { L"uparrow", 32516 },   // OCR_UP
        { L"wait", 32514 }       // OCR_WAIT
    };

    for (const auto& item : cursorMap) {
        if (name == item.name) {
            return LoadCursorW(nullptr, MAKEINTRESOURCEW(item.id));
        }
    }

    std::wstring dir = element->GetCursorsDir();
    if (!dir.empty() && !name.empty()) {
        std::wstring candidate = PathUtils::NormalizePath(dir + L"\\" + name);
        if (std::filesystem::exists(candidate)) {
            if (HCURSOR c = LoadCustomCursorFile(candidate)) return c;
        }

        const std::wstring exts[] = { L".cur", L".ani" };
        for (const auto& ext : exts) {
            std::wstring withExt = candidate + ext;
            if (std::filesystem::exists(withExt)) {
                if (HCURSOR c = LoadCustomCursorFile(withExt)) return c;
            }
        }
    }

    return LoadCursorW(nullptr, MAKEINTRESOURCEW(32649)); // OCR_HAND
}
