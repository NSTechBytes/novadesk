/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_BITMAP_ELEMENT_H__
#define __NOVADESK_BITMAP_ELEMENT_H__

#include "Element.h"
#include "GeneralImage.h"

/**
 * @brief Horizontal alignment of bitmap frames.
 */
enum BitmapAlign {
  BITMAP_ALIGN_LEFT = 0,   ///< Align frames to the left.
  BITMAP_ALIGN_CENTER = 1, ///< Center frames horizontally.
  BITMAP_ALIGN_RIGHT = 2   ///< Align frames to the right.
};

/**
 * @brief Multi-frame bitmap element for numeric displays (e.g., counters).
 *
 * @note Supports horizontal/vertical frame strips, digit mapping, and
 *       auto-orientation detection from image dimensions.
 */
class BitmapElement : public Element {
public:
  /**
   * @brief Constructs a bitmap element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param path Path or URL to the bitmap image.
   */
  BitmapElement(const std::wstring &id, int x, int y, const std::wstring &path);
  virtual ~BitmapElement();

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;

  virtual void OnOwnerHWNDSet() override;
  virtual void OnImageDownloaded(const std::wstring &url,
                                 const std::vector<BYTE> &buffer) override;
  std::wstring GetImageUrl() const override { return m_BitmapImage.GetPath(); }

  /// @return True if the bitmap image loaded successfully.
  bool IsLoaded() const { return m_BitmapImage.IsLoaded(); }

  /// Updates the bitmap image source.
  void UpdateImage(const std::wstring &path) { m_BitmapImage.SetPath(path); }

  /// Sets the current numeric value for frame selection.
  void SetValue(double value);

  /// Sets the total number of frames in the bitmap strip.
  void SetBitmapFrames(int frameCount);

  /// Enables zero-frame mode (first frame represents zero).
  void SetBitmapZeroFrame(bool zeroFrame) { m_ZeroFrame = zeroFrame; }

  /// Enables extend mode (repeat last frame for overflow values).
  void SetBitmapExtend(bool extend) { m_Extend = extend; }

  /// Sets the minimum value for the range.
  void SetMinValue(double minValue) { m_MinValue = minValue; }

  /// Sets the maximum value for the range.
  void SetMaxValue(double maxValue) {
    m_MaxValue = (maxValue > m_MinValue) ? maxValue : (m_MinValue + 0.001);
  }

  /// Sets the number of decimal digits to display.
  void SetBitmapDigits(int digits) { m_Digits = (digits < 0) ? 0 : digits; }

  /// Sets the frame strip orientation ("horizontal" or "vertical").
  void SetBitmapOrientation(const std::wstring &orientation);

  /// Sets the horizontal alignment of frames.
  void SetBitmapAlign(BitmapAlign align) { m_Align = align; }

  /// Sets the pixel separation between digits.
  void SetBitmapSeparation(int separation) { m_Separation = separation; }

  /// Sets the image tint color and opacity.
  void SetImageTint(COLORREF color, BYTE alpha) {
    m_BitmapImage.SetImageTint(color, alpha);
  }

  /// Sets the image opacity (0-255).
  void SetImageAlpha(BYTE alpha) { m_BitmapImage.SetImageAlpha(alpha); }

  /// Enables or disables grayscale rendering.
  void SetGrayscale(bool enable) { m_BitmapImage.SetGrayscale(enable); }

  /// Sets a custom 5x5 color transformation matrix.
  void SetColorMatrix(const float *matrix) {
    m_BitmapImage.SetColorMatrix(matrix);
  }

  /// Enables or disables EXIF orientation handling.
  void SetUseExifOrientation(bool enabled) {
    m_BitmapImage.SetUseExifOrientation(enabled);
  }

  /// Sets the image flip mode.
  void SetImageFlip(ImageFlipMode flip) { m_BitmapImage.SetImageFlip(flip); }

  /// Sets a fallback image path for load failures.
  void SetFallbackPath(const std::wstring &path) {
    m_BitmapImage.SetFallbackPath(path);
  }

  const std::wstring &GetImagePath() const { return m_BitmapImage.GetPath(); }
  const std::wstring &GetFallbackPath() const {
    return m_BitmapImage.GetFallbackPath();
  }
  double GetValue() const { return m_Value; }
  int GetBitmapFrames() const { return m_FrameCount; }
  bool GetBitmapZeroFrame() const { return m_ZeroFrame; }
  bool GetBitmapExtend() const { return m_Extend; }
  double GetMinValue() const { return m_MinValue; }
  double GetMaxValue() const { return m_MaxValue; }
  int GetBitmapDigits() const { return m_Digits; }
  std::wstring GetBitmapOrientation() const;
  BitmapAlign GetBitmapAlign() const { return m_Align; }
  int GetBitmapSeparation() const { return m_Separation; }

  bool HasImageTint() const { return m_BitmapImage.HasImageTint(); }
  COLORREF GetImageTint() const { return m_BitmapImage.GetImageTint(); }
  BYTE GetImageTintAlpha() const { return m_BitmapImage.GetImageTintAlpha(); }
  BYTE GetImageAlpha() const { return m_BitmapImage.GetImageAlpha(); }
  bool IsGrayscale() const { return m_BitmapImage.IsGrayscale(); }
  bool GetUseExifOrientation() const {
    return m_BitmapImage.GetUseExifOrientation();
  }
  bool HasColorMatrix() const { return m_BitmapImage.HasColorMatrix(); }
  const float *GetColorMatrix() const { return m_BitmapImage.GetColorMatrix(); }
  ImageFlipMode GetImageFlip() const { return m_BitmapImage.GetImageFlip(); }

private:
  bool ComputeFrameGeometry(float imageWidth, float imageHeight,
                            bool &verticalFrames, float &frameWidth,
                            float &frameHeight) const;
  int GetRealFrameCount() const;
  int GetDigitCountForValue(long long value, int realFrames) const;
  int ResolveFrameForNormalizedValue(double value) const;
  int ResolveFrameForDigit(int digit);

private:
  GeneralImage m_BitmapImage; ///< Bitmap image source.

  double m_Value = 0.0;                    ///< Current numeric value.
  int m_FrameCount = 1;                    ///< Number of frames in the strip.
  bool m_ZeroFrame = false;                ///< First frame represents zero.
  bool m_Extend = false;                   ///< Repeat last frame for overflow.
  double m_MinValue = 0.0;                 ///< Minimum value of the range.
  double m_MaxValue = 1.0;                 ///< Maximum value of the range.
  int m_Digits = 0;                        ///< Number of decimal digits.
  BitmapAlign m_Align = BITMAP_ALIGN_LEFT; ///< Frame alignment.
  int m_Separation = 0;                    ///< Pixel separation between digits.
  bool m_Vertical = false;       ///< True if frames are stacked vertically.
  bool m_AutoOrientation = true; ///< Auto-detect orientation from image.
};

#endif
