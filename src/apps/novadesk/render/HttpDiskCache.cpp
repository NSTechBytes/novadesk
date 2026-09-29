/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "HttpDiskCache.h"

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

namespace HttpDiskCache {
namespace {

constexpr int kTempFileMaxAgeSeconds = 3600;

long long NowSeconds() {
  return std::chrono::duration_cast<std::chrono::seconds>(
             std::chrono::system_clock::now().time_since_epoch())
      .count();
}

} // anonymous namespace

struct Cache::Impl {
  explicit Impl(const Config &c) : cfg(c) {}

  // -------------------------------------------------------------------------
  // Configuration and state
  // -------------------------------------------------------------------------
  Config cfg;
  std::mutex mutex;
  bool shuttingDown = false;

  struct InFlightState {
    bool done = false;
    bool ok = false;
    std::vector<BYTE> bytes; // copy delivered to waiters
    std::condition_variable cv;
  };
  std::unordered_map<std::wstring, std::shared_ptr<InFlightState>> inFlight;

  struct Meta {
    std::wstring url;
    long long fetchedAt = 0;
    long long expiresAt = 0; // absolute epoch when the entry must revalidate
    bool mustRevalidate = false; // forbid serving stale when the network fails
    std::wstring etag;
    std::wstring lastModified;
  };

  // -------------------------------------------------------------------------
  // Small helpers
  // -------------------------------------------------------------------------
  template <typename... Args>
  void Logf(LogLevel level, const wchar_t *fmt, Args... args) {
    // The prefix is folded into the format string so printf semantics and
    // the resulting log text stay identical to the hard-coded form.
    Logging::Log(level, (cfg.logPrefix + L" " + fmt).c_str(), args...);
  }

  std::wstring CacheDir() const {
    // All disk caches live under one "cache" folder in AppData, each in
    // its own subfolder, e.g. <AppData>\<Product>\cache\image-cache.
    return PathUtils::GetAppDataPath() + L"cache\\" + cfg.dirName;
  }

  fs::path BinPath(const std::wstring &key) const {
    return fs::path(CacheDir()) / (key + L".bin");
  }

  fs::path MetaPath(const std::wstring &key) const {
    return fs::path(CacheDir()) / (key + L".meta");
  }

  // Write via temp + rename so readers never observe a partial file.
  static bool WriteFileAtomic(const fs::path &path,
                              const std::vector<BYTE> &data) {
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

  static std::wstring SerializeMeta(const Meta &meta) {
    std::wstring out;
    out += L"url=" + meta.url + L"\n";
    out += L"fetchedAt=" + std::to_wstring(meta.fetchedAt) + L"\n";
    out += L"expiresAt=" + std::to_wstring(meta.expiresAt) + L"\n";
    out +=
        L"mustRevalidate=" + std::wstring(meta.mustRevalidate ? L"1" : L"0") +
        L"\n";
    out += L"etag=" + meta.etag + L"\n";
    out += L"lastModified=" + meta.lastModified + L"\n";
    return out;
  }

  bool ParseMeta(const std::string &text, Meta &meta) const {
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
      meta.expiresAt = meta.fetchedAt + cfg.ttlSeconds;
    return true;
  }

  static bool ReadFileBytes(const fs::path &path, std::vector<BYTE> &out) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
      return false;
    out.assign(std::istreambuf_iterator<char>(f),
               std::istreambuf_iterator<char>());
    return !out.empty();
  }

  // Loads <hash>.meta + <hash>.bin; fails if either is missing, empty, or
  // the stored URL does not match (guards against key collisions).
  bool ReadEntry(const std::wstring &key, const std::wstring &url, Meta &meta,
                 std::vector<BYTE> &bytes) const {
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
                  const std::vector<BYTE> &bytes) const {
    const std::string metaUtf8 = Utils::ToString(SerializeMeta(meta));
    const std::vector<BYTE> metaData(metaUtf8.begin(), metaUtf8.end());
    // Write meta last: an entry without meta is ignored by readers.
    if (!WriteFileAtomic(BinPath(key), bytes))
      return false;
    return WriteFileAtomic(MetaPath(key), metaData);
  }

  void DeleteEntry(const std::wstring &key) const {
    std::error_code ec;
    fs::remove(BinPath(key), ec);
    fs::remove(MetaPath(key), ec);
  }

  // Freshness lifetime for a stored entry, per RFC 9111 §4.2: explicit
  // max-age (minus the upstream Age), else Expires relative to the server's
  // Date (avoids clock skew), else heuristic (10% of the time since
  // Last-Modified), else the configured default TTL. no-cache/max-age=0
  // make the entry immediately stale so every reuse revalidates. The result
  // is capped by cfg.ttlSeconds so a long server max-age still results in
  // cheap 304 checks.
  long long ComputeFreshLifetime(const Direct2D::ImageHttpMeta &resp) const {
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
        lifetime = cfg.ttlSeconds; // no signal at all: our default
      }
    }

