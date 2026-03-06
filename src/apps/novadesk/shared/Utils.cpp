/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Utils.h"
#include <Windows.h>
#include "ColorUtil.h"
#include <algorithm>
#include <cwctype>
#include <shellapi.h>
#include <cstdio>

#pragma comment(lib, "version.lib")

namespace Utils
{

    namespace
    {
        bool SaveIconToIcoFile(HICON hIcon, FILE *fp)
        {
            ICONINFO iconInfo = {};
            BITMAP bmColor = {};
            BITMAP bmMask = {};
            if (!fp || !hIcon || !GetIconInfo(hIcon, &iconInfo) ||
                !GetObject(iconInfo.hbmColor, sizeof(bmColor), &bmColor) ||
                !GetObject(iconInfo.hbmMask, sizeof(bmMask), &bmMask))
            {
                if (iconInfo.hbmColor)
                    DeleteObject(iconInfo.hbmColor);
                if (iconInfo.hbmMask)
                    DeleteObject(iconInfo.hbmMask);
                return false;
            }

            // Keep plugin-safe ICO serialization path:
            // this writer only supports 16/32 bpp icon bitmaps.
            if (bmColor.bmBitsPixel != 16 && bmColor.bmBitsPixel != 32)
            {
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                return false;
            }

            HDC dc = GetDC(nullptr);
            if (!dc)
            {
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                return false;
            }

            BYTE bmiBytes[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)] = {};
            BITMAPINFO *bmi = (BITMAPINFO *)bmiBytes;

            memset(bmi, 0, sizeof(BITMAPINFO));
            bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
            int colorBytesCount = (int)bmi->bmiHeader.biSizeImage;
            if (colorBytesCount <= 0 || colorBytesCount > (64 * 1024 * 1024))
            {
                ReleaseDC(nullptr, dc);
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                return false;
            }
            BYTE *colorBits = new BYTE[colorBytesCount];
            if (!GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, colorBits, bmi, DIB_RGB_COLORS))
            {
                delete[] colorBits;
                ReleaseDC(nullptr, dc);
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                return false;
            }

            memset(bmi, 0, sizeof(BITMAPINFO));
            bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, nullptr, bmi, DIB_RGB_COLORS);
            int maskBytesCount = (int)bmi->bmiHeader.biSizeImage;
            if (maskBytesCount <= 0 || maskBytesCount > (64 * 1024 * 1024))
            {
                delete[] colorBits;
                ReleaseDC(nullptr, dc);
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                return false;
            }
            BYTE *maskBits = new BYTE[maskBytesCount];
            if (!GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, maskBits, bmi, DIB_RGB_COLORS))
            {
                delete[] colorBits;
                delete[] maskBits;
                ReleaseDC(nullptr, dc);
                DeleteObject(iconInfo.hbmColor);
                DeleteObject(iconInfo.hbmMask);
                return false;
            }
            ReleaseDC(nullptr, dc);

#pragma pack(push, 1)
            struct ICONDIRENTRY_LOCAL
            {
                BYTE bWidth;
                BYTE bHeight;
                BYTE bColorCount;
                BYTE bReserved;
                WORD wPlanes;
                WORD wBitCount;
                DWORD dwBytesInRes;
                DWORD dwImageOffset;
            };
            struct ICONDIR_LOCAL
            {
                WORD idReserved;
                WORD idType;
                WORD idCount;
                ICONDIRENTRY_LOCAL idEntries[1];
            };
