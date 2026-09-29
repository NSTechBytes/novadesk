/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ImageCache.h"

#include "Direct2DHelper.h"
#include "../shared/Logging.h"
#include "../shared/PathUtils.h"
#include "../shared/Utils.h"

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <unordered_map>

namespace fs = std::filesystem;

namespace ImageCache {
namespace {

// ---------------------------------------------------------------------------
// Policy
// ---------------------------------------------------------------------------
// Default freshness when the server sends no cache directives, and the cap
// applied to any server-provided max-age/Expires lifetime (RFC 9111 lets a
// server pin an entry for a year; we revalidate through the disk hit
// instead). Server directives below kTtlSeconds are honored as-is.
constexpr long long kTtlSeconds = 24 * 60 * 60;
constexpr long long kMaxCacheBytes = 256ll * 1024 * 1024;
constexpr int kTempFileMaxAgeSeconds = 3600;

// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------
std::mutex g_Mutex;
bool g_ShuttingDown = false;

struct InFlightState {
  bool done = false;
  bool ok = false;
  std::vector<BYTE> bytes; // copy delivered to waiters
  std::condition_variable cv;
};
std::unordered_map<std::wstring, std::shared_ptr<InFlightState>> g_InFlight;

struct Meta {
  std::wstring url;
  long long fetchedAt = 0;
  long long expiresAt = 0; // absolute epoch when the entry must revalidate
  bool mustRevalidate = false; // forbid serving stale when the network fails
  std::wstring etag;
  std::wstring lastModified;
};

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------
long long NowSeconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

std::wstring CacheDir() {
  return PathUtils::GetAppDataPath() + L"image-cache";
}

fs::path BinPath(const std::wstring &key) {
  return fs::path(CacheDir()) / (key + L".bin");
}

fs::path MetaPath(const std::wstring &key) {
  return fs::path(CacheDir()) / (key + L".meta");
}

// Write via temp + rename so readers never observe a partial file.
bool WriteFileAtomic(const fs::path &path, const std::vector<BYTE> &data) {
  const fs::path tmp = path.wstring() + L".tmp";
  {
    std::ofstream f(tmp, std::ios::binary | std::ios::trunc);
    if (!f)
      return false;
    if (!data.empty())
      f.write(reinterpret_cast<const char *>(data.data()),
              static_cast<std::streamsize>(data.size()));
    if (!f) {
      f.close();
      std::error_code ec;
      fs::remove(tmp, ec);
      return false;
    }
  }
  if (!MoveFileExW(tmp.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING)) {
    std::error_code ec;
    fs::remove(tmp, ec);
    return false;
  }
  return true;
}

std::wstring SerializeMeta(const Meta &meta) {
  std::wstring out;
  out += L"url=" + meta.url + L"\n";
  out += L"fetchedAt=" + std::to_wstring(meta.fetchedAt) + L"\n";
  out += L"expiresAt=" + std::to_wstring(meta.expiresAt) + L"\n";
  out += L"mustRevalidate=" + std::wstring(meta.mustRevalidate ? L"1" : L"0") +
         L"\n";
  out += L"etag=" + meta.etag + L"\n";
  out += L"lastModified=" + meta.lastModified + L"\n";
  return out;
}

bool ParseMeta(const std::string &text, Meta &meta) {
  std::istringstream stream(text);
  std::string line;
  bool hasUrl = false;
  bool hasExpiresAt = false;
  while (std::getline(stream, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();
    const size_t eq = line.find('=');
    if (eq == std::string::npos)
      continue;
    const std::string key = line.substr(0, eq);
    const std::string value = line.substr(eq + 1);
    if (key == "url") {
      meta.url = Utils::ToWString(value);
      hasUrl = true;
    } else if (key == "fetchedAt") {
      try {
        meta.fetchedAt = std::stoll(value);
      } catch (...) {
        return false;
      }
    } else if (key == "expiresAt") {
      try {
        meta.expiresAt = std::stoll(value);
        hasExpiresAt = true;
      } catch (...) {
        return false;
      }
    } else if (key == "mustRevalidate") {
      meta.mustRevalidate = (value == "1");
    } else if (key == "etag") {
      meta.etag = Utils::ToWString(value);
    } else if (key == "lastModified") {
      meta.lastModified = Utils::ToWString(value);
    }
  }
  if (!hasUrl)
    return false;
  // Entries written before server directives were tracked: fall back to
  // the old flat-TTL behavior instead of treating them as malformed.
  if (!hasExpiresAt)
    meta.expiresAt = meta.fetchedAt + kTtlSeconds;
  return true;
}

bool ReadFileBytes(const fs::path &path, std::vector<BYTE> &out) {
  std::ifstream f(path, std::ios::binary);
  if (!f)
    return false;
  out.assign(std::istreambuf_iterator<char>(f),
             std::istreambuf_iterator<char>());
  return !out.empty();
}

// Loads <hash>.meta + <hash>.bin; fails if either is missing, empty, or the
// stored URL does not match (guards against key collisions).
bool ReadEntry(const std::wstring &key, const std::wstring &url, Meta &meta,
               std::vector<BYTE> &bytes) {
  std::vector<BYTE> metaBytes;
  if (!ReadFileBytes(MetaPath(key), metaBytes))
    return false;
  if (!ParseMeta(std::string(metaBytes.begin(), metaBytes.end()), meta))
    return false;
  if (meta.url != url)
    return false;
  return ReadFileBytes(BinPath(key), bytes);
}

bool WriteEntry(const std::wstring &key, const Meta &meta,
                const std::vector<BYTE> &bytes) {
  const std::string metaUtf8 = Utils::ToString(SerializeMeta(meta));
  const std::vector<BYTE> metaData(metaUtf8.begin(), metaUtf8.end());
  // Write meta last: an entry without meta is ignored by readers.
  if (!WriteFileAtomic(BinPath(key), bytes))
    return false;
  return WriteFileAtomic(MetaPath(key), metaData);
}

void DeleteEntry(const std::wstring &key) {
  std::error_code ec;
  fs::remove(BinPath(key), ec);
  fs::remove(MetaPath(key), ec);
}

// Freshness lifetime for a stored entry, per RFC 9111 §4.2: explicit
// max-age (minus the upstream Age), else Expires relative to the server's
// Date (avoids clock skew), else heuristic (10% of the time since
// Last-Modified), else our default TTL. no-cache/max-age=0 make the entry
// immediately stale so every reuse revalidates. The result is capped by
// kTtlSeconds so a long server max-age still results in cheap 304 checks.
long long ComputeFreshLifetime(const Direct2D::ImageHttpMeta &resp) {
  long long lifetime = -1;

  const long long maxAge = Utils::ParseCacheControlMaxAge(resp.cacheControl);
  if (resp.noCache || maxAge == 0) {
    lifetime = 0;
  } else if (maxAge > 0) {
    long long age = 0;
    if (!resp.age.empty()) {
      const long long parsed = _wtoi64(resp.age.c_str());
      if (parsed > 0)
        age = parsed;
    }
    lifetime = maxAge - age;
  } else {
    const long long date = Utils::ParseHttpDate(resp.date);
    const long long expires = Utils::ParseHttpDate(resp.expires);
    const long long lastModified = Utils::ParseHttpDate(resp.lastModified);
    if (expires >= 0 && date >= 0) {
      lifetime = expires - date; // Expires present (no max-age)
    } else if (lastModified >= 0 && date > lastModified) {
      lifetime = (date - lastModified) / 10; // heuristic freshness
    } else {
      lifetime = kTtlSeconds; // no signal at all: our default
    }
  }

  if (lifetime < 0)
    lifetime = 0;
  if (lifetime > kTtlSeconds)
    lifetime = kTtlSeconds;
  return lifetime;
}

// ---------------------------------------------------------------------------
// Download (once per URL) with revalidation and stale fallback
// ---------------------------------------------------------------------------
bool DownloadAndCache(const std::wstring &url, const std::wstring &key,
                      std::vector<BYTE> &outBytes) {
  Meta stale;
  std::vector<BYTE> staleBytes;
  const bool haveStale = ReadEntry(key, url, stale, staleBytes);

  if (haveStale && NowSeconds() < stale.expiresAt) {
    outBytes = std::move(staleBytes);
    return true;
  }

  Logging::Log(LogLevel::Debug, L"[ImageCache] Fetching '%s'", url.c_str());

  Direct2D::ImageHttpMeta resp;
  std::vector<BYTE> bytes;
  const bool netOk = Direct2D::DownloadImageFromURL(
      url, bytes, &resp, haveStale ? stale.etag : std::wstring(),
      haveStale ? stale.lastModified : std::wstring());

  if (netOk && resp.notModified) {
    if (resp.noStore) {
      // The server now forbids storing this resource; drop the entry but
      // still serve the confirmed-unchanged bytes for this load.
      DeleteEntry(key);
      outBytes = std::move(staleBytes);
      return true;
    }
    Meta refreshed = stale;
    refreshed.fetchedAt = NowSeconds();
    refreshed.expiresAt = refreshed.fetchedAt + ComputeFreshLifetime(resp);
    refreshed.mustRevalidate = resp.mustRevalidate;
    if (!resp.etag.empty())
      refreshed.etag = resp.etag;
    if (!resp.lastModified.empty())
      refreshed.lastModified = resp.lastModified;
    WriteEntry(key, refreshed, staleBytes);
    Logging::Log(LogLevel::Debug,
                 L"[ImageCache] Revalidated (304), served cached bytes for '%s'",
                 url.c_str());
    outBytes = std::move(staleBytes);
    return true;
  }

  if (netOk && !bytes.empty()) {
    outBytes = std::move(bytes);
    if (resp.noStore) {
      // no-store anywhere in the redirect chain: never persist, and purge
      // any entry left by an older build so the next load hits the network.
      DeleteEntry(key);
      Logging::Log(LogLevel::Debug,
                   L"[ImageCache] no-store directive; served without caching "
                   L"for '%s'",
                   url.c_str());
      return true;
    }
    Meta fresh;
    fresh.url = url;
    fresh.fetchedAt = NowSeconds();
    fresh.expiresAt = fresh.fetchedAt + ComputeFreshLifetime(resp);
    fresh.mustRevalidate = resp.mustRevalidate;
    fresh.etag = resp.etag;
    fresh.lastModified = resp.lastModified;
    if (WriteEntry(key, fresh, outBytes))
      Logging::Log(LogLevel::Debug,
                   L"[ImageCache] Downloaded and cached %llu bytes for '%s'",
                   (unsigned long long)outBytes.size(), url.c_str());
    else
      Logging::Log(LogLevel::Debug,
                   L"[ImageCache] Downloaded %llu bytes but cache write "
                   L"failed for '%s'",
                   (unsigned long long)outBytes.size(), url.c_str());
    return true;
  }

  if (haveStale && !stale.mustRevalidate) {
    Logging::Log(LogLevel::Warn,
                 L"[ImageCache] Network failed, serving stale cache for '%s'",
                 url.c_str());
    outBytes = std::move(staleBytes);
    return true;
  }

  if (haveStale)
    Logging::Log(LogLevel::Warn,
                 L"[ImageCache] Network failed and must-revalidate forbids "
                 L"stale reuse for '%s'",
                 url.c_str());
  else
    Logging::Log(LogLevel::Warn, L"[ImageCache] Download failed for '%s'",
                 url.c_str());
  return false;
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void Startup() {
  std::error_code ec;
  const fs::path dir = CacheDir();
  fs::create_directories(dir, ec);
  if (ec) {
    Logging::Log(LogLevel::Warn, L"[ImageCache] Could not create cache dir: %s",
                 dir.c_str());
    return;
  }

  struct Entry {
    fs::path bin;
    fs::path meta;
    uintmax_t size = 0;
    fs::file_time_type mtime;
  };
  std::vector<Entry> entries;
  long long total = 0;
  const auto now = fs::file_time_type::clock::now();

  for (fs::directory_iterator it(dir, ec), end; !ec && it != end;
       it.increment(ec)) {
    const fs::path path = it->path();
    std::error_code fec;
    const auto ftime = fs::last_write_time(path, fec);
    if (fec)
      continue;
    const std::wstring name = path.filename().wstring();
    if (name.size() >= 4 && name.compare(name.size() - 4, 4, L".tmp") == 0) {
      // Reap temp files from crashed writes.
      if (std::chrono::duration_cast<std::chrono::seconds>(now - ftime)
              .count() > kTempFileMaxAgeSeconds) {
        fs::remove(path, ec);
      }
      continue;
    }
    if (name.size() >= 4 && name.compare(name.size() - 4, 4, L".bin") == 0) {
      Entry e;
      e.bin = path;
      e.meta = path.wstring().erase(path.wstring().size() - 4) + L".meta";
      e.size = (uintmax_t)fs::file_size(path, fec);
      if (fec)
        continue;
      e.mtime = ftime;
      total += (long long)e.size;
      entries.push_back(std::move(e));
    }
  }

  // Evict oldest entries first until under the size cap.
  if (total > kMaxCacheBytes) {
    std::sort(entries.begin(), entries.end(),
              [](const Entry &a, const Entry &b) { return a.mtime < b.mtime; });
    for (const Entry &e : entries) {
      if (total <= kMaxCacheBytes)
        break;
      std::error_code dec;
      fs::remove(e.bin, dec);
      fs::remove(e.meta, dec);
      total -= (long long)e.size;
    }
    Logging::Log(LogLevel::Info, L"[ImageCache] Evicted entries, %lld bytes "
                                 L"remain under cap",
                 total);
  }

  {
    std::lock_guard<std::mutex> lk(g_Mutex);
    g_ShuttingDown = false;
  }
  Logging::Log(LogLevel::Debug,
               L"[ImageCache] Startup: %d entries, %lld bytes in '%s'",
               (int)entries.size(), total, dir.c_str());
}

void Shutdown() {
  std::lock_guard<std::mutex> lk(g_Mutex);
  g_ShuttingDown = true;
  // Wake waiters so they fail fast; in-flight downloaders clean up their
  // own map entries when they finish.
  for (auto &pair : g_InFlight)
    pair.second->cv.notify_all();
}

bool Clear() {
  {
    std::lock_guard<std::mutex> lk(g_Mutex);
    if (g_ShuttingDown)
      return false;
  }

  // File I/O happens without holding g_Mutex: FetchBytes never touches it
  // during I/O and writes are atomic temp+rename, so a concurrent download
  // either lands before the sweep (removed) or after (one fresh entry).
  std::error_code ec;
  const fs::path dir = CacheDir();
  long long freed = 0;
  int removed = 0;
  for (fs::directory_iterator it(dir, ec), end; !ec && it != end;
       it.increment(ec)) {
    const std::wstring name = it->path().filename().wstring();
    const bool isEntry =
        (name.size() >= 4 &&
         (name.compare(name.size() - 4, 4, L".bin") == 0 ||
          name.compare(name.size() - 4, 4, L".tmp") == 0));
    const bool isMeta =
        (name.size() >= 5 && name.compare(name.size() - 5, 5, L".meta") == 0);
    if (!isEntry && !isMeta)
      continue;
    std::error_code fec;
    if (isEntry)
      freed += (long long)fs::file_size(it->path(), fec);
    fs::remove(it->path(), ec);
    if (!ec)
      ++removed;
  }

  Logging::Log(LogLevel::Info,
               L"[ImageCache] Cleared cache: %d files, %lld bytes freed",
               removed, freed);
  return true;
}

bool FetchBytes(const std::wstring &url, std::vector<BYTE> &outBytes) {
  outBytes.clear();
  if (url.empty())
    return false;

  const std::wstring key = PathUtils::GetUrlCacheKey(url);

  // Fresh disk hit needs no coordination or network. Entries with
  // no-cache/max-age=0 have expiresAt <= now and fall through to
  // revalidation below.
  {
    Meta meta;
    std::vector<BYTE> bytes;
    if (ReadEntry(key, url, meta, bytes) && NowSeconds() < meta.expiresAt) {
      outBytes = std::move(bytes);
      Logging::Log(LogLevel::Debug, L"[ImageCache] Cache hit for '%s'",
                   url.c_str());
      return true;
    }
  }

  std::unique_lock<std::mutex> lk(g_Mutex);
  if (g_ShuttingDown)
    return false;

  auto it = g_InFlight.find(key);
  if (it != g_InFlight.end()) {
    // Another thread is already fetching this URL; share its result.
    std::shared_ptr<InFlightState> state = it->second;
    Logging::Log(LogLevel::Debug,
                 L"[ImageCache] '%s' already downloading; waiting",
                 url.c_str());
    state->cv.wait(lk,
                   [&] { return state->done || g_ShuttingDown; });
    if (state->done && state->ok) {
      outBytes = state->bytes;
      return true;
    }
    return false;
  }

  auto state = std::make_shared<InFlightState>();
  g_InFlight.emplace(key, state);
  lk.unlock();

  const bool ok = DownloadAndCache(url, key, outBytes);

  lk.lock();
  state->bytes = outBytes;
  state->ok = ok;
  state->done = true;
  g_InFlight.erase(key);
  lk.unlock();
  state->cv.notify_all();
  return ok;
}

} // namespace ImageCache
