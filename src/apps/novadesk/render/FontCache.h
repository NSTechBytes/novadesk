/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>
#include <windows.h>

/**
 * @brief Disk cache for fonts downloaded from HTTP/HTTPS URLs.
 *
 * @note Backed by the generic HttpDiskCache engine: raw response bytes
 *       (WOFF2 included) are stored under <AppData>\cache\font-cache keyed by a
 *       hash of the URL, revalidated per RFC 9111 server directives, and
 *       served stale on network failure unless must-revalidate applies.
 *       WOFF2-to-TTF conversion stays in FontDownloader. Performs file I/O
 *       only — safe to call from worker threads.
 */
namespace FontCache {

/// Creates the cache directory and prunes stale/oversized entries.
/// Call once during application startup; failures degrade to no caching.
void Startup();

/// Fails all in-flight and future downloads fast.
/// Must be called before FontDownloader joins its worker threads.
void Shutdown();

/// Deletes all cached font files (entries and stray temp files).
/// Returns false only when called during shutdown.
bool Clear();

/// Returns the font bytes for a URL, using the disk cache when possible
/// and downloading (once) otherwise. Blocks the calling thread.
bool FetchBytes(const std::wstring &url, std::vector<BYTE> &outBytes);

/// Removes the cached entry for a URL, e.g. after its bytes failed font
/// format validation, so the next fetch re-downloads cleanly.
void DeleteUrl(const std::wstring &url);

} // namespace FontCache
