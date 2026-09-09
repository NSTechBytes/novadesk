/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_BUTTON_ELEMENT_H__
#define __NOVADESK_BUTTON_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"
#include <wrl/client.h>
#include <d2d1.h>
#include <string>

/**
 * @brief Visual state of a button element.
 */
enum ButtonState {
  BUTTON_STATE_NORMAL = 0,  ///< Default state.
  BUTTON_STATE_CLICKED = 1, ///< Mouse button pressed.
  BUTTON_STATE_HOVERED = 2  ///< Mouse hovering over button.
};

/**
 * @brief Clickable button element with image states and transparent hit
 * testing.
 *
 * @note Supports normal, hovered, and clicked image states. Transparent
 *       areas of the image are ignored during hit testing.
 */
class ButtonElement : public Element {
public:
  /**
   * @brief Constructs a button element with an image path.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param path Path or URL to the button image.
   */
  ButtonElement(const std::wstring &id, int x, int y, const std::wstring &path);
  virtual ~ButtonElement();

  virtual void Render(ID2D1DeviceContext *context) override;

  /// @return Auto-calculated width from button image.
  virtual int GetAutoWidth() override;

  /// @return Auto-calculated height from button image.
  virtual int GetAutoHeight() override;

  virtual void OnOwnerHWNDSet() override;
  virtual void OnImageDownloaded(const std::wstring &url,
                                 const std::vector<BYTE> &buffer) override;
  std::wstring GetImageUrl() const override { return m_ButtonImage.GetPath(); }

  /// @return True if the button image loaded successfully.
  bool IsLoaded() const { return m_ButtonImage.IsLoaded(); }

  /**
   * @brief Updates the button image source.
   *
   * @param path New path or URL to load.
   */
  void UpdateImage(const std::wstring &path);

  virtual bool HitTest(int x, int y) override;

  /// Returns true; transparent areas of the button image are ignored.
  virtual bool IsTransparentHit() const override { return true; }

  /// Sets the button image tint color and opacity.
  void SetImageTint(COLORREF color, BYTE alpha) {
    m_ButtonImage.SetImageTint(color, alpha);
  }

  /// Sets the button image opacity (0-255).
  void SetImageAlpha(BYTE alpha) { m_ButtonImage.SetImageAlpha(alpha); }

  /// Enables or disables grayscale rendering.
  void SetGrayscale(bool enable) { m_ButtonImage.SetGrayscale(enable); }

  /// Sets a custom 5x5 color matrix for advanced effects.
  void SetColorMatrix(const float *matrix) {
    m_ButtonImage.SetColorMatrix(matrix);
  }

  /// Enables or disables EXIF orientation handling.
  void SetUseExifOrientation(bool enabled) {
    m_ButtonImage.SetUseExifOrientation(enabled);
  }

  /// Sets the image flip mode.
  void SetImageFlip(ImageFlipMode flip) { m_ButtonImage.SetImageFlip(flip); }

  /// Sets the image crop region.
  void SetImageCrop(float x, float y, float w, float h,
                    ImageCropOrigin origin) {
    m_ButtonImage.SetImageCrop(x, y, w, h, origin);
  }

  /// Removes the image crop region.
  void ClearImageCrop() { m_ButtonImage.ClearImageCrop(); }

  /// Sets a fallback image path for load failures.
  void SetFallbackPath(const std::wstring &path) {
    m_ButtonImage.SetFallbackPath(path);
  }

  const std::wstring &GetImagePath() const { return m_ButtonImage.GetPath(); }
  const std::wstring &GetFallbackPath() const {
    return m_ButtonImage.GetFallbackPath();
  }
  bool HasImageTint() const { return m_ButtonImage.HasImageTint(); }
  COLORREF GetImageTint() const { return m_ButtonImage.GetImageTint(); }
  BYTE GetImageTintAlpha() const { return m_ButtonImage.GetImageTintAlpha(); }
  BYTE GetImageAlpha() const { return m_ButtonImage.GetImageAlpha(); }
  bool IsGrayscale() const { return m_ButtonImage.IsGrayscale(); }
  bool GetUseExifOrientation() const {
    return m_ButtonImage.GetUseExifOrientation();
  }
  bool HasColorMatrix() const { return m_ButtonImage.HasColorMatrix(); }
  const float *GetColorMatrix() const { return m_ButtonImage.GetColorMatrix(); }
  ImageFlipMode GetImageFlip() const { return m_ButtonImage.GetImageFlip(); }
  bool HasImageCrop() const { return m_ButtonImage.HasImageCrop(); }
  float GetImageCropX() const { return m_ButtonImage.GetImageCropX(); }
  float GetImageCropY() const { return m_ButtonImage.GetImageCropY(); }
  float GetImageCropW() const { return m_ButtonImage.GetImageCropW(); }
  float GetImageCropH() const { return m_ButtonImage.GetImageCropH(); }
  ImageCropOrigin GetImageCropOrigin() const {
    return m_ButtonImage.GetImageCropOrigin();
  }

  /// Sets the current visual state of the button.
  void SetButtonState(ButtonState state) { m_State = state; }

  /// @return The current visual state.
  ButtonState GetButtonState() const { return m_State; }

protected:
  ButtonState m_State = BUTTON_STATE_NORMAL; ///< Current button state.
  GeneralImage m_ButtonImage; ///< Button image (supports multi-state).
};

#endif
