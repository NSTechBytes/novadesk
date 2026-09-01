#include "ColorPickerElement.h"
#include "Direct2DHelper.h"
#include <wrl/client.h>
#include <algorithm>

void ColorPickerElement::Render(ID2D1DeviceContext *context) {
  if (!context || !m_Show)
    return;

  const GfxRect bounds = GetBounds();
  const float left = static_cast<float>(bounds.X);
  const float top = static_cast<float>(bounds.Y);
  const float right = left + static_cast<float>(bounds.Width);
  const float bottom = top + static_cast<float>(bounds.Height);
  const float opacity = std::max(0.0f, std::min(1.0f, m_Opacity));

  // ── Fill brush ────────────────────────────────────────────────
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fillBrush;
  context->CreateSolidColorBrush(
      D2D1::ColorF(GetRValue(m_Color) / 255.0f, GetGValue(m_Color) / 255.0f,
                   GetBValue(m_Color) / 255.0f, opacity),
      fillBrush.GetAddressOf());

  // ── Optional border brush ─────────────────────────────────────
  Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> borderBrush;
  const bool hasBorder = (m_BorderWidth > 0.0f);
  if (hasBorder) {
    context->CreateSolidColorBrush(
        D2D1::ColorF(GetRValue(m_BorderColor) / 255.0f,
                     GetGValue(m_BorderColor) / 255.0f,
                     GetBValue(m_BorderColor) / 255.0f,
                     (m_BorderAlpha / 255.0f) * opacity),
        borderBrush.GetAddressOf());
  }

  // ── Draw shape ────────────────────────────────────────────────
  if (m_CircleShape) {
    // Ellipse swatch
    const D2D1_ELLIPSE ellipse = D2D1::Ellipse(
        D2D1::Point2F((left + right) * 0.5f, (top + bottom) * 0.5f),
        (right - left) * 0.5f, (bottom - top) * 0.5f);

    if (fillBrush)
      context->FillEllipse(ellipse, fillBrush.Get());
    if (hasBorder && borderBrush)
      context->DrawEllipse(ellipse, borderBrush.Get(), m_BorderWidth);
  } else if (m_BorderRadius > 0.0f) {
    // Rounded rectangle swatch
    const D2D1_ROUNDED_RECT rr = D2D1::RoundedRect(
        D2D1::RectF(left, top, right, bottom), m_BorderRadius, m_BorderRadius);

    if (fillBrush)
      context->FillRoundedRectangle(rr, fillBrush.Get());
    if (hasBorder && borderBrush)
      context->DrawRoundedRectangle(rr, borderBrush.Get(), m_BorderWidth);
  } else {
    // Plain rectangle swatch
    const D2D1_RECT_F rc = D2D1::RectF(left, top, right, bottom);

    if (fillBrush)
      context->FillRectangle(rc, fillBrush.Get());
    if (hasBorder && borderBrush)
      context->DrawRectangle(rc, borderBrush.Get(), m_BorderWidth);
  }
}
