/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "DropDownPopup.h"
#include "DesktopManager.h"
#include "Widget.h"
#include "../render/Direct2DHelper.h"
#include "../render/DropDownElement.h"
#include "../scripting/quickjs/engine/JSEngine.h"

#include <wrl/client.h>
#include <algorithm>
#include <cmath>

namespace {
constexpr UINT OUTSIDE_CLICK_MESSAGE = WM_APP + 0x42;
constexpr UINT_PTR SHOW_DESKTOP_TIMER = 1;
constexpr int kScrollHitWidth = 12;   ///< Grab width on the right edge.
constexpr int kThumbMinHeight = 24;
constexpr wchar_t kEllipsis = L'\x2026';
/// Vertical room kept above and below a row's text when the row is taller than
/// the font, so labels sit centred rather than hanging from the top.
constexpr int kTextVerticalPad = 3;

HHOOK g_OutsideClickHook = nullptr;
DropDownPopup *g_OutsideClickPopup = nullptr;

int ClampInt(int v, int lo, int hi) { return (std::max)(lo, (std::min)(v, hi)); }

/// Returns the widget-relative bounding rect of an element, correctly handling
/// elements that are children of a layout box. A contained element's m_X/m_Y
/// are container-local, so we walk up the container chain and accumulate the
/// offsets to get the true widget-client-area position.
GfxRect GetAbsoluteBounds(const Element *element) {
  if (!element)
    return GfxRect(0, 0, 0, 0);
  GfxRect b = const_cast<Element *>(element)->GetBounds();
  const Element *container = element->GetContainer();
  while (container) {
    GfxRect cb = const_cast<Element *>(container)->GetBounds();
    b.X += cb.X;
    b.Y += cb.Y;
    container = container->GetContainer();
  }
  return b;
}

/// Height of the font for a drop-down's point size (negative = character
/// height, which is what CreateFontIndirectW expects for UI text).
HFONT CreateDropDownFont(const DropDownElement *dropDown) {
  const float size = dropDown ? static_cast<float>(dropDown->m_FontSize) : 12.0f;
  const int weight = dropDown ? dropDown->m_FontWeight : 400;
  const std::wstring fallback = L"Segoe UI";
  const std::wstring &face = dropDown ? dropDown->m_FontFace : fallback;
  LOGFONTW lf{};
  lf.lfHeight = -MulDiv(static_cast<int>(size + 0.5f),
                        GetDeviceCaps(nullptr, LOGPIXELSY), 72);
  lf.lfWeight = weight;
  lf.lfCharSet = DEFAULT_CHARSET;
  lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lf.lfQuality = CLEARTYPE_QUALITY;
  lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  lstrcpynW(lf.lfFaceName, face.c_str(), LF_FACESIZE);
  return CreateFontIndirectW(&lf);
}
} // namespace

DropDownPopup::DropDownPopup(Widget *widget, DropDownElement *dropDown)
    : m_Widget(widget), m_DropDown(dropDown) {}

DropDownPopup::~DropDownPopup() { Close(); }

