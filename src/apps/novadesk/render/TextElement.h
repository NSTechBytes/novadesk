/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_TEXT_ELEMENT_H__
#define __NOVADESK_TEXT_ELEMENT_H__

#include "Element.h"
#include <string>
#include <windows.h>
#include <vector>
#include <optional>
#include <wincodec.h>
#include <wrl/client.h>

/**
 * @brief Style overrides for a single text segment (inline formatting).
 */
struct TextSegmentStyle {
  std::optional<int> fontWeight;        ///< Override font weight (100-900).
  std::optional<bool> italic;           ///< Override italic style.
  std::optional<bool> underline;        ///< Override underline decoration.
  std::optional<bool> strikethrough;    ///< Override strikethrough decoration.
  std::optional<COLORREF> color;        ///< Override text color.
  std::optional<BYTE> alpha;            ///< Override text opacity.
  std::optional<int> fontSize;          ///< Override font size.
  std::optional<std::wstring> fontFace; ///< Override font family.
  std::optional<GradientInfo> gradient; ///< Override gradient fill.
  std::optional<TextCase> textCase;     ///< Override text case transformation.
};

/**
 * @brief A styled segment of text within a TextElement.
 */
struct TextSegment {
  std::wstring text;      ///< The text content of this segment.
  TextSegmentStyle style; ///< Style overrides for this segment.
  UINT32 startPos;        ///< Start position in the original text.
  UINT32 length;          ///< Length of this segment.
};

/**
 * @brief Text alignment modes for horizontal and vertical positioning.
 */
enum TextAlignment {
  TEXT_ALIGN_LEFT_TOP,      ///< Left-aligned, top-anchored.
  TEXT_ALIGN_CENTER_TOP,    ///< Center-aligned, top-anchored.
  TEXT_ALIGN_RIGHT_TOP,     ///< Right-aligned, top-anchored.
  TEXT_ALIGN_LEFT_CENTER,   ///< Left-aligned, vertically centered.
  TEXT_ALIGN_CENTER_CENTER, ///< Center-aligned, vertically centered.
  TEXT_ALIGN_RIGHT_CENTER,  ///< Right-aligned, vertically centered.
  TEXT_ALIGN_LEFT_BOTTOM,   ///< Left-aligned, bottom-anchored.
  TEXT_ALIGN_CENTER_BOTTOM, ///< Center-aligned, bottom-anchored.
  TEXT_ALIGN_RIGHT_BOTTOM   ///< Right-aligned, bottom-anchored.
};

/**
 * @brief Text clipping behavior when content exceeds bounds.
 */
enum TextClip {
  TEXT_CLIP_NONE = 0,     ///< No clipping; text may overflow.
  TEXT_CLIP_ON = 1,       ///< Hard clip at element bounds.
  TEXT_CLIP_ELLIPSIS = 2, ///< Truncate with "..." ellipsis.
  TEXT_CLIP_WRAP = 3      ///< Wrap text to next line.
};

/**
 * @brief Renders styled text with inline formatting, shadows, and selection.
 *
 * @note Supports rich text via inline style tags and text selection for copy.
 */
class TextElement : public Element {
public:
  /**
   * @brief Constructs a text element with full styling configuration.
   *
   * @param id Unique element identifier.
   * @param x X-coordinate.
   * @param y Y-coordinate.
   * @param w Width in pixels (0 = auto).
   * @param h Height in pixels (0 = auto).
   * @param text The text content (may include inline style tags).
   * @param fontFace Font family name.
   * @param fontSize Font size in pixels.
   * @param fontColor Text color in COLORREF format.
   * @param alpha Text opacity (0-255).
   * @param fontWeight Font weight (100-900).
   * @param italic Whether text is italic.
   * @param textAlign Text alignment mode.
   * @param clip Text clipping behavior.
   * @param fontPath Optional path to a custom font file.
   */
  TextElement(const std::wstring &id, int x, int y, int w, int h,
              const std::wstring &text, const std::wstring &fontFace,
              int fontSize, COLORREF fontColor, BYTE alpha, int fontWeight,
              bool italic, TextAlignment textAlign,
              TextClip clip = TEXT_CLIP_NONE,
              const std::wstring &fontPath = L"");

