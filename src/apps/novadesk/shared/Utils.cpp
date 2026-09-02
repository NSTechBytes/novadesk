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

namespace Utils {

// ============================================================================
// Internal Helpers
// ============================================================================

namespace {
/**
 * @brief Saves an HICON to an ICO file on disk.
 *
 * @param hIcon Handle to the icon to save.
 * @param fp Open file pointer for writing.
 *
 * @return True if the icon was written successfully.
 *
 * @note Only supports 16/32 bpp icon bitmaps for plugin-safe serialization.
 * @warning Caller must ensure fp is open in binary write mode.
 */
bool SaveIconToIcoFile(HICON hIcon, FILE *fp) {
  ICONINFO iconInfo = {};
  BITMAP bmColor = {};
  BITMAP bmMask = {};
  if (!fp || !hIcon || !GetIconInfo(hIcon, &iconInfo) ||
      !GetObject(iconInfo.hbmColor, sizeof(bmColor), &bmColor) ||
      !GetObject(iconInfo.hbmMask, sizeof(bmMask), &bmMask)) {
    if (iconInfo.hbmColor)
      DeleteObject(iconInfo.hbmColor);
    if (iconInfo.hbmMask)
      DeleteObject(iconInfo.hbmMask);
    return false;
  }

  // SAFETY: This writer only supports 16/32 bpp icon bitmaps for plugin
  // compatibility
  if (bmColor.bmBitsPixel != 16 && bmColor.bmBitsPixel != 32) {
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return false;
  }

  HDC dc = GetDC(nullptr);
  if (!dc) {
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return false;
  }

  // Extract color bits using GetDIBits
  BYTE bmiBytes[sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD)] = {};
  BITMAPINFO *bmi = (BITMAPINFO *)bmiBytes;

  memset(bmi, 0, sizeof(BITMAPINFO));
  bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, nullptr, bmi,
            DIB_RGB_COLORS);
  int colorBytesCount = (int)bmi->bmiHeader.biSizeImage;
  if (colorBytesCount <= 0 || colorBytesCount > (64 * 1024 * 1024)) {
    ReleaseDC(nullptr, dc);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return false;
  }
  BYTE *colorBits = new BYTE[colorBytesCount];
  if (!GetDIBits(dc, iconInfo.hbmColor, 0, bmColor.bmHeight, colorBits, bmi,
                 DIB_RGB_COLORS)) {
    delete[] colorBits;
    ReleaseDC(nullptr, dc);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return false;
  }

  // Extract mask bits
  memset(bmi, 0, sizeof(BITMAPINFO));
  bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, nullptr, bmi,
            DIB_RGB_COLORS);
  int maskBytesCount = (int)bmi->bmiHeader.biSizeImage;
  if (maskBytesCount <= 0 || maskBytesCount > (64 * 1024 * 1024)) {
    delete[] colorBits;
    ReleaseDC(nullptr, dc);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return false;
  }
  BYTE *maskBits = new BYTE[maskBytesCount];
  if (!GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.mHeight, maskBits, bmi,
                 DIB_RGB_COLORS)) {
    delete[] colorBits;
    delete[] maskBits;
    ReleaseDC(nullptr, dc);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    return false;
  }
  ReleaseDC(nullptr, dc);

#pragma pack(push, 1)
  struct ICONDIRENTRY_LOCAL {
    BYTE bWidth;
    BYTE bHeight;
    BYTE bColorCount;
    BYTE bReserved;
    WORD wPlanes;
    WORD wBitCount;
    DWORD dwBytesInRes;
    DWORD dwImageOffset;
  };
  struct ICONDIR_LOCAL {
    WORD idReserved;
    WORD idType;
    WORD idCount;
    ICONDIRENTRY_LOCAL idEntries[1];
  };
#pragma pack(pop)

  // Construct ICO file headers
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

  // Write ICO file structure: directory, header, color bits, mask bits
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
} // namespace

// ============================================================================
// Public API Implementation
// ============================================================================