void DropDownPopup::LayoutRows() {
  if (!m_DropDown)
    return;
  m_RowHeight = (std::max)(12, m_DropDown->m_PopupItemHeight);
  m_Padding = (std::max)(0, m_DropDown->m_PopupPadding);
  const int maxVisible = (std::max)(1, m_DropDown->m_PopupMaxVisibleItems);
  const int count = (std::max)(0, m_DropDown->OptionCount());
  m_VisibleRows = (count > 0) ? (std::min)(count, maxVisible) : 1;
  m_Scrollable = count > m_VisibleRows;
  m_Height = m_Padding * 2 + m_VisibleRows * m_RowHeight;

  // Width: at least the control, wide enough for the longest label, capped to
  // the work area.
  RECT r{};
  GfxRect b = m_DropDown->GetBounds();
  GetWindowRect(m_Widget ? m_Widget->GetWindow() : nullptr, &r);
  int width = (std::max)(1, b.Width);
  HDC dc = GetDC(m_hWnd ? m_hWnd : GetDesktopWindow());
  if (dc) {
    HFONT oldFont = m_Font ? static_cast<HFONT>(SelectObject(dc, m_Font))
                          : nullptr;
    for (const DropDownOption &option : m_DropDown->Options()) {
      SIZE sz{};
      if (GetTextExtentPoint32W(dc, option.label.c_str(),
                                static_cast<int>(option.label.length()), &sz)) {
        const int needed = static_cast<int>(sz.cx) + m_Padding * 2 +
                           kScrollHitWidth + 8;
        width = (std::max)(width, needed);
      }
    }
    if (oldFont)
      SelectObject(dc, oldFont);
    ReleaseDC(m_hWnd ? m_hWnd : GetDesktopWindow(), dc);
  }
  HMONITOR mon = MonitorFromPoint({r.left + b.X, r.top + b.Y},
                                  MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{sizeof(mi)};
  if (GetMonitorInfoW(mon, &mi)) {
    const int workWidth = mi.rcWork.right - mi.rcWork.left;
    width = ClampInt(width, 1, workWidth);
  }
  m_Width = width;
}

void DropDownPopup::Show() {
  static ATOM atom = 0;
  if (!atom) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"NovadeskDropDownPopup";
    // Paint() presents a fully rendered back buffer, so Windows must not erase
    // the client area with a separate white frame first.
    wc.hbrBackground = nullptr;
    atom = RegisterClassW(&wc);
  }
  if (!m_Widget || !m_DropDown)
    return;
  if (m_hWnd) {
    BringWindowToTop(m_hWnd);
    SetForegroundWindow(m_hWnd);
    return;
  }

  m_Font = CreateDropDownFont(m_DropDown);
  LayoutRows();
  m_OriginalIndex = m_DropDown->GetSelectedIndex();
  m_EscapeIndex = m_OriginalIndex;
  m_Highlight = m_OriginalIndex;
  m_FirstVisible = 0;
  if (m_Highlight >= 0) {
    // Centre the current choice in the visible window instead of pinning it to
    // the first row.
    const int count = m_DropDown->OptionCount();
    m_FirstVisible = ClampInt(m_Highlight - m_VisibleRows / 2, 0,
                              (std::max)(0, count - m_VisibleRows));
  }
  m_Canceled = false;
  m_MouseActive = false;
  m_DraggingThumb = false;

  RECT r{};
  GetWindowRect(m_Widget->GetWindow(), &r);
  GfxRect b = GetAbsoluteBounds(m_DropDown);
  int x = r.left + b.X;
  int y = r.top + b.Y + b.Height;
  HMONITOR mon = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  if (y + m_Height > mi.rcWork.bottom)
    y = r.top + b.Y - m_Height;
  y = ClampInt(y, mi.rcWork.top, (std::max)(mi.rcWork.top, mi.rcWork.bottom - m_Height));
  x = ClampInt(x, mi.rcWork.left,
               (std::max)(mi.rcWork.left, mi.rcWork.right - m_Width));

  Widget::IncrementDropDownCount();
  m_hWnd = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"NovadeskDropDownPopup", L"",
      WS_POPUP | WS_BORDER | WS_CLIPCHILDREN, x, y, m_Width, m_Height,
      m_Widget->GetWindow(), nullptr, GetModuleHandleW(nullptr), this);
  if (!m_hWnd) {
    Widget::DecrementDropDownCount();
    if (m_Font) {
      DeleteObject(m_Font);
      m_Font = nullptr;
    }
    return;
  }
  SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, m_Width, m_Height, SWP_SHOWWINDOW);
  BringWindowToTop(m_hWnd);
  SetForegroundWindow(m_hWnd);
  InstallOutsideClickHook();
  m_ShowDesktopWasActive = System::GetShowDesktop();
  SetTimer(m_hWnd, SHOW_DESKTOP_TIMER, 100, nullptr);

  if (m_DropDown->m_OnOpenCallbackId != -1)
    JSEngine::CallEventCallbackWithText(m_DropDown->m_OnOpenCallbackId,
                                        m_Widget, m_DropDown->SelectedValue());
}

