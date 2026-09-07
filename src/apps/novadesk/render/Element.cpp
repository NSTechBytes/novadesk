/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Element.h"
#include "../shared/Logging.h"
#include "Direct2DHelper.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

Element::Element(ElementType type, const std::wstring &id, int x, int y,
                 int width, int height)
    : m_Type(type), m_Id(id), m_X(x), m_Y(y) {
  m_Width = (width > 0) ? width : 0;
  m_Height = (height > 0) ? height : 0;
  m_WDefined = (width > 0);
  m_HDefined = (height > 0);
  m_ToolTipDisabled = false;
}

Element::~Element() {

  // Defensively remove self from parent container to prevent dangling
  // pointers.  Normally the Widget removal path calls
  // UpdateContainerForElement() before erasing, which clears this link.
  // This covers the case where an element is destroyed without going
  // through that path.
  if (m_ContainerElement) {
    m_ContainerElement->RemoveContainerItem(this);
    m_ContainerElement = nullptr;
  }
}

// Get the width of the element.
int Element::GetWidth() {
  int w = m_WDefined ? m_Width : GetAutoWidth();
  return w + m_PaddingLeft + m_PaddingRight;
}

// Get the height of the element.
int Element::GetHeight() {
  int h = m_HDefined ? m_Height : GetAutoHeight();
  return h + m_PaddingTop + m_PaddingBottom;
}

// Get the bounding box of the element.
GfxRect Element::GetBounds() {
  if (!m_Show) {
    return GfxRect(m_X, m_Y, 0, 0);
  }
  return GfxRect(m_X, m_Y, GetWidth(), GetHeight());
}

GfxRect Element::GetBackgroundBounds() { return GetBounds(); }

// Check if a point is within the element's bounds.
bool Element::HitTest(int x, int y) {
  if (!m_Show)
    return false;
  if (!m_HasTransformMatrix && m_Rotate == 0.0f) {
    GfxRect bounds = GetBounds();
    return (x >= bounds.X && x < bounds.X + bounds.Width && y >= bounds.Y &&
            y < bounds.Y + bounds.Height);
  }

  GfxRect bounds = GetBounds();
  float centerX = bounds.X + bounds.Width / 2.0f;
  float centerY = bounds.Y + bounds.Height / 2.0f;

  D2D1::Matrix3x2F matrix;

  if (m_HasTransformMatrix) {
    matrix = D2D1::Matrix3x2F(m_TransformMatrix[0], m_TransformMatrix[1],
                              m_TransformMatrix[2], m_TransformMatrix[3],
                              m_TransformMatrix[4], m_TransformMatrix[5]);
  } else {
    matrix =
        D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY));
  }

  // If inversion fails (degenerate matrix), fallback to standard bounds
  if (!matrix.Invert()) {
    return (x >= bounds.X && x < bounds.X + bounds.Width && y >= bounds.Y &&
            y < bounds.Y + bounds.Height);
  }

  D2D1_POINT_2F p = D2D1::Point2F((float)x, (float)y);
  D2D1_POINT_2F transformed = matrix.TransformPoint(p);

  return (
      transformed.x >= bounds.X && transformed.x < bounds.X + bounds.Width &&
      transformed.y >= bounds.Y && transformed.y < bounds.Y + bounds.Height);
}

// Check if the element has an action associated with it.
bool Element::HasAction(UINT message, WPARAM wParam) const {
  switch (message) {
  case WM_LBUTTONUP:
    return m_OnLeftMouseUpCallbackId != -1;
  case WM_LBUTTONDOWN:
    return m_OnLeftMouseDownCallbackId != -1;
  case WM_LBUTTONDBLCLK:
    return m_OnLeftDoubleClickCallbackId != -1;
  case WM_RBUTTONUP:
    return m_OnRightMouseUpCallbackId != -1;
  case WM_RBUTTONDOWN:
    return m_OnRightMouseDownCallbackId != -1;
  case WM_RBUTTONDBLCLK:
    return m_OnRightDoubleClickCallbackId != -1;
  case WM_MBUTTONUP:
    return m_OnMiddleMouseUpCallbackId != -1;
  case WM_MBUTTONDOWN:
    return m_OnMiddleMouseDownCallbackId != -1;
  case WM_MBUTTONDBLCLK:
    return m_OnMiddleDoubleClickCallbackId != -1;
  case WM_XBUTTONUP:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
      return m_OnX1MouseUpCallbackId != -1;
    else
      return m_OnX2MouseUpCallbackId != -1;
  case WM_XBUTTONDOWN:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
      return m_OnX1MouseDownCallbackId != -1;
    else
      return m_OnX2MouseDownCallbackId != -1;
  case WM_XBUTTONDBLCLK:
    if (GET_XBUTTON_WPARAM(wParam) == XBUTTON1)
      return m_OnX1DoubleClickCallbackId != -1;
    else
      return m_OnX2DoubleClickCallbackId != -1;
  case WM_MOUSEWHEEL:
    if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
      return m_OnScrollUpCallbackId != -1;
    else
      return m_OnScrollDownCallbackId != -1;
  case WM_MOUSEHWHEEL:
    if (GET_WHEEL_DELTA_WPARAM(wParam) > 0)
      return m_OnScrollRightCallbackId != -1;
    else
      return m_OnScrollLeftCallbackId != -1;
  case WM_MOUSEMOVE:
    return m_OnMouseOverCallbackId != -1 || m_OnMouseLeaveCallbackId != -1;
  }
  return false;
}

