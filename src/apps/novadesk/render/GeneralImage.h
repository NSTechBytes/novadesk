/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_GENERAL_IMAGE_H__
#define __NOVADESK_GENERAL_IMAGE_H__

#include <array>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <windows.h>
#include <d2d1_1.h>
#include <wincodec.h>
#include <wrl/client.h>

/**
 * @brief Image flip modes for horizontal/vertical mirroring.
 */
enum ImageFlipMode {
  IMAGE_FLIP_NONE = 0,       ///< No flip applied.
  IMAGE_FLIP_HORIZONTAL,     ///< Flip left-to-right.
  IMAGE_FLIP_VERTICAL,       ///< Flip top-to-bottom.
  IMAGE_FLIP_BOTH            ///< Flip both axes (180° rotation).
};

/**
 * @brief Origin point for image crop operations.
 */
enum ImageCropOrigin {
  IMAGE_CROP_ORIGIN_TOP_LEFT = 0,    ///< Crop from top-left corner.
  IMAGE_CROP_ORIGIN_TOP_RIGHT = 1,   ///< Crop from top-right corner.
  IMAGE_CROP_ORIGIN_BOTTOM_RIGHT = 2, ///< Crop from bottom-right corner.
  IMAGE_CROP_ORIGIN_BOTTOM_LEFT = 3,  ///< Crop from bottom-left corner.
  IMAGE_CROP_ORIGIN_CENTER = 4        ///< Crop from center.
};

/**
 * @brief Decoded image data with raw pixel buffer.
 */
struct DecodedImageData {
  UINT width = 0;   ///< Image width in pixels.
  UINT height = 0;  ///< Image height in pixels.
  UINT stride = 0;  ///< Row stride in bytes (width * 4 for BGRA).
  std::vector<BYTE> pixels; ///< Raw BGRA pixel data.

  /// @return True if the image data is valid and complete.
  bool IsValid() const {
    return width > 0 && height > 0 && stride >= width * 4 && !pixels.empty();
  }
};

/**
 * @brief Result of an asynchronous image download and decode operation.
 */
struct AsyncImageResult {
  std::vector<BYTE> encodedBytes;   ///< Original encoded image bytes.
  DecodedImageData decodedImage;    ///< Decoded pixel data.
};

/**
 * @brief Manages image loading, decoding, caching, and rendering via Direct2D.
 *
 * @note Supports local files, HTTP/HTTPS URLs, async downloads, EXIF
 *       orientation, tinting, grayscale, color matrices, cropping, and flip.
 *       Thread-safe for concurrent download and render operations.
 */
class GeneralImage {
public:
  GeneralImage();
  ~GeneralImage();

  /**
   * @brief Sets the image source path (file or URL).
   *
   * @param path Path to the image file or HTTP/HTTPS URL.
   */
  void SetPath(const std::wstring &path);

  /// @return The current image path.
  const std::wstring &GetPath() const { return m_ImagePath; }

  /// Sets a fallback image path for load failures.
  void SetFallbackPath(const std::wstring &path);

  /// @return The fallback image path.
  const std::wstring &GetFallbackPath() const { return m_FallbackPath; }

  /**
   * @brief Ensures the Direct2D bitmap is loaded and ready for rendering.
   *
   * @param context The Direct2D device context for bitmap creation.
   */
  void EnsureBitmap(ID2D1DeviceContext *context);

  /// @return True if the image bitmap is loaded and ready.
  bool IsLoaded() const { return m_D2DBitmap != nullptr; }

  /// @return The Direct2D bitmap for rendering.
  ID2D1Bitmap *GetBitmap() const { return m_D2DBitmap.Get(); }

  /// @return The WIC bitmap for pixel-level access.
  IWICBitmap *GetWICBitmap() const { return m_pWICBitmap.Get(); }

  /**
   * @brief Gets the alpha value of a specific pixel.
   *
   * @param x X-coordinate of the pixel.
   * @param y Y-coordinate of the pixel.
   *
   * @return Alpha value (0-255), or 0 if coordinates are out of bounds.
   */
  BYTE GetPixelAlpha(int x, int y) const;

  /// Sets the image tint color and opacity.
  void SetImageTint(COLORREF color, BYTE alpha);
  bool HasImageTint() const { return m_HasImageTint; }
  COLORREF GetImageTint() const { return m_ImageTint; }
  BYTE GetImageTintAlpha() const { return m_ImageTintAlpha; }

  /// Sets the overall image opacity (0-255).
  void SetImageAlpha(BYTE alpha) { m_ImageAlpha = alpha; }
  BYTE GetImageAlpha() const { return m_ImageAlpha; }

  /// Enables or disables grayscale rendering.
  void SetGrayscale(bool enable) { m_Grayscale = enable; }
  bool IsGrayscale() const { return m_Grayscale; }

  /// Sets a custom 5x5 color transformation matrix.
  void SetColorMatrix(const float *matrix);
  bool HasColorMatrix() const { return m_HasColorMatrix; }
  const float *GetColorMatrix() const { return m_ColorMatrix.data(); }

  /// Sets the flip mode for the image.
  void SetImageFlip(ImageFlipMode flip) { m_ImageFlip = flip; }
  ImageFlipMode GetImageFlip() const { return m_ImageFlip; }

