/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_IMAGE_ELEMENT_H__
#define __NOVADESK_IMAGE_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"
#include <wrl/client.h>
#include <d2d1.h>

/**
 * @brief Image aspect ratio handling modes.
 */
enum ImageAspectRatio {
  IMAGE_ASPECT_STRETCH,  ///< Stretch image to fill element bounds.
  IMAGE_ASPECT_PRESERVE, ///< Preserve aspect ratio, fit within bounds.
  IMAGE_ASPECT_CROP      ///< Preserve aspect ratio, crop excess.
};

/**
 * @brief Renders an image from a local file or remote URL.
 *
 * @note Supports automatic download, tinting, grayscale, cropping, and
 *       nine-patch scale margins. Falls back to a placeholder on load failure.
 */
class ImageElement : public Element {
public:
  /**
   * @brief Constructs an image element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param w Width in pixels (0 = auto from image).
   * @param h Height in pixels (0 = auto from image).
   * @param path Path or URL to the image.
   */
  ImageElement(const std::wstring &id, int x, int y, int w, int h,
               const std::wstring &path);

  virtual ~ImageElement();

  virtual void Render(ID2D1DeviceContext *context) override;

  /// @return Auto-calculated width from image dimensions.
  virtual int GetAutoWidth() override;

  /// @return Auto-calculated height from image dimensions.
  virtual int GetAutoHeight() override;

  virtual void OnOwnerHWNDSet() override;
  virtual void OnImageDownloaded(const std::wstring &url,
                                 const std::vector<BYTE> &buffer) override;
  std::wstring GetImageUrl() const override { return m_GeneralImage.GetPath(); }

  /// @return True if the image loaded successfully.
  bool IsLoaded() const { return m_GeneralImage.IsLoaded(); }

  /**
   * @brief Updates the image source path.
   *
   * @param path New path or URL to load.
   */
  void UpdateImage(const std::wstring &path);

  virtual bool HitTest(int x, int y) override;

  /// Sets the aspect ratio handling mode.
  void SetPreserveAspectRatio(ImageAspectRatio mode) {
    m_PreserveAspectRatio = mode;
  }

  /// Sets the image tint color and opacity.
  void SetImageTint(COLORREF color, BYTE alpha) {
    m_GeneralImage.SetImageTint(color, alpha);
  }

  /// Sets the image opacity (0-255).
  void SetImageAlpha(BYTE alpha) { m_GeneralImage.SetImageAlpha(alpha); }

  /// Enables or disables grayscale rendering.
  void SetGrayscale(bool enable) { m_GeneralImage.SetGrayscale(enable); }

  /// Sets a custom 5x5 color matrix for advanced image effects.
  void SetColorMatrix(const float *matrix) {
    m_GeneralImage.SetColorMatrix(matrix);
  }

  /// Enables or disables image tiling.
  void SetTile(bool tile) { m_Tile = tile; }

  /// Sets the image flip mode.
  void SetImageFlip(ImageFlipMode flip) { m_GeneralImage.SetImageFlip(flip); }

  /// Enables or disables EXIF orientation handling.
  void SetUseExifOrientation(bool enabled) {
    m_GeneralImage.SetUseExifOrientation(enabled);
  }

  /// Sets the image crop region.
  void SetImageCrop(float x, float y, float w, float h,
                    ImageCropOrigin origin) {
    m_GeneralImage.SetImageCrop(x, y, w, h, origin);
  }

  /// Removes the image crop region.
  void ClearImageCrop() { m_GeneralImage.ClearImageCrop(); }

  /**
   * @brief Sets nine-patch scale margins for button-like scaling.
   *
   * @param left Left margin (pixels from left edge).
   * @param top Top margin (pixels from top edge).
   * @param right Right margin (pixels from right edge).
   * @param bottom Bottom margin (pixels from bottom edge).
   */
  void SetScaleMargins(float left, float top, float right, float bottom);

  /// Clears nine-patch scale margins.
  void ClearScaleMargins() { m_HasScaleMargins = false; }

  /// Sets a fallback image path for load failures.
  void SetFallbackPath(const std::wstring &path) {
    m_GeneralImage.SetFallbackPath(path);
  }

  const std::wstring &GetImagePath() const { return m_GeneralImage.GetPath(); }
  const std::wstring &GetFallbackPath() const {
    return m_GeneralImage.GetFallbackPath();
  }
  ImageAspectRatio GetPreserveAspectRatio() const {
    return m_PreserveAspectRatio;
  }
  bool HasImageTint() const { return m_GeneralImage.HasImageTint(); }
  COLORREF GetImageTint() const { return m_GeneralImage.GetImageTint(); }
  BYTE GetImageTintAlpha() const { return m_GeneralImage.GetImageTintAlpha(); }
  BYTE GetImageAlpha() const { return m_GeneralImage.GetImageAlpha(); }
  bool IsGrayscale() const { return m_GeneralImage.IsGrayscale(); }
  bool IsTile() const { return m_Tile; }
  ImageFlipMode GetImageFlip() const { return m_GeneralImage.GetImageFlip(); }
  bool GetUseExifOrientation() const {
    return m_GeneralImage.GetUseExifOrientation();
  }
  bool HasImageCrop() const { return m_GeneralImage.HasImageCrop(); }
  float GetImageCropX() const { return m_GeneralImage.GetImageCropX(); }
  float GetImageCropY() const { return m_GeneralImage.GetImageCropY(); }
  float GetImageCropW() const { return m_GeneralImage.GetImageCropW(); }
  float GetImageCropH() const { return m_GeneralImage.GetImageCropH(); }
  ImageCropOrigin GetImageCropOrigin() const {
    return m_GeneralImage.GetImageCropOrigin();
  }
  bool HasScaleMargins() const { return m_HasScaleMargins; }
  float GetScaleMarginLeft() const { return m_ScaleMarginLeft; }
  float GetScaleMarginTop() const { return m_ScaleMarginTop; }
  float GetScaleMarginRight() const { return m_ScaleMarginRight; }
  float GetScaleMarginBottom() const { return m_ScaleMarginBottom; }
  bool HasColorMatrix() const { return m_GeneralImage.HasColorMatrix(); }
  const float *GetColorMatrix() const {
    return m_GeneralImage.GetColorMatrix();
  }

private:
  /**
   * @brief Internal layout calculation for image rendering.
   */
  struct ImageLayout {
    int contentX = 0; ///< Content X offset.
    int contentY = 0; ///< Content Y offset.
    int contentW = 0; ///< Content width.
    int contentH = 0; ///< Content height.
    D2D1_RECT_F finalRect = D2D1::RectF(0, 0, 0, 0); ///< Destination rect.
    D2D1_RECT_F srcRect = D2D1::RectF(0, 0, 0, 0);   ///< Source rect.
  };

  GeneralImage m_GeneralImage;
  ImageAspectRatio m_PreserveAspectRatio = IMAGE_ASPECT_STRETCH;
  bool m_Tile = false;
  bool m_HasScaleMargins = false;
  float m_ScaleMarginLeft = 0.0f;
  float m_ScaleMarginTop = 0.0f;
  float m_ScaleMarginRight = 0.0f;
  float m_ScaleMarginBottom = 0.0f;

  bool ResolveImageCropRect(float imageWidth, float imageHeight,
                            D2D1_RECT_F &rect) const;
  bool ComputeImageLayout(float imageWidth, float imageHeight,
                          ImageLayout &layout);
  bool MapPointToImagePixel(float targetX, float targetY, UINT imageWidth,
                            UINT imageHeight, float &pixelX, float &pixelY);
};

#endif
