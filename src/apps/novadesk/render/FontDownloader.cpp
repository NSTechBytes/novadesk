/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "FontDownloader.h"
#include "../shared/Logging.h"
#include "../shared/System.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "../domain/Widget.h"

// woff2 / brotli vendored decode API
#include "woff2/include/woff2/decode.h"
#include "woff2/include/woff2/output.h"

#include <windows.h>
#include <shlobj.h>        // SHGetFolderPathW
#include <filesystem>
#include <fstream>
#include <mutex>
#include <set>
#include <string>
#include <algorithm>
#include <functional>

namespace FontDownloader
{
    namespace
    {
        // -----------------------------------------------------------------------
        // Cache directory helpers
        // -----------------------------------------------------------------------

        std::wstring g_CacheDir;
        std::once_flag g_CacheDirFlag;

        std::wstring BuildCacheDir()
        {
            wchar_t localApp[MAX_PATH] = {};
            if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_LOCAL_APPDATA, nullptr, SHGFP_TYPE_CURRENT, localApp)))
            {
                std::wstring path = std::wstring(localApp) + L"\\Novadesk\\FontCache\\";
                std::error_code ec;
                std::filesystem::create_directories(path, ec);
                return path;
            }
            // Fallback to temp
            wchar_t tmp[MAX_PATH] = {};
            GetTempPathW(MAX_PATH, tmp);
            std::wstring path = std::wstring(tmp) + L"NovadeskFontCache\\";
            std::error_code ec;
            std::filesystem::create_directories(path, ec);
            return path;
        }

        // -----------------------------------------------------------------------
        // URL → cache path helpers
        // -----------------------------------------------------------------------

        std::wstring UrlToCacheSubdir(const std::wstring &url)
        {
            // Lowercase for consistent hashing
            std::wstring lower = url;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

            // Simple FNV-1a hash on wide characters
            uint64_t hash = 14695981039346656037ULL;
            for (wchar_t c : lower)
            {
                hash ^= static_cast<uint64_t>(c);
                hash *= 1099511628211ULL;
            }

            wchar_t buf[32];
            swprintf_s(buf, L"%016llx", hash);
            return CacheDir() + buf + L"\\";
        }

        std::wstring UrlToCacheFilePath(const std::wstring &url)
        {
            std::wstring subdir = UrlToCacheSubdir(url);

            // Determine extension from URL
            std::wstring ext = L".ttf";
            std::wstring::size_type qpos = url.find(L'?');
            std::wstring urlNoQuery = (qpos != std::wstring::npos) ? url.substr(0, qpos) : url;
            std::wstring::size_type dotpos = urlNoQuery.rfind(L'.');
            if (dotpos != std::wstring::npos)
            {
                std::wstring rawExt = urlNoQuery.substr(dotpos);
                std::transform(rawExt.begin(), rawExt.end(), rawExt.begin(), ::towlower);
                if (rawExt == L".woff2")
                    ext = L".ttf";
                else if (rawExt == L".otf" || rawExt == L".ttf" || rawExt == L".ttc")
                    ext = rawExt;
            }
            return subdir + L"font" + ext;
        }

        // -----------------------------------------------------------------------
        // In-progress tracking to avoid double-downloads
        // -----------------------------------------------------------------------

        std::mutex g_InProgressMutex;
        std::set<std::wstring> g_InProgress; // keyed by subdirectory path

        // -----------------------------------------------------------------------
        // WOFF2 detection and conversion
        // -----------------------------------------------------------------------

        bool IsWoff2(const std::string &data)
        {
            // WOFF2 magic: 0x774F4632 ('wOF2')
            return data.size() >= 4 &&
                   static_cast<unsigned char>(data[0]) == 0x77 &&
                   static_cast<unsigned char>(data[1]) == 0x4F &&
                   static_cast<unsigned char>(data[2]) == 0x46 &&
                   static_cast<unsigned char>(data[3]) == 0x32;
        }

        // Convert WOFF2 bytes to TTF bytes using vendored google/woff2 library.
        // Returns true on success, false on failure.
        bool ConvertWoff2ToTtf(const std::string &woff2Data, std::string &ttfOut)
        {
            const uint8_t *data = reinterpret_cast<const uint8_t *>(woff2Data.data());
            size_t len = woff2Data.size();

            // Determine output size
            size_t ttfSize = woff2::ComputeWOFF2FinalSize(data, len);
            if (ttfSize == 0)
            {
                Logging::Log(LogLevel::Error, L"FontDownloader: WOFF2 ComputeWOFF2FinalSize returned 0");
                return false;
            }

            ttfOut.resize(ttfSize);

            woff2::WOFF2StringOut out(&ttfOut);
            if (!woff2::ConvertWOFF2ToTTF(data, len, &out))
            {
                Logging::Log(LogLevel::Error, L"FontDownloader: WOFF2 conversion failed");
                ttfOut.clear();
                return false;
            }

            return true;
        }

        // -----------------------------------------------------------------------
        // DispatchFontReady — posted back to the main thread via PostMessage
        // -----------------------------------------------------------------------

        void DispatchFontReadyInternal(void *vPayload)
        {
            std::unique_ptr<FontReadyPayload> payload(static_cast<FontReadyPayload *>(vPayload));
            if (!payload)
                return;

            if (payload->cachedDir.empty())
            {
                Logging::Log(LogLevel::Warn, L"FontDownloader: Font download failed for element '%s'",
                             payload->elementId.c_str());
                return;
            }

            // Find the widget by HWND and update the element
            Widget *widget = Widget::GetWidgetFromHWND(payload->widgetHwnd);
            if (!widget)
            {
                Logging::Log(LogLevel::Warn, L"FontDownloader: Widget no longer exists, discarding font for '%s'",
                             payload->elementId.c_str());
                return;
            }

            Logging::Log(LogLevel::Info, L"FontDownloader: Applying downloaded font to element '%s' from '%s'",
                         payload->elementId.c_str(), payload->cachedDir.c_str());

            widget->SetElementFontPath(payload->elementId, payload->cachedDir);
        }

    } // anonymous namespace

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    std::wstring CacheDir()
    {
        std::call_once(g_CacheDirFlag, []() { g_CacheDir = BuildCacheDir(); });
        return g_CacheDir;
    }

    std::wstring GetCachedDir(const std::wstring &url)
    {
        std::wstring filepath = UrlToCacheFilePath(url);
        if (std::filesystem::exists(filepath))
            return UrlToCacheSubdir(url);
        return L"";
    }

    void RequestAsync(const std::wstring &url, HWND widgetHwnd, const std::wstring &elementId)
    {
        std::wstring subdir = UrlToCacheSubdir(url);
        std::wstring filepath = UrlToCacheFilePath(url);

        // Check cache again inside lock to avoid race conditions
        {
            std::lock_guard<std::mutex> lk(g_InProgressMutex);
            if (std::filesystem::exists(filepath))
            {
                // Already cached — dispatch immediately
                auto *payload = new FontReadyPayload{widgetHwnd, elementId, subdir};
                HWND msgWnd = JSEngine::GetMessageWindow();
                if (msgWnd)
                {
                    PostMessageW(msgWnd, JSEngine::WM_NOVADESK_DISPATCH,
                                 reinterpret_cast<WPARAM>(&DispatchFontReady),
                                 reinterpret_cast<LPARAM>(payload));
                }
                else
                {
                    delete payload;
                }
                return;
            }

            // Check if already in-progress
            if (g_InProgress.count(subdir))
            {
                Logging::Log(LogLevel::Debug, L"FontDownloader: '%s' is already downloading", url.c_str());
                return;
            }

            g_InProgress.insert(subdir);
        }

        Logging::Log(LogLevel::Info, L"FontDownloader: Starting async download of '%s'", url.c_str());

        // Capture everything needed by value
        std::thread([url, subdir, filepath, widgetHwnd, elementId]()
        {
            std::wstring cachedDir;

            std::string rawData;
            bool ok = novadesk::shared::system::WebFetch(url, rawData);

            if (ok && !rawData.empty())
            {
                // Convert WOFF2 → TTF if needed
                if (IsWoff2(rawData))
                {
                    Logging::Log(LogLevel::Info, L"FontDownloader: Detected WOFF2, converting to TTF: '%s'", url.c_str());
                    std::string ttfData;
                    if (ConvertWoff2ToTtf(rawData, ttfData))
                    {
                        rawData = std::move(ttfData);
                    }
                    else
                    {
                        Logging::Log(LogLevel::Error, L"FontDownloader: WOFF2→TTF conversion failed for '%s'", url.c_str());
                        ok = false;
                    }
                }

                if (ok)
                {
                    // Create sub-directory if it doesn't exist
                    std::error_code ec;
                    std::filesystem::create_directories(subdir, ec);

                    // Write to cache
                    std::ofstream f(std::filesystem::path(filepath), std::ios::binary | std::ios::trunc);
                    if (f.is_open())
                    {
                        f.write(rawData.data(), static_cast<std::streamsize>(rawData.size()));
                        f.close();
                        Logging::Log(LogLevel::Info, L"FontDownloader: Cached font at '%s'", filepath.c_str());
                        cachedDir = subdir;
                    }
                    else
                    {
                        Logging::Log(LogLevel::Error, L"FontDownloader: Could not write cache file '%s'", filepath.c_str());
                    }
                }
            }
            else
            {
                Logging::Log(LogLevel::Error, L"FontDownloader: Download failed for '%s'", url.c_str());
            }

            {
                std::lock_guard<std::mutex> lk(g_InProgressMutex);
                g_InProgress.erase(subdir);
            }

            // Post result back to main thread
            HWND msgWnd = JSEngine::GetMessageWindow();
            if (msgWnd)
            {
                auto *payload = new FontReadyPayload{widgetHwnd, elementId, cachedDir};
                if (!PostMessageW(msgWnd, JSEngine::WM_NOVADESK_DISPATCH,
                                  reinterpret_cast<WPARAM>(&DispatchFontReady),
                                  reinterpret_cast<LPARAM>(payload)))
                {
                    delete payload;
                }
            }
        }).detach();
    }

    void DispatchFontReady(void *payload)
    {
        DispatchFontReadyInternal(payload);
    }

} // namespace FontDownloader
