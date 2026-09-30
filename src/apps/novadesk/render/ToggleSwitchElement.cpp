/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ToggleSwitchElement.h"
#include "Direct2DHelper.h"
#include "../domain/animation/AnimationEasing.h"
#include <wrl/client.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr float kClamp01(float v) {
  return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
} // namespace

void ToggleSwitchElement::SetChecked(bool checked, bool animate) {
  if (m_Checked == checked && !m_Animating) {
    m_KnobProgress = checked ? 1.0f : 0.0f;
    return;
  }
  m_Checked = checked;
  const float target = checked ? 1.0f : 0.0f;
  if (!animate || m_DurationMs <= 0) {
    m_Animating = false;
    m_KnobProgress = target;
    m_KnobOffsetPx = 0.0f;
    return;
  }
  m_KnobOffsetPx = 0.0f;
  m_AnimFrom = m_KnobProgress;
  m_AnimTo = target;
  m_AnimStartTick = GetTickCount();
  m_Animating = true;
}

void ToggleSwitchElement::Toggle() {
  SetChecked(!m_Checked, m_DurationMs > 0);
}

bool ToggleSwitchElement::StepAnimation() {
  if (!m_Animating)
    return false;
  DWORD elapsed = GetTickCount() - m_AnimStartTick;
  if (static_cast<int>(elapsed) >= m_DurationMs) {
    m_KnobProgress = m_AnimTo;
    m_Animating = false;
    return false;
  }
  float t = static_cast<float>(elapsed) / static_cast<float>(m_DurationMs);
  float eased = AnimationEasing::Evaluate(kClamp01(t), m_Easing);
  m_KnobProgress = m_AnimFrom + (m_AnimTo - m_AnimFrom) * eased;
  return true;
}

