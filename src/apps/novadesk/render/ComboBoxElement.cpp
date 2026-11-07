/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ComboBoxElement.h"
#include "Direct2DHelper.h"

#include <wrl/client.h>
#include <algorithm>

ComboBoxElement::ComboBoxElement(const std::wstring &id, int x, int y,
                                 int width, int height)
    : InputBoxElement(ELEMENT_COMBOBOX, id, x, y, width, height) {
  // ComboBox styling defaults
  m_BorderWidth = 1.0f;
  m_BorderColor = RGB(63, 63, 70);
  m_BorderRadius = 6.0f;
  m_FillColor = RGB(45, 45, 54);
  m_HasFillColor = true;
  m_FontColor = RGB(228, 228, 231);
  m_FontSize = 13;
  m_PaddingLeft = 10.0f;
  m_PaddingRight = 6.0f;
}

void ComboBoxElement::SetOptions(const std::vector<DropDownOption> &options) {
  m_Options = options;
  if (m_SelectedIndex >= static_cast<int>(m_Options.size())) {
    m_SelectedIndex = m_Options.empty() ? -1 : static_cast<int>(m_Options.size()) - 1;
  }
}

void ComboBoxElement::SetSelectedIndex(int index) {
  const int count = static_cast<int>(m_Options.size());
  if (count == 0) {
    m_SelectedIndex = -1;
    return;
  }
  m_SelectedIndex = (std::max)(-1, (std::min)(index, count - 1));
}

void ComboBoxElement::SelectOption(int index) {
  SetSelectedIndex(index);
  if (m_SelectedIndex >= 0 &&
      m_SelectedIndex < static_cast<int>(m_Options.size())) {
    SetText(m_Options[static_cast<size_t>(m_SelectedIndex)].label);
    m_CaretPos = static_cast<UINT32>(m_Text.size());
    ClearSelection();
  }
}

std::wstring ComboBoxElement::SelectedValue() const {
  if (m_SelectedIndex >= 0 &&
      m_SelectedIndex < static_cast<int>(m_Options.size())) {
    return m_Options[static_cast<size_t>(m_SelectedIndex)].value;
  }
  return m_Text;
}

std::wstring ComboBoxElement::SelectedLabel() const {
  if (m_SelectedIndex >= 0 &&
      m_SelectedIndex < static_cast<int>(m_Options.size())) {
    return m_Options[static_cast<size_t>(m_SelectedIndex)].label;
  }
  return m_Text;
}

int ComboBoxElement::IndexForValue(const std::wstring &value) const {
  for (size_t i = 0; i < m_Options.size(); ++i) {
    if (m_Options[i].value == value)
      return static_cast<int>(i);
  }
  return -1;
}

int ComboBoxElement::IndexForLabel(const std::wstring &label) const {
  for (size_t i = 0; i < m_Options.size(); ++i) {
    if (m_Options[i].label == label)
      return static_cast<int>(i);
  }
  return -1;
}

D2D1_RECT_F ComboBoxElement::GetContentRect() const {
  D2D1_RECT_F r = InputBoxElement::GetContentRect();
  if (!m_ShowChevron)
    return r;
  const float reserved = m_ChevronAreaWidth + m_ChevronGap;
  if (m_RightToLeft) {
    r.left += reserved;
    if (r.left > r.right)
      r.left = r.right;
  } else {
    r.right -= reserved;
    if (r.right < r.left)
      r.right = r.left;
  }
  return r;
}

D2D1_RECT_F ComboBoxElement::GetChevronRect() const {
  GfxRect b = const_cast<ComboBoxElement *>(this)->GetBounds();
  const float w = (std::max)(1.0f, m_ChevronAreaWidth);
  if (m_RightToLeft) {
    return D2D1::RectF(static_cast<float>(b.X), static_cast<float>(b.Y),
                       static_cast<float>(b.X) + w,
                       static_cast<float>(b.Y + b.Height));
  } else {
    return D2D1::RectF(static_cast<float>(b.X + b.Width) - w,
                       static_cast<float>(b.Y),
                       static_cast<float>(b.X + b.Width),
                       static_cast<float>(b.Y + b.Height));
  }
}

bool ComboBoxElement::IsPointInChevron(int x, int y) const {
  if (!m_ShowChevron)
    return false;
  D2D1_RECT_F cr = GetChevronRect();
  return (x >= cr.left && x <= cr.right && y >= cr.top && y <= cr.bottom);
}

bool ComboBoxElement::HitTest(int x, int y) {
  GfxRect b = GetBounds();
  return (x >= b.X && x <= b.X + b.Width && y >= b.Y && y <= b.Y + b.Height);
}

