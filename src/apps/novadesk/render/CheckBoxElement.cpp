/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "CheckBoxElement.h"
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

void CheckBoxElement::SetState(State state, bool animate) {
  const State oldState = m_State;
  if (oldState == state && !m_Animating) {
    m_MarkProgress = state == State::Unchecked ? 0.0f : 1.0f;
    return;
  }
  m_State = state;
  const float target = state == State::Unchecked ? 0.0f : 1.0f;
  if (!animate || m_DurationMs <= 0) {
    m_Animating = false;
    m_MarkProgress = target;
    return;
  }
  m_AnimFrom = m_MarkProgress;
  m_AnimTo = target;
  m_AnimStartTick = GetTickCount();
  m_Animating = true;
}

void CheckBoxElement::Toggle() {
  if (!m_TriState) {
    SetState(IsChecked() ? State::Unchecked : State::Checked,
             m_DurationMs > 0);
    return;
  }
  const State next = m_State == State::Unchecked
                         ? State::Checked
                         : (m_State == State::Checked ? State::Indeterminate
                                                      : State::Unchecked);
  SetState(next, m_DurationMs > 0);
}

bool CheckBoxElement::StepAnimation() {
  if (!m_Animating)
    return false;
  DWORD elapsed = GetTickCount() - m_AnimStartTick;
  if (static_cast<int>(elapsed) >= m_DurationMs) {
    m_MarkProgress = m_AnimTo;
    m_Animating = false;
    return false;
  }
  float t = static_cast<float>(elapsed) / static_cast<float>(m_DurationMs);
  float eased = AnimationEasing::Evaluate(kClamp01(t), m_Easing);
  m_MarkProgress = m_AnimFrom + (m_AnimTo - m_AnimFrom) * eased;
  return true;
}