// Check if the element has any interactive mouse action.
bool Element::HasMouseAction() const {
  return m_OnLeftMouseUpCallbackId != -1 || m_OnLeftMouseDownCallbackId != -1 ||
         m_OnLeftDoubleClickCallbackId != -1 ||
         m_OnRightMouseUpCallbackId != -1 ||
         m_OnRightMouseDownCallbackId != -1 ||
         m_OnRightDoubleClickCallbackId != -1 ||
         m_OnMiddleMouseUpCallbackId != -1 ||
         m_OnMiddleMouseDownCallbackId != -1 ||
         m_OnMiddleDoubleClickCallbackId != -1 ||
         m_OnX1MouseUpCallbackId != -1 || m_OnX1MouseDownCallbackId != -1 ||
         m_OnX1DoubleClickCallbackId != -1 || m_OnX2MouseUpCallbackId != -1 ||
         m_OnX2MouseDownCallbackId != -1 || m_OnX2DoubleClickCallbackId != -1 ||
         m_OnScrollUpCallbackId != -1 || m_OnScrollDownCallbackId != -1 ||
         m_OnScrollLeftCallbackId != -1 || m_OnScrollRightCallbackId != -1 ||
         m_OnMouseOverCallbackId != -1 || m_OnMouseLeaveCallbackId != -1 ||
         m_OnDragStartCallbackId != -1 || m_OnDragCallbackId != -1 ||
         m_OnDragEndCallbackId != -1;
}

bool Element::HasDragAction() const {
  return m_OnDragStartCallbackId != -1 || m_OnDragCallbackId != -1 ||
         m_OnDragEndCallbackId != -1;
}

bool Element::HasDropAction() const {
  return m_OnDropCallbackId != -1 || m_OnDragEnterCallbackId != -1 ||
         m_OnDragOverCallbackId != -1 || m_OnDragLeaveCallbackId != -1;
}

// Set the padding for the element.
void Element::SetPadding(int left, int top, int right, int bottom) {
  m_PaddingLeft = left;
  m_PaddingTop = top;
  m_PaddingRight = right;
  m_PaddingBottom = bottom;
}

void Element::RemoveContainerItem(Element *item) {
  m_ContainerItems.erase(
      std::remove(m_ContainerItems.begin(), m_ContainerItems.end(), item),
      m_ContainerItems.end());
}

void Element::ClearContainerItems() { m_ContainerItems.clear(); }

void Element::SetScrollX(int x) {
  RecalcContentExtents();
  int maxX = GetMaxScrollX();
  m_ScrollX = (x < 0) ? 0 : ((x > maxX) ? maxX : x);
}

void Element::SetScrollY(int y) {
  RecalcContentExtents();
  int maxY = GetMaxScrollY();
  m_ScrollY = (y < 0) ? 0 : ((y > maxY) ? maxY : y);
}

int Element::GetMaxScrollX() const {
  int containerW = m_WDefined ? m_Width : 0;
  return (m_ContentWidth > containerW) ? (m_ContentWidth - containerW) : 0;
}

int Element::GetMaxScrollY() const {
  int containerH = m_HDefined ? m_Height : 0;
  return (m_ContentHeight > containerH) ? (m_ContentHeight - containerH) : 0;
}

void Element::RecalcContentExtents() {
  int maxRight = 0;
  int maxBottom = 0;
  for (Element *child : m_ContainerItems) {
    if (!child || !child->IsVisible())
      continue;
    int right = child->GetX() + child->GetWidth();
    int bottom = child->GetY() + child->GetHeight();
    if (right > maxRight)
      maxRight = right;
    if (bottom > maxBottom)
      maxBottom = bottom;
  }
  m_ContentWidth = maxRight;
  m_ContentHeight = maxBottom;
}

bool Element::IsScrollableX() const {
  if (m_OverflowX == OverflowMode::Hidden)
    return false;
  if (m_OverflowX == OverflowMode::Scroll)
    return true;
  // Auto: scrollable only if content exceeds
  int containerW = m_WDefined ? m_Width : 0;
  return m_ContentWidth > containerW;
}