void ToggleSwitchElement::Render(ID2D1DeviceContext *context) {
  if (!context || !m_Show)
    return;

  const GfxRect bounds = GetBounds();
  const float left = static_cast<float>(bounds.X);
  const float top = static_cast<float>(bounds.Y);
  const float width = static_cast<float>(bounds.Width);
  const float height = static_cast<float>(bounds.Height);
  if (width <= 0.0f || height <= 0.0f)
    return;

  const float opacity = kClamp01(m_Opacity);
  const bool useDisabled = m_Disabled;

  //  Resolve colors 
  COLORREF trackColor = m_Checked ? m_OnColor : m_OffColor;
  BYTE trackAlpha = m_Checked ? m_OnAlpha : m_OffAlpha;
  COLORREF knobColor = m_KnobColor;
  BYTE knobAlpha = m_KnobAlpha;
  if (useDisabled) {
    trackColor = m_DisabledTrackColor;
    trackAlpha = m_DisabledTrackAlpha;
    knobColor = m_DisabledKnobColor;
    knobAlpha = m_DisabledKnobAlpha;
  } else if (m_Hovered && m_HasHoverTrackColor) {
    trackColor = m_HoverTrackColor;
    trackAlpha = m_HoverTrackAlpha;
  }

  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> trackBrush;
  context->CreateSolidColorBrush(
      D2D1::ColorF(GetRValue(trackColor) / 255.0f,
                   GetGValue(trackColor) / 255.0f,
                   GetBValue(trackColor) / 255.0f,
                   (trackAlpha / 255.0f) * opacity),
      trackBrush.GetAddressOf());

  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> knobBrush;
  context->CreateSolidColorBrush(
      D2D1::ColorF(GetRValue(knobColor) / 255.0f, GetGValue(knobColor) / 255.0f,
                   GetBValue(knobColor) / 255.0f,
                   (knobAlpha / 255.0f) * opacity),
      knobBrush.GetAddressOf());

  const float radius = m_BorderRadius >= 0.0f
                           ? std::min(m_BorderRadius, height * 0.5f)
                           : height * 0.5f;

  //  Track geometry
  const D2D1_ROUNDED_RECT track =
      D2D1::RoundedRect(D2D1::RectF(left, top, left + width, top + height),
                        radius, radius);
  if (trackBrush)
    context->FillRoundedRectangle(track, trackBrush.Get());
  if (m_BorderWidth > 0.0f) {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderBrush;
    context->CreateSolidColorBrush(
        D2D1::ColorF(GetRValue(m_BorderColor) / 255.0f,
                     GetGValue(m_BorderColor) / 255.0f,
                     GetBValue(m_BorderColor) / 255.0f,
                     (m_BorderAlpha / 255.0f) * opacity),
        borderBrush.GetAddressOf());
    if (borderBrush) {
      const float inset = m_BorderWidth * 0.5f;
      const D2D1_ROUNDED_RECT borderRect = D2D1::RoundedRect(
          D2D1::RectF(left + inset, top + inset, left + width - inset,
                      top + height - inset),
          std::max(0.0f, radius - inset), std::max(0.0f, radius - inset));
      context->DrawRoundedRectangle(borderRect, borderBrush.Get(),
                                    m_BorderWidth);
    }
  }

  //  Knob geometry
  float padding = std::max(0.0f, m_KnobPadding);
  float diameter = m_KnobSize > 0.0f
                       ? std::min(m_KnobSize, height - 2.0f * padding)
                       : height - 2.0f * padding;
  if (diameter <= 0.0f)
    return;
  if (m_BorderWidth > 0.0f) {
    padding += m_BorderWidth;
    diameter = std::max(1.0f, height - 2.0f * padding);
  }
  const float travel = std::max(0.0f, width - diameter - 2.0f * padding);
  const float progress = m_Animating ? m_KnobProgress
                                      : (m_Checked ? 1.0f : 0.0f);
  const float knobCenterX = left + padding + diameter * 0.5f +
                            travel * kClamp01(progress) + m_KnobOffsetPx;
  const float knobCenterY = top + height * 0.5f;
  const D2D1_ELLIPSE knob =
      D2D1::Ellipse(D2D1::Point2F(knobCenterX, knobCenterY), diameter * 0.5f,
                    diameter * 0.5f);
  if (knobBrush)
    context->FillEllipse(knob, knobBrush.Get());
  if (m_KnobBorderWidth > 0.0f) {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> knobBorderBrush;
    context->CreateSolidColorBrush(
        D2D1::ColorF(GetRValue(m_KnobBorderColor) / 255.0f,
                     GetGValue(m_KnobBorderColor) / 255.0f,
                     GetBValue(m_KnobBorderColor) / 255.0f,
                     (m_KnobBorderAlpha / 255.0f) * opacity),
        knobBorderBrush.GetAddressOf());
    if (knobBorderBrush)
      context->DrawEllipse(knob, knobBorderBrush.Get(), m_KnobBorderWidth);
  }

  //  Labels 
  const std::wstring &label = m_Checked ? m_OnText : m_OffText;
  if (label.empty() || !Direct2D::GetWriteFactory())
    return;

  Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
  HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
      m_LabelFontFace.c_str(), nullptr,
      static_cast<DWRITE_FONT_WEIGHT>(m_LabelFontWeight),
      DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
      static_cast<FLOAT>(m_LabelFontSize), L"", format.GetAddressOf());
  if (FAILED(hr) || !format)
    return;
  format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
  format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  // Keep the label on the half of the track the knob is not covering.
  const float labelLeft = left + (progress > 0.5f ? 0.0f : width * 0.5f);
  const float labelWidth = width * 0.5f;
  Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
  hr = Direct2D::GetWriteFactory()->CreateTextLayout(
      label.c_str(), static_cast<UINT32>(label.length()), format.Get(),
      labelWidth, height, layout.GetAddressOf());
  if (FAILED(hr) || !layout)
    return;

  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> labelBrush;
  context->CreateSolidColorBrush(
      D2D1::ColorF(GetRValue(m_LabelFontColor) / 255.0f,
                   GetGValue(m_LabelFontColor) / 255.0f,
                   GetBValue(m_LabelFontColor) / 255.0f,
                   (m_LabelFontAlpha / 255.0f) * opacity),
      labelBrush.GetAddressOf());
  if (labelBrush)
    context->DrawTextLayout(D2D1::Point2F(labelLeft, top), layout.Get(),
                            labelBrush.Get(),
                            D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
