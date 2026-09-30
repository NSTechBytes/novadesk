/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Element.h"

/**
 * @brief On/off switch element with an animated sliding knob.
 *
 * @note A left click toggles the state by itself (no JS callback required);
 *       the engine then fires the onChange callback or routes the change to
 *       the widget's input sink. Knob motion is tweened on a widget timer.
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

  /// @brief Sets the checked state, optionally sliding the knob.
  void SetChecked(bool checked, bool animate);

  /// @brief Flips the checked state with animation per durationMs.
  void Toggle();

  /// @brief Advances the knob tween; call from the widget animation timer.
  /// @return True while an animation is still running.
  bool StepAnimation();

  /// @return True if the knob is currently animating.
  bool IsAnimating() const { return m_Animating; }

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
  // Labels
  // ============================================================================

  std::wstring m_OnText;  ///< Text drawn inside the track when checked.
  std::wstring m_OffText; ///< Text drawn inside the track when unchecked.
  std::wstring m_LabelFontFace = L"Segoe UI";
  int m_LabelFontSize = 10;
  int m_LabelFontWeight = 600;
  COLORREF m_LabelFontColor = RGB(255, 255, 255);
  BYTE m_LabelFontAlpha = 255;

  // ============================================================================
  // Animation
  // ============================================================================

  int m_DurationMs = 180;                ///< Knob slide duration; 0 = instant.
  std::wstring m_Easing = L"ease-out";   ///< AnimationEasing function name.

  /// @brief Pixel snap applied to the resting knob center (set when an
  ///        animation finishes) so the final frame matches a fresh paint.
  float m_KnobOffsetPx = 0.0f;

  // ============================================================================
  // Event Callbacks
  // ============================================================================

  int m_OnChangeCallbackId = -1; ///< Checked state changed.

  void Render(ID2D1DeviceContext *context) override;
  int GetAutoWidth() override { return 48; }
  int GetAutoHeight() override { return 26; }

private:
  bool m_Checked = false;
  bool m_Animating = false;
  float m_KnobProgress = 0.0f; ///< Rendered knob position 0.0 (off)..1.0 (on).
  float m_AnimFrom = 0.0f;
  float m_AnimTo = 1.0f;
  DWORD m_AnimStartTick = 0;
};
