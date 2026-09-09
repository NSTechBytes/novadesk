/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <dwrite_1.h>
#include <wrl/client.h>
#include <string>
#include <vector>
#include <map>

/**
 * @brief Manages font collections, custom font loading, and font caching.
 *
 * @note Provides both directory-based and in-memory font registration.
 *       Thread-safe for concurrent access from render and download threads.
 */
namespace FontManager {

/// Initializes the font manager system.
bool Initialize();

/// Cleans up all cached font collections.
void Cleanup();

/**
 * @brief Gets or creates a font collection from a directory.
 *
 * @param directoryPath Path to the directory containing font files.
 *
 * @return The font collection, or nullptr on failure.
 *
 * @note The path can be absolute or relative to the executable.
 */
Microsoft::WRL::ComPtr<IDWriteFontCollection>
GetFontCollection(const std::wstring &directoryPath);

/**
 * @brief Registers font data in memory for async font loading.
 *
 * @param url The font URL (used as key).
 * @param data The raw font file data.
 */
void AddMemoryFont(const std::wstring &url, const std::string &data);

/**
 * @brief Gets previously registered font data.
 *
 * @param url The font URL.
 *
 * @return Reference to the font data vector.
 */
const std::vector<uint8_t> &GetMemoryFont(const std::wstring &url);

/**
 * @brief Checks if font data is registered in memory.
 *
 * @param url The font URL.
 *
 * @return True if font data exists.
 */
bool HasMemoryFont(const std::wstring &url);

} // namespace FontManager
