/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "SliderElement.h"
#include <wrl/client.h>
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
constexpr float kClamp01(float v) {
  return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}
} // namespace

double SliderElement::SnapAndClamp(double v) const {
  double lo = std::min(m_MinValue, m_MaxValue);
  double hi = std::max(m_MinValue, m_MaxValue);
  if (v < lo)
    v = lo;
  if (v > hi)
    v = hi;
  if (m_Step > 0.0 && std::isfinite(m_Step) && hi > lo) {
    const double n = std::round((v - lo) / m_Step);
    const double maxN = std::floor((hi - lo) / m_Step + 1e-9);
    v = lo + std::min(std::max(n, 0.0), maxN) * m_Step;
    if (v > hi)
      v = hi;
  }
  return v;
}

void SliderElement::SetRange(double minV, double maxV) {
  if (!(minV < maxV))
    return;
  m_MinValue = minV;
  m_MaxValue = maxV;
  m_Value = SnapAndClamp(m_Value);
}

void SliderElement::SetValue(double value) { m_Value = SnapAndClamp(value); }

SliderElement::TrackGeometry SliderElement::GetTrackGeometry() {
  TrackGeometry g{};
  const GfxRect bounds = GetBounds();
  const float left = static_cast<float>(bounds.X);
  const float top = static_cast<float>(bounds.Y);
  const float width = static_cast<float>(bounds.Width);
  const float height = static_cast<float>(bounds.Height);

  float thumb = m_ThumbSize;
  if (thumb <= 0.0f) {
    const float shortAxis = IsVertical() ? width : height;
    thumb = std::max(12.0f, std::min(shortAxis, m_TrackThickness + 12.0f));
  }
  thumb = std::max(4.0f, thumb);

  if (IsVertical()) {
    // Bottom = min, top = max. Inset by half the thumb so the thumb stays
    // fully inside the element bounds across the whole travel.
    g.left = left;
    g.top = top + thumb * 0.5f;
    g.length = std::max(0.0f, height - thumb);
    g.thickness = std::max(1.0f, m_TrackThickness);
    g.center = left + width * 0.5f;
  } else {
    g.left = left + thumb * 0.5f;
    g.top = top;
    g.length = std::max(0.0f, width - thumb);
    g.thickness = std::max(1.0f, m_TrackThickness);
    g.center = top + height * 0.5f;
  }
  g.thumbDiameter = thumb;
  return g;
}

double SliderElement::ValueFromPoint(int px, int py) {
  const TrackGeometry g = GetTrackGeometry();
  if (g.length <= 0.0f || m_MaxValue <= m_MinValue)
    return m_Value;
  const float along = IsVertical() ? static_cast<float>(py)
                                   : static_cast<float>(px);
  const float f = kClamp01((along - g.left) / g.length);
  // Vertical fills from the bottom, so the fraction is inverted there.
  const double frac = IsVertical() ? (1.0 - static_cast<double>(f))
                                   : static_cast<double>(f);
  return m_MinValue + frac * (m_MaxValue - m_MinValue);
}

bool SliderElement::BeginDrag(int px, int py) {
  m_Dragging = true;
  m_DragStartValue = m_Value;
  SetValue(ValueFromPoint(px, py));
  return m_Value != m_DragStartValue;
}

bool SliderElement::UpdateDrag(int px, int py) {
  if (!m_Dragging)
    return false;
  const double before = m_Value;
  SetValue(ValueFromPoint(px, py));
  return m_Value != before;
}

void SliderElement::EndDrag() { m_Dragging = false; }

bool SliderElement::HandleKeyDown(unsigned int vk, bool shift,
                                  double &outNewValue) {
  if (m_Disabled || m_MaxValue <= m_MinValue)
    return false;

  const double step = m_Step > 0.0 ? m_Step : (m_MaxValue - m_MinValue) / 100.0;
  const double delta = shift ? step * 0.1 : step;

  bool vertical = IsVertical();
  double next = m_Value;
  switch (vk) {
  case VK_LEFT:
    if (vertical)
      return false;
    next = m_Value - delta;
    break;
  case VK_RIGHT:
    if (vertical)
      return false;
    next = m_Value + delta;
    break;
  case VK_UP:
    if (!vertical)
      return false;
    next = m_Value + delta;
    break;
  case VK_DOWN:
    if (!vertical)
      return false;
    next = m_Value - delta;
    break;
  case VK_HOME:
    next = m_MinValue;
    break;
  case VK_END:
    next = m_MaxValue;
    break;
  case VK_PRIOR:
    next = m_Value + step * 10.0;
    break;
  case VK_NEXT:
    next = m_Value - step * 10.0;
    break;
  default:
    return false;
  }

  const double snapped = SnapAndClamp(next);
  outNewValue = snapped;
  return snapped != m_Value;
}

