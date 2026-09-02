/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_INPUT_BOX_ELEMENT_H__
#define __NOVADESK_INPUT_BOX_ELEMENT_H__

#include "Element.h"
#include "TextElement.h" // for TextAlignment
#include <d2d1_1.h>
#include <dwrite_1.h>
#include <wrl/client.h>
#include <string>
#include <vector>

/**
 * @brief Input type restrictions for text entry.
 */
enum class InputType {
  Any,          ///< No restriction (default).
  Integer,      ///< Digits and optional leading '-'.
  Float,        ///< Digits, optional '-' and one '.'.
  Letters,      ///< Unicode alphabetic only.
  Alphanumeric, ///< Alphanumeric characters only.
  Hex,          ///< 0-9 a-f A-F.
  Email,        ///< Alphanum + @ . - _ +.
  Custom,       ///< Only characters in the allowedChars set.
};

/**
 * @brief Custom text input field rendered with Direct2D/DirectWrite.
 *
 * @note Lives in the same element list as other elements, compositing
 *       in insertion order (CSS-like document flow). Supports undo/redo,
 *       selection, password masking, and input type validation.
 */
class InputBoxElement : public Element {
public:
  /**
   * @brief Constructs an input box element.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param width Width in pixels.
   * @param height Height in pixels.
   */
  InputBoxElement(const std::wstring &id, int x, int y, int width, int height);
  virtual ~InputBoxElement() {}

  virtual void Render(ID2D1DeviceContext *context) override;
  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;
  virtual GfxRect GetBounds() override;
  virtual bool HitTest(int x, int y) override;

  // ============================================================================
  // Text Content
  // ============================================================================

  /// Sets the current text content.
  void SetText(const std::wstring &text);
  const std::wstring &GetText() const { return m_Text; }

  // ============================================================================
  // Typography
  // ============================================================================

  void SetFontFace(const std::wstring &font) { m_FontFace = font; }
  void SetFontSize(int size) { m_FontSize = size; }
  void SetFontColor(COLORREF color, BYTE alpha) {
    m_FontColor = color;
    m_FontAlpha = alpha;
    m_FontGradient.type = GRADIENT_NONE;
  }
  void SetFontWeight(int weight) { m_FontWeight = weight; }
  void SetItalic(bool italic) { m_Italic = italic; }
  void SetFontPath(const std::wstring &path) { m_FontPath = path; }
  void SetTextAlign(TextAlignment align) { m_TextAlign = align; }

  const std::wstring &GetFontFace() const { return m_FontFace; }
  int GetFontSize() const { return m_FontSize; }
  COLORREF GetFontColor() const { return m_FontColor; }
  BYTE GetFontAlpha() const { return m_FontAlpha; }
  int GetFontWeight() const { return m_FontWeight; }
  bool IsItalic() const { return m_Italic; }
  TextAlignment GetTextAlign() const { return m_TextAlign; }
  const std::wstring &GetFontPath() const { return m_FontPath; }

  // ============================================================================
  // Placeholder
  // ============================================================================

  void SetPlaceholder(const std::wstring &placeholder) {
    m_Placeholder = placeholder;
  }
  const std::wstring &GetPlaceholder() const { return m_Placeholder; }
  void SetPlaceholderColor(COLORREF color, BYTE alpha) {
    m_PlaceholderColor = color;
    m_PlaceholderAlpha = alpha;
    m_PlaceholderGradient.type = GRADIENT_NONE;
  }
  COLORREF GetPlaceholderColor() const { return m_PlaceholderColor; }
  BYTE GetPlaceholderAlpha() const { return m_PlaceholderAlpha; }

  // ============================================================================
  // Caret & Selection Colors
  // ============================================================================

  void SetCaretColor(COLORREF color, BYTE alpha) {
    m_CaretColor = color;
    m_CaretAlpha = alpha;
    m_CaretGradient.type = GRADIENT_NONE;
  }
  COLORREF GetCaretColor() const { return m_CaretColor; }
  BYTE GetCaretAlpha() const { return m_CaretAlpha; }

  void SetSelectionColor(COLORREF color, BYTE alpha) {
    m_SelectionColor = color;
    m_SelectionAlpha = alpha;
    m_SelectionGradient.type = GRADIENT_NONE;
  }
  COLORREF GetSelectionColor() const { return m_SelectionColor; }
  BYTE GetSelectionAlpha() const { return m_SelectionAlpha; }

  // ============================================================================
  // Fill & Border
  // ============================================================================

  void SetFillColor(COLORREF color, BYTE alpha) {
    m_FillColor = color;
    m_FillAlpha = alpha;
    m_HasFillColor = true;
    m_FillGradient.type = GRADIENT_NONE;
  }
  void ClearFillColor() {
    m_HasFillColor = false;
    m_FillGradient.type = GRADIENT_NONE;
  }
  bool HasFillColor() const { return m_HasFillColor; }
  COLORREF GetFillColor() const { return m_FillColor; }
  BYTE GetFillAlpha() const { return m_FillAlpha; }

