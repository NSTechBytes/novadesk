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
 * @brief Disk cache for images downloaded from HTTP/HTTPS URLs.
 *
 * @note Downloaded bytes are stored under <AppData>\cache\image-cache keyed by a
 *       hash of the URL, with a sidecar metadata file holding the original
 *       URL, fetch time and HTTP validators (ETag / Last-Modified). Entries
 *       are served from disk until the TTL expires, then revalidated with a
 *       conditional GET; on network failure the stale copy is served.
 *       Concurrent requests for the same URL share a single download.
 *       Performs file I/O only — safe to call from worker threads.
 */
namespace ImageCache {

/**
 * @brief Creates the cache directory and prunes stale/oversized entries.
 *
 * @note Call once during application startup. Failures degrade to
 *       "no caching", never a crash.
 */
void Startup();

/**
 * @brief Fails all in-flight and future downloads fast.
 *
 * @note Call before worker threads are joined during shutdown.
 */
void Shutdown();

/**
 * @brief Deletes all cached image files (entries and stray temp files).
 *
 * @return False only when called during shutdown; true otherwise.
 *
 * @note Safe to call at runtime from any thread. Images already loaded in
 *       memory keep rendering; the next load of a URL re-downloads and
 *       re-caches. A download in flight may re-create its own single
 *       entry after the sweep, which is the fresh result.
 */
bool Clear();

/**
 * @brief Returns the image bytes for a URL, using the disk cache when
 *        possible and downloading (once) otherwise.
 *
 * @param url The HTTP/HTTPS URL to fetch.
 * @param outBytes Receives the encoded image bytes on success.
 *
 * @return True if bytes were obtained (fresh, revalidated, or stale cache).
 *
 * @note Blocks the calling thread; intended for worker threads. If several
 *       threads request the same URL concurrently, only one network
 *       download runs and all callers receive its result.
 */
bool FetchBytes(const std::wstring &url, std::vector<BYTE> &outBytes);

} // namespace ImageCache