void DropDownPopup::UpdatePosition() {
  if (!m_hWnd || !m_Widget || !m_DropDown)
    return;
  const int previousWidth = m_Width;
  const int previousHeight = m_Height;
  LayoutRows();
  RECT r{};
  GetWindowRect(m_Widget->GetWindow(), &r);
  GfxRect b = GetAbsoluteBounds(m_DropDown);
  int x = r.left + b.X;
  int y = r.top + b.Y + b.Height;
  HMONITOR mon = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  // Keep the flip decision stable: only reconsider above/below when the popup
  // actually changed size, otherwise a resize would make it jump sides.
  const bool resized = (m_Width != previousWidth || m_Height != previousHeight);
  RECT current{};
  if (!resized && GetWindowRect(m_hWnd, &current)) {
    if (current.top >= r.top + b.Y)
      y = r.top + b.Y - m_Height;
  } else if (y + m_Height > mi.rcWork.bottom) {
    y = r.top + b.Y - m_Height;
  }
  y = ClampInt(y, mi.rcWork.top,
               (std::max)(mi.rcWork.top, mi.rcWork.bottom - m_Height));
  x = ClampInt(x, mi.rcWork.left,
               (std::max)(mi.rcWork.left, mi.rcWork.right - m_Width));
  SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, m_Width, m_Height, SWP_NOACTIVATE);
  InvalidateRect(m_hWnd, nullptr, FALSE);
}

void DropDownPopup::Close() {
  if (m_Closing)
    return;
  m_Closing = true;
  // Clear the element's visual state immediately before anything else so
  // the very first widget repaint (triggered below) shows the idle border
  // and down-chevron, not the open/hovered state.
  if (m_DropDown && m_Widget && Widget::IsValid(m_Widget) &&
      m_Widget->FindElementById(m_DropDown->GetId()) == m_DropDown) {
    m_DropDown->m_Open = false;
    m_DropDown->m_Hovered = false;
  }
  if (m_hWnd && !m_Canceled && m_Widget && m_DropDown &&
      m_DropDown->m_OnCloseCallbackId != -1) {
    JSEngine::CallEventCallbackWithText(m_DropDown->m_OnCloseCallbackId,
                                        m_Widget, m_DropDown->SelectedValue());
  }
  FlushWidgetRedraw();
  RemoveOutsideClickHook();
  if (m_hWnd) {
    KillTimer(m_hWnd, SHOW_DESKTOP_TIMER);
  }
  if (m_hWnd) {
    Widget::DecrementDropDownCount();
    ReleaseCapture();
    DestroyWindow(m_hWnd);
    m_hWnd = nullptr;
  }
  if (m_Font) {
    DeleteObject(m_Font);
    m_Font = nullptr;
  }
  // Redraw the widget now that the popup is gone so the dropdown control
  // immediately paints its idle state (normal border, down-chevron).
  if (m_Widget && Widget::IsValid(m_Widget))
    m_Widget->Redraw();
  m_Closing = false;
}

bool DropDownPopup::ElementStillValid() const {
  if (!m_Widget || !m_DropDown)
    return false;
  Element *found = m_Widget->FindElementById(m_DropDown->GetId());
  return found == static_cast<Element *>(m_DropDown);
}

void DropDownPopup::FlushWidgetRedraw() {
  if (!m_WidgetNeedsRedraw)
    return;
  if (m_Widget && Widget::IsValid(m_Widget))
    m_Widget->Redraw();
  m_WidgetNeedsRedraw = false;
}

