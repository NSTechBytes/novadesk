/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_COMBO_BOX_ELEMENT_H__
#define __NOVADESK_COMBO_BOX_ELEMENT_H__

#include "InputBoxElement.h"
#include "OptionListHost.h"
#include "DropDownElement.h" // For DropDownOption

#include <string>
#include <vector>

/**
 * @brief ComboBox element combining an editable text field with a drop-down list.
 *
 * Inherits from InputBoxElement to reuse DirectWrite text editing, selection,
 * caret blinking, and clipboard operations, while providing a chevron button
 * and a popup window for option choices.
 */
class ComboBoxElement : public InputBoxElement, public IOptionListHost {
public:
  /**
   * @brief Constructs a combo box element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels (default 180).
   * @param height Height in pixels (default 30).
   */
  ComboBoxElement(const std::wstring &id, int x, int y, int width = 180,
                  int height = 30);
  virtual ~ComboBoxElement() {}

  // ============================================================================
  // Value & Options Model
  // ============================================================================

  /// Replaces the option list.
  void SetOptions(const std::vector<DropDownOption> &options);
  const std::vector<DropDownOption> &Options() const { return m_Options; }
  int OptionCount() const { return static_cast<int>(m_Options.size()); }

  /// Selects an option by index (-1 clears selection).
  void SetSelectedIndex(int index);
  virtual int GetSelectedIndex() const override { return m_SelectedIndex; }

  /// Selects an option and updates the text field to match its label.
  void SelectOption(int index);

  /// Returns selected option's value (or empty if unselected).
  std::wstring SelectedValue() const;

  /// Returns selected option's label (or empty if unselected).
  std::wstring SelectedLabel() const;

  /// Resolves an option value to its index, or -1.
  int IndexForValue(const std::wstring &value) const;

  /// Resolves an option label to its index, or -1.
  int IndexForLabel(const std::wstring &label) const;

  // ============================================================================
  // Geometry & Hit Testing
  // ============================================================================

  virtual D2D1_RECT_F GetContentRect() const override;
  D2D1_RECT_F GetChevronRect() const;
  bool IsPointInChevron(int x, int y) const;
  virtual bool HitTest(int x, int y) override;

  // ============================================================================
  // Rendering & Overrides
  // ============================================================================

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual int GetAutoWidth() override { return 180; }
  virtual int GetAutoHeight() override { return 30; }

  // ============================================================================
  // Mode & State Flags
  // ============================================================================

  bool m_Editable = true;        ///< If false, text entry is blocked; click opens popup.
  bool m_ShowChevron = true;     ///< Show/hide the dropdown chevron button.
  bool m_OpenOnType = false;     ///< Automatically open the popup when the user starts typing.
  bool m_FilterOnType = false;   ///< Filter popup options as user types in input.
  bool m_AutoComplete = false;   ///< Suggest best option match while typing.
  bool m_Disabled = false;       ///< Dimmed visuals, input ignored.
  bool m_Hovered = false;        ///< Mouse is hovering the control.
  bool m_Open = false;           ///< Dropdown popup currently open.
  bool m_HoveredChevron = false; ///< Pointer hovering chevron button.
  bool m_RightToLeft = false;    ///< Place chevron on left side.

  // ============================================================================
  // Chevron Styling
  // ============================================================================

  COLORREF m_ChevronColor = RGB(200, 200, 208);
  BYTE m_ChevronAlpha = 255;
  COLORREF m_ChevronHoverColor = RGB(255, 255, 255);
  BYTE m_ChevronHoverAlpha = 255;
  float m_ChevronSize = 5.0f;
  float m_ChevronAreaWidth = 26.0f;
  float m_ChevronGap = 4.0f;

  // ============================================================================
  // Box Styling States
  // ============================================================================

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
  // Popup Styling (Consumed by ComboBoxPopup)
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
  int m_PopupItemHeight = 28;
  int m_PopupMaxVisibleItems = 8;
  int m_PopupPadding = 4;
  int m_PopupMinWidth = 0;

  // ============================================================================
  // Event Callbacks
  // ============================================================================

  int m_OnChangeCallbackId = -1;  ///< Value committed.
  int m_OnSelectCallbackId = -1;  ///< Option selected from dropdown.
  int m_OnOpenCallbackId = -1;    ///< Popup opened.
  int m_OnCloseCallbackId = -1;   ///< Popup dismissed.


  // ============================================================================
  // IOptionListHost Implementation
  // ============================================================================
  virtual Element *GetElement() override { return this; }
  virtual const std::vector<DropDownOption> &GetOptions() const override { return m_Options; }
  virtual OptionListStyle GetOptionListStyle() const override {
    OptionListStyle s;
    s.background = m_PopupBackground;
    s.backgroundAlpha = m_PopupBackgroundAlpha;
    s.borderColor = m_PopupBorderColor;
    s.borderAlpha = m_PopupBorderAlpha;
    s.hoverColor = m_PopupHoverColor;
    s.hoverAlpha = m_PopupHoverAlpha;
    s.selectedColor = m_PopupSelectedColor;
    s.selectedAlpha = m_PopupSelectedAlpha;
    s.textColor = m_PopupTextColor;
    s.textAlpha = m_PopupTextAlpha;
    s.checkColor = m_PopupSelectedColor;
    s.checkAlpha = m_PopupSelectedAlpha;
    s.itemHeight = m_PopupItemHeight;
    s.maxVisibleItems = m_PopupMaxVisibleItems;
    s.padding = m_PopupPadding;
    s.minWidth = m_PopupMinWidth;
    s.rightToLeft = m_RightToLeft;
    s.fontFace = m_FontFace;
    s.fontSize = m_FontSize;
    s.fontWeight = m_FontWeight;
    return s;
  }
  virtual void OnOptionCommitted(int originalIndex) override {
    SelectOption(originalIndex);
  }
  virtual void OnOptionHighlighted(int originalIndex) override {}
  virtual void OnOptionCancelled(int escapeIndex) override {}
  virtual void OnPopupClosed() override {
    m_Open = false;
    m_HoveredChevron = false;
  }
  virtual bool ShouldPreviewOnHighlight() const override { return false; }
  virtual bool SupportsFiltering() const override { return m_FilterOnType; }
  virtual std::wstring GetInitialFilter() const override {
    return m_FilterOnType ? GetText() : L"";
  }
  virtual int GetOnOpenCallbackId() const override { return m_OnOpenCallbackId; }
  virtual int GetOnCloseCallbackId() const override { return m_OnCloseCallbackId; }

protected:
  std::vector<DropDownOption> m_Options;
  int m_SelectedIndex = -1;
};

#endif // __NOVADESK_COMBO_BOX_ELEMENT_H__
