/* Copyright (C) 2026 OfficialNovadesk 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <d2d1.h>
#include <vector>
#include "Element.h"

struct duk_hthread;
using duk_context = duk_hthread;

namespace Utils {

    std::wstring ToWString(const std::string& str);
    std::string ToString(const std::wstring& wstr);
    std::wstring TrimUpper(const std::wstring& s);

    std::vector<std::wstring> SplitByComma(const std::wstring& s);
    bool ExtractFileIconToIco(const std::wstring& filePath, const std::wstring& outIcoPath, int size = 48);
}