void DropDownPopup::SetHighlight(int index, bool scrollIntoView) {
  const int count =
      (m_DropDown && ElementStillValid()) ? m_DropDown->OptionCount() : 0;
  if (count <= 0)
    index = -1;
  else
    index = ClampInt(index, 0, count - 1);
  if (scrollIntoView && index >= 0) {
    if (index < m_FirstVisible)
      m_FirstVisible = index;
    else if (index >= m_FirstVisible + m_VisibleRows)
      m_FirstVisible = index - m_VisibleRows + 1;
    m_FirstVisible = ClampInt(m_FirstVisible, 0,
                              (std::max)(0, count - m_VisibleRows));
  }
  if (index == m_Highlight)
    return;
  m_Highlight = index;
  if (m_hWnd)
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void DropDownPopup::ScrollBy(int rows) {
  if (!m_DropDown || !ElementStillValid())
    return;
  const int maxFirst =
      (std::max)(0, m_DropDown->OptionCount() - m_VisibleRows);
  const int next = ClampInt(m_FirstVisible + rows, 0, maxFirst);
  if (next == m_FirstVisible)
    return;
  m_FirstVisible = next;
  InvalidateRect(m_hWnd, nullptr, FALSE);
}

int DropDownPopup::RowIndexAt(POINT clientPt) const {
  if (!m_DropDown || !ElementStillValid())
    return -1;
  const int row = (clientPt.y - m_Padding) / (std::max)(1, m_RowHeight);
  if (row < 0)
    return -1;
  const int index = m_FirstVisible + row;
  if (index >= m_DropDown->OptionCount())
    return -1;
  return index;
}

void DropDownPopup::SelectFromKeyboard(int index) {
  m_MouseActive = false;
  SetHighlight(index, true);
  // Immediately apply the highlighted selection to the dropdown so the
  // control reflects the current arrow-key position. The popup stays open;
  // Enter or a click will confirm and close. Escape restores m_OriginalIndex.
  ApplyHighlighted();
}

void DropDownPopup::ApplyHighlighted() {
  // Applies the current highlight to the dropdown's selected index and fires
  // onChange, but does NOT close the popup. Used by keyboard navigation so
  // the control updates live while the user browses with arrow keys.
  if (!m_DropDown || !ElementStillValid() || m_Highlight < 0)
    return;
  if (m_Highlight >= m_DropDown->OptionCount())
    return;
  if (m_DropDown->GetSelectedIndex() == m_Highlight)
    return; // no change, skip the callback
  m_DropDown->SetSelectedIndex(m_Highlight);
  // Advance m_OriginalIndex so Paint() highlights the newly committed row
  // rather than leaving the old row lit up.
  m_OriginalIndex = m_Highlight;
  m_WidgetNeedsRedraw = true;
  if (m_Widget)
    m_Widget->NotifyDropDownChange(m_DropDown);
  // NotifyDropDownChange may have destroyed the widget or the element;
  // re-validate before touching anything else.
  if (!Widget::IsValid(m_Widget) || !ElementStillValid())
    return;
  // Redraw the dropdown control itself so it shows the updated label.
  FlushWidgetRedraw();
  m_WidgetNeedsRedraw = false;
}

void DropDownPopup::CommitHighlighted() {
  if (!m_DropDown || !ElementStillValid() || m_Highlight < 0)
    return;
  if (m_Highlight >= m_DropDown->OptionCount())
    return;
  m_DropDown->SetSelectedIndex(m_Highlight);
  m_WidgetNeedsRedraw = true;
  if (m_Widget)
    m_Widget->NotifyDropDownChange(m_DropDown);
  if (!Widget::IsValid(m_Widget))
    return;
  // The WM_LBUTTONUP that paired with this WM_LBUTTONDOWN will be delivered
  // to the widget window after the popup closes. If another dropdown element
  // sits at that screen position it would immediately open a new popup. Tell
  // the widget to suppress the next dropdown open from WM_LBUTTONUP.
  if (m_Widget)
    m_Widget->SetSuppressNextDropDownOpen(true);
  Close();
}

void DropDownPopup::InstallOutsideClickHook() {
  if (g_OutsideClickPopup == this && g_OutsideClickHook)
    return;
  if (g_OutsideClickHook) {
    UnhookWindowsHookEx(g_OutsideClickHook);
    g_OutsideClickHook = nullptr;
    g_OutsideClickPopup = nullptr;
  }
  g_OutsideClickPopup = this;
  g_OutsideClickHook = SetWindowsHookExW(WH_MOUSE_LL, OutsideClickMouseHook,
                                         GetModuleHandleW(nullptr), 0);
  if (!g_OutsideClickHook)
    g_OutsideClickPopup = nullptr;
}

void DropDownPopup::RemoveOutsideClickHook() {
  if (g_OutsideClickPopup != this)
    return;
  if (g_OutsideClickHook)
    UnhookWindowsHookEx(g_OutsideClickHook);
  g_OutsideClickHook = nullptr;
  g_OutsideClickPopup = nullptr;
}

LRESULT CALLBACK DropDownPopup::OutsideClickMouseHook(int code, WPARAM message,
                                                      LPARAM data) {
  if (code >= 0 && g_OutsideClickPopup && g_OutsideClickPopup->m_hWnd &&
      (message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
       message == WM_MBUTTONDOWN)) {
    const MSLLHOOKSTRUCT *info = reinterpret_cast<const MSLLHOOKSTRUCT *>(data);
    RECT popupRect{};
    GetWindowRect(g_OutsideClickPopup->m_hWnd, &popupRect);
    const bool inside = PtInRect(&popupRect, info->pt) != FALSE;
    if (!inside)
      PostMessageW(g_OutsideClickPopup->m_hWnd, OUTSIDE_CLICK_MESSAGE, 0, 0);
  }
  return CallNextHookEx(g_OutsideClickHook, code, message, data);
}

LRESULT CALLBACK DropDownPopup::WndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
  DropDownPopup *popup =
      reinterpret_cast<DropDownPopup *>(GetWindowLongPtrW(h, GWLP_USERDATA));
  if (m == WM_NCCREATE) {
    popup = static_cast<DropDownPopup *>(
        reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
    SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(popup));
    popup->m_hWnd = h;
  }
  return popup ? popup->Handle(m, w, l) : DefWindowProcW(h, m, w, l);
}

