/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Element.h"

#include <string>

/**
 * @brief Draggable slider element with a thumb on a track.
 *
 * Renders a track with a draggable thumb. Press anywhere on the track to
 * jump the thumb to that position, then drag to scrub the value.
 *
 * @note The Widget drives interaction: BeginDrag/UpdateDrag/EndDrag from the
 *       mouse messages and HandleKeyDown from WM_KEYDOWN. Drag moves fire
 *       onInput per snapped-value change; release fires onChange when the
 *       value differs from the drag-start value.
 */
class SliderElement : public Element {
public:
  /// @brief Thumb travel axis.
  enum class Orientation { Horizontal = 0, Vertical = 1 };

  /**
   * @brief Constructs a slider element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels (default 140).
   * @param height Height in pixels (default 26).
   */
  SliderElement(const std::wstring &id, int x, int y, int width = 140,
                int height = 26)
      : Element(ELEMENT_SLIDER, id, x, y, width, height) {}

  // ============================================================================
  // Value model
  // ============================================================================

  double GetValue() const { return m_Value; }

  /// @return Current value clamped/snapped.
  double GetSnappedValue() const { return SnapAndClamp(m_Value); }

  /**
   * @brief Sets the value (clamped to [min,max] and snapped to step).
   *
   * @param value Requested raw value.
   */
  void SetValue(double value);

  /// @brief Sets the range; the current value is re-clamped instantly.
  void SetRange(double minV, double maxV);

  /// @brief Clamps to [min,max] and snaps to the nearest multiple of step
  ///        offset from minValue. Continuous when step <= 0.
  double SnapAndClamp(double v) const;

  // ============================================================================
  // Interaction (driven by Widget)
  // ============================================================================

  /**
   * @brief Starts a drag at the given element-space point, jumping the thumb
   *        to the pointer position.
   *
   * @return True if the resulting value changed (caller may fire onInput).
   */
  bool BeginDrag(int px, int py);

  /// @brief Updates the drag to the given point. @return True on value change.
  bool UpdateDrag(int px, int py);

  /// @brief Ends the drag started by BeginDrag.
  void EndDrag();

  /// @return True while a mouse drag is active.
  bool IsDragging() const { return m_Dragging; }

  /// @return The value captured at BeginDrag, for the release-time commit
  ///         check in Widget.
  double DragStartValue() const { return m_DragStartValue; }

  /**
   * @brief Applies a keyboard step (arrows/Home/End/PageUp/PageDown).
   *
   * @param vk Virtual key code.
   * @param shift Shift held: fine control at 1/10 of step.
   * @param outNewValue Receives the new value when handled and changed.
   * @return True when the key was handled and the value changed.
   */
  bool HandleKeyDown(unsigned int vk, bool shift, double &outNewValue);

  // ============================================================================
  // Geometry helpers
  // ============================================================================

  bool IsVertical() const { return m_Orientation == Orientation::Vertical; }

  /// @brief Formats a value as a plain decimal string ("50", "2.5").
  static std::string FormatValue(double v);

  // ============================================================================
  // Range
  // ============================================================================

  double m_MinValue = 0.0;
  double m_MaxValue = 100.0;
  double m_Step = 1.0; ///< <= 0 disables snapping.
  Orientation m_Orientation = Orientation::Horizontal;

  // ============================================================================
  // Track styling
  // ============================================================================

  float m_TrackThickness = 4.0f;   ///< Short-axis thickness of the track.
  float m_TrackBorderRadius = -1.0f; ///< < 0 = thickness / 2.
  COLORREF m_TrackColor = RGB(63, 63, 70); ///< Unfilled portion.
  BYTE m_TrackAlpha = 255;
  COLORREF m_FillColor = RGB(59, 130, 246); ///< Filled portion.
  BYTE m_FillAlpha = 255;
  float m_SliderOpacity = 1.0f; ///< Overall opacity.

  // ============================================================================
  // Thumb styling
  // ============================================================================

  float m_ThumbSize = 0.0f; ///< Explicit diameter; 0 = auto.
  COLORREF m_ThumbColor = RGB(255, 255, 255);
  BYTE m_ThumbAlpha = 255;
  float m_ThumbBorderWidth = 0.0f;
  COLORREF m_ThumbBorderColor = RGB(0, 0, 0);
  BYTE m_ThumbBorderAlpha = 255;
  bool m_HasHoverThumbColor = false;
  COLORREF m_HoverThumbColor = RGB(0, 0, 0);
  BYTE m_HoverThumbAlpha = 255;
  bool m_HasPressedThumbColor = false;
  COLORREF m_PressedThumbColor = RGB(0, 0, 0);
  BYTE m_PressedThumbAlpha = 255;

  // ============================================================================
  // Disabled & hover states
  // ============================================================================

  bool m_Disabled = false; ///< Ignore input, dim visuals.
  COLORREF m_DisabledTrackColor = RGB(80, 80, 88);
  BYTE m_DisabledTrackAlpha = 255;
  COLORREF m_DisabledFillColor = RGB(80, 80, 88);
  BYTE m_DisabledFillAlpha = 255;
  COLORREF m_DisabledThumbColor = RGB(150, 150, 158);
  BYTE m_DisabledThumbAlpha = 255;
  bool m_Hovered = false; ///< Set by the widget on hover changes.

  // ============================================================================
  // Event callbacks
  // ============================================================================

  int m_OnChangeCallbackId = -1; ///< Final value committed.
  int m_OnInputCallbackId = -1;  ///< Live value during drag.

  void Render(ID2D1DeviceContext *context) override;
  int GetAutoWidth() override { return 140; }
  int GetAutoHeight() override { return 26; }

private:
  struct TrackGeometry {
    float left, top, length, thickness, center, thumbDiameter;
  };
  TrackGeometry GetTrackGeometry();
  double ValueFromPoint(int px, int py);

  double m_Value = 0.0;
  bool m_Dragging = false;
  double m_DragStartValue = 0.0;
};