void CheckBoxElement::Render(ID2D1DeviceContext *context) {
  if (!context || !m_Show)
    return;

  const GfxRect bounds = GetBounds();
  const float left = static_cast<float>(bounds.X);
  const float top = static_cast<float>(bounds.Y);
  const float width = static_cast<float>(bounds.Width);
  const float height = static_cast<float>(bounds.Height);
  if (width <= 0.0f || height <= 0.0f)
    return;

  const float opacity = kClamp01(m_BoxOpacity);
  const bool nonEmpty = m_State != State::Unchecked;

  //  Box geometry: square, vertically centered, aligned to the text side 
  const float size = m_BoxSize > 0.0f
                         ? std::min(m_BoxSize, std::min(width, height))
                         : std::min(width, height);
  if (size <= 0.0f)
    return;
  const float boxLeft = left;
  const float boxTop = top + (height - size) * 0.5f;
  const D2D1_RECT_F boxRect =
      D2D1::RectF(boxLeft, boxTop, boxLeft + size, boxTop + size);
  const float radius = std::min(m_BorderRadius, size * 0.5f);
  const float bw = std::max(0.0f, m_UncheckedBorderWidth);

  //  Box fill 
  if (nonEmpty || m_Disabled) {
    COLORREF fillColor = m_CheckedColor;
    BYTE fillAlpha = m_CheckedAlpha;
    if (m_Disabled) {
      fillColor = m_DisabledBoxColor;
      fillAlpha = m_DisabledBoxAlpha;
    }
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fillBrush;
    context->CreateSolidColorBrush(
        D2D1::ColorF(GetRValue(fillColor) / 255.0f,
                     GetGValue(fillColor) / 255.0f,
                     GetBValue(fillColor) / 255.0f,
                     (fillAlpha / 255.0f) * opacity),
        fillBrush.GetAddressOf());
    if (fillBrush)
      context->FillRoundedRectangle(
          D2D1::RoundedRect(boxRect, radius, radius), fillBrush.Get());
  }

  //  Box border 
  {
    COLORREF borderColor = m_UncheckedBorderColor;
    BYTE borderAlpha = m_UncheckedBorderAlpha;
    if (!nonEmpty && !m_Disabled && m_Hovered && m_HasHoverBorderColor) {
      borderColor = m_HoverBorderColor;
      borderAlpha = m_HoverBorderAlpha;
    } else if (m_Disabled) {
      borderColor = m_DisabledBoxColor;
      borderAlpha = m_DisabledBoxAlpha;
    }
    if (bw > 0.0f) {
      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderBrush;
      context->CreateSolidColorBrush(
          D2D1::ColorF(GetRValue(borderColor) / 255.0f,
                       GetGValue(borderColor) / 255.0f,
                       GetBValue(borderColor) / 255.0f,
                       (borderAlpha / 255.0f) * opacity),
          borderBrush.GetAddressOf());
      if (borderBrush) {
        const float inset = bw * 0.5f;
        context->DrawRoundedRectangle(
            D2D1::RoundedRect(
                D2D1::RectF(boxLeft + inset, boxTop + inset,
                            boxLeft + size - inset, boxTop + size - inset),
                std::max(0.0f, radius - inset), std::max(0.0f, radius - inset)),
            borderBrush.Get(), bw);
      }
    }
  }

  //  Mark (checkmark polyline or indeterminate dash), scaled about the
  //  box center and faded in by the tween progress. Two non-empty states
  //  morph at full progress so switching kinds never double-fades. 
  const float progress =
      m_Animating ? kClamp01(m_MarkProgress) : (nonEmpty ? 1.0f : 0.0f);
  const bool morphBetweenNonEmpty =
      nonEmpty && m_Animating && m_AnimTo > 0.5f && m_AnimFrom > 0.5f;
  const float markAlpha =
      morphBetweenNonEmpty ? 1.0f : progress;
  const float markScale =
      morphBetweenNonEmpty ? 1.0f : (0.6f + 0.4f * progress);
  if (markAlpha > 0.001f) {
    COLORREF checkColor = m_CheckColor;
    BYTE checkAlpha = m_CheckAlpha;
    if (m_Disabled) {
      checkColor = m_DisabledCheckColor;
      checkAlpha = m_DisabledCheckAlpha;
    }
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> markBrush;
    context->CreateSolidColorBrush(
        D2D1::ColorF(GetRValue(checkColor) / 255.0f,
                     GetGValue(checkColor) / 255.0f,
                     GetBValue(checkColor) / 255.0f,
                     (checkAlpha / 255.0f) * opacity * markAlpha),
        markBrush.GetAddressOf());
    if (markBrush) {
      const float thickness = std::max(0.5f, m_CheckThickness);
      const float cx = boxLeft + size * 0.5f;
      const float cy = boxTop + size * 0.5f;
      const float s = size * markScale;
      if (m_State == State::Indeterminate) {
        context->DrawLine(
            D2D1::Point2F(cx - s * 0.28f, cy), D2D1::Point2F(cx + s * 0.28f, cy),
            markBrush.Get(), thickness, nullptr);
      } else {
        // Polyline: left-mid -> bottom-mid -> top-right, centred on the box.
        const D2D1_POINT_2F p0 = D2D1::Point2F(cx - s * 0.32f, cy + s * 0.02f);
        const D2D1_POINT_2F p1 = D2D1::Point2F(cx - s * 0.10f, cy + s * 0.24f);
        const D2D1_POINT_2F p2 = D2D1::Point2F(cx + s * 0.34f, cy - s * 0.24f);
        context->DrawLine(p0, p1, markBrush.Get(), thickness, nullptr);
        context->DrawLine(p1, p2, markBrush.Get(), thickness, nullptr);
      }
    }
  }

  //  Label 
  if (m_Text.empty() || !Direct2D::GetWriteFactory())
    return;

  Microsoft::WRL::ComPtr<IDWriteTextFormat> format;
  HRESULT hr = Direct2D::GetWriteFactory()->CreateTextFormat(
      m_FontFace.c_str(), nullptr,
      static_cast<DWRITE_FONT_WEIGHT>(m_FontWeight), DWRITE_FONT_STYLE_NORMAL,
      DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(m_FontSize), L"",
      format.GetAddressOf());
  if (FAILED(hr) || !format)
    return;
  format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
  format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);

  const float labelLeft = boxLeft + size + m_LabelGap;
  const float labelWidth = left + width - labelLeft;
  if (labelWidth <= 0.0f)
    return;
  Microsoft::WRL::ComPtr<IDWriteTextLayout> layout;
  hr = Direct2D::GetWriteFactory()->CreateTextLayout(
      m_Text.c_str(), static_cast<UINT32>(m_Text.length()), format.Get(),
      labelWidth, height, layout.GetAddressOf());
  if (FAILED(hr) || !layout)
    return;

  COLORREF textColor = m_FontColor;
  BYTE textAlpha = m_FontAlpha;
  if (m_Disabled) {
    textColor = m_DisabledTextColor;
    textAlpha = m_DisabledTextAlpha;
  }
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> textBrush;
  context->CreateSolidColorBrush(
      D2D1::ColorF(GetRValue(textColor) / 255.0f, GetGValue(textColor) / 255.0f,
                   GetBValue(textColor) / 255.0f,
                   (textAlpha / 255.0f) * opacity),
      textBrush.GetAddressOf());
  if (textBrush)
    context->DrawTextLayout(D2D1::Point2F(labelLeft, top), layout.Get(),
                            textBrush.Get(), D2D1_DRAW_TEXT_OPTIONS_CLIP);
}
