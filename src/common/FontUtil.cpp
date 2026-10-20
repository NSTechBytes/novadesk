/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FontUtil.h"

#ifndef USER_DEFAULT_SCREEN_DPI
#define USER_DEFAULT_SCREEN_DPI 96
#endif

HFONT FontUtil::CreateSystemFont(UINT dpi, int weight, int sizeScalePercent) {
  NONCLIENTMETRICSW ncm{};
  ncm.cbSize = sizeof(ncm);
  if (!SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0))
    return nullptr;

  // Metrics are reported at the system DPI; rescale to the caller's DPI so
  // windows on secondary monitors (or after a DPI change) keep text at the
  // correct physical size.
  HDC dc = GetDC(nullptr);
  const int sysDpi = dc ? GetDeviceCaps(dc, LOGPIXELSX) : USER_DEFAULT_SCREEN_DPI;
  if (dc) ReleaseDC(nullptr, dc);
  if (sysDpi > 0 && (UINT)sysDpi != dpi)
    ncm.lfMenuFont.lfHeight = MulDiv(ncm.lfMenuFont.lfHeight, (int)dpi, sysDpi);

  if (weight >= 0)
    ncm.lfMenuFont.lfWeight = weight;
  if (sizeScalePercent != 100)
    ncm.lfMenuFont.lfHeight =
        MulDiv(ncm.lfMenuFont.lfHeight, sizeScalePercent, 100);

  return CreateFontIndirectW(&ncm.lfMenuFont);
}
