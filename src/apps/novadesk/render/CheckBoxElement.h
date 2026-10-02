/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Element.h"

/**
 * @brief Check box element with a vector checkmark and optional label.
 *
 * @note A left click cycles the state unchecked -> checked -> indeterminate
 *       by itself (no JS callback required); the engine then fires the
 *       onChange callback or routes the change to the widget's input sink.
 */
class CheckBoxElement : public Element {
public:
  /// @brief Tri-state value stored internally.
  enum class State { Unchecked = 0, Checked = 1, Indeterminate = 2 };

  /**
   * @brief Constructs a check box element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels (default 16).
   * @param height Height in pixels (default 22).
   */
  CheckBoxElement(const std::wstring &id, int x, int y, int width = 16,
                  int height = 22)
      : Element(ELEMENT_CHECK_BOX, id, x, y, width, height) {}

  // ============================================================================
  // State
  // ============================================================================

  State GetState() const { return m_State; }

  /// @brief When false (default), Toggle flips checked/unchecked only;
  ///        indeterminate stays reachable programmatically.
  bool m_TriState = false;

  /// @brief True for both Checked and Indeterminate.
  bool IsChecked() const { return m_State != State::Unchecked; }

  /// @brief Sets the state.
  void SetState(State state);

  /// @brief Convenience: sets Checked/Unchecked from a bool.
  void SetChecked(bool checked) {
    SetState(checked ? State::Checked : State::Unchecked);
  }

  /// @brief Advances unchecked -> checked -> indeterminate -> unchecked.
  void Toggle();

  // ============================================================================
  // Box Styling
  // ============================================================================

  float m_BoxSize = 0.0f; ///< Explicit square side; 0 = min(width, height).
  float m_BorderRadius = 3.0f; ///< Box corner radius.
  COLORREF m_UncheckedBorderColor = RGB(113, 113, 122);
  BYTE m_UncheckedBorderAlpha = 255;
  float m_UncheckedBorderWidth = 1.5f;
  COLORREF m_CheckedColor = RGB(59, 130, 246); ///< Box fill when non-empty.
  BYTE m_CheckedAlpha = 255;
  float m_BoxOpacity = 1.0f; ///< Overall opacity of box + mark.

  // ============================================================================
  // Mark Styling
  // ============================================================================

  COLORREF m_CheckColor = RGB(255, 255, 255); ///< Checkmark/dash stroke.
  BYTE m_CheckAlpha = 255;
  float m_CheckThickness = 2.0f; ///< Stroke width of the mark.

  // ============================================================================
  // Disabled & Hover States
  // ============================================================================

  bool m_Disabled = false; ///< Ignore clicks, dim visuals.
  COLORREF m_DisabledBoxColor = RGB(80, 80, 88);
  BYTE m_DisabledBoxAlpha = 255;
  COLORREF m_DisabledCheckColor = RGB(150, 150, 158);
  BYTE m_DisabledCheckAlpha = 255;
  bool m_HasHoverBorderColor = false; ///< Hover border enabled.
  COLORREF m_HoverBorderColor = RGB(0, 0, 0);
  BYTE m_HoverBorderAlpha = 255;
  bool m_Hovered = false; ///< Set by the widget on hover changes.

  // ============================================================================
  // Event Callbacks
  // ============================================================================

  int m_OnChangeCallbackId = -1; ///< State changed.

  void Render(ID2D1DeviceContext *context) override;
  int GetAutoWidth() override { return 16; }
  int GetAutoHeight() override { return 22; }

private:
  State m_State = State::Unchecked;
};
