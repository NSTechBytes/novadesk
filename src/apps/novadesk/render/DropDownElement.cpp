/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "DropDownElement.h"
#include "Direct2DHelper.h"

#include <wrl/client.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float kClamp01(float v) {
  return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

/// Ellipsis glyph used when a label does not fit the box.
constexpr wchar_t kEllipsis = L'\x2026';
} // namespace

void DropDownElement::SetOptions(
    const std::vector<DropDownOption> &options) {
  m_Options = options;
  if (!m_Options.empty())
    m_SelectedIndex =
        std::clamp(m_SelectedIndex, -1, static_cast<int>(m_Options.size()) - 1);
  else
    m_SelectedIndex = -1;
}

void DropDownElement::SetSelectedIndex(int index) {
  const int last = static_cast<int>(m_Options.size()) - 1;
  m_SelectedIndex = std::clamp(index, -1, last < -1 ? -1 : last);
}

std::wstring DropDownElement::SelectedValue() const {
  if (m_SelectedIndex < 0 || m_SelectedIndex >= OptionCount())
    return std::wstring();
  return m_Options[static_cast<size_t>(m_SelectedIndex)].value;
}

std::wstring DropDownElement::SelectedLabel() const {
  if (m_SelectedIndex < 0 || m_SelectedIndex >= OptionCount())
    return std::wstring();
  return m_Options[static_cast<size_t>(m_SelectedIndex)].label;
}

int DropDownElement::IndexForValue(const std::wstring &value) const {
  for (size_t i = 0; i < m_Options.size(); ++i) {
    if (m_Options[i].value == value)
      return static_cast<int>(i);
  }
  return -1;
}

std::wstring DropDownElement::DisplayText() const {
  std::wstring text = SelectedLabel();
  if (text.empty())
    text = m_Placeholder;
  return Ellipsize(text, m_MaxDisplayLength);
}

std::wstring DropDownElement::Ellipsize(const std::wstring &text,
                                        UINT maxChars) {
  if (maxChars == 0 || text.length() <= maxChars)
    return text;
  // Keep room for the ellipsis itself unless the cap is a single character.
  const size_t keep = maxChars > 1 ? maxChars - 1 : 1;
  return text.substr(0, keep) + std::wstring(1, kEllipsis);
}

void DropDownElement::Render(ID2D1DeviceContext *context) {
  if (!context || !m_Show)
    return;

  const GfxRect bounds = GetBounds();
  const float left = static_cast<float>(bounds.X);
  const float top = static_cast<float>(bounds.Y);
  const float width = static_cast<float>(bounds.Width);
  const float height = static_cast<float>(bounds.Height);
  if (width <= 0.0f || height <= 0.0f)
    return;

  const float opacity = kClamp01(m_DropdownOpacity);

  auto MakeBrush = [&](COLORREF color, BYTE alpha) {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    context->CreateSolidColorBrush(
        D2D1::ColorF(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f,
                     GetBValue(color) / 255.0f, (alpha / 255.0f) * opacity),
        brush.GetAddressOf());
    return brush;
  };

  //  Box: state priority disabled > open > hovered > idle.
  COLORREF bgColor = m_BackgroundColor;
  BYTE bgAlpha = m_BackgroundAlpha;
  COLORREF borderColor = m_BorderColor;
  BYTE borderAlpha = m_BorderAlpha;
  if (m_Disabled) {
    bgColor = m_DisabledBackgroundColor;
    bgAlpha = m_DisabledBackgroundAlpha;
    borderColor = m_DisabledBorderColor;
    borderAlpha = m_DisabledBorderAlpha;
  } else if (m_Open) {
    borderColor = m_OpenBorderColor;
    borderAlpha = m_OpenBorderAlpha;
  } else if (m_Hovered) {
    borderColor = m_HoverBorderColor;
    borderAlpha = m_HoverBorderAlpha;
  }

  const float inset = m_BorderWidth * 0.5f;
  const D2D1_RECT_F rect = D2D1::RectF(
      left + inset, top + inset, left + width - inset, top + height - inset);
  const float radius =
      std::min(m_BorderRadius, std::min(width, height) * 0.5f);

  auto fillBrush = MakeBrush(bgColor, bgAlpha);
  if (fillBrush && radius > 0.0f) {
    context->FillRoundedRectangle(D2D1::RoundedRect(rect, radius, radius),
                                  fillBrush.Get());
  } else if (fillBrush) {
    context->FillRectangle(rect, fillBrush.Get());
  }

  if (m_BorderWidth > 0.0f) {
    auto borderBrush = MakeBrush(borderColor, borderAlpha);
    if (borderBrush) {
      if (radius > 0.0f)
        context->DrawRoundedRectangle(D2D1::RoundedRect(rect, radius, radius),
                                      borderBrush.Get(), m_BorderWidth);
      else
        context->DrawRectangle(rect, borderBrush.Get(), m_BorderWidth);
    }
  }

  //  Chevron on the right edge.
  const float chevronRight = left + width - m_PaddingRight;
  const float chevronCenterY = top + height * 0.5f;
  const float half = std::max(2.0f, m_ChevronSize);
  // armX controls horizontal spread, armY controls vertical drop.
  // Ratio ~1.6:1 (width:height) gives a natural chevron that is wider than
  // tall without looking flat.
  const float armX = half * 0.75f;
  const float armY = half * 0.55f;
  auto chevronBrush = MakeBrush(m_ChevronColor, m_ChevronAlpha);
  if (chevronBrush) {
    // apex is the bottom-centre point of the V
    const D2D1_POINT_2F apex =
        D2D1::Point2F(chevronRight - armX, chevronCenterY + armY * 0.5f);
    context->DrawLine(D2D1::Point2F(apex.x - armX, apex.y - armY),
                      apex, chevronBrush.Get(), 1.5f);
    context->DrawLine(apex,
                      D2D1::Point2F(apex.x + armX, apex.y - armY),
                      chevronBrush.Get(), 1.5f);
  }

  //  Label: DirectWrite, vertically centred, clipped to the box.
  const std::wstring text = DisplayText();
  IDWriteFactory *writeFactory = Direct2D::GetWriteFactory();
  if (text.empty() || !writeFactory)
    return;

  Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
  HRESULT hr = writeFactory->CreateTextFormat(
      m_FontFace.c_str(), nullptr,
      static_cast<DWRITE_FONT_WEIGHT>(m_FontWeight), DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(m_FontSize), L"",
      format.GetAddressOf());
  if (FAILED(hr) || !format)
    return;
  format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
  format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
  format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

  const float textLeft = left + m_PaddingLeft;
  const float textWidth = chevronRight - half * 2.0f - m_ChevronGap - textLeft;
  if (textWidth <= 0.0f)
    return;

  Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
  hr = writeFactory->CreateTextLayout(text.c_str(),
                                      static_cast<UINT32>(text.length()),
                                      format.Get(), textWidth, height,
                                      layout.GetAddressOf());
  if (FAILED(hr) || !layout)
    return;

  COLORREF textColor = m_FontColor;
  BYTE textAlpha = m_FontAlpha;
  if (m_Disabled) {
    textColor = m_DisabledTextColor;
    textAlpha = m_DisabledTextAlpha;
  } else if (GetSelectedIndex() < 0) {
    textColor = m_PlaceholderColor;
    textAlpha = m_PlaceholderAlpha;
  }
  auto textBrush = MakeBrush(textColor, textAlpha);
  if (textBrush)
    context->DrawTextLayout(D2D1::Point2F(textLeft, top), layout.Get(),
                            textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
