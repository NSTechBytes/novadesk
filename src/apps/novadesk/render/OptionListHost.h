/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_OPTION_LIST_HOST_H__
#define __NOVADESK_OPTION_LIST_HOST_H__

#include <windows.h>
#include <string>
#include <vector>

class Element;

/**
 * @brief Represents a single selectable choice in an option list.
 */
struct DropDownOption {
  std::wstring label;
  std::wstring value;
};

/**
 * @brief Visual styling configuration for the option list popup window.
 */
struct OptionListStyle {
  COLORREF background = RGB(36, 36, 44);
  BYTE backgroundAlpha = 255;
  COLORREF borderColor = RGB(63, 63, 70);
  BYTE borderAlpha = 255;
  COLORREF hoverColor = RGB(52, 52, 64);
  BYTE hoverAlpha = 255;
  COLORREF selectedColor = RGB(59, 130, 246);
  BYTE selectedAlpha = 255;
  COLORREF textColor = RGB(228, 228, 231);
  BYTE textAlpha = 255;
  COLORREF checkColor = RGB(59, 130, 246);
  BYTE checkAlpha = 255;
  int itemHeight = 28;
  int maxVisibleItems = 8;
  int padding = 4;
  int minWidth = 0;
  bool rightToLeft = false;
  std::wstring fontFace = L"Segoe UI";
  int fontSize = 12;
  int fontWeight = 400;
};

/**
 * @brief Interface implemented by elements that host an option list popup window
 *        (e.g., DropDownElement and ComboBoxElement).
 */
class IOptionListHost {
public:
  virtual ~IOptionListHost() = default;

  /// Pointer to underlying element (for identity and bounds calculations).
  virtual Element *GetElement() = 0;

  /// Full list of options.
  virtual const std::vector<DropDownOption> &GetOptions() const = 0;

  /// Current selected index in full options list (-1 if none).
  virtual int GetSelectedIndex() const = 0;

  /// Popup visual style.
  virtual OptionListStyle GetOptionListStyle() const = 0;

  /// Commit callback when user clicks a row or presses Enter.
  virtual void OnOptionCommitted(int originalIndex) = 0;

  /// Optional preview callback when user highlights a row with arrow keys.
  virtual void OnOptionHighlighted(int originalIndex) {}

  /// Cancel callback (e.g. Escape pressed without commit).
  virtual void OnOptionCancelled(int escapeIndex) {}

  /// Notification when popup closes.
  virtual void OnPopupClosed() = 0;

  /// Whether highlighting a row immediately updates the element's visible value.
  virtual bool ShouldPreviewOnHighlight() const { return false; }

  /// Whether option filtering is enabled.
  virtual bool SupportsFiltering() const { return false; }

  /// Initial filter query to apply when opening.
  virtual std::wstring GetInitialFilter() const { return L""; }

  /// Event callback IDs.
  virtual int GetOnOpenCallbackId() const { return -1; }
  virtual int GetOnCloseCallbackId() const { return -1; }
};

#endif // __NOVADESK_OPTION_LIST_HOST_H__