void ComboBoxElement::Render(ID2D1DeviceContext *context) {
  if (!context)
    return;

  // Save current styling to temporarily apply state visuals
  const COLORREF origBorderColor = m_BorderColor;
  const BYTE origBorderAlpha = m_BorderAlpha;
  const COLORREF origFillColor = m_FillColor;
  const BYTE origFillAlpha = m_FillAlpha;
  const COLORREF origFontColor = m_FontColor;
  const BYTE origFontAlpha = m_FontAlpha;
  const bool origHasBorderFocus = m_HasBorderFocusColor;
  const COLORREF origBorderFocusColor = m_BorderFocusColor;
  const BYTE origBorderFocusAlpha = m_BorderFocusAlpha;

  if (m_Disabled) {
    m_FillColor = m_DisabledBackgroundColor;
    m_FillAlpha = m_DisabledBackgroundAlpha;
    m_BorderColor = m_DisabledBorderColor;
    m_BorderAlpha = m_DisabledBorderAlpha;
    m_FontColor = m_DisabledTextColor;
    m_FontAlpha = m_DisabledTextAlpha;
  } else if (m_Open) {
    m_BorderColor = m_OpenBorderColor;
    m_BorderAlpha = m_OpenBorderAlpha;
    m_HasBorderFocusColor = true;
    m_BorderFocusColor = m_OpenBorderColor;
    m_BorderFocusAlpha = m_OpenBorderAlpha;
  } else if (m_Hovered && !m_Focused) {
    m_BorderColor = m_HoverBorderColor;
    m_BorderAlpha = m_HoverBorderAlpha;
  }

  // Render base text input (background, border, text layout, selection, caret)
  InputBoxElement::Render(context);

  // Restore styling
  m_BorderColor = origBorderColor;
  m_BorderAlpha = origBorderAlpha;
  m_FillColor = origFillColor;
  m_FillAlpha = origFillAlpha;
  m_FontColor = origFontColor;
  m_FontAlpha = origFontAlpha;
  m_HasBorderFocusColor = origHasBorderFocus;
  m_BorderFocusColor = origBorderFocusColor;
  m_BorderFocusAlpha = origBorderFocusAlpha;

  // Render chevron glyph in the chevron area (only when visible)
  if (!m_ShowChevron)
    return;
  D2D1_RECT_F chevronRect = GetChevronRect();
  IDWriteFactory *writeFactory = Direct2D::GetWriteFactory();
  if (writeFactory) {
    const float chevronFontSize = (std::max)(4.0f, m_ChevronSize * 1.4f);
    Microsoft::WRL::ComPtr<IDWriteTextFormat> chevronFormat;
    if (SUCCEEDED(writeFactory->CreateTextFormat(
            L"Segoe MDL2 Assets", nullptr,
            DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL, chevronFontSize, L"",
            chevronFormat.GetAddressOf()))) {
      chevronFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
      chevronFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
      chevronFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

      // U+E70E is up chevron, U+E70D is down chevron
      const wchar_t *chevronGlyph = m_Open ? L"\uE70E" : L"\uE70D";
      Microsoft::WRL::ComPtr<IDWriteTextLayout> chevronLayout;
      const float chevronW = chevronRect.right - chevronRect.left;
      const float chevronH = chevronRect.bottom - chevronRect.top;
      if (SUCCEEDED(writeFactory->CreateTextLayout(
              chevronGlyph, 1, chevronFormat.Get(), chevronW, chevronH,
              chevronLayout.GetAddressOf()))) {
        COLORREF chevronCol = m_ChevronColor;
        BYTE chevronAl = m_ChevronAlpha;
        if (m_Disabled) {
          chevronCol = m_DisabledTextColor;
          chevronAl = m_DisabledTextAlpha;
        } else if (m_HoveredChevron || m_Open) {
          chevronCol = m_ChevronHoverColor;
          chevronAl = m_ChevronHoverAlpha;
        }

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
        const D2D1_COLOR_F c = D2D1::ColorF(
            GetRValue(chevronCol) / 255.0f,
            GetGValue(chevronCol) / 255.0f,
            GetBValue(chevronCol) / 255.0f,
            chevronAl / 255.0f);
        if (SUCCEEDED(context->CreateSolidColorBrush(c, brush.GetAddressOf()))) {
          context->DrawTextLayout(
              D2D1::Point2F(chevronRect.left, chevronRect.top),
              chevronLayout.Get(), brush.Get(), D2D1_DRAW_TEXT_OPTIONS_NONE);
        }
      }
    }
  }
}