void DropDownPopup::Paint(HDC targetDc) {
  RECT clientRect{};
  GetClientRect(m_hWnd, &clientRect);
  const int clientWidth = clientRect.right - clientRect.left;
  const int clientHeight = clientRect.bottom - clientRect.top;
  if (clientWidth <= 0 || clientHeight <= 0)
    return;

  DropDownElement *dropDown = ElementStillValid() ? m_DropDown : nullptr;
  // The tick always stays on the row that was selected when the popup opened.
  // It never follows the hover cursor.
  const COLORREF bgColor =
      dropDown ? dropDown->m_PopupBackground : RGB(36, 36, 44);
  const COLORREF hoverColor =
      dropDown ? dropDown->m_PopupHoverColor : RGB(52, 52, 64);
  const COLORREF selectedColor =
      dropDown ? dropDown->m_PopupSelectedColor : RGB(59, 130, 246);
  const COLORREF textColor =
      dropDown ? dropDown->m_PopupTextColor : RGB(228, 228, 231);

  HDC dc = CreateCompatibleDC(targetDc);
  if (!dc)
    return;
  HBITMAP bitmap = CreateCompatibleBitmap(targetDc, clientWidth, clientHeight);
  if (!bitmap) {
    DeleteDC(dc);
    return;
  }
  HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
  HGDIOBJ oldFont = m_Font ? SelectObject(dc, m_Font) : nullptr;

  RECT rc{0, 0, clientWidth, clientHeight};
  HBRUSH bgBrush = CreateSolidBrush(bgColor);
  FillRect(dc, &rc, bgBrush);
  DeleteObject(bgBrush);

  const COLORREF oldTextColor = SetTextColor(dc, textColor);
  const int oldBackMode = SetBkMode(dc, TRANSPARENT);
  // TA_TOP is zero, so this reads as "left edge, no update CP".
  const UINT oldAlign = SetTextAlign(dc, TA_LEFT | TA_NOUPDATECP);

  const int count = dropDown ? dropDown->OptionCount() : 0;
  const int textLeft = m_Padding + 4;
  const int textRight = clientWidth - m_Padding - kScrollHitWidth - 4;
  for (int row = 0; row < m_VisibleRows; ++row) {
    const int index = m_FirstVisible + row;
    if (index >= count)
      break;
    const int rowTop = m_Padding + row * m_RowHeight;
    RECT rowRect{m_Padding, rowTop, clientWidth - m_Padding,
                 rowTop + m_RowHeight};
    if (index == m_OriginalIndex) {
      // The originally-selected row always shows with selectedColor regardless
      // of where the hover cursor is.
      HBRUSH highlight = CreateSolidBrush(selectedColor);
      FillRect(dc, &rowRect, highlight);
      DeleteObject(highlight);
    }
    if (index == m_Highlight && m_MouseActive && index != m_OriginalIndex) {
      // Hovered row (mouse-driven) that is not the selected row uses hoverColor.
      HBRUSH highlight = CreateSolidBrush(hoverColor);
      FillRect(dc, &rowRect, highlight);
      DeleteObject(highlight);
    } else if (index == m_Highlight && !m_MouseActive && index != m_OriginalIndex) {
      // Keyboard cursor on a non-selected row uses selectedColor.
      HBRUSH highlight = CreateSolidBrush(selectedColor);
      FillRect(dc, &rowRect, highlight);
      DeleteObject(highlight);
    }
    const DropDownOption &option = dropDown->Options()[static_cast<size_t>(index)];
    SIZE measured{};
    std::wstring label = option.label;
    GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.length()),
                          &measured);
    if (measured.cx > textRight - textLeft) {
      // Trim to fit with an ellipsis rather than relying on clipping alone.
      while (label.length() > 1) {
        label.erase(label.length() - 1);
        label.back() = kEllipsis;
        GetTextExtentPoint32W(dc, label.c_str(),
                              static_cast<int>(label.length()), &measured);
        if (measured.cx <= textRight - textLeft)
          break;
        label.pop_back();
      }
    }
    // TextOutW draws from the top-left, so reserve a little air above and below
    // and centre whatever is left over.
    const int textTop = rowTop + kTextVerticalPad;
    const int textBottom = rowTop + m_RowHeight - kTextVerticalPad;
    const int textY = (measured.cy <= textBottom - textTop)
                          ? textTop + (textBottom - textTop - measured.cy) / 2
                          : rowTop;
    TextOutW(dc, textLeft, label.empty() ? rowTop : textY, label.c_str(),
             static_cast<int>(label.length()));
  }

  // ── Direct2D layer: selection tick and scrollbar thumb ──────────
  Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> vectorTarget;
  if (ID2D1Factory1 *factory = Direct2D::GetFactory()) {
    const D2D1_RENDER_TARGET_PROPERTIES properties =
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM,
                              D2D1_ALPHA_MODE_IGNORE),
            0.0f, 0.0f, D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);
    if (SUCCEEDED(factory->CreateDCRenderTarget(&properties,
                                                 vectorTarget.GetAddressOf())) &&
        SUCCEEDED(vectorTarget->BindDC(dc, &rc))) {
      vectorTarget->BeginDraw();
    } else {
      vectorTarget.Reset();
    }
  }
  if (vectorTarget) {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> thumbBrush;
    if (dropDown) {
      Direct2D::CreateSolidBrush(vectorTarget.Get(), textColor, 0.45f,
                                 thumbBrush.GetAddressOf());
    }
    if (thumbBrush && count > m_VisibleRows) {
      const float trackTop = static_cast<float>(m_Padding);
      const float trackHeight =
          static_cast<float>(m_VisibleRows * m_RowHeight);
      const float thumbHeight = (std::max)(
          static_cast<float>(kThumbMinHeight),
          trackHeight * (static_cast<float>(m_VisibleRows) /
                         static_cast<float>(count)));
      const float maxFirst = static_cast<float>(count - m_VisibleRows);
      const float ratio =
          maxFirst > 0.0f ? static_cast<float>(m_FirstVisible) / maxFirst : 0.0f;
      const float thumbTop =
          trackTop + ratio * (trackHeight - thumbHeight);
      const float thumbLeft = static_cast<float>(clientWidth - kScrollHitWidth);
      const D2D1_RECT_F thumbRect =
          D2D1::RectF(thumbLeft + 3.0f, thumbTop, thumbLeft + 7.0f,
                      thumbTop + thumbHeight);
      vectorTarget->FillRectangle(thumbRect, thumbBrush.Get());
    }
    vectorTarget->EndDraw();
  }

  SetTextAlign(dc, oldAlign);
  SetBkMode(dc, oldBackMode);
  SetTextColor(dc, oldTextColor);
  BitBlt(targetDc, 0, 0, clientWidth, clientHeight, dc, 0, 0, SRCCOPY);
  if (oldFont)
    SelectObject(dc, oldFont);
  SelectObject(dc, oldBitmap);
  DeleteObject(bitmap);
  DeleteDC(dc);
}