  virtual ~TextElement() {}

  virtual void Render(ID2D1DeviceContext *context) override;

  /// Sets the text content and re-parses inline styles.
  void SetText(const std::wstring &text) {
    m_Text = text;
    ParseInlineStyles();
    InvalidateHitTestCache();
  }

  /// Sets the font family and re-parses inline styles.
  void SetFontFace(const std::wstring &font) {
    m_FontFace = font;
    ParseInlineStyles();
    InvalidateHitTestCache();
  }

  /// Sets the font size in pixels.
  void SetFontSize(int size) {
    m_FontSize = size;
    ParseInlineStyles();
    InvalidateHitTestCache();
  }

  /// Sets the text color and opacity.
  void SetFontColor(COLORREF color, BYTE alpha) {
    m_FontColor = color;
    m_Alpha = alpha;
    ParseInlineStyles();
    InvalidateHitTestCache();
  }

  /// Sets the font weight (100-900).
  void SetFontWeight(int weight) {
    m_FontWeight = weight;
    ParseInlineStyles();
    InvalidateHitTestCache();
  }

  /// Enables or disables italic rendering.
  void SetItalic(bool italic) {
    m_Italic = italic;
    ParseInlineStyles();
    InvalidateHitTestCache();
  }

  /// Sets the text alignment mode.
  void SetTextAlign(TextAlignment align) {
    m_TextAlign = align;
    InvalidateHitTestCache();
  }

  /// Sets the text clipping behavior.
  void SetClip(TextClip clip) {
    m_textClip = clip;
    InvalidateHitTestCache();
  }

  /// Sets the path to a custom font file.
  void SetFontPath(const std::wstring &path) {
    m_FontPath = path;
    InvalidateHitTestCache();
  }

  /// Configures text drop shadows.
  void SetShadows(const std::vector<TextShadow> &shadows) {
    m_Shadows = shadows;
    InvalidateHitTestCache();
  }

  /// Sets a gradient fill for the text.
  void SetFontGradient(const GradientInfo &gradient) {
    m_FontGradient = gradient;
    InvalidateHitTestCache();
  }

  /// Sets letter spacing in pixels.
  void SetLetterSpacing(float spacing) {
    m_LetterSpacing = spacing;
    InvalidateHitTestCache();
  }

  /// Enables or disables underline decoration.
  void SetUnderline(bool underline) {
    m_UnderLine = underline;
    InvalidateHitTestCache();
  }

  /// Enables or disables strikethrough decoration.
  void SetStrikethrough(bool strikethrough) {
    m_StrikeThrough = strikethrough;
    InvalidateHitTestCache();
  }

  /// Sets the text case transformation mode.
  void SetTextCase(TextCase textCase) {
    m_TextCase = textCase;
    InvalidateHitTestCache();
  }

  /// Enables or disables text selection (copy support).
  void SetTextSelection(bool selectable) { m_TextSelection = selectable; }

  /// Sets the background color for selected text.
  void SetSelectionBackgroundColor(COLORREF color, BYTE alpha) {
    m_SelectionBackgroundColor = color;
    m_SelectionBackgroundAlpha = alpha;
  }

  /// Sets the text color for selected text.
  void SetSelectionTextColor(COLORREF color, BYTE alpha) {
    m_SelectionTextColor = color;
    m_SelectionTextAlpha = alpha;
    m_HasSelectionTextColor = true;
  }

  const std::wstring &GetText() const { return m_Text; }
  const std::wstring &GetCleanText() const { return m_CleanText; }
  const std::wstring &GetFontFace() const { return m_FontFace; }
  int GetFontSize() const { return m_FontSize; }
  COLORREF GetFontColor() const { return m_FontColor; }
  BYTE GetFontAlpha() const { return m_Alpha; }
  int GetFontWeight() const { return m_FontWeight; }
  bool IsItalic() const { return m_Italic; }
  TextAlignment GetTextAlign() const { return m_TextAlign; }
  TextClip GettextClip() const { return m_textClip; }
  const std::wstring &GetFontPath() const { return m_FontPath; }

