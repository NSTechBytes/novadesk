/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <string>

namespace Utils {
    /*
    ** Convert a UTF-8 encoded std::string to a wide character std::wstring.
    ** Returns an empty wstring if the input string is empty.
    ** Uses Windows MultiByteToWideChar for conversion.
    */
    std::wstring ToWString(const std::string& str);
}
