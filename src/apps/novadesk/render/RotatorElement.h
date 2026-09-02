/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ROTATOR_ELEMENT_H__
#define __NOVADESK_ROTATOR_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"

/**
 * @brief Rotating knob/dial element that maps a value to rotation angle.
 *
 * @note Supports configurable rotation range, start angle, and pivot offset.
 *       The image rotates based on the value within the min/max range.
 */
class RotatorElement : public Element {
public:
  /**
   * @brief Constructs a rotator element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param path Path or URL to the rotator image.
   */
  RotatorElement(const std::wstring &id, int x, int y,
                 const std::wstring &path);
  virtual ~RotatorElement();

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual bool HitTest(int x, int y) override;
  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;

  virtual void OnOwnerHWNDSet() override;
  virtual void OnImageDownloaded(const std::wstring &url,
                                 const std::vector<BYTE> &buffer) override;
  std::wstring GetImageUrl() const override { return m_RotatorImage.GetPath(); }

  /// @return True if the rotator image loaded successfully.
  bool IsLoaded() const { return m_RotatorImage.IsLoaded(); }

  /// Updates the rotator image source.
  void UpdateImage(const std::wstring &path) { m_RotatorImage.SetPath(path); }

  /// Sets the current value for rotation calculation.
  void SetValue(double value) { m_Value = value; }

  /// Sets the pivot point X offset from center.
  void SetOffsetX(double offsetX) { m_OffsetX = offsetX; }

  /// Sets the pivot point Y offset from center.
  void SetOffsetY(double offsetY) { m_OffsetY = offsetY; }

  /// Sets the starting angle in radians.
  void SetStartAngle(double startAngle) { m_StartAngle = startAngle; }

  /// Sets the total rotation angle in radians (default: 2π for full circle).
  void SetRotationAngle(double rotationAngle) {
    m_RotationAngle = rotationAngle;
  }

  /// Sets the value remainder for fine rotation control.
  void SetValueRemainder(int valueRemainder) {
    m_ValueRemainder = (valueRemainder < 0) ? 0 : valueRemainder;
  }

  /// Sets the minimum value of the range.
  void SetMinValue(double minValue) { m_MinValue = minValue; }

  /// Sets the maximum value of the range.
  void SetMaxValue(double maxValue) {
    m_MaxValue = (maxValue > m_MinValue) ? maxValue : (m_MinValue + 0.001);
  }

  /// Sets the image tint color and opacity.
  void SetImageTint(COLORREF color, BYTE alpha) {
    m_RotatorImage.SetImageTint(color, alpha);
  }

  /// Sets the image opacity (0-255).
  void SetImageAlpha(BYTE alpha) { m_RotatorImage.SetImageAlpha(alpha); }

  /// Enables or disables grayscale rendering.
  void SetGrayscale(bool enable) { m_RotatorImage.SetGrayscale(enable); }

  /// Sets a custom 5x5 color transformation matrix.
  void SetColorMatrix(const float *matrix) {
    m_RotatorImage.SetColorMatrix(matrix);
  }

  /// Enables or disables EXIF orientation handling.
  void SetUseExifOrientation(bool enabled) {
    m_RotatorImage.SetUseExifOrientation(enabled);
  }

  /// Sets the image flip mode.
  void SetImageFlip(ImageFlipMode flip) { m_RotatorImage.SetImageFlip(flip); }

  /// Sets a fallback image path for load failures.
  void SetFallbackPath(const std::wstring &path) {
    m_RotatorImage.SetFallbackPath(path);
  }

  const std::wstring &GetImagePath() const { return m_RotatorImage.GetPath(); }
  const std::wstring &GetFallbackPath() const {
    return m_RotatorImage.GetFallbackPath();
  }
  double GetValue() const { return m_Value; }
  double GetOffsetX() const { return m_OffsetX; }
  double GetOffsetY() const { return m_OffsetY; }
  double GetStartAngle() const { return m_StartAngle; }
  double GetRotationAngle() const { return m_RotationAngle; }
  int GetValueRemainder() const { return m_ValueRemainder; }
  double GetMinValue() const { return m_MinValue; }
  double GetMaxValue() const { return m_MaxValue; }

  bool HasImageTint() const { return m_RotatorImage.HasImageTint(); }
  COLORREF GetImageTint() const { return m_RotatorImage.GetImageTint(); }
  BYTE GetImageTintAlpha() const { return m_RotatorImage.GetImageTintAlpha(); }
  BYTE GetImageAlpha() const { return m_RotatorImage.GetImageAlpha(); }
  bool IsGrayscale() const { return m_RotatorImage.IsGrayscale(); }
  bool GetUseExifOrientation() const {
    return m_RotatorImage.GetUseExifOrientation();
  }
  bool HasColorMatrix() const { return m_RotatorImage.HasColorMatrix(); }
  const float *GetColorMatrix() const {
    return m_RotatorImage.GetColorMatrix();
  }
  ImageFlipMode GetImageFlip() const { return m_RotatorImage.GetImageFlip(); }

private:
  GeneralImage m_RotatorImage; ///< Rotator image source.

  double m_Value = 0.0;        ///< Current value.
  double m_OffsetX = 0.0;     ///< Pivot X offset from center.
  double m_OffsetY = 0.0;     ///< Pivot Y offset from center.
  double m_StartAngle = 0.0;  ///< Starting angle in radians.
  double m_RotationAngle = 6.283185307179586; ///< Total rotation (default: 2π).
  int m_ValueRemainder = 0;   ///< Value remainder for fine control.
  double m_MinValue = 0.0;    ///< Minimum value of the range.
  double m_MaxValue = 1.0;    ///< Maximum value of the range.
};

#endif