    if (lifetime < 0)
      lifetime = 0;
    if (lifetime > cfg.ttlSeconds)
      lifetime = cfg.ttlSeconds;
    return lifetime;
  }

  // -------------------------------------------------------------------------
  // Download (once per URL) with revalidation and stale fallback.
  // Direct2D::DownloadImageFromURL is a generic WinHTTP downloader despite
  // its name: it walks the redirect chain and aggregates cache directives.
  // -------------------------------------------------------------------------
  bool DownloadAndCache(const std::wstring &url, const std::wstring &key,
                        std::vector<BYTE> &outBytes) {
    Meta stale;
    std::vector<BYTE> staleBytes;
    const bool haveStale = ReadEntry(key, url, stale, staleBytes);

    if (haveStale && NowSeconds() < stale.expiresAt) {
      outBytes = std::move(staleBytes);
      return true;
    }

    Logf(LogLevel::Debug, L"Fetching '%s'", url.c_str());

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
      Logf(LogLevel::Debug,
           L"Revalidated (304), served cached bytes for '%s'", url.c_str());
      outBytes = std::move(staleBytes);
      return true;
    }

    if (netOk && !bytes.empty()) {
      outBytes = std::move(bytes);
      if (resp.noStore) {
        // no-store anywhere in the redirect chain: never persist, and purge
        // any entry left by an older build so the next load hits the network.
        DeleteEntry(key);
        Logf(LogLevel::Debug,
             L"no-store directive; served without caching for '%s'",
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
        Logf(LogLevel::Debug, L"Downloaded and cached %llu bytes for '%s'",
             (unsigned long long)outBytes.size(), url.c_str());
      else
        Logf(LogLevel::Debug,
             L"Downloaded %llu bytes but cache write failed for '%s'",
             (unsigned long long)outBytes.size(), url.c_str());
      return true;
    }

    if (haveStale && !stale.mustRevalidate) {
      Logf(LogLevel::Warn, L"Network failed, serving stale cache for '%s'",
           url.c_str());
      outBytes = std::move(staleBytes);
      return true;
    }

    if (haveStale)
      Logf(LogLevel::Warn,
           L"Network failed and must-revalidate forbids stale reuse for '%s'",
           url.c_str());
    else
      Logf(LogLevel::Warn, L"Download failed for '%s'", url.c_str());
    return false;
  }

  // -------------------------------------------------------------------------
  // Public operations
  // -------------------------------------------------------------------------
  void Startup() {
    std::error_code ec;
    const fs::path dir = CacheDir();
    fs::create_directories(dir, ec);
    if (ec) {
      Logf(LogLevel::Warn, L"Could not create cache dir: %s", dir.c_str());
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
    if (total > cfg.maxCacheBytes) {
      std::sort(entries.begin(), entries.end(),
                [](const Entry &a, const Entry &b) { return a.mtime < b.mtime; });
      for (const Entry &e : entries) {
        if (total <= cfg.maxCacheBytes)
          break;
        std::error_code dec;
        fs::remove(e.bin, dec);
        fs::remove(e.meta, dec);
        total -= (long long)e.size;
      }
      Logf(LogLevel::Info, L"Evicted entries, %lld bytes remain under cap",
           total);
    }

    {
      std::lock_guard<std::mutex> lk(mutex);
      shuttingDown = false;
    }
    Logf(LogLevel::Debug, L"Startup: %d entries, %lld bytes in '%s'",
         (int)entries.size(), total, dir.c_str());
  }

  void Shutdown() {
    std::lock_guard<std::mutex> lk(mutex);
    shuttingDown = true;
    // Wake waiters so they fail fast; in-flight downloaders clean up their
    // own map entries when they finish.
    for (auto &pair : inFlight)
      pair.second->cv.notify_all();
  }

  bool Clear() {
    {
      std::lock_guard<std::mutex> lk(mutex);
      if (shuttingDown)
        return false;
    }

    // File I/O happens without holding the mutex: FetchBytes never touches
    // it during I/O and writes are atomic temp+rename, so a concurrent
    // download either lands before the sweep (removed) or after (one fresh
    // entry).
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

    Logf(LogLevel::Info, L"Cleared cache: %d files, %lld bytes freed", removed,
         freed);
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
        Logf(LogLevel::Debug, L"Cache hit for '%s'", url.c_str());
        return true;
      }
    }

    std::unique_lock<std::mutex> lk(mutex);
    if (shuttingDown)
      return false;

    auto it = inFlight.find(key);
    if (it != inFlight.end()) {
      // Another thread is already fetching this URL; share its result.
      std::shared_ptr<InFlightState> state = it->second;
      Logf(LogLevel::Debug, L"'%s' already downloading; waiting", url.c_str());
      state->cv.wait(lk, [&] { return state->done || shuttingDown; });
      if (state->done && state->ok) {
        outBytes = state->bytes;
        return true;
      }
      return false;
    }

    auto state = std::make_shared<InFlightState>();
    inFlight.emplace(key, state);
    lk.unlock();

    const bool ok = DownloadAndCache(url, key, outBytes);

    lk.lock();
    state->bytes = outBytes;
    state->ok = ok;
    state->done = true;
    inFlight.erase(key);
    lk.unlock();
    state->cv.notify_all();
    return ok;
  }
};

Cache::Cache(const Config &cfg) : m_Impl(std::make_unique<Impl>(cfg)) {}

Cache::~Cache() = default;

void Cache::Startup() { m_Impl->Startup(); }

void Cache::Shutdown() { m_Impl->Shutdown(); }

bool Cache::Clear() { return m_Impl->Clear(); }

bool Cache::FetchBytes(const std::wstring &url, std::vector<BYTE> &outBytes) {
  return m_Impl->FetchBytes(url, outBytes);
}

void Cache::DeleteUrl(const std::wstring &url) {
  m_Impl->DeleteEntry(PathUtils::GetUrlCacheKey(url));
}

} // namespace HttpDiskCache
