#ifndef __NOVADESK_COLORUTIL_H__
#define __NOVADESK_COLORUTIL_H__

#include <windows.h>
#include <string>
#include <cstdio>

class ColorUtil
{
public:
    static bool ParseRGBA(const std::wstring& rgbaStr, COLORREF& color, BYTE& alpha)
    {
        // Handle rgba(r,g,b,alpha) or rgb(r,g,b)
        int r, g, b, a = 255;
        if (swscanf_s(rgbaStr.c_str(), L"rgba(%d,%d,%d,%d)", &r, &g, &b, &a) == 4)
        {
            color = RGB(r, g, b);
            alpha = (BYTE)a;
            return true;
        }
        else if (swscanf_s(rgbaStr.c_str(), L"rgb(%d,%d,%d)", &r, &g, &b) == 3)
        {
            color = RGB(r, g, b);
            alpha = 255;
            return true;
        }
        return false;
    }
};

#endif