bool Element::IsScrollableY() const {
  if (m_OverflowY == OverflowMode::Hidden)
    return false;
  if (m_OverflowY == OverflowMode::Scroll)
    return true;
  // Auto: scrollable only if content exceeds
  int containerH = m_HDefined ? m_Height : 0;
  return m_ContentHeight > containerH;
}

void Element::SetOverflow(const std::wstring &value) {
  if (value == L"hidden") {
    m_OverflowX = OverflowMode::Hidden;
    m_OverflowY = OverflowMode::Hidden;
  } else if (value == L"auto" || value == L"both") {
    m_OverflowX = OverflowMode::Auto;
    m_OverflowY = OverflowMode::Auto;
  } else if (value == L"scroll") {
    m_OverflowX = OverflowMode::Scroll;
    m_OverflowY = OverflowMode::Scroll;
  } else if (value == L"vertical") {
    m_OverflowX = OverflowMode::Hidden;
    m_OverflowY = OverflowMode::Auto;
  } else if (value == L"horizontal") {
    m_OverflowX = OverflowMode::Auto;
    m_OverflowY = OverflowMode::Hidden;
  }
}

// Render the background of the element.
void Element::RenderBackground(ID2D1DeviceContext *context) {
  if (!m_HasSolidColor)
    return;

  context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                                        : D2D1_ANTIALIAS_MODE_ALIASED);

  GfxRect bounds = GetBackgroundBounds();
  D2D1_RECT_F rect = D2D1::RectF((float)bounds.X, (float)bounds.Y,
                                 (float)(bounds.X + bounds.Width),
                                 (float)(bounds.Y + bounds.Height));

  if (rect.right <= rect.left || rect.bottom <= rect.top)
    return;

  Microsoft::WRL::ComPtr<ID2D1Brush> brush;
  Direct2D::CreateBrushFromGradientOrColor(context, rect, &m_SolidGradient,
                                           m_SolidColor, m_SolidAlpha / 255.0f,
                                           brush.GetAddressOf());

  if (brush) {
    if (m_CornerRadius > 0) {
      D2D1_ROUNDED_RECT roundedRect =
          D2D1::RoundedRect(rect, (float)m_CornerRadius, (float)m_CornerRadius);
      context->FillRoundedRectangle(roundedRect, brush.Get());
    } else {
      context->FillRectangle(rect, brush.Get());
    }
  }
}