  /// Enables or disables EXIF orientation handling.
  void SetUseExifOrientation(bool enabled);
  bool GetUseExifOrientation() const { return m_UseExifOrientation; }

  /// Sets the image crop region.
  void SetImageCrop(float x, float y, float w, float h, ImageCropOrigin origin);

  /// Clears the image crop region.
  void ClearImageCrop() { m_HasImageCrop = false; }
  bool HasImageCrop() const { return m_HasImageCrop; }
  float GetImageCropX() const { return m_ImageCropX; }
  float GetImageCropY() const { return m_ImageCropY; }
  float GetImageCropW() const { return m_ImageCropW; }
  float GetImageCropH() const { return m_ImageCropH; }
  ImageCropOrigin GetImageCropOrigin() const { return m_ImageCropOrigin; }

  bool ResolveImageCropRect(float imageWidth, float imageHeight,
                            D2D1_RECT_F &rect) const;
  void ApplyFlipToPixel(float &pixelX, float &pixelY,
                        const D2D1_RECT_F &srcRect) const;
  bool BuildFlipTransform(const D2D1_RECT_F &dstRect,
                          D2D1_MATRIX_3X2_F &outTransform) const;

  bool BuildProcessedImage(ID2D1DeviceContext *context,
                           Microsoft::WRL::ComPtr<ID2D1Image> &outImage) const;

  /// @return Auto-calculated width preserving aspect ratio.
  int GetAutoWidth() const;

  /// @return Auto-calculated height preserving aspect ratio.
  int GetAutoHeight() const;

  /// Sets the owning widget's HWND for async download callbacks.
  void SetOwnerHWND(HWND hWnd);

  /// Shuts down all pending async download threads.
  void ShutdownAsyncDownloads();

  /**
   * @brief Called when an async image download completes.
   *
   * @param url The URL that was downloaded.
   * @param buffer The downloaded image bytes.
   */
  void OnImageDownloaded(const std::wstring &url,
                         const std::vector<BYTE> &buffer);

  /**
   * @brief Called when an image has been decoded on a worker thread.
   *
   * @param url The URL of the decoded image.
   * @param image The decoded image data.
   */
  void OnImageDecoded(const std::wstring &url, DecodedImageData &&image);

  /**
   * @brief Decodes raw image bytes into pixel data.
   *
   * @param bytes The encoded image bytes.
   * @param out Receives the decoded pixel data.
   *
   * @return True if decoding succeeded.
   *
   * @note Used by the WM_USER+500 handler for on-demand decoding to avoid
   *       2× memory peak for large images.
   */
  static bool DecodeFromBytes(const std::vector<BYTE> &bytes,
                              DecodedImageData &out);

private:
  void ReloadWICBitmap();
  void ResetBitmapCache();
  void StartAsyncDownload(const std::wstring &url);
  bool IsAsyncDownloadShutdown();
  void LoadFallbackFromResource();

private:
  // ============================================================================
  // Image Source
  // ============================================================================

  std::wstring m_ImagePath;      ///< Current image path.
  std::wstring m_LoadedPath;     ///< Path of the actually loaded image.
  std::wstring m_FallbackPath;   ///< Fallback image path.
  HWND m_OwnerHWND = nullptr;   ///< Owning widget HWND.

  // ============================================================================
  // Async Download State
  // ============================================================================

  std::mutex m_AsyncDownloadMutex;
  std::vector<std::thread> m_AsyncDownloadThreads;
  bool m_AsyncDownloadsShutdown = false;

  // ============================================================================
  // Image State (thread-safe via m_ImageStateMutex)
  // ============================================================================

  mutable std::recursive_mutex m_ImageStateMutex;
  bool m_IsFallbackShowing = false; ///< True while showing the fallback image.
  std::vector<BYTE> m_DownloadedBuffer; ///< In-memory buffer for async downloads.
  DecodedImageData m_DecodedImage; ///< Decoded pixel data.
  Microsoft::WRL::ComPtr<ID2D1Bitmap> m_D2DBitmap; ///< Cached Direct2D bitmap.
  Microsoft::WRL::ComPtr<IWICBitmap> m_pWICBitmap; ///< WIC bitmap for pixel access.
  ID2D1RenderTarget *m_pLastTarget = nullptr; ///< Last render target (for cache invalidation).

  // ============================================================================
  // Visual Effects
  // ============================================================================

  bool m_HasImageTint = false;
  COLORREF m_ImageTint = RGB(0, 0, 0);
  BYTE m_ImageTintAlpha = 255;
  BYTE m_ImageAlpha = 255;
  bool m_Grayscale = false;
  bool m_HasColorMatrix = false;
  std::array<float, 20> m_ColorMatrix{};
  ImageFlipMode m_ImageFlip = IMAGE_FLIP_NONE;
  bool m_UseExifOrientation = false;

  // ============================================================================
  // Crop Region
  // ============================================================================

  bool m_HasImageCrop = false;
  float m_ImageCropX = 0.0f;
  float m_ImageCropY = 0.0f;
  float m_ImageCropW = 0.0f;
  float m_ImageCropH = 0.0f;
  ImageCropOrigin m_ImageCropOrigin = IMAGE_CROP_ORIGIN_TOP_LEFT;
};

#endif
