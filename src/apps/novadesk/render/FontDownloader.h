/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <cstdint>
#include <string>
#include <windows.h>

/**
 * @brief Provides async font downloading and in-memory registration.
 *
 * @note Supports .ttf, .otf, and .woff2 (auto-converted to TTF) formats.
 *       Downloaded fonts are registered and loaded entirely in-memory using
 *       IDWriteInMemoryFontFileLoader (re-downloaded every application launch).
 *
 * @usage
 *   // If already downloaded in memory, returns url immediately:
 *   std::wstring path = FontDownloader::GetCachedDir(url);
 *
 *   // If not yet downloaded, start async download:
 *   FontDownloader::RequestAsync(url, widgetInstanceId, elementId);
 */
namespace FontDownloader {

/**
 * @brief Checks if a font URL is already downloaded and loaded in memory.
 *
 * @param url The font URL to check.
 *
 * @return The URL if loaded, or empty string if not yet loaded.
 *
 * @note Safe to call from any thread.
 */
std::wstring GetCachedDir(const std::wstring &url);

/**
 * @brief Starts an asynchronous font download.
 *
 * @param url The URL to download the font from.
 * @param widgetInstanceId The widget instance ID for callback routing.
 * @param elementId The element ID to update on completion.
 *
 * @note On completion, registers the in-memory font with FontManager,
 *       then posts WM_NOVADESK_DISPATCH back to the main thread which causes
 *       the engine to call SetElementFontPath(elementId, url) + Redraw().
 *       If url is already downloading or loaded, this is a no-op.
 */
void RequestAsync(const std::wstring &url, uint64_t widgetInstanceId,
                  const std::wstring &elementId);

/**
 * @brief Shuts down the font downloader and joins all active downloads.
 *
 * @note Must be called before the engine message window is destroyed.
 */
void Shutdown();

/**
 * @brief Payload for font-ready dispatch callback.
 */
struct FontReadyPayload {
  uint64_t widgetInstanceId; ///< Widget instance ID (stable across HWND reuse).
  std::wstring elementId;    ///< Element to update.
  std::wstring cachedDir;    ///< URL on success, empty on failure.
};

/**
 * @brief Dispatch callback for font download completion.
 *
 * @param payload Pointer to FontReadyPayload (allocated with new).
 *
 * @note Called on the main thread from the JS engine dispatcher.
 */
void DispatchFontReady(void *payload);

} // namespace FontDownloader
