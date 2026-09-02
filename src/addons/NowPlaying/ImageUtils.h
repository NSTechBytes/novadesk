/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <filesystem>
#include <Windows.h>
#include <gdiplus.h>
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.Storage.Streams.h>

/**
 * @brief Utilities for saving and processing album cover art.
 *
 * @note Uses WinRT Storage APIs for stream handling and GDI+ for
 *       image processing (transparent border detection, cropping).
 */
namespace ImageUtils {

/**
 * @brief Saves album cover art from a stream reference to a temp file.
 *
 * @param image The stream reference containing the cover image.
 *
 * @return Path to the saved cover art file.
 */
winrt::hstring
SaveCover(winrt::Windows::Storage::Streams::IRandomAccessStreamReference image);

/**
 * @brief Checks if an image has transparent borders that can be cropped.
 *
 * @param original Path to the image file.
 *
 * @return True if transparent borders were detected.
 */
bool CoverHasTransparentBorder(winrt::hstring original);

/**
 * @brief Crops transparent borders from an album cover image.
 *
 * @param original Path to the image file.
 *
 * @return Path to the cropped image file.
 */
winrt::hstring CropCover(winrt::hstring original);

} // namespace ImageUtils
