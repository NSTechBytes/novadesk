/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ScrollbarRenderer.h"
#include "Widget.h"
#include "../render/Direct2DHelper.h"
#include <algorithm>

void ScrollbarRenderer::DrawArrowTriangle(ID2D1RenderTarget *context,
                                          D2D1_POINT_2F p1, D2D1_POINT_2F p2,
                                          D2D1_POINT_2F p3, ID2D1Brush *brush) {
  if (!context || !brush)
    return;
  Microsoft::WRL::ComPtr<ID2D1Factory> factory;
  context->GetFactory(&factory);
  if (!factory)
    return;

  Microsoft::WRL::ComPtr<ID2D1PathGeometry> path;
  if (SUCCEEDED(factory->CreatePathGeometry(&path))) {
    Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
    if (SUCCEEDED(path->Open(&sink))) {
      sink->BeginFigure(p1, D2D1_FIGURE_BEGIN_FILLED);
      sink->AddLine(p2);
      sink->AddLine(p3);
      sink->EndFigure(D2D1_FIGURE_END_CLOSED);
      sink->Close();
      context->FillGeometry(path.Get(), brush);
    }
  }
}

void ScrollbarRenderer::DrawScrollbars(const Widget &widget, Element *container,
                                       ID2D1RenderTarget *context,
                                       const GfxRect &bounds) {
  if (!container || !context || !container->GetShowScrollbar())
    return;

  // Ensure content extents are up to date before checking MaxScroll
  container->RecalcContentExtents();

  const bool isVertDragging = (widget.m_IsScrollbarDragging &&
                               widget.m_ScrollbarDragContainer == container &&
                               widget.m_ScrollbarDragIsVertical);
  const bool isHorizDragging = (widget.m_IsScrollbarDragging &&
                                widget.m_ScrollbarDragContainer == container &&
                                !widget.m_ScrollbarDragIsVertical);
  const bool isVertHovered =
      (widget.m_ScrollbarHoverContainer == container &&
       (widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::VerticalThumb ||
        widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::VerticalTrack ||
        widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::VerticalTopButton ||
        widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::VerticalBottomButton));
  const bool isHorizHovered =
      (widget.m_ScrollbarHoverContainer == container &&
       (widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::HorizontalThumb ||
        widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::HorizontalTrack ||
        widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::HorizontalLeftButton ||
        widget.m_ScrollbarHoverPart ==
            Widget::ScrollbarHitPart::HorizontalRightButton));

  const float inset = container->GetScrollbarInset();
  const float minThumbLen = container->GetScrollbarMinThumbLength();

  const bool hasVert = container->GetShowScrollbarY() &&
                       container->IsScrollableY() &&
                       container->GetMaxScrollY() > 0;
  const bool hasHoriz = container->GetShowScrollbarX() &&
                        container->IsScrollableX() &&
                        container->GetMaxScrollX() > 0;

  const bool hasButtons = container->GetShowScrollbarButtons();
  const float btnRadius = container->GetScrollbarButtonRadius();

  // Layout width: always the normal (non-hover) width — used for geometry,
  // track size, cross-axis offsets. Visual width: expands inward on hover/drag
  // (right/bottom edge stays pinned to container edge).
  const float layoutVertW = (float)container->GetScrollbarWidth();
  const float layoutHorizW = (float)container->GetScrollbarWidth();
  const float visualVertW = (float)(isVertDragging || isVertHovered
                                        ? container->GetScrollbarHoverWidth()
                                        : layoutVertW);
  const float visualHorizW = (float)(isHorizDragging || isHorizHovered
                                         ? container->GetScrollbarHoverWidth()
                                         : layoutHorizW);

  // 1. Vertical Scrollbar
  if (hasVert) {
    const float btnSize =
        hasButtons ? container->GetScrollbarButtonSize() : 0.0f;
    const int maxScrollY = container->GetMaxScrollY();
    const int contentH = container->GetContentHeight();

    // Track height uses layout width so cross-axis dimensions don't jump
    const float trackH =
        (std::max)(0.0f, (float)bounds.Height - (2.0f * inset) -
                             (2.0f * btnSize) -
                             (hasHoriz ? (layoutHorizW + inset) : 0.0f));

    // Pin right edge; visual width expands inward
    const float sbRight = (float)(bounds.X + bounds.Width) - inset;
    const float sbLeft = sbRight - visualVertW;
    const float fullTop = (float)bounds.Y + inset;
    const float fullBottom = (float)(bounds.Y + bounds.Height) - inset -
                             (hasHoriz ? (layoutHorizW + inset) : 0.0f);
    const float trackTop = fullTop + btnSize;

    // Top Button (Up Arrow)
    if (hasButtons && btnSize > 0.0f) {
      const bool isTopActive =
          (widget.m_ScrollbarActivePart ==
               Widget::ScrollbarHitPart::VerticalTopButton &&
           widget.m_ScrollbarDragContainer == container);
      const bool isTopHovered =
          (widget.m_ScrollbarHoverContainer == container &&
           widget.m_ScrollbarHoverPart ==
               Widget::ScrollbarHitPart::VerticalTopButton);

      D2D1_ROUNDED_RECT topBtnRect = D2D1::RoundedRect(
          D2D1::RectF(sbLeft, fullTop, sbRight, fullTop + btnSize), btnRadius,
          btnRadius);

      if (isTopActive || isTopHovered ||
          container->GetScrollbarButtonBgAlpha() > 0) {
        COLORREF bgCol = (isTopActive || isTopHovered)
                             ? container->GetScrollbarButtonHoverBgColor()
                             : container->GetScrollbarButtonBgColor();
        BYTE bgAlpha =
            isTopActive
                ? (BYTE)(std::min)(255,
                                   (int)container
                                           ->GetScrollbarButtonHoverBgAlpha() +
                                       40)
                : (isTopHovered ? container->GetScrollbarButtonHoverBgAlpha()
                                : container->GetScrollbarButtonBgAlpha());
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> btnBgBrush;
        if (Direct2D::CreateSolidBrush(context, bgCol, (float)bgAlpha / 255.0f,
                                       &btnBgBrush))
          context->FillRoundedRectangle(topBtnRect, btnBgBrush.Get());
      }

      COLORREF arrowCol =
          isTopActive ? container->GetScrollbarArrowActiveColor()
                      : (isTopHovered ? container->GetScrollbarArrowHoverColor()
                                      : container->GetScrollbarArrowColor());
      BYTE arrowAlpha =
          isTopActive ? container->GetScrollbarArrowActiveAlpha()
                      : (isTopHovered ? container->GetScrollbarArrowHoverAlpha()
                                      : container->GetScrollbarArrowAlpha());
      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> arrowBrush;
      if (Direct2D::CreateSolidBrush(context, arrowCol,
                                     (float)arrowAlpha / 255.0f, &arrowBrush)) {
        const float cx = sbLeft + visualVertW / 2.0f;
        const float cy = fullTop + btnSize / 2.0f;
        const float ahw = (std::min)(visualVertW * 0.28f, 3.5f);
        const float ahh = (std::min)(btnSize * 0.22f, 3.0f);
        DrawArrowTriangle(context, D2D1::Point2F(cx, cy - ahh),
                          D2D1::Point2F(cx - ahw, cy + ahh),
                          D2D1::Point2F(cx + ahw, cy + ahh), arrowBrush.Get());
      }
    }

    // Track
    if (trackH > 0.0f) {
      if (container->GetScrollbarTrackAlpha() > 0) {
        const float trackRadius = container->GetScrollbarTrackRadius();
        D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
            D2D1::RectF(sbLeft, trackTop, sbRight, trackTop + trackH),
            trackRadius, trackRadius);

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> trackBrush;
        if (Direct2D::CreateSolidBrush(
                context, container->GetScrollbarTrackColor(),
                (float)container->GetScrollbarTrackAlpha() / 255.0f,
                &trackBrush))
          context->FillRoundedRectangle(trackRect, trackBrush.Get());
      }

      // Thumb
      const float thumbH =
          (std::min)(trackH,
                     (std::max)(minThumbLen, trackH * ((float)bounds.Height /
                                                       (float)contentH)));
      const float thumbTravel = (std::max)(0.0f, trackH - thumbH);
      const float thumbY =
          (maxScrollY > 0) ? (thumbTravel * ((float)container->GetScrollY() /
                                             (float)maxScrollY))
                           : 0.0f;

      COLORREF thumbColor = container->GetScrollbarColor();
      BYTE thumbAlpha = container->GetScrollbarAlpha();
      if (isVertDragging) {
        thumbColor = container->GetScrollbarActiveColor();
        thumbAlpha = container->GetScrollbarActiveAlpha();
      } else if (isVertHovered) {
        thumbColor = container->GetScrollbarHoverColor();
        thumbAlpha = container->GetScrollbarHoverAlpha();
      }

      const float sbR = container->GetScrollbarRadius();
      D2D1_ROUNDED_RECT thumbRect =
          D2D1::RoundedRect(D2D1::RectF(sbLeft, trackTop + thumbY, sbRight,
                                        trackTop + thumbY + thumbH),
                            sbR, sbR);

      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> thumbBrush;
      if (Direct2D::CreateSolidBrush(context, thumbColor,
                                     (float)thumbAlpha / 255.0f, &thumbBrush))
        context->FillRoundedRectangle(thumbRect, thumbBrush.Get());
    }

    // Bottom Button (Down Arrow)
    if (hasButtons && btnSize > 0.0f) {
      const bool isBotActive =
          (widget.m_ScrollbarActivePart ==
               Widget::ScrollbarHitPart::VerticalBottomButton &&
           widget.m_ScrollbarDragContainer == container);
      const bool isBotHovered =
          (widget.m_ScrollbarHoverContainer == container &&
           widget.m_ScrollbarHoverPart ==
               Widget::ScrollbarHitPart::VerticalBottomButton);

      D2D1_ROUNDED_RECT botBtnRect = D2D1::RoundedRect(
          D2D1::RectF(sbLeft, fullBottom - btnSize, sbRight, fullBottom),
          btnRadius, btnRadius);

      if (isBotActive || isBotHovered ||
          container->GetScrollbarButtonBgAlpha() > 0) {
        COLORREF bgCol = (isBotActive || isBotHovered)
                             ? container->GetScrollbarButtonHoverBgColor()
                             : container->GetScrollbarButtonBgColor();
        BYTE bgAlpha =
            isBotActive
                ? (BYTE)(std::min)(255,
                                   (int)container
                                           ->GetScrollbarButtonHoverBgAlpha() +
                                       40)
                : (isBotHovered ? container->GetScrollbarButtonHoverBgAlpha()
                                : container->GetScrollbarButtonBgAlpha());
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> btnBgBrush;
        if (Direct2D::CreateSolidBrush(context, bgCol, (float)bgAlpha / 255.0f,
                                       &btnBgBrush))
          context->FillRoundedRectangle(botBtnRect, btnBgBrush.Get());
      }

      COLORREF arrowCol =
          isBotActive ? container->GetScrollbarArrowActiveColor()
                      : (isBotHovered ? container->GetScrollbarArrowHoverColor()
                                      : container->GetScrollbarArrowColor());
      BYTE arrowAlpha =
          isBotActive ? container->GetScrollbarArrowActiveAlpha()
                      : (isBotHovered ? container->GetScrollbarArrowHoverAlpha()
                                      : container->GetScrollbarArrowAlpha());
      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> arrowBrush;
      if (Direct2D::CreateSolidBrush(context, arrowCol,
                                     (float)arrowAlpha / 255.0f, &arrowBrush)) {
        const float cx = sbLeft + visualVertW / 2.0f;
        const float cy = fullBottom - btnSize / 2.0f;
        const float ahw = (std::min)(visualVertW * 0.28f, 3.5f);
        const float ahh = (std::min)(btnSize * 0.22f, 3.0f);
        DrawArrowTriangle(context, D2D1::Point2F(cx, cy + ahh),
                          D2D1::Point2F(cx - ahw, cy - ahh),
                          D2D1::Point2F(cx + ahw, cy - ahh), arrowBrush.Get());
      }
    }
  }

  // 2. Horizontal Scrollbar
  if (hasHoriz) {
    const float btnSize =
        hasButtons ? container->GetScrollbarButtonSize() : 0.0f;
    const int maxScrollX = container->GetMaxScrollX();
    const int contentW = container->GetContentWidth();

    // Track width uses layout width so cross-axis dimensions don't jump
    const float trackW =
        (std::max)(0.0f, (float)bounds.Width - (2.0f * inset) -
                             (2.0f * btnSize) -
                             (hasVert ? (layoutVertW + inset) : 0.0f));

    // Pin bottom edge; visual width expands inward (upward)
    const float sbBottom = (float)(bounds.Y + bounds.Height) - inset;
    const float sbTop = sbBottom - visualHorizW;
    const float fullLeft = (float)bounds.X + inset;
    const float fullRight = (float)(bounds.X + bounds.Width) - inset -
                            (hasVert ? (layoutVertW + inset) : 0.0f);
    const float trackLeft = fullLeft + btnSize;

    // Left Button (Left Arrow)
    if (hasButtons && btnSize > 0.0f) {
      const bool isLeftActive =
          (widget.m_ScrollbarActivePart ==
               Widget::ScrollbarHitPart::HorizontalLeftButton &&
           widget.m_ScrollbarDragContainer == container);
      const bool isLeftHovered =
          (widget.m_ScrollbarHoverContainer == container &&
           widget.m_ScrollbarHoverPart ==
               Widget::ScrollbarHitPart::HorizontalLeftButton);

      D2D1_ROUNDED_RECT leftBtnRect = D2D1::RoundedRect(
          D2D1::RectF(fullLeft, sbTop, fullLeft + btnSize, sbBottom), btnRadius,
          btnRadius);

      if (isLeftActive || isLeftHovered ||
          container->GetScrollbarButtonBgAlpha() > 0) {
        COLORREF bgCol = (isLeftActive || isLeftHovered)
                             ? container->GetScrollbarButtonHoverBgColor()
                             : container->GetScrollbarButtonBgColor();
        BYTE bgAlpha =
            isLeftActive
                ? (BYTE)(std::min)(255,
                                   (int)container
                                           ->GetScrollbarButtonHoverBgAlpha() +
                                       40)
                : (isLeftHovered ? container->GetScrollbarButtonHoverBgAlpha()
                                 : container->GetScrollbarButtonBgAlpha());
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> btnBgBrush;
        if (Direct2D::CreateSolidBrush(context, bgCol, (float)bgAlpha / 255.0f,
                                       &btnBgBrush))
          context->FillRoundedRectangle(leftBtnRect, btnBgBrush.Get());
      }

      COLORREF arrowCol =
          isLeftActive
              ? container->GetScrollbarArrowActiveColor()
              : (isLeftHovered ? container->GetScrollbarArrowHoverColor()
                               : container->GetScrollbarArrowColor());
      BYTE arrowAlpha =
          isLeftActive
              ? container->GetScrollbarArrowActiveAlpha()
              : (isLeftHovered ? container->GetScrollbarArrowHoverAlpha()
                               : container->GetScrollbarArrowAlpha());
      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> arrowBrush;
      if (Direct2D::CreateSolidBrush(context, arrowCol,
                                     (float)arrowAlpha / 255.0f, &arrowBrush)) {
        const float cx = fullLeft + btnSize / 2.0f;
        const float cy = sbTop + visualHorizW / 2.0f;
        const float ahw = (std::min)(visualHorizW * 0.28f, 3.5f);
        const float ahh = (std::min)(btnSize * 0.22f, 3.0f);
        DrawArrowTriangle(context, D2D1::Point2F(cx - ahh, cy),
                          D2D1::Point2F(cx + ahh, cy - ahw),
                          D2D1::Point2F(cx + ahh, cy + ahw), arrowBrush.Get());
      }
    }

    // Track
    if (trackW > 0.0f) {
      if (container->GetScrollbarTrackAlpha() > 0) {
        const float trackRadius = container->GetScrollbarTrackRadius();
        D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
            D2D1::RectF(trackLeft, sbTop, trackLeft + trackW, sbBottom),
            trackRadius, trackRadius);

        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> trackBrush;
        if (Direct2D::CreateSolidBrush(
                context, container->GetScrollbarTrackColor(),
                (float)container->GetScrollbarTrackAlpha() / 255.0f,
                &trackBrush))
          context->FillRoundedRectangle(trackRect, trackBrush.Get());
      }

      // Thumb
      const float thumbW =
          (std::min)(trackW,
                     (std::max)(minThumbLen, trackW * ((float)bounds.Width /
                                                       (float)contentW)));
      const float thumbTravel = (std::max)(0.0f, trackW - thumbW);
      const float thumbX =
          (maxScrollX > 0) ? (thumbTravel * ((float)container->GetScrollX() /
                                             (float)maxScrollX))
                           : 0.0f;

      COLORREF thumbColor = container->GetScrollbarColor();
      BYTE thumbAlpha = container->GetScrollbarAlpha();
      if (isHorizDragging) {
        thumbColor = container->GetScrollbarActiveColor();
        thumbAlpha = container->GetScrollbarActiveAlpha();
      } else if (isHorizHovered) {
        thumbColor = container->GetScrollbarHoverColor();
        thumbAlpha = container->GetScrollbarHoverAlpha();
      }

      const float sbR = container->GetScrollbarRadius();
      D2D1_ROUNDED_RECT thumbRect =
          D2D1::RoundedRect(D2D1::RectF(trackLeft + thumbX, sbTop,
                                        trackLeft + thumbX + thumbW, sbBottom),
                            sbR, sbR);

      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> thumbBrush;
      if (Direct2D::CreateSolidBrush(context, thumbColor,
                                     (float)thumbAlpha / 255.0f, &thumbBrush))
        context->FillRoundedRectangle(thumbRect, thumbBrush.Get());
    }

    // Right Button (Right Arrow)
    if (hasButtons && btnSize > 0.0f) {
      const bool isRightActive =
          (widget.m_ScrollbarActivePart ==
               Widget::ScrollbarHitPart::HorizontalRightButton &&
           widget.m_ScrollbarDragContainer == container);
      const bool isRightHovered =
          (widget.m_ScrollbarHoverContainer == container &&
           widget.m_ScrollbarHoverPart ==
               Widget::ScrollbarHitPart::HorizontalRightButton);

      D2D1_ROUNDED_RECT rightBtnRect = D2D1::RoundedRect(
          D2D1::RectF(fullRight - btnSize, sbTop, fullRight, sbBottom),
          btnRadius, btnRadius);

      if (isRightActive || isRightHovered ||
          container->GetScrollbarButtonBgAlpha() > 0) {
        COLORREF bgCol = (isRightActive || isRightHovered)
                             ? container->GetScrollbarButtonHoverBgColor()
                             : container->GetScrollbarButtonBgColor();
        BYTE bgAlpha =
            isRightActive
                ? (BYTE)(std::min)(255,
                                   (int)container
                                           ->GetScrollbarButtonHoverBgAlpha() +
                                       40)
                : (isRightHovered ? container->GetScrollbarButtonHoverBgAlpha()
                                  : container->GetScrollbarButtonBgAlpha());
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> btnBgBrush;
        if (Direct2D::CreateSolidBrush(context, bgCol, (float)bgAlpha / 255.0f,
                                       &btnBgBrush))
          context->FillRoundedRectangle(rightBtnRect, btnBgBrush.Get());
      }

      COLORREF arrowCol =
          isRightActive
              ? container->GetScrollbarArrowActiveColor()
              : (isRightHovered ? container->GetScrollbarArrowHoverColor()
                                : container->GetScrollbarArrowColor());
      BYTE arrowAlpha =
          isRightActive
              ? container->GetScrollbarArrowActiveAlpha()
              : (isRightHovered ? container->GetScrollbarArrowHoverAlpha()
                                : container->GetScrollbarArrowAlpha());
      Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> arrowBrush;
      if (Direct2D::CreateSolidBrush(context, arrowCol,
                                     (float)arrowAlpha / 255.0f, &arrowBrush)) {
        const float cx = fullRight - btnSize / 2.0f;
        const float cy = sbTop + visualHorizW / 2.0f;
        const float ahw = (std::min)(visualHorizW * 0.28f, 3.5f);
        const float ahh = (std::min)(btnSize * 0.22f, 3.0f);
        DrawArrowTriangle(context, D2D1::Point2F(cx + ahh, cy),
                          D2D1::Point2F(cx - ahh, cy - ahw),
                          D2D1::Point2F(cx - ahh, cy + ahw), arrowBrush.Get());
      }
    }
  }
}
