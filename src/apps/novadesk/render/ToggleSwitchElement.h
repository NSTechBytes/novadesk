/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Element.h"

/**
 * @brief On/off switch element with a sliding knob.
 *
 * @note A left click toggles the state by itself (no JS callback required);
 *       the engine then fires the onChange callback or routes the change to
 *       the widget's input sink. The knob jumps straight to its resting
 *       position.
 */
class ToggleSwitchElement : public Element {
public:
  /**
   * @brief Constructs a toggle switch element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels (default 48).
   * @param height Height in pixels (default 26).
   */
  ToggleSwitchElement(const std::wstring &id, int x, int y, int width = 48,
                      int height = 26)
      : Element(ELEMENT_TOGGLE_SWITCH, id, x, y, width, height) {}

  // ============================================================================
  // State
  // ============================================================================

  bool IsChecked() const { return m_Checked; }

  /// @brief Sets the checked state.
  void SetChecked(bool checked);

  /// @brief Flips the checked state.
  void Toggle();

  // ============================================================================
  // Track Styling
  // ============================================================================

  COLORREF m_OnColor = RGB(59, 130, 246);  ///< Track fill when checked.
  BYTE m_OnAlpha = 255;
  COLORREF m_OffColor = RGB(63, 63, 70);   ///< Track fill when unchecked.
  BYTE m_OffAlpha = 255;
  float m_BorderWidth = 0.0f;              ///< Track border width.
  COLORREF m_BorderColor = RGB(0, 0, 0);   ///< Track border color.
  BYTE m_BorderAlpha = 255;
  float m_BorderRadius = -1.0f; ///< Track corner radius; < 0 = height / 2.
  float m_Opacity = 1.0f;       ///< Overall opacity.

  // ============================================================================
  // Knob Styling
  // ============================================================================

  COLORREF m_KnobColor = RGB(255, 255, 255); ///< Knob fill color.
  BYTE m_KnobAlpha = 255;
  float m_KnobBorderWidth = 0.0f;         ///< Knob outline width.
  COLORREF m_KnobBorderColor = RGB(0, 0, 0);
  BYTE m_KnobBorderAlpha = 255;
  float m_KnobSize = 0.0f;    ///< Explicit knob diameter; 0 = auto.
  float m_KnobPadding = 2.0f; ///< Gap between knob and track edge.

  // ============================================================================
  // Disabled & Hover States
  // ============================================================================

  bool m_Disabled = false;                  ///< Ignore clicks, dim visuals.
  COLORREF m_DisabledTrackColor = RGB(80, 80, 88);
  BYTE m_DisabledTrackAlpha = 255;
  COLORREF m_DisabledKnobColor = RGB(150, 150, 158);
  BYTE m_DisabledKnobAlpha = 255;
  bool m_HasHoverTrackColor = false;        ///< Hover tint enabled.
  COLORREF m_HoverTrackColor = RGB(0, 0, 0);
  BYTE m_HoverTrackAlpha = 255;
  bool m_Hovered = false; ///< Set by the widget on hover changes.

  // ============================================================================
  // Event Callbacks
  // ============================================================================

  int m_OnChangeCallbackId = -1; ///< Checked state changed.

  void Render(ID2D1DeviceContext *context) override;
  int GetAutoWidth() override { return 48; }
  int GetAutoHeight() override { return 26; }

private:
  bool m_Checked = false;
};
