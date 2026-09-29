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
#include <cstdlib>
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
  if (!GetDIBits(dc, iconInfo.hbmMask, 0, bmMask.bmHeight, maskBits, bmi,
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

long long ParseHttpDate(const std::wstring &value) {
  // IMF-fixdate: "Wdy, DD Mon YYYY HH:MM:SS GMT"
  int day = 0, year = 0, hour = 0, minute = 0, second = 0;
  wchar_t mon[4] = {};
  wchar_t zone[8] = {};
  const int parsed =
      swscanf_s(value.c_str(), L"%*3[^,], %d %3s %d %d:%d:%d %7s", &day, mon,
                (unsigned)_countof(mon), &year, &hour, &minute, &second, zone,
                (unsigned)_countof(zone));
  if (parsed != 7)
    return -1;

  static const wchar_t *kMonths[12] = {L"Jan", L"Feb", L"Mar", L"Apr",
                                       L"May", L"Jun", L"Jul", L"Aug",
                                       L"Sep", L"Oct", L"Nov", L"Dec"};
  int month = 0;
  for (int i = 0; i < 12; ++i) {
    if (_wcsnicmp(mon, kMonths[i], 3) == 0) {
      month = i + 1;
      break;
    }
  }
  if (month == 0 || year < 1970 || year > 2100 || day < 1 || day > 31 ||
      hour > 23 || minute > 59 || second > 60)
    return -1;

  SYSTEMTIME st = {};
  st.wYear = (WORD)year;
  st.wMonth = (WORD)month;
  st.wDay = (WORD)day;
  st.wHour = (WORD)hour;
  st.wMinute = (WORD)minute;
  st.wSecond = (WORD)second;

  FILETIME ft;
  if (!SystemTimeToFileTime(&st, &ft))
    return -1;
  ULARGE_INTEGER ull;
  ull.LowPart = ft.dwLowDateTime;
  ull.HighPart = ft.dwHighDateTime;
  // 116444736000000000 = 100ns ticks between 1601-01-01 and 1970-01-01.
  if (ull.QuadPart < 116444736000000000ull)
    return -1;
  return (long long)((ull.QuadPart - 116444736000000000ull) / 10000000ull);
}

bool HasCacheControlDirective(const std::wstring &cacheControl,
                              const std::wstring &name) {
  size_t pos = 0;
  while (pos <= cacheControl.size()) {
    size_t end = cacheControl.find(L',', pos);
    const bool last = (end == std::wstring::npos);
    if (last)
      end = cacheControl.size();

    size_t s = pos, e = end;
    while (s < e && iswspace(cacheControl[s]))
      ++s;
    while (e > s && iswspace(cacheControl[e - 1]))
      --e;
    const size_t eq = cacheControl.find(L'=', s);
    if (eq != std::wstring::npos && eq < e)
      e = eq; // strip the directive parameter before comparing tokens

    if (e - s == name.size() &&
        _wcsnicmp(cacheControl.c_str() + s, name.c_str(), name.size()) == 0)
      return true;

    if (last)
      break;
    pos = end + 1;
  }
  return false;
}

long long ParseCacheControlMaxAge(const std::wstring &cacheControl) {
  size_t pos = 0;
  static const std::wstring kMaxAge = L"max-age";
  while (pos <= cacheControl.size()) {
    size_t end = cacheControl.find(L',', pos);
    const bool last = (end == std::wstring::npos);
    if (last)
      end = cacheControl.size();

    size_t s = pos, e = end;
    while (s < e && iswspace(cacheControl[s]))
      ++s;
    while (e > s && iswspace(cacheControl[e - 1]))
      --e;
    const size_t eq = cacheControl.find(L'=', s);
    const bool hasParam = (eq != std::wstring::npos && eq < e);
    const size_t tokenEnd = hasParam ? eq : e;

    if (tokenEnd - s == kMaxAge.size() &&
        _wcsnicmp(cacheControl.c_str() + s, kMaxAge.c_str(), kMaxAge.size()) ==
            0 &&
        hasParam) {
      const wchar_t *digits = cacheControl.c_str() + eq + 1;
      wchar_t *stop = nullptr;
      const long long v = wcstoll(digits, &stop, 10);
      if (stop != digits && v >= 0)
        return v;
      return -1;
    }

    if (last)
      break;
    pos = end + 1;
  }
  return -1;
}

} // namespace Utils
