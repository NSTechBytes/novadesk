/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_OPTION_LIST_POPUP_H__
#define __NOVADESK_OPTION_LIST_POPUP_H__

#include <windows.h>
#include <string>
#include <vector>
#include "../render/OptionListHost.h"

class Widget;

/**
 * @brief Unified option list popup window for DropDown and ComboBox elements.
 *
 * Rendered as an owner-less-child WS_POPUP owned by the widget HWND with
 * WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE.
 * Painted with double-buffered GDI plus a transient Direct2D layer for the
 * scrollbar and check marks. Dismissed by an outside-click low-level hook,
 * focus loss, or Escape.
 */
class OptionListPopup {
public:
  OptionListPopup(Widget *widget, IOptionListHost *host);
  ~OptionListPopup();

  /// Shows the popup, sizing it from the options and anchor element.
  void Show();

  /// Closes the popup and releases its resources.
  void Close();

  /// Updates position and size when the owning widget moves or resizes.
  void UpdatePosition();

  /// Filters displayed options according to query string (case-insensitive).
  void FilterBy(const std::wstring &query);

  /// Moves the keyboard cursor, optionally scrolling into view.
  void SetHighlight(int index, bool scrollIntoView = true);

  /// Commits the currently highlighted row into the element.
  void CommitHighlighted();

  /// Scrolls the list by the given number of rows.
  void ScrollBy(int rows);

  /// Scrolls a row into view without disturbing the keyboard cursor.
  void ScrollIntoView(int index);

  /// Selects and optionally previews row from keyboard.
  void SelectFromKeyboard(int index);

  /// @return True if the popup window exists and is visible.
  bool IsOpen() const { return m_hWnd != nullptr; }

  /// @return The host interface.
  IOptionListHost *GetHost() const { return m_Host; }

  /// @return The underlying element.
  Element *GetElement() const;

  /// Number of currently displayed (filtered) options.
  int FilteredCount() const { return static_cast<int>(m_FilteredOptions.size()); }

  /// Highlighted index in filtered list.
  int GetHighlight() const { return m_Highlight; }

private:
  static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
  static LRESULT CALLBACK OutsideClickMouseHook(int, WPARAM, LPARAM);
  LRESULT Handle(UINT, WPARAM, LPARAM);

  void Paint(HDC);
  void LayoutRows();
  int RowIndexAt(POINT clientPt) const;
  void ApplyHighlighted();
  void InstallOutsideClickHook();
  void RemoveOutsideClickHook();
  void FlushWidgetRedraw();
  bool ElementStillValid() const;

  HWND m_hWnd = nullptr;
  Widget *m_Widget = nullptr;
  IOptionListHost *m_Host = nullptr;
  HFONT m_Font = nullptr;

  std::vector<DropDownOption> m_FilteredOptions;
  std::vector<int> m_OriginalIndices; // Map from filtered index to full options index

  int m_Highlight = -1;   ///< Hover/keyboard cursor row in filtered list.
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
  int m_OriginalIndex = -1;      ///< Tracks current committed selection (for Paint highlight).
  int m_EscapeIndex = -1;        ///< Selection when popup first opened (for Escape restore).
  bool m_ShowDesktopWasActive = false;
  std::wstring m_CurrentFilter;
};

#endif // __NOVADESK_OPTION_LIST_POPUP_H__
