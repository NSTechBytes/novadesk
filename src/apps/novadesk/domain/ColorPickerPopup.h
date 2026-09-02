/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <windows.h>
#include <vector>

class Widget;
class ColorPickerElement;

/**
 * @brief Popup color picker dialog with HSV/RGB editing and eyedropper.
 *
 * @note Renders a custom Win32 popup with saturation/value gradient, hue strip,
 *       RGB/HEX input fields, and an eyedropper tool with magnifier preview.
 */
class ColorPickerPopup {
public:
  /**
   * @brief Constructs a color picker popup.
   *
   * @param widget The owning widget.
   * @param picker The color picker element that opened this popup.
   */
  ColorPickerPopup(Widget *widget, ColorPickerElement *picker);
  ~ColorPickerPopup();

  /// Shows the color picker popup.
  void Show();

  /// Closes the color picker popup.
  void Close();

  /// @return True if the popup is currently open.
  bool IsOpen() const { return m_hWnd != nullptr; }

  /// @return True if the eyedropper tool is active.
  bool IsEyedropperActive() const { return m_eye; }

  /// Activates the eyedropper tool.
  void StartEyedropper();

  /// @return The associated color picker element.
  ColorPickerElement *GetPickerElement() const { return m_Picker; }

private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK MagnifierWndProc(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK OutsideClickMouseHook(int, WPARAM, LPARAM);
  LRESULT Handle(UINT, WPARAM, LPARAM);

  void Paint(HDC);
  void EnsureSaturationValueBitmap();
  void SetHSV(float h, float s, float v, bool notify = true);
  void SetRGB(COLORREF color, bool notify = true);
  void SyncEdits();
  void SetHexMode(bool enabled);
  void Notify();
  void FlushWidgetRedraw();
  void ShowEyedropperMagnifier(POINT screenPosition);
  void HideEyedropperMagnifier();
  void PaintEyedropperMagnifier(HDC);
  void UpdateEyedropperSample(POINT screenPosition);
  void ApplyEyedropperSelection(POINT screenPosition);
  void InstallOutsideClickHook();
  void RemoveOutsideClickHook();
  COLORREF HSV() const;

  Widget *m_Widget;             ///< Owning widget.
  ColorPickerElement *m_Picker; ///< Associated color picker element.
  HWND m_hWnd = nullptr;        ///< Popup window handle.

  // Input fields
  HWND m_R = nullptr;   ///< Red input field.
  HWND m_G = nullptr;   ///< Green input field.
  HWND m_B = nullptr;   ///< Blue input field.
  HWND m_Hex = nullptr; ///< Hex color input field.

  // Magnifier
  HWND m_Magnifier = nullptr; ///< Eyedropper magnifier window.
  HDC m_MagnifierFrameDc = nullptr;
  HBITMAP m_MagnifierFrameBitmap = nullptr;
  HGDIOBJ m_MagnifierFrameOldBitmap = nullptr;

  HFONT m_Font = nullptr;          ///< UI font.
  HBRUSH m_InputBgBrush = nullptr; ///< Input field background brush.
  COLORREF m_CachedInputBgColor = CLR_INVALID;

  // HSV state
  float m_H = 0, m_S = 0, m_V = 0; ///< Current HSV values.

  // Drag state
  bool m_dragSV = false;  ///< Dragging saturation/value area.
  bool m_dragHue = false; ///< Dragging hue strip.
  bool m_eye = false;     ///< Eyedropper active.
  bool m_eyeAwaitingFirstRelease = false;
  bool m_sync = false; ///< Suppresses recursive sync.

  float m_SaturationValueHue = -1.0f;         ///< Cached hue for SV bitmap.
  std::vector<DWORD> m_SaturationValuePixels; ///< Cached SV gradient pixels.

  bool m_WidgetNeedsRedraw = false; ///< Pending widget redraw flag.
  bool m_HexMode = false;           ///< HEX input mode (vs RGBA).
  int m_EditingControl = 0;         ///< Currently focused input field.
  bool m_FormatHover = false;       ///< Hovering over format toggle.

  bool m_IgnoreEyedropperFocusLoss = false;
  POINT m_LastSampledPos = {-1, -1}; ///< Last eyedropper sample position.
  COLORREF m_LastSampledColor = CLR_INVALID;
  COLORREF m_OriginalColor = RGB(0, 0, 0); ///< Color when popup opened.
  bool m_Canceled = false;                 ///< True if user canceled selection.

  /// Show Desktop state when popup opened (to detect transitions).
  bool m_ShowDesktopWasActive = false;
};