bool ExtractFileIconToIco(const std::wstring &filePath,
                          const std::wstring &outIcoPath, int size) {
  if (filePath.empty() || outIcoPath.empty())
    return false;

  // Clamp icon size to valid range
  if (size <= 0)
    size = 48;
  if (size > 256)
    size = 256;

  // Try multiple sizes in priority order
  const int candidates[] = {size, 32, 48, 64};
  for (int s : candidates) {
    if (s <= 0 || s > 256)
      continue;

    HICON icon = nullptr;
    UINT extracted = PrivateExtractIconsW(filePath.c_str(), 0, s, s, &icon,
                                          nullptr, 1, LR_LOADTRANSPARENT);

    // Fallback to shell icon if PrivateExtractIcons fails
    if (extracted == 0 || !icon) {
      SHFILEINFO shFileInfo = {};
      UINT flags = SHGFI_ICON;
      flags |= (s <= 16) ? SHGFI_SMALLICON : SHGFI_LARGEICON;
      if (!SHGetFileInfoW(filePath.c_str(), 0, &shFileInfo, sizeof(shFileInfo),
                          flags)) {
        continue;
      }
      icon = shFileInfo.hIcon;
      if (!icon)
        continue;
    }

    FILE *fp = nullptr;
    errno_t error = _wfopen_s(&fp, outIcoPath.c_str(), L"wb");
    bool ok = false;
    if (error == 0 && fp) {
      ok = SaveIconToIcoFile(icon, fp);
      fclose(fp);
    }
    DestroyIcon(icon);

    if (ok)
      return true;
  }

  // Create empty file as fallback to prevent repeated extraction attempts
  FILE *clearFp = nullptr;
  if (_wfopen_s(&clearFp, outIcoPath.c_str(), L"wb") == 0 && clearFp) {
    fwrite(outIcoPath.c_str(), 1, 1, clearFp);
    fclose(clearFp);
  }
  return false;
}

std::wstring ToWString(const std::string &str) {
  if (str.empty())
    return std::wstring();

  // Determine required buffer size for wide character conversion
  int size_needed =
      MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
  std::wstring wstrTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0],
                      size_needed);
  return wstrTo;
}

std::string ToString(const std::wstring &wstr) {
  if (wstr.empty())
    return std::string();

  // Determine required buffer size for UTF-8 conversion
  int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(),
                                        NULL, 0, NULL, NULL);
  std::string strTo(size_needed, 0);
  WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0],
                      size_needed, NULL, NULL);
  return strTo;
}

std::wstring TrimUpper(const std::wstring &s) {
  // Trim leading whitespace
  size_t a = 0;
  while (a < s.size() && iswspace(s[a]))
    ++a;

  // Trim trailing whitespace
  size_t b = s.size();
  while (b > a && iswspace(s[b - 1]))
    --b;

  std::wstring out = s.substr(a, b - a);

  // Convert to uppercase
  for (auto &ch : out)
    ch = towupper(ch);
  return out;
}

bool TrySplitByComma(const std::wstring &s, std::vector<std::wstring> &parts) {
  parts.clear();
  int depth = 0;
  size_t last = 0;

  for (size_t i = 0; i < s.length(); i++) {
    if (s[i] == L'(') {
      depth++;
    } else if (s[i] == L')') {
      // Unbalanced parentheses detected
      if (depth == 0) {
        parts.clear();
        return false;
      }
      depth--;
    } else if (s[i] == L',' && depth == 0) {
      // Split at top-level comma
      parts.push_back(s.substr(last, i - last));
      last = i + 1;
    }
  }

  // Check for unbalanced parentheses
  if (depth != 0) {
    parts.clear();
    return false;
  }

  parts.push_back(s.substr(last));

  // Trim whitespace from each part
  for (auto &p : parts) {
    p.erase(0, p.find_first_not_of(L' '));
    p.erase(p.find_last_not_of(L' ') + 1);
  }
  return true;
}

std::vector<std::wstring> SplitByComma(const std::wstring &s) {
  std::vector<std::wstring> parts;
  TrySplitByComma(s, parts);
  return parts;
}

} // namespace Utils
