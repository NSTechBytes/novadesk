/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Element.h"

/**
 * @brief Color picker element with swatch display and popup editor.
 *
 * @note Supports eyedropper, hex/rgba format toggle, and customizable
 *       popup appearance. The popup is managed by ColorPickerPopup.
 */
class ColorPickerElement : public Element {
public:
  /**
   * @brief Constructs a color picker element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels (default 32).
   * @param height Height in pixels (default 32).
   */
  ColorPickerElement(const std::wstring &id, int x, int y, int width = 32,
                     int height = 32)
      : Element(ELEMENT_COLOR_PICKER, id, x, y, width, height) {}

  // ============================================================================
  // Color
  // ============================================================================

  /// Sets the selected color.
  void SetColor(COLORREF color) { m_Color = color; }

  /// @return The currently selected color.
  COLORREF GetColor() const { return m_Color; }

  // ============================================================================
  // Swatch Styling
  // ============================================================================

  float m_BorderRadius = 0.0f;     ///< Corner radius of the swatch.
  float m_BorderWidth = 0.0f;      ///< Border width around the swatch.
  COLORREF m_BorderColor = RGB(0, 0, 0); ///< Border color.
  BYTE m_BorderAlpha = 255;        ///< Border opacity.
  float m_Opacity = 1.0f;          ///< Overall swatch opacity.
  bool m_CircleShape = false;      ///< True = elliptical swatch shape.

  // ============================================================================
  // Popup Appearance
  // ============================================================================

  COLORREF m_PopupBackground = RGB(255, 255, 255); ///< Popup background color.
  BYTE m_PopupBackgroundAlpha = 255;
  COLORREF m_PopupAccentColor = RGB(0, 0, 0); ///< Popup accent color.
  BYTE m_PopupAccentAlpha = 255;
  COLORREF m_PopupBorderColor = RGB(0, 0, 0); ///< Popup border color.
  BYTE m_PopupBorderAlpha = 255;
  COLORREF m_PopupInputBackground = RGB(255, 255, 255); ///< Input field background.
  BYTE m_PopupInputBackgroundAlpha = 255;
  bool m_HasPopupInputBackground = false;
  COLORREF m_PopupInputColor = RGB(0, 0, 0); ///< Input field text color.
  BYTE m_PopupInputColorAlpha = 255;
  bool m_HasPopupInputColor = false;

  // ============================================================================
  // Popup Behavior
  // ============================================================================

  bool m_ShowEyedropper = true;     ///< Show eyedropper tool in popup.
  bool m_ShowFormatToggle = true;   ///< Show format toggle (hex/rgba).
  bool m_DefaultHexMode = false;    ///< Open in HEX mode by default.

  // ============================================================================
  // Event Callbacks
  // ============================================================================

  int m_OnChangeCallbackId = -1;            ///< Color value changed.
  int m_OnOpenCallbackId = -1;              ///< Popup opened.
  int m_OnCloseCallbackId = -1;             ///< Popup closed.
  int m_OnCancelCallbackId = -1;            ///< User canceled selection.
  int m_OnEyedropperOpenCallbackId = -1;    ///< Eyedropper activated.
  int m_OnEyedropperPickCallbackId = -1;    ///< Eyedropper color picked.

  void Render(ID2D1DeviceContext *context) override;
  int GetAutoWidth() override { return 32; }
  int GetAutoHeight() override { return 32; }

private:
  COLORREF m_Color = RGB(0, 0, 0); ///< Currently selected color.
};
