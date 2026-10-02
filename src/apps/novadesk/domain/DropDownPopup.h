/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once
#include <windows.h>

class Widget;
class DropDownElement;

/**
 * @brief Option list for a drop-down element, shown as its own top-level
 * window.
 *
 * Follows the color picker popup: an owner-less-child WS_POPUP owned by the
 * widget HWND, painted with GDI plus a transient Direct2D layer for the check
 * mark and scrollbar, and dismissed by an outside-click low-level hook, focus
 * loss or Escape.
 *
 * @note The popup holds raw pointers to the widget and element. Every re-entry
 *       point re-validates them, because a JS callback fired from here may
 *       remove elements or destroy the widget.
 */
class DropDownPopup {
public:
  /**
   * @brief Constructs a drop-down popup.
   *
   * @param widget The owning widget.
   * @param dropDown The drop-down element that opened this popup.
   */
  DropDownPopup(Widget *widget, DropDownElement *dropDown);
  ~DropDownPopup();

  /// Shows the popup, sizing it from the option list.
  void Show();

  /// Closes the popup and releases its resources.
  void Close();

  /// Updates position and size when the owning widget moves or resizes.
  void UpdatePosition();

  /**
   * @brief Moves the keyboard cursor, optionally scrolling it into view.
   *
   * Ignored while the pointer is driving the highlight, so hover and keyboard
   * never fight over the same row.
   */
  void SetHighlight(int index, bool scrollIntoView);

  /// @return True if the popup window exists.
  bool IsOpen() const { return m_hWnd != nullptr; }

  /**
   * @brief Scrolls a row into view without disturbing the keyboard cursor.
   *
   * Used when a scripted selection changes behind an open menu: the window must
   * show the new value, but the user's own highlight stays where it was.
   */
  void ScrollIntoView(int index) {
    if (index < 0 || !m_Scrollable)
      return;
    const int before = m_Highlight;
    SetHighlight(index, true);
    m_Highlight = before;
  }

  /// @return The associated drop-down element.
  DropDownElement *GetDropDownElement() const { return m_DropDown; }

private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK OutsideClickMouseHook(int, WPARAM, LPARAM);
  LRESULT Handle(UINT, WPARAM, LPARAM);

  void Paint(HDC);
  void LayoutRows();
  int RowIndexAt(POINT clientPt) const;
  void ScrollBy(int rows);
  void CommitHighlighted();
  void SelectFromKeyboard(int index);
  void InstallOutsideClickHook();
  void RemoveOutsideClickHook();
  void FlushWidgetRedraw();
  bool ElementStillValid() const;

  HWND m_hWnd = nullptr;      ///< Popup window handle.
  Widget *m_Widget;           ///< Owning widget.
  DropDownElement *m_DropDown; ///< Associated drop-down element.
  HFONT m_Font = nullptr;     ///< Row font.

  int m_Highlight = -1;   ///< Hover/keyboard cursor row.
  int m_FirstVisible = 0; ///< Scroll offset in rows.
  int m_RowHeight = 28;   ///< Row height in pixels.
  int m_VisibleRows = 8;  ///< Rows that fit without scrolling.
  int m_Width = 160;      ///< Popup width in pixels.
  int m_Height = 0;       ///< Popup height in pixels.
  int m_Padding = 4;      ///< Inner padding in pixels.
  bool m_Scrollable = false; ///< More rows than the window shows.

  bool m_MouseActive = false;    ///< Mouse drives the highlight, not keys.
  bool m_DraggingThumb = false;  ///< Pointer grabbed the scrollbar thumb.
  float m_ThumbDragOffset = 0.0f;
  bool m_Canceled = false;       ///< Escape/close-without-commit.
  bool m_Closing = false;        ///< Re-entrancy guard for Close().
  bool m_WidgetNeedsRedraw = false;
  int m_OriginalIndex = -1;      ///< Selection when the popup opened.
  bool m_ShowDesktopWasActive = false;
};
