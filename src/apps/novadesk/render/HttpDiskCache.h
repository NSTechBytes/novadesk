/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <memory>
#include <string>
#include <vector>
#include <windows.h>

/**
 * @brief Generic HTTP-response disk cache, parameterized by directory,
 *        log prefix, freshness TTL and size cap.
 *
 * Downloaded bytes are stored under <AppData>\<dirName> keyed by a hash of
 * the URL, with a sidecar metadata file holding the original URL, fetch
 * time, freshness deadline and HTTP validators (ETag / Last-Modified).
 * Server Cache-Control directives are honored per RFC 9111 (no-store is
 * never persisted, no-cache/max-age=0 force revalidation, max-age sets the
 * freshness lifetime capped by the configured TTL). Entries are served from
 * disk until stale, then revalidated with a conditional GET; on network
 * failure the stale copy is served unless the entry was must-revalidate.
 * Concurrent requests for the same URL share a single download.
 *
 * Instances are safe for concurrent use and perform file I/O on the calling
 * thread, so FetchBytes is intended for worker threads.
 */
namespace HttpDiskCache {

/// Configuration for one cache instance.
struct Config {
  std::wstring dirName;   ///< Subfolder of the AppData "cache" folder, e.g. L"image-cache".
  std::wstring logPrefix; ///< Log tag, e.g. L"[ImageCache]".
  long long ttlSeconds = 24 * 60 * 60; ///< Default lifetime and max-age cap.
  long long maxCacheBytes = 256ll * 1024 * 1024; ///< Eviction budget.
};

/// A single independent disk cache (images, fonts, ...).
class Cache {
public:
  explicit Cache(const Config &cfg);
  ~Cache();
  Cache(const Cache &) = delete;
  Cache &operator=(const Cache &) = delete;

  /// Creates the cache directory and prunes stale/oversized entries.
  /// Call once during application startup; failures degrade to no caching.
  void Startup();

  /// Fails all in-flight and future downloads fast.
  /// Call before worker threads that use this cache are joined.
  void Shutdown();

  /// Deletes all cached files (entries and stray temp files).
  /// Returns false only when called during shutdown.
  bool Clear();

  /// Returns the bytes for a URL, using the disk cache when possible and
  /// downloading (once) otherwise. Blocks the calling thread.
  bool FetchBytes(const std::wstring &url, std::vector<BYTE> &outBytes);

  /// Removes the cached entry for a URL, if any. Used to purge entries
  /// whose bytes failed content validation after a disk hit.
  void DeleteUrl(const std::wstring &url);

private:
  struct Impl;
  std::unique_ptr<Impl> m_Impl;
};

} // namespace HttpDiskCache
