/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <string>
#include "Element.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "winhttp.lib")

/**
 * @brief Core Direct2D rendering utilities and factory accessors.
 *
 * @note Must call Initialize() before use and Cleanup() on shutdown.
 */
namespace Direct2D {

/// Initializes Direct2D, DirectWrite, and WIC factories.
bool Initialize();

/// Cleans up all Direct2D resources.
void Cleanup();

/// @return The Direct2D factory instance.
ID2D1Factory1 *GetFactory();

/// @return The DirectWrite factory instance.
IDWriteFactory1 *GetWriteFactory();

/// @return The WIC imaging factory instance.
IWICImagingFactory *GetWICFactory();

// ============================================================================
// Brush Helpers
// ============================================================================

/**
 * @brief Creates a solid color brush.
 *
 * @param context The render target.
 * @param color The brush color in COLORREF format.
 * @param alpha The brush opacity (0.0-1.0).
 * @param brush Receives the created brush.
 *
 * @return True if successful.
 */
bool CreateSolidBrush(ID2D1RenderTarget *context, COLORREF color, float alpha,
                      ID2D1SolidColorBrush **brush);

/**
 * @brief Creates a linear gradient brush.
 *
 * @param context The render target.
 * @param start Start point of the gradient.
 * @param end End point of the gradient.
 * @param color1 Start color.
 * @param alpha1 Start opacity.
 * @param color2 End color.
 * @param alpha2 End opacity.
 * @param brush Receives the created brush.
 *
 * @return True if successful.
 */
bool CreateLinearGradientBrush(ID2D1RenderTarget *context,
                               const D2D1_POINT_2F &start,
                               const D2D1_POINT_2F &end, COLORREF color1,
                               float alpha1, COLORREF color2, float alpha2,
                               ID2D1LinearGradientBrush **brush);

/**
 * @brief Creates a gradient brush from GradientInfo configuration.
 *
 * @param context The render target.
 * @param rect The bounding rectangle for the gradient.
 * @param info The gradient configuration.
 * @param brush Receives the created brush.
 *
 * @return True if successful.
 */
bool CreateGradientBrush(ID2D1RenderTarget *context, const D2D1_RECT_F &rect,
                         const GradientInfo &info, ID2D1Brush **brush);

/**
 * @brief Creates a brush from either gradient info or solid color.
 *
 * @param context The render target.
 * @param rect The bounding rectangle.
 * @param gradient Optional gradient configuration (nullptr for solid color).
 * @param color Solid color fallback.
 * @param alpha Solid color opacity.
 * @param brush Receives the created brush.
 *
 * @return True if successful.
 */
bool CreateBrushFromGradientOrColor(ID2D1RenderTarget *context,
                                    const D2D1_RECT_F &rect,
                                    const GradientInfo *gradient,
                                    COLORREF color, float alpha,
                                    ID2D1Brush **brush);

// ============================================================================
// Bitmap Loading
// ============================================================================

/**
 * @brief Loads a bitmap from a file path.
 *
 * @param context The render target for bitmap creation.
 * @param path Path to the image file.
 * @param bitmap Receives the Direct2D bitmap.
 * @param wicBitmap Optional WIC bitmap output for pixel access.
 * @param useExifOrientation Whether to apply EXIF rotation.
 *
 * @return True if successful.
 */
bool LoadBitmapFromFile(ID2D1RenderTarget *context, const std::wstring &path,
                        ID2D1Bitmap **bitmap, IWICBitmap **wicBitmap = nullptr,
                        bool useExifOrientation = false);

/**
 * @brief Loads a WIC bitmap from a file path.
 *
 * @param path Path to the image file.
 * @param wicBitmap Receives the WIC bitmap.
 * @param useExifOrientation Whether to apply EXIF rotation.
 *
 * @return True if successful.
 */
bool LoadWICBitmapFromFile(const std::wstring &path, IWICBitmap **wicBitmap,
                           bool useExifOrientation = false);

/**
 * @brief Loads a WIC bitmap from a resource.
 *
 * @param hModule Module handle containing the resource.
 * @param resourceName Resource identifier.
 * @param resourceType Resource type identifier.
 * @param wicBitmap Receives the WIC bitmap.
 *
 * @return True if successful.
 */
bool LoadWICBitmapFromResource(HMODULE hModule, LPCWSTR resourceName,
                               LPCWSTR resourceType, IWICBitmap **wicBitmap);

/**
 * @brief Loads a WIC bitmap from memory buffer.
 *
 * @param data Pointer to encoded image data.
 * @param size Size of the data in bytes.
 * @param wicBitmap Receives the WIC bitmap.
 *
 * @return True if successful.
 */
bool LoadWICBitmapFromMemory(const BYTE *data, DWORD size,
                             IWICBitmap **wicBitmap);

// ============================================================================
// URL Support
// ============================================================================

/**
 * @brief Loads a WIC bitmap from a URL (HTTP/HTTPS).
 *
 * @param url The URL to download from.
 * @param wicBitmap Receives the WIC bitmap.
 * @param useExifOrientation Whether to apply EXIF rotation.
 *
 * @return True if successful.
 */
bool LoadWICBitmapFromURL(const std::wstring &url, IWICBitmap **wicBitmap,
                          bool useExifOrientation = false);

/**
 * @brief Downloads image data from a URL.
 *
 * @param url The URL to download from.
 * @param buffer Receives the downloaded bytes.
 *
 * @return True if successful.
 */
bool DownloadImageFromURL(const std::wstring &url, std::vector<BYTE> &buffer);

// ============================================================================
// Conversion Helpers
// ============================================================================

/// Converts a COLORREF to a D2D1_COLOR_F.
D2D1_COLOR_F ColorToD2D(COLORREF color, float alpha = 1.0f);

/// Finds the edge point of an ellipse at a given angle.
D2D1_POINT_2F FindEdgePoint(float angle, const D2D1_RECT_F &rect);

} // namespace Direct2D