LRESULT DropDownPopup::Handle(UINT m, WPARAM w, LPARAM l) {
  if (m == OUTSIDE_CLICK_MESSAGE) {
    Close();
    return 0;
  }
  if (m == WM_ERASEBKGND)
    return 1;
  if (m == WM_PAINT) {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(m_hWnd, &ps);
    Paint(dc);
    EndPaint(m_hWnd, &ps);
    return 0;
  }
  if (m == WM_TIMER && w == SHOW_DESKTOP_TIMER) {
    const bool isShowingDesktop = System::GetShowDesktop();
    if (isShowingDesktop && !m_ShowDesktopWasActive) {
      Close();
      return 0;
    }
    m_ShowDesktopWasActive = isShowingDesktop;
    return 0;
  }
  if (m == WM_MOUSEWHEEL) {
    const int delta = GET_WHEEL_DELTA_WPARAM(w);
    ScrollBy(-(delta / WHEEL_DELTA));
    return 0;
  }
  if (m == WM_MOUSEMOVE) {
    POINT pt{LOWORD(l), HIWORD(l)};
    const int count = ElementStillValid() ? m_DropDown->OptionCount() : 0;
    const int hovered = RowIndexAt(pt);
    if (hovered >= 0) {
      // Mark mouse active before updating the highlight so Paint() uses
      // hoverColor (not the keyboard selectedColor) for the highlighted row.
      m_MouseActive = true;
      SetHighlight(hovered, false);
    }
    if (m_DraggingThumb && count > m_VisibleRows) {
      const float trackHeight =
          static_cast<float>(m_VisibleRows * m_RowHeight);
      const float thumbHeight = (std::max)(
          static_cast<float>(kThumbMinHeight),
          trackHeight * (static_cast<float>(m_VisibleRows) /
                         static_cast<float>(count)));
      const float usable = trackHeight - thumbHeight;
      const float top = static_cast<float>(m_Padding) + m_ThumbDragOffset;
      const float ratio = usable > 0.0f ? ClampInt(
                              static_cast<int>(pt.y - top), 0,
                              static_cast<int>(usable)) /
                                                    usable
                                        : 0.0f;
      const int maxFirst = count - m_VisibleRows;
      const int next = ClampInt(static_cast<int>(ratio * maxFirst + 0.5f), 0,
                                maxFirst);
      if (next != m_FirstVisible) {
        m_FirstVisible = next;
        InvalidateRect(m_hWnd, nullptr, FALSE);
      }
      return 0;
    }
    // Edge auto-scroll: hovering the first/last few pixels nudges the list.
    const int row = (pt.y - m_Padding) / (std::max)(1, m_RowHeight);
    if (pt.y <= m_Padding && row < 0 && m_FirstVisible > 0)
      ScrollBy(-1);
    else if (pt.y >= m_Height - m_Padding && count > m_VisibleRows &&
             m_FirstVisible < count - m_VisibleRows)
      ScrollBy(1);
    TRACKMOUSEEVENT tracking{sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
    TrackMouseEvent(&tracking);
    return 0;
  }
  if (m == WM_MOUSELEAVE) {
    if (m_MouseActive && !m_DraggingThumb) {
      m_MouseActive = false;
      m_Highlight = m_OriginalIndex;
      InvalidateRect(m_hWnd, nullptr, FALSE);
    }
    // Clear the widget's cached cursor element and immediately restore the
    // arrow cursor. The widget's own WM_MOUSELEAVE may never fire when moving
    // from the popup directly to the desktop (the widget never re-armed
    // TrackMouseEvent while the popup was open), so we must do this here.
    if (m_Widget)
      m_Widget->ClearCursorElement();
    SetCursor(LoadCursor(nullptr, IDC_ARROW));
    return 0;
  }
  if (m == WM_SETCURSOR) {
    if (LOWORD(l) == HTCLIENT) {
      POINT cursor{};
      GetCursorPos(&cursor);
      ScreenToClient(m_hWnd, &cursor);
      if (cursor.x >= m_Width - kScrollHitWidth) {
        SetCursor(LoadCursor(nullptr, IDC_ARROW));
        return TRUE;
      }
      SetCursor(LoadCursor(nullptr, IDC_HAND));
      return TRUE;
    }
  }
  if (m == WM_LBUTTONDOWN) {
    POINT pt{LOWORD(l), HIWORD(l)};
    if (pt.x >= m_Width - kScrollHitWidth) {
      const int count = ElementStillValid() ? m_DropDown->OptionCount() : 0;
      if (count > m_VisibleRows) {
        const float trackHeight =
            static_cast<float>(m_VisibleRows * m_RowHeight);
        const float thumbHeight = (std::max)(
            static_cast<float>(kThumbMinHeight),
            trackHeight * (static_cast<float>(m_VisibleRows) /
                           static_cast<float>(count)));
        const int maxFirst = count - m_VisibleRows;
        const float ratio =
            maxFirst > 0 ? static_cast<float>(m_FirstVisible) / maxFirst : 0.0f;
        const float thumbTop =
            static_cast<float>(m_Padding) +
            ratio * (trackHeight - thumbHeight);
        m_ThumbDragOffset =
            (pt.y >= thumbTop && pt.y <= thumbTop + thumbHeight)
                ? static_cast<float>(pt.y) - thumbTop
                : thumbHeight * 0.5f;
        m_DraggingThumb = true;
        SetCapture(m_hWnd);
        return 0;
      }
    }
    const int index = RowIndexAt(pt);
    if (index >= 0) {
      m_MouseActive = true;
      SetHighlight(index, false);
      CommitHighlighted();
    }
    return 0;
  }
  if (m == WM_LBUTTONUP) {
    if (m_DraggingThumb) {
      m_DraggingThumb = false;
      ReleaseCapture();
    }
    return 0;
  }
  if (m == WM_KEYDOWN) {
    const int count = ElementStillValid() ? m_DropDown->OptionCount() : 0;
    switch (w) {
    case VK_ESCAPE:
      m_Canceled = true;
      if (m_DropDown && ElementStillValid() &&
          m_DropDown->m_OnCancelCallbackId != -1) {
        std::wstring payload =
            (m_EscapeIndex >= 0 && m_EscapeIndex < count)
                ? m_DropDown->Options()[static_cast<size_t>(m_EscapeIndex)]
                      .value
                : std::wstring();
        JSEngine::CallEventCallbackWithText(m_DropDown->m_OnCancelCallbackId,
                                            m_Widget, payload);
        if (!Widget::IsValid(m_Widget))
          return 0;
      }
      if (m_DropDown && ElementStillValid()) {
        m_DropDown->SetSelectedIndex(m_EscapeIndex);
        m_WidgetNeedsRedraw = true;
      }
      Close();
      return 0;
    case VK_UP:
      SelectFromKeyboard(m_Highlight < 0 ? count - 1 : m_Highlight - 1);
      return 0;
    case VK_DOWN:
      SelectFromKeyboard(m_Highlight < 0 ? 0 : m_Highlight + 1);
      return 0;
    case VK_HOME:
      SelectFromKeyboard(0);
      return 0;
    case VK_END:
      SelectFromKeyboard(count - 1);
      return 0;
    case VK_PRIOR:
      SelectFromKeyboard(m_Highlight - m_VisibleRows);
      return 0;
    case VK_NEXT:
      SelectFromKeyboard(m_Highlight + m_VisibleRows);
      return 0;
    case VK_RETURN:
      CommitHighlighted();
      return 0;
    default:
      return 0;
    }
  }
  if (m == WM_CHAR) {
    // Type-ahead: jump to the next row whose label starts with this letter.
    const wchar_t ch = static_cast<wchar_t>(w);
    if (ch == 0x1B || ch == 0x0D || ch == 0x09)
      return 0;
    if (!ElementStillValid())
      return 0;
    wchar_t lowered = static_cast<wchar_t>(towlower(ch));
    const int count = m_DropDown->OptionCount();
    for (int step = 1; step <= count; ++step) {
      const int index = (m_Highlight + step + count) % count;
      const std::wstring &label =
          m_DropDown->Options()[static_cast<size_t>(index)].label;
      if (!label.empty() && towlower(label[0]) == lowered) {
        m_MouseActive = true;
        SetHighlight(index, true);
        break;
      }
    }
    return 0;
  }
  if (m == WM_KILLFOCUS) {
    // No child windows, so any focus loss means the user left the popup.
    Close();
    return 0;
  }
  return DefWindowProcW(m_hWnd, m, w, l);
}
