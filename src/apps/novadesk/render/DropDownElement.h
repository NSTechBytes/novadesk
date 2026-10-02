/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Element.h"

#include <string>
#include <vector>

/**
 * @brief One entry in a drop-down list.
 *
 * @note label is what the user reads, value is what onChange reports. Authoring
 *       an option as a plain string makes both fields equal.
 */
struct DropDownOption {
  std::wstring label; ///< Text shown in the box and the popup row.
  std::wstring value; ///< Payload delivered to onChange.
};

/**
 * @brief Select-style control whose option list opens in its own window.
 *
 * Renders a bordered box with the current label and a chevron. Clicking it
 * asks the owning Widget to open a DropDownPopup — a separate top-level
 * window, exactly like the color picker editor — where the user picks a row.
 *
 * @note The Widget owns the popup and the interaction: it toggles the popup on
 *       button up, tracks hover, routes keyboard input from WidgetProc and
 *       commits changes through Widget::NotifyDropDownChange. This class only
 *       holds the value model and styling.
 */
class DropDownElement : public Element {
public:
  /**
   * @brief Constructs a drop-down element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels (default 160).
   * @param height Height in pixels (default 28).
   */
  DropDownElement(const std::wstring &id, int x, int y, int width = 160,
                  int height = 28)
      : Element(ELEMENT_DROPDOWN, id, x, y, width, height) {}

  // ============================================================================
  // Value model
  // ============================================================================

  /// @brief Replaces the option list; the selection is re-clamped to fit.
  void SetOptions(const std::vector<DropDownOption> &options);

  /// @return The option list (read-only).
  const std::vector<DropDownOption> &Options() const { return m_Options; }

  /// @return Number of options.
  int OptionCount() const { return static_cast<int>(m_Options.size()); }

  /**
   * @brief Selects an index; -1 clears the selection. Out-of-range values are
   *        clamped into [-1, optionCount-1].
   */
  void SetSelectedIndex(int index);

  /// @return Current index, or -1 when nothing is selected.
  int GetSelectedIndex() const { return m_SelectedIndex; }

  /// @return The selected option's value, or an empty string when unselected.
  std::wstring SelectedValue() const;

  /// @return The selected option's label, or an empty string when unselected.
  std::wstring SelectedLabel() const;

  /**
   * @brief Resolves a value string to its first matching option index.
   *
   * @param value Option value to look for.
   * @return Matching index, or -1 when no option carries that value.
   */
  int IndexForValue(const std::wstring &value) const;

  /// @return Text to paint in the box: the selected label, else placeholder.
  std::wstring DisplayText() const;

  // ============================================================================
  // Geometry helpers
  // ============================================================================

  /// @brief Truncates text to maxChars and appends an ellipsis (0 = no limit).
  static std::wstring Ellipsize(const std::wstring &text, UINT maxChars);

  // ============================================================================
  // Choices
  // ============================================================================

  std::vector<DropDownOption> m_Options;
  int m_SelectedIndex = -1;          ///< -1 = nothing chosen.
  std::wstring m_Placeholder;        ///< Shown dimmed while unselected.
  UINT m_MaxDisplayLength = 0;       ///< 0 = no character cap.

  // ============================================================================
  // Box styling
  // ============================================================================

  COLORREF m_BackgroundColor = RGB(45, 45, 54);
  BYTE m_BackgroundAlpha = 255;
  float m_BorderWidth = 1.0f;
  COLORREF m_BorderColor = RGB(63, 63, 70);
  BYTE m_BorderAlpha = 255;
  float m_BorderRadius = 6.0f;
  float m_DropdownOpacity = 1.0f; ///< Overall opacity.
  float m_PaddingLeft = 10.0f;
  float m_PaddingRight = 10.0f;

  // ============================================================================
  // Chevron
  // ============================================================================

  COLORREF m_ChevronColor = RGB(200, 200, 208);
  BYTE m_ChevronAlpha = 255;
  float m_ChevronSize = 6.0f;   ///< Half-width of the V.
  float m_ChevronGap = 10.0f;   ///< Space between text and chevron.

  // ============================================================================
  // Text
  // ============================================================================

  std::wstring m_FontFace = L"Segoe UI";
  int m_FontSize = 12;
  int m_FontWeight = 400;
  COLORREF m_FontColor = RGB(228, 228, 231);
  BYTE m_FontAlpha = 255;
  COLORREF m_PlaceholderColor = RGB(140, 140, 150);
  BYTE m_PlaceholderAlpha = 255;

  // ============================================================================
  // States
  // ============================================================================

  bool m_Disabled = false; ///< Ignore input, dim visuals.
  bool m_Hovered = false;  ///< Set by the widget on hover changes.
  /// @brief True while this element owns the open popup. The widget sets it in
  /// its hover fan-out; the element cannot ask, since it holds no widget
  /// pointer.
  bool m_Open = false;
  COLORREF m_HoverBorderColor = RGB(90, 90, 104);
  BYTE m_HoverBorderAlpha = 255;
  COLORREF m_OpenBorderColor = RGB(59, 130, 246);
  BYTE m_OpenBorderAlpha = 255;
  COLORREF m_DisabledBackgroundColor = RGB(40, 40, 48);
  BYTE m_DisabledBackgroundAlpha = 255;
  COLORREF m_DisabledBorderColor = RGB(60, 60, 68);
  BYTE m_DisabledBorderAlpha = 255;
  COLORREF m_DisabledTextColor = RGB(115, 115, 125);
  BYTE m_DisabledTextAlpha = 255;

  // ============================================================================
  // Popup styling (consumed by DropDownPopup)
  // ============================================================================

  COLORREF m_PopupBackground = RGB(36, 36, 44);
  BYTE m_PopupBackgroundAlpha = 255;
  COLORREF m_PopupBorderColor = RGB(63, 63, 70);
  BYTE m_PopupBorderAlpha = 255;
  COLORREF m_PopupHoverColor = RGB(52, 52, 64);
  BYTE m_PopupHoverAlpha = 255;
  COLORREF m_PopupSelectedColor = RGB(59, 130, 246);
  BYTE m_PopupSelectedAlpha = 255;
  COLORREF m_PopupTextColor = RGB(228, 228, 231);
  BYTE m_PopupTextAlpha = 255;
  COLORREF m_PopupCheckColor = RGB(59, 130, 246);
  BYTE m_PopupCheckAlpha = 255;
  int m_PopupItemHeight = 28;    ///< Row height in pixels.
  int m_PopupMaxVisibleItems = 8;///< Rows before the list scrolls.
  int m_PopupPadding = 4;        ///< Inner padding in pixels.

  // ============================================================================
  // Event callbacks
  // ============================================================================

  int m_OnChangeCallbackId = -1; ///< Selection committed.
  int m_OnOpenCallbackId = -1;   ///< Popup opened.
  int m_OnCloseCallbackId = -1;  ///< Popup dismissed.
  int m_OnCancelCallbackId = -1; ///< Escape / cancel while open.

  void Render(ID2D1DeviceContext *context) override;
  int GetAutoWidth() override { return 160; }
  int GetAutoHeight() override { return 28; }
};