  void SetBorderColor(COLORREF color, BYTE alpha) {
    m_BorderColor = color;
    m_BorderAlpha = alpha;
    m_BorderGradient.type = GRADIENT_NONE;
  }
  void SetBorderWidth(float width) { m_BorderWidth = width; }
  void SetBorderRadius(float radius) { m_BorderRadius = radius; }
  COLORREF GetBorderColor() const { return m_BorderColor; }
  BYTE GetBorderAlpha() const { return m_BorderAlpha; }
  float GetBorderWidth() const { return m_BorderWidth; }
  float GetBorderRadius() const { return m_BorderRadius; }

  void SetBorderFocusColor(COLORREF color, BYTE alpha) {
    m_BorderFocusColor = color;
    m_BorderFocusAlpha = alpha;
    m_HasBorderFocusColor = true;
    m_BorderFocusGradient.type = GRADIENT_NONE;
  }
  void ClearBorderFocusColor() {
    m_HasBorderFocusColor = false;
    m_BorderFocusGradient.type = GRADIENT_NONE;
  }
  bool HasBorderFocusColor() const { return m_HasBorderFocusColor; }
  COLORREF GetBorderFocusColor() const { return m_BorderFocusColor; }
  BYTE GetBorderFocusAlpha() const { return m_BorderFocusAlpha; }

  // ============================================================================
  // Gradients
  // ============================================================================

  void SetFillGradient(const GradientInfo &gradient) {
    m_FillGradient = gradient;
    m_HasFillColor = (gradient.type != GRADIENT_NONE);
  }
  const GradientInfo &GetFillGradient() const { return m_FillGradient; }

  void SetBorderGradient(const GradientInfo &gradient) {
    m_BorderGradient = gradient;
  }
  const GradientInfo &GetBorderGradient() const { return m_BorderGradient; }

  void SetBorderFocusGradient(const GradientInfo &gradient) {
    m_BorderFocusGradient = gradient;
    m_HasBorderFocusColor = (gradient.type != GRADIENT_NONE);
  }
  const GradientInfo &GetBorderFocusGradient() const {
    return m_BorderFocusGradient;
  }

  void SetFontGradient(const GradientInfo &gradient) {
    m_FontGradient = gradient;
  }
  const GradientInfo &GetFontGradient() const { return m_FontGradient; }

  void SetPlaceholderGradient(const GradientInfo &gradient) {
    m_PlaceholderGradient = gradient;
  }
  const GradientInfo &GetPlaceholderGradient() const {
    return m_PlaceholderGradient;
  }

  void SetCaretGradient(const GradientInfo &gradient) {
    m_CaretGradient = gradient;
  }
  const GradientInfo &GetCaretGradient() const { return m_CaretGradient; }

  void SetSelectionGradient(const GradientInfo &gradient) {
    m_SelectionGradient = gradient;
  }
  const GradientInfo &GetSelectionGradient() const {
    return m_SelectionGradient;
  }

  // ============================================================================
  // Input Configuration
  // ============================================================================

  /// Enables or disables password masking mode.
  void SetPasswordMode(bool enabled) { m_Password = enabled; }
  bool IsPasswordMode() const { return m_Password; }

  /// Sets the input type restriction.
  void SetInputType(InputType type) { m_InputType = type; }
  InputType GetInputType() const { return m_InputType; }

  /// Sets allowed characters for Custom input type.
  void SetAllowedChars(const std::wstring &chars) { m_AllowedChars = chars; }
  const std::wstring &GetAllowedChars() const { return m_AllowedChars; }

  /// Sets maximum character length (0 = unlimited).
  void SetMaxLength(int len) { m_MaxLength = len; }
  int GetMaxLength() const { return m_MaxLength; }

  /// Enables or disables multiline mode.
  void SetMultiline(bool enabled) { m_Multiline = enabled; }
  bool IsMultiline() const { return m_Multiline; }

  // ============================================================================
  // Focus & Editing
  // ============================================================================

  bool IsFocused() const { return m_Focused; }
  void SetFocus(bool focused);

  /// Updates caret blink state (called each repaint).
  void UpdateBlink();

  /// Result of HandleChar for invalid input detection.
  enum class HandleCharResult { Ignored, Changed, Rejected };

  /// Handles character input. Returns result for onChange/onInvalidInput.
  HandleCharResult HandleChar(wchar_t ch);

  /// Handles key down events (backspace, delete, arrows, etc.).
  bool HandleKeyDown(WPARAM vk, bool shift, bool control);

  /// Handles mouse click for caret positioning.
  void HandleMouseDown(int x, int y, bool shift);
  void HandleMouseMove(int x, int y);
  void HandleMouseUp();