// Render the bevel of the element.
void Element::RenderBevel(ID2D1DeviceContext *context) {
  if (m_BevelType == 0 || m_BevelWidth <= 0)
    return;

  context->SetAntialiasMode(m_AntiAlias ? D2D1_ANTIALIAS_MODE_PER_PRIMITIVE
                                        : D2D1_ANTIALIAS_MODE_ALIASED);

  const float pad = 2.0f;
  GfxRect bounds = GetBounds();
  D2D1_RECT_F rect = D2D1::RectF((float)bounds.X - pad, (float)bounds.Y - pad,
                                 (float)(bounds.X + bounds.Width) + pad,
                                 (float)(bounds.Y + bounds.Height) + pad);

  Microsoft::WRL::ComPtr<ID2D1Brush> highlightBrush;
  Microsoft::WRL::ComPtr<ID2D1Brush> shadowBrush;
  Direct2D::CreateBrushFromGradientOrColor(context, rect, &m_BevelGradient,
                                           m_BevelColor, m_BevelAlpha / 255.0f,
                                           highlightBrush.GetAddressOf());
  Direct2D::CreateBrushFromGradientOrColor(
      context, rect, &m_BevelGradient2, m_BevelColor2, m_BevelAlpha2 / 255.0f,
      shadowBrush.GetAddressOf());

  float offset = m_BevelWidth / 2.0f;

  switch (m_BevelType) {
  case 1: // Raised
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset),
                      D2D1::Point2F(rect.right - offset, rect.top + offset),
                      highlightBrush.Get(), (float)m_BevelWidth);
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset),
                      D2D1::Point2F(rect.left + offset, rect.bottom - offset),
                      highlightBrush.Get(), (float)m_BevelWidth);
    context->DrawLine(D2D1::Point2F(rect.right - offset, rect.top + offset),
                      D2D1::Point2F(rect.right - offset, rect.bottom - offset),
                      shadowBrush.Get(), (float)m_BevelWidth);
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.bottom - offset),
                      D2D1::Point2F(rect.right - offset, rect.bottom - offset),
                      shadowBrush.Get(), (float)m_BevelWidth);
    break;

  case 2: // Sunken
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset),
                      D2D1::Point2F(rect.right - offset, rect.top + offset),
                      shadowBrush.Get(), (float)m_BevelWidth);
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset),
                      D2D1::Point2F(rect.left + offset, rect.bottom - offset),
                      shadowBrush.Get(), (float)m_BevelWidth);
    context->DrawLine(D2D1::Point2F(rect.right - offset, rect.top + offset),
                      D2D1::Point2F(rect.right - offset, rect.bottom - offset),
                      highlightBrush.Get(), (float)m_BevelWidth);
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.bottom - offset),
                      D2D1::Point2F(rect.right - offset, rect.bottom - offset),
                      highlightBrush.Get(), (float)m_BevelWidth);
    break;

  case 3: // Emboss
  {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> midBrush;
    Direct2D::CreateSolidBrush(context, m_BevelColor,
                               (m_BevelAlpha / 2.0f) / 255.0f,
                               midBrush.GetAddressOf());
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset),
                      D2D1::Point2F(rect.right - offset, rect.top + offset),
                      highlightBrush.Get(), (float)m_BevelWidth);
    context->DrawLine(D2D1::Point2F(rect.left + offset, rect.top + offset),
                      D2D1::Point2F(rect.left + offset, rect.bottom - offset),
                      midBrush.Get(), (float)m_BevelWidth);
  } break;

  case 4: // Pillow
    if (m_BevelGradient.type != GRADIENT_NONE &&
        !m_BevelGradient.stops.empty()) {
      for (int i = 0; i < m_BevelWidth; i++) {
        D2D1_RECT_F r = D2D1::RectF(rect.left + i, rect.top + i, rect.right - i,
                                    rect.bottom - i);
        context->DrawRectangle(r, highlightBrush.Get(), 1.0f);
      }
    } else {
      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fadeBrush;
      Direct2D::CreateSolidBrush(context, m_BevelColor, 1.0f,
                                 fadeBrush.GetAddressOf());
      if (fadeBrush) {
        for (int i = 0; i < m_BevelWidth; i++) {
          float alpha =
              (m_BevelAlpha / 255.0f) * (1.0f - (float)i / m_BevelWidth);
          fadeBrush->SetOpacity(alpha);
          D2D1_RECT_F r = D2D1::RectF(rect.left + i, rect.top + i,
                                      rect.right - i, rect.bottom - i);
          context->DrawRectangle(r, fadeBrush.Get(), 1.0f);
        }
      }
    }
    break;
  }
}

void Element::ApplyRenderTransform(ID2D1DeviceContext *context,
                                   D2D1_MATRIX_3X2_F &originalTransform) {
  if (!context)
    return;
  context->GetTransform(&originalTransform);

  if (!m_HasTransformMatrix && m_Rotate == 0.0f) {
    return;
  }

  if (m_HasTransformMatrix) {
    D2D1::Matrix3x2F matrix = D2D1::Matrix3x2F(
        m_TransformMatrix[0], m_TransformMatrix[1], m_TransformMatrix[2],
        m_TransformMatrix[3], m_TransformMatrix[4], m_TransformMatrix[5]);
    context->SetTransform(matrix * originalTransform);
    return;
  }

  GfxRect bounds = GetBounds();
  float centerX = bounds.X + bounds.Width / 2.0f;
  float centerY = bounds.Y + bounds.Height / 2.0f;
  context->SetTransform(
      D2D1::Matrix3x2F::Rotation(m_Rotate, D2D1::Point2F(centerX, centerY)) *
      originalTransform);
}

void Element::RestoreRenderTransform(
    ID2D1DeviceContext *context, const D2D1_MATRIX_3X2_F &originalTransform) {
  if (!context)
    return;
  context->SetTransform(originalTransform);
}

bool Element::CreateGeometry(
    ID2D1Factory *factory,
    Microsoft::WRL::ComPtr<ID2D1Geometry> &geometry) const {
  if (!factory)
    return false;

  if (m_CornerRadius > 0) {
    const GfxRect bounds = const_cast<Element *>(this)->GetBackgroundBounds();
    D2D1_ROUNDED_RECT rect;
    rect.rect = D2D1::RectF(
        static_cast<float>(bounds.X), static_cast<float>(bounds.Y),
        static_cast<float>(bounds.X + bounds.Width),
        static_cast<float>(bounds.Y + bounds.Height));
    rect.radiusX = static_cast<float>(m_CornerRadius);
    rect.radiusY = static_cast<float>(m_CornerRadius);
    Microsoft::WRL::ComPtr<ID2D1RoundedRectangleGeometry> rounded;
    if (SUCCEEDED(factory->CreateRoundedRectangleGeometry(rect, &rounded))) {
      geometry = rounded;
      return true;
    }
  }

  return false;
}