  const std::vector<TextShadow> &GetShadows() const { return m_Shadows; }
  const GradientInfo &GetFontGradient() const { return m_FontGradient; }
  float GetLetterSpacing() const { return m_LetterSpacing; }
  bool GetUnderline() const { return m_UnderLine; }
  bool GetStrikethrough() const { return m_StrikeThrough; }
  TextCase GetTextCase() const { return m_TextCase; }
  bool GetTextSelection() const { return m_TextSelection; }
  COLORREF GetSelectionBackgroundColor() const {
    return m_SelectionBackgroundColor;
  }
  BYTE GetSelectionBackgroundAlpha() const {
    return m_SelectionBackgroundAlpha;
  }
  COLORREF GetSelectionTextColor() const { return m_SelectionTextColor; }
  BYTE GetSelectionTextAlpha() const { return m_SelectionTextAlpha; }
  bool HasSelectionTextColor() const { return m_HasSelectionTextColor; }

  virtual int GetAutoWidth() override;
  virtual int GetAutoHeight() override;
  virtual GfxRect GetBounds() override;
  virtual bool HitTest(int x, int y) override;

  /// @return Text with inline style tags processed and removed.
  std::wstring GetProcessedText() const;

  // ============================================================================
  // Text Selection Methods
  // ============================================================================

  void HandleTextSelectionMouseDown(int x, int y);
  void HandleTextSelectionMouseMove(int x, int y);
  void HandleTextSelectionMouseUp();
  void HandleTextSelectionDoubleClick(int x, int y);
  bool HasTextSelection() const { return m_SelectionStart != m_SelectionEnd; }
  void ClearTextSelection();
  std::wstring GetSelectedText() const;
  void SelectAll();
  void SelectWordAt(UINT32 position);

private:
  void ParseInlineStyles();
  void InvalidateHitTestCache();
  UINT32 HitTestTextPosition(int x, int y);
  void FindWordBoundaries(UINT32 position, UINT32 &wordStart, UINT32 &wordEnd);

  // ============================================================================
  // Text Properties
  // ============================================================================

  std::wstring m_Text;
  std::wstring m_CleanText;
  std::wstring m_FontFace;
  int m_FontSize;
  COLORREF m_FontColor;
  BYTE m_Alpha;
  int m_FontWeight;
  bool m_Italic;
  TextAlignment m_TextAlign;
  TextClip m_textClip;
  std::wstring m_FontPath;

  // ============================================================================
  // Visual Effects
  // ============================================================================

  std::vector<TextShadow> m_Shadows;
  GradientInfo m_FontGradient;
  float m_LetterSpacing = 0.0f;
  bool m_UnderLine = false;
  bool m_StrikeThrough = false;
  TextCase m_TextCase = TEXT_CASE_NORMAL;

  // ============================================================================
  // Text Selection
  // ============================================================================

  bool m_TextSelection = false;
  COLORREF m_SelectionBackgroundColor = 0x87CEEB; ///< Default: Sky blue.
  BYTE m_SelectionBackgroundAlpha = 128;          ///< Default: 50% opacity.
  COLORREF m_SelectionTextColor = 0xFFFFFF;       ///< Default: White.
  BYTE m_SelectionTextAlpha = 255;                ///< Default: Fully opaque.
  bool m_HasSelectionTextColor = false; ///< Whether custom text color is set.

  std::vector<TextSegment> m_Segments;

  // ============================================================================
  // Hit Test Cache (for pixel-perfect text hit testing)
  // ============================================================================

  Microsoft::WRL::ComPtr<IWICBitmap> m_HitTestBitmap;
  float m_HitTestCachedW = 0;
  float m_HitTestCachedH = 0;
  bool m_HitTestCachedAntiAlias = false;
  uint32_t m_HitTestCacheGeneration = 0;
  uint32_t m_HitTestBuiltGeneration = 0;

  // ============================================================================
  // Selection State
  // ============================================================================

  bool m_IsSelecting = false;
  UINT32 m_SelectionStart = 0;
  UINT32 m_SelectionEnd = 0;
  UINT32 m_SelectionAnchor = 0; ///< Anchor point for selection drag.
};

#endif