  // ============================================================================
  // Selection & Undo
  // ============================================================================

  bool HasSelection() const { return m_SelectionStart != m_SelectionEnd; }
  void SelectAll();
  void ClearSelection();
  std::wstring GetSelectedText() const;
  void DeleteSelection();
  void ReplaceSelection(const std::wstring &text);
  bool CanUndo() const { return !m_UndoStack.empty(); }
  bool CanRedo() const { return !m_RedoStack.empty(); }
  bool Undo();
  bool Redo();

  // ============================================================================
  // Event Callbacks
  // ============================================================================

  int m_OnTextChangeCallbackId = -1;    ///< Text content changed.
  int m_OnEnterCallbackId = -1;         ///< Enter key pressed.
  int m_OnFocusCallbackId = -1;         ///< Input received focus.
  int m_OnBlurCallbackId = -1;          ///< Input lost focus.
  int m_OnInvalidInputCallbackId = -1;  ///< Typed char rejected by inputType.

private:
  Microsoft::WRL::ComPtr<IDWriteTextLayout>
  CreateTextLayout(ID2D1DeviceContext *context, const std::wstring &text,
                   float layoutW, float layoutH) const;
  D2D1_RECT_F GetContentRect() const;
  float CaretIndexToX(UINT32 index) const;
  void CaretIndexToXY(UINT32 index, float &outX, float &outY,
                      float &outH) const;
  UINT32 PointToCaretIndex(int x, int y) const;
  void NormalizeSelection(UINT32 &outStart, UINT32 &outEnd) const;
  void EnsureCaretVisible();
  float MeasureTextWidth() const;
  void SaveUndoState();

  // ============================================================================
  // Text & Typography State
  // ============================================================================

  std::wstring m_Text;
  std::wstring m_Placeholder;
  std::wstring m_FontFace = L"Segoe UI";
  int m_FontSize = 14;
  COLORREF m_FontColor = RGB(240, 240, 240);
  BYTE m_FontAlpha = 255;
  int m_FontWeight = 400;
  bool m_Italic = false;
  TextAlignment m_TextAlign = TEXT_ALIGN_LEFT_CENTER;
  std::wstring m_FontPath;

  // ============================================================================
  // Colors
  // ============================================================================

  COLORREF m_PlaceholderColor = RGB(150, 150, 150);
  BYTE m_PlaceholderAlpha = 255;
  COLORREF m_CaretColor = RGB(255, 255, 255);
  BYTE m_CaretAlpha = 255;
  COLORREF m_SelectionColor = RGB(135, 206, 235);
  BYTE m_SelectionAlpha = 128;

  // ============================================================================
  // Fill & Border State
  // ============================================================================

  bool m_HasFillColor = true;
  COLORREF m_FillColor = RGB(30, 30, 34);
  BYTE m_FillAlpha = 255;

  float m_BorderWidth = 0.0f;
  float m_BorderRadius = 0.0f;
  COLORREF m_BorderColor = RGB(0, 0, 0);
  BYTE m_BorderAlpha = 255;
  bool m_HasBorderFocusColor = false;
  COLORREF m_BorderFocusColor = RGB(0, 0, 0);
  BYTE m_BorderFocusAlpha = 255;

  // ============================================================================
  // Gradient State
  // ============================================================================

  GradientInfo m_FillGradient;
  GradientInfo m_BorderGradient;
  GradientInfo m_BorderFocusGradient;
  GradientInfo m_FontGradient;
  GradientInfo m_PlaceholderGradient;
  GradientInfo m_CaretGradient;
  GradientInfo m_SelectionGradient;

  // ============================================================================
  // Input Configuration
  // ============================================================================

  bool m_Password = false;
  int m_MaxLength = 0;
  bool m_Multiline = false;
  InputType m_InputType = InputType::Any;
  std::wstring m_AllowedChars; ///< Characters allowed for Custom input type.

  // ============================================================================
  // Caret & Selection State
  // ============================================================================

  bool m_Focused = false;
  UINT32 m_CaretPos = 0;        ///< Caret position (0..text.length()).
  UINT32 m_SelectionAnchor = 0; ///< Anchor for shift-click/drag selection.
  UINT32 m_SelectionStart = 0;  ///< Normalized selection start.
  UINT32 m_SelectionEnd = 0;    ///< Normalized selection end.
  bool m_IsDragging = false;

  // Blink state
  DWORD m_LastBlinkTick = 0;
  bool m_CaretVisible = true;

  /// Horizontal scroll offset (positive = text shifted left).
  float m_ScrollOffset = 0.0f;

  // Undo/Redo state
  struct UndoState {
    std::wstring text;
    UINT32 caretPos;
    UINT32 selectionStart;
    UINT32 selectionEnd;
  };
  std::vector<UndoState> m_UndoStack;
  std::vector<UndoState> m_RedoStack;
};

#endif
