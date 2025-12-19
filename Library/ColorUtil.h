/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_COLORUTIL_H__
#define __NOVADESK_COLORUTIL_H__

#include <windows.h>
#include <string>
#include <cstdio>

class ColorUtil
{
public:
    /*
    ** Parse an RGBA color string and extract color and alpha values.
    ** Supports formats: "rgba(r,g,b,a)" or "rgb(r,g,b)" with flexible whitespace.
    ** Values: r,g,b are 0-255, alpha is 0-255 (defaults to 255 for rgb format).
    ** Returns true on successful parse, false otherwise.
    */
    static bool ParseRGBA(const std::wstring& rgbaStr, COLORREF& color, BYTE& alpha)
    {
        // Handle rgba(r, g, b, alpha) or rgb(r, g, b) with flexible whitespace
        int r, g, b, a = 255;
        // swscanf_s with space in format string matches any amount of whitespace
        if (swscanf_s(rgbaStr.c_str(), L"rgba ( %d , %d , %d , %d )", &r, &g, &b, &a) == 4 ||
            swscanf_s(rgbaStr.c_str(), L"rgba(%d,%d,%d,%d)", &r, &g, &b, &a) == 4)
        {
            color = RGB(r, g, b);
            alpha = (BYTE)a;
            return true;
        }
        else if (swscanf_s(rgbaStr.c_str(), L"rgb ( %d , %d , %d )", &r, &g, &b) == 3 ||
                 swscanf_s(rgbaStr.c_str(), L"rgb(%d,%d,%d)", &r, &g, &b) == 3)
        {
            color = RGB(r, g, b);
            alpha = 255;
            return true;
        }
        return false;
    }
};

#endif
