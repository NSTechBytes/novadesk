/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FontCache.h"

#include "HttpDiskCache.h"

namespace FontCache {
namespace {

HttpDiskCache::Cache &Instance() {
  static HttpDiskCache::Cache cache(
      {L"font-cache", L"[FontCache]", 24 * 60 * 60, 64ll * 1024 * 1024});
  return cache;
}

} // anonymous namespace

void Startup() { Instance().Startup(); }

void Shutdown() { Instance().Shutdown(); }

bool Clear() { return Instance().Clear(); }

bool FetchBytes(const std::wstring &url, std::vector<BYTE> &outBytes) {
  return Instance().FetchBytes(url, outBytes);
}

void DeleteUrl(const std::wstring &url) { Instance().DeleteUrl(url); }

} // namespace FontCache