std::string SliderElement::FormatValue(double v) {
  if (!std::isfinite(v))
    return "0";
  char buf[64];
  std::snprintf(buf, sizeof(buf), "%.10g", v);
  return buf;
}

void SliderElement::Render(ID2D1DeviceContext *context) {
  if (!context || !m_Show)
    return;

  const TrackGeometry g = GetTrackGeometry();
  const float opacity = kClamp01(m_SliderOpacity);
  if (g.length <= 0.0f)
    return;

  auto MakeBrush = [&](COLORREF color, BYTE alpha) {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    context->CreateSolidColorBrush(
        D2D1::ColorF(GetRValue(color) / 255.0f, GetGValue(color) / 255.0f,
                     GetBValue(color) / 255.0f, (alpha / 255.0f) * opacity),
        brush.GetAddressOf());
    return brush;
  };

  const float progress = kClamp01(
      m_MaxValue > m_MinValue
          ? static_cast<float>((m_Value - m_MinValue) / (m_MaxValue - m_MinValue))
          : 0.0f);
  const float radius = m_TrackBorderRadius >= 0.0f
                           ? std::min(m_TrackBorderRadius, g.thickness * 0.5f)
                           : g.thickness * 0.5f;

  //  Track (unfilled) and fill (value side). 
  {
    COLORREF trackColor = m_TrackColor;
    BYTE trackAlpha = m_TrackAlpha;
    COLORREF fillColor = m_FillColor;
    BYTE fillAlpha = m_FillAlpha;
    if (m_Disabled) {
      trackColor = m_DisabledTrackColor;
      trackAlpha = m_DisabledTrackAlpha;
      fillColor = m_DisabledFillColor;
      fillAlpha = m_DisabledFillAlpha;
    }
    auto trackBrush = MakeBrush(trackColor, trackAlpha);
    if (trackBrush) {
      if (IsVertical()) {
        const D2D1_RECT_F rect = D2D1::RectF(
            g.center - g.thickness * 0.5f, g.top,
            g.center + g.thickness * 0.5f, g.top + g.length);
        context->FillRoundedRectangle(
            D2D1::RoundedRect(rect, radius, radius), trackBrush.Get());
      } else {
        const D2D1_RECT_F rect = D2D1::RectF(
            g.left, g.center - g.thickness * 0.5f, g.left + g.length,
            g.center + g.thickness * 0.5f);
        context->FillRoundedRectangle(
            D2D1::RoundedRect(rect, radius, radius), trackBrush.Get());
      }
    }
    auto fillBrush = MakeBrush(fillColor, fillAlpha);
    if (fillBrush) {
      if (IsVertical()) {
        const float yTop = g.top + g.length * (1.0f - progress);
        const D2D1_RECT_F rect =
            D2D1::RectF(g.center - g.thickness * 0.5f, yTop,
                        g.center + g.thickness * 0.5f, g.top + g.length);
        context->FillRoundedRectangle(
            D2D1::RoundedRect(rect, radius, radius), fillBrush.Get());
      } else {
        const D2D1_RECT_F rect =
            D2D1::RectF(g.left, g.center - g.thickness * 0.5f,
                        g.left + g.length * progress,
                        g.center + g.thickness * 0.5f);
        context->FillRoundedRectangle(
            D2D1::RoundedRect(rect, radius, radius), fillBrush.Get());
      }
    }
  }

  //  Thumb. 
  const D2D1_POINT_2F thumbPos =
      IsVertical()
          ? D2D1::Point2F(g.center, g.top + g.length * (1.0f - progress))
          : D2D1::Point2F(g.left + g.length * progress, g.center);
  const float thumbRadius = g.thumbDiameter * 0.5f;
  COLORREF thumbColor = m_ThumbColor;
  BYTE thumbAlpha = m_ThumbAlpha;
  if (m_Disabled) {
    thumbColor = m_DisabledThumbColor;
    thumbAlpha = m_DisabledThumbAlpha;
  } else if (m_Dragging && m_HasPressedThumbColor) {
    thumbColor = m_PressedThumbColor;
    thumbAlpha = m_PressedThumbAlpha;
  } else if (m_Hovered && m_HasHoverThumbColor) {
    thumbColor = m_HoverThumbColor;
    thumbAlpha = m_HoverThumbAlpha;
  }
  const D2D1_ELLIPSE thumb =
      D2D1::Ellipse(thumbPos, thumbRadius, thumbRadius);
  auto thumbBrush = MakeBrush(thumbColor, thumbAlpha);
  if (thumbBrush)
    context->FillEllipse(thumb, thumbBrush.Get());
  if (m_ThumbBorderWidth > 0.0f) {
    auto borderBrush = MakeBrush(m_ThumbBorderColor, m_ThumbBorderAlpha);
    if (borderBrush)
      context->DrawEllipse(thumb, borderBrush.Get(), m_ThumbBorderWidth);
  }
}