#pragma pack(pop)

            BITMAPINFOHEADER bmihIcon = {};
            bmihIcon.biSize = sizeof(BITMAPINFOHEADER);
            bmihIcon.biWidth = bmColor.bmWidth;
            bmihIcon.biHeight = bmColor.bmHeight * 2;
            bmihIcon.biPlanes = bmColor.bmPlanes;
            bmihIcon.biBitCount = bmColor.bmBitsPixel;
            bmihIcon.biSizeImage = colorBytesCount + maskBytesCount;

            ICONDIR_LOCAL dir = {};
            dir.idReserved = 0;
            dir.idType = 1;
            dir.idCount = 1;
            dir.idEntries[0].bWidth = (BYTE)bmColor.bmWidth;
            dir.idEntries[0].bHeight = (BYTE)bmColor.bmHeight;
            dir.idEntries[0].bColorCount = 0;
            dir.idEntries[0].bReserved = 0;
            dir.idEntries[0].wPlanes = bmColor.bmPlanes;
            dir.idEntries[0].wBitCount = bmColor.bmBitsPixel;
            dir.idEntries[0].dwBytesInRes = sizeof(bmihIcon) + bmihIcon.biSizeImage;
            dir.idEntries[0].dwImageOffset = sizeof(ICONDIR_LOCAL);

            fwrite(&dir, 1, sizeof(dir), fp);
            fwrite(&bmihIcon, 1, sizeof(bmihIcon), fp);
            fwrite(colorBits, 1, colorBytesCount, fp);
            fwrite(maskBits, 1, maskBytesCount, fp);

            DeleteObject(iconInfo.hbmColor);
            DeleteObject(iconInfo.hbmMask);
            delete[] colorBits;
            delete[] maskBits;
            return true;
        }
    }

    bool ExtractFileIconToIco(const std::wstring &filePath, const std::wstring &outIcoPath, int size)
    {
        if (filePath.empty() || outIcoPath.empty())
            return false;
        if (size <= 0)
            size = 48;
        if (size > 256)
            size = 256;

        const int candidates[] = {size, 32, 48, 64};
        for (int s : candidates)
        {
            if (s <= 0 || s > 256)
                continue;

            HICON icon = nullptr;
            UINT extracted = PrivateExtractIconsW(
                filePath.c_str(),
                0,
                s,
                s,
                &icon,
                nullptr,
                1,
                LR_LOADTRANSPARENT);

            if (extracted == 0 || !icon)
            {
                SHFILEINFO shFileInfo = {};
                UINT flags = SHGFI_ICON;
                flags |= (s <= 16) ? SHGFI_SMALLICON : SHGFI_LARGEICON;
                if (!SHGetFileInfoW(filePath.c_str(), 0, &shFileInfo, sizeof(shFileInfo), flags))
                {
                    continue;
                }
                icon = shFileInfo.hIcon;
                if (!icon)
                    continue;
            }

            FILE *fp = nullptr;
            errno_t error = _wfopen_s(&fp, outIcoPath.c_str(), L"wb");
            bool ok = false;
            if (error == 0 && fp)
            {
                ok = SaveIconToIcoFile(icon, fp);
                fclose(fp);
            }
            DestroyIcon(icon);

            if (ok)
                return true;
        }

        FILE *clearFp = nullptr;
        if (_wfopen_s(&clearFp, outIcoPath.c_str(), L"wb") == 0 && clearFp)
        {
            fwrite(outIcoPath.c_str(), 1, 1, clearFp);
            fclose(clearFp);
        }
        return false;
    }

    /*
    ** Convert a UTF-8 encoded std::string to a wide character std::wstring.
    ** Returns an empty wstring if the input string is empty.
    ** Uses Windows MultiByteToWideChar for conversion.
    */

    std::wstring ToWString(const std::string &str)
    {
        if (str.empty())
            return std::wstring();
        int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
        std::wstring wstrTo(size_needed, 0);
        MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
        return wstrTo;
    }

    /*
    ** Convert a wide character std::wstring to a UTF-8 encoded std::string.
    ** Returns an empty string if the input wstring is empty.
    */

    std::string ToString(const std::wstring &wstr)
    {
        if (wstr.empty())
            return std::string();
        int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
        std::string strTo(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
        return strTo;
    }

    std::wstring TrimUpper(const std::wstring &s)
    {
        size_t a = 0;
        while (a < s.size() && iswspace(s[a]))
            ++a;
        size_t b = s.size();
        while (b > a && iswspace(s[b - 1]))
            --b;
        std::wstring out = s.substr(a, b - a);
        for (auto &ch : out)
            ch = towupper(ch);
        return out;
    }

    std::vector<std::wstring> SplitByComma(const std::wstring &s)
    {
        std::vector<std::wstring> parts;
        int depth = 0;
        size_t last = 0;
        for (size_t i = 0; i < s.length(); i++)
        {
            if (s[i] == L'(')
                depth++;
            else if (s[i] == L')')
                depth--;
            else if (s[i] == L',' && depth == 0)
            {
                parts.push_back(s.substr(last, i - last));
                last = i + 1;
            }
        }
        parts.push_back(s.substr(last));
        for (auto &p : parts)
        {
            p.erase(0, p.find_first_not_of(L' '));
            p.erase(p.find_last_not_of(L' ') + 1);
        }
        return parts;
    }

}
