/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "OptionListPopup.h"
#include "DesktopManager.h"
#include "Widget.h"
#include "../render/Direct2DHelper.h"
#include "../render/Element.h"
#include "../render/DropDownElement.h"
#include "../render/ComboBoxElement.h"
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
constexpr int kTextVerticalPad = 3;

HHOOK g_OutsideClickHook = nullptr;
OptionListPopup *g_OutsideClickPopup = nullptr;

void ApplyDarkAndDisableTransitions(HWND hWnd) {
  HMODULE hDwm = GetModuleHandleW(L"dwmapi.dll");
  if (!hDwm)
    hDwm = LoadLibraryW(L"dwmapi.dll");
  if (hDwm) {
    auto pfn = reinterpret_cast<HRESULT(WINAPI *)(HWND, DWORD, LPCVOID, DWORD)>(
        GetProcAddress(hDwm, "DwmSetWindowAttribute"));
    if (pfn) {
      BOOL disableTransitions = TRUE;
      pfn(hWnd, 3 /* DWMWA_TRANSITIONS_FORCEDISABLED */, &disableTransitions, sizeof(disableTransitions));
      BOOL useDarkMode = TRUE;
      pfn(hWnd, 20 /* DWMWA_USE_IMMERSIVE_DARK_MODE */, &useDarkMode, sizeof(useDarkMode));
      pfn(hWnd, 19 /* DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 */, &useDarkMode, sizeof(useDarkMode));
    }
  }
}

int ClampInt(int v, int lo, int hi) {
  return (std::max)(lo, (std::min)(v, hi));
}

GfxRect GetAbsoluteBounds(const Element *element) {
  if (!element)
    return GfxRect(0, 0, 0, 0);
  GfxRect b = const_cast<Element *>(element)->GetBounds();
  const Element *container = element->GetContainer();
  while (container) {
    GfxRect cb = const_cast<Element *>(container)->GetBounds();
    b.X += cb.X - const_cast<Element *>(container)->GetScrollX();
    b.Y += cb.Y - const_cast<Element *>(container)->GetScrollY();
    container = container->GetContainer();
  }
  return b;
}

HFONT CreateOptionListFont(const OptionListStyle &style) {
  const float size = static_cast<float>(style.fontSize > 0 ? style.fontSize : 12);
  const int weight = style.fontWeight > 0 ? style.fontWeight : 400;
  const std::wstring fallback = L"Segoe UI";
  const std::wstring &face = style.fontFace.empty() ? fallback : style.fontFace;
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

OptionListPopup::OptionListPopup(Widget *widget, IOptionListHost *host)
    : m_Widget(widget), m_Host(host) {}

OptionListPopup::~OptionListPopup() {
  Close();
}

Element *OptionListPopup::GetElement() const {
  return m_Host ? m_Host->GetElement() : nullptr;
}

bool OptionListPopup::ElementStillValid() const {
  if (!m_Host || !m_Widget || !Widget::IsValid(m_Widget))
    return false;
  Element *element = m_Host->GetElement();
  if (!element)
    return false;
  return m_Widget->FindElementById(element->GetId()) == element;
}

void OptionListPopup::FlushWidgetRedraw() {
  if (m_WidgetNeedsRedraw && m_Widget && Widget::IsValid(m_Widget)) {
    m_WidgetNeedsRedraw = false;
    m_Widget->Redraw();
  }
}

void OptionListPopup::FilterBy(const std::wstring &query) {
  if (!m_Host || !ElementStillValid())
    return;
  m_CurrentFilter = query;
  m_FilteredOptions.clear();
  m_OriginalIndices.clear();

  const auto &opts = m_Host->GetOptions();
  if (query.empty() || !m_Host->SupportsFiltering()) {
    m_FilteredOptions = opts;
    m_OriginalIndices.resize(opts.size());
    for (size_t i = 0; i < opts.size(); ++i)
      m_OriginalIndices[i] = static_cast<int>(i);
  } else {
    std::wstring lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::towlower);
    for (size_t i = 0; i < opts.size(); ++i) {
      std::wstring lowerLabel = opts[i].label;
      std::transform(lowerLabel.begin(), lowerLabel.end(), lowerLabel.begin(), ::towlower);
      if (lowerLabel.find(lowerQuery) != std::wstring::npos) {
        m_FilteredOptions.push_back(opts[i]);
        m_OriginalIndices.push_back(static_cast<int>(i));
      }
    }
  }

  m_Highlight = m_FilteredOptions.empty() ? -1 : 0;
  m_FirstVisible = 0;
  if (m_hWnd) {
    UpdatePosition();
    InvalidateRect(m_hWnd, nullptr, FALSE);
  }
}

void OptionListPopup::LayoutRows() {
  if (!m_Host)
    return;
  const OptionListStyle style = m_Host->GetOptionListStyle();
  m_RowHeight = (std::max)(12, style.itemHeight);
  m_Padding = (std::max)(0, style.padding);
  const int maxVisible = (std::max)(1, style.maxVisibleItems);
  const int count = (std::max)(0, FilteredCount());
  m_VisibleRows = (count > 0) ? (std::min)(count, maxVisible) : 1;
  m_Scrollable = count > m_VisibleRows;
  m_Height = m_Padding * 2 + m_VisibleRows * m_RowHeight;

  RECT r{};
  Element *elem = m_Host->GetElement();
  GfxRect b = GetAbsoluteBounds(elem);
  GetWindowRect(m_Widget ? m_Widget->GetWindow() : nullptr, &r);
  int width = (std::max)({1, b.Width, style.minWidth});
  HDC dc = GetDC(m_hWnd ? m_hWnd : GetDesktopWindow());
  if (dc) {
    HFONT oldFont = m_Font ? static_cast<HFONT>(SelectObject(dc, m_Font)) : nullptr;
    for (const DropDownOption &option : m_FilteredOptions) {
      SIZE sz{};
      if (GetTextExtentPoint32W(dc, option.label.c_str(),
                                static_cast<int>(option.label.length()), &sz)) {
        const int needed = static_cast<int>(sz.cx) + m_Padding * 2 + kScrollHitWidth + 8;
        width = (std::max)(width, needed);
      }
    }
    if (oldFont)
      SelectObject(dc, oldFont);
    ReleaseDC(m_hWnd ? m_hWnd : GetDesktopWindow(), dc);
  }
  HMONITOR mon = MonitorFromPoint({r.left + b.X, r.top + b.Y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{sizeof(mi)};
  if (GetMonitorInfoW(mon, &mi)) {
    const int workWidth = mi.rcWork.right - mi.rcWork.left;
    width = ClampInt(width, 1, workWidth);
  }
  m_Width = width;
}

void OptionListPopup::Show() {
  static ATOM atom = 0;
  if (!atom) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"NovadeskOptionListPopup";
    static HBRUSH s_DarkBrush = CreateSolidBrush(RGB(36, 36, 44));
    wc.hbrBackground = s_DarkBrush;
    atom = RegisterClassW(&wc);
  }

  if (!m_Widget || !m_Host || !ElementStillValid())
    return;

  if (m_hWnd) {
    BringWindowToTop(m_hWnd);
    return;
  }

  // Initialize filter
  FilterBy(m_Host->GetInitialFilter());

  const OptionListStyle style = m_Host->GetOptionListStyle();
  m_Font = CreateOptionListFont(style);
  LayoutRows();

  m_OriginalIndex = m_Host->GetSelectedIndex();
  m_EscapeIndex = m_OriginalIndex;
  m_Highlight = -1;

  for (size_t i = 0; i < m_OriginalIndices.size(); ++i) {
    if (m_OriginalIndices[i] == m_OriginalIndex) {
      m_Highlight = static_cast<int>(i);
      break;
    }
  }

  m_FirstVisible = 0;
  if (m_Highlight >= 0) {
    const int count = FilteredCount();
    m_FirstVisible = ClampInt(m_Highlight - m_VisibleRows / 2, 0,
                              (std::max)(0, count - m_VisibleRows));
  }
  m_Canceled = false;
  m_MouseActive = false;
  m_DraggingThumb = false;

  RECT r{};
  GetWindowRect(m_Widget->GetWindow(), &r);
  Element *elem = m_Host->GetElement();
  GfxRect b = GetAbsoluteBounds(elem);
  int x = r.left + b.X + (style.rightToLeft ? b.Width - m_Width : 0);
  int y = r.top + b.Y + b.Height;
  HMONITOR mon = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{sizeof(mi)};
  GetMonitorInfoW(mon, &mi);
  if (y + m_Height > mi.rcWork.bottom)
    y = r.top + b.Y - m_Height;
  y = ClampInt(y, mi.rcWork.top, (std::max)(mi.rcWork.top, mi.rcWork.bottom - m_Height));
  x = ClampInt(x, mi.rcWork.left, (std::max)(mi.rcWork.left, mi.rcWork.right - m_Width));

  Widget::IncrementDropDownCount();
  m_hWnd = CreateWindowExW(
      WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
      L"NovadeskOptionListPopup", L"",
      WS_POPUP | WS_CLIPCHILDREN, x, y, m_Width, m_Height,
      m_Widget->GetWindow(), nullptr, GetModuleHandleW(nullptr), this);
  if (!m_hWnd) {
    Widget::DecrementDropDownCount();
    if (m_Font) {
      DeleteObject(m_Font);
      m_Font = nullptr;
    }
    return;
  }

  ApplyDarkAndDisableTransitions(m_hWnd);
  SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, m_Width, m_Height, SWP_SHOWWINDOW | SWP_NOACTIVATE);
  UpdateWindow(m_hWnd);
  InstallOutsideClickHook();

  m_ShowDesktopWasActive = System::GetShowDesktop();
  SetTimer(m_hWnd, SHOW_DESKTOP_TIMER, 100, nullptr);

  if (m_Widget && Widget::IsValid(m_Widget))
    m_Widget->Redraw();

  const int onOpenCb = m_Host->GetOnOpenCallbackId();
  if (onOpenCb != -1) {
    JSEngine::CallEventCallback(onOpenCb, m_Widget);
  }
}

void OptionListPopup::UpdatePosition() {
  if (!m_hWnd || !m_Widget || !m_Host || !ElementStillValid())
    return;
  const int previousWidth = m_Width;
  const int previousHeight = m_Height;
  LayoutRows();
  RECT r{};
  GetWindowRect(m_Widget->GetWindow(), &r);
  Element *elem = m_Host->GetElement();
  GfxRect b = GetAbsoluteBounds(elem);
  const OptionListStyle style = m_Host->GetOptionListStyle();
  int x = r.left + b.X + (style.rightToLeft ? b.Width - m_Width : 0);
  int y = r.top + b.Y + b.Height;
  HMONITOR mon = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
  MONITORINFO mi{sizeof(mi)};
  GetMonitorInfoW(mon, &mi);

  const bool resized = (m_Width != previousWidth || m_Height != previousHeight);
  RECT current{};
  if (!resized && GetWindowRect(m_hWnd, &current)) {
    if (current.top >= r.top + b.Y) {
      y = r.top + b.Y + b.Height;
      if (y + m_Height > mi.rcWork.bottom)
        y = r.top + b.Y - m_Height;
    } else {
      y = r.top + b.Y - m_Height;
      if (y < mi.rcWork.top)
        y = r.top + b.Y + b.Height;
    }
  } else {
    if (y + m_Height > mi.rcWork.bottom)
      y = r.top + b.Y - m_Height;
  }

  y = ClampInt(y, mi.rcWork.top, (std::max)(mi.rcWork.top, mi.rcWork.bottom - m_Height));
  x = ClampInt(x, mi.rcWork.left, (std::max)(mi.rcWork.left, mi.rcWork.right - m_Width));

  SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, m_Width, m_Height, SWP_NOACTIVATE);
  InvalidateRect(m_hWnd, nullptr, FALSE);
}

void OptionListPopup::Close() {
  if (m_Closing)
    return;
  m_Closing = true;

  if (m_Host && ElementStillValid()) {
    if (m_Canceled && m_Host->ShouldPreviewOnHighlight()) {
      m_Host->OnOptionCancelled(m_EscapeIndex);
    }
    m_Host->OnPopupClosed();
    const int onCloseCb = m_Host->GetOnCloseCallbackId();
    if (onCloseCb != -1 && m_Widget && Widget::IsValid(m_Widget)) {
      JSEngine::CallEventCallback(onCloseCb, m_Widget);
    }
  }

  RemoveOutsideClickHook();
  if (m_hWnd) {
    KillTimer(m_hWnd, SHOW_DESKTOP_TIMER);
    DestroyWindow(m_hWnd);
    m_hWnd = nullptr;
    Widget::DecrementDropDownCount();
  }
  if (m_Font) {
    DeleteObject(m_Font);
    m_Font = nullptr;
  }
  m_Closing = false;
  FlushWidgetRedraw();
  if (m_Widget && Widget::IsValid(m_Widget))
    m_Widget->Redraw();
}

void OptionListPopup::SetHighlight(int index, bool scrollIntoView) {
  if (!m_Host || !ElementStillValid())
    return;
  const int count = FilteredCount();
  if (count <= 0) {
    m_Highlight = -1;
    if (m_hWnd)
      InvalidateRect(m_hWnd, nullptr, FALSE);
    return;
  }
  index = ClampInt(index, 0, count - 1);
  if (scrollIntoView && m_Scrollable) {
    if (index < m_FirstVisible)
      m_FirstVisible = index;
    else if (index >= m_FirstVisible + m_VisibleRows)
      m_FirstVisible = index - m_VisibleRows + 1;
    m_FirstVisible = ClampInt(m_FirstVisible, 0, (std::max)(0, count - m_VisibleRows));
  }
  if (index == m_Highlight)
    return;
  m_Highlight = index;
  if (m_hWnd)
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void OptionListPopup::ScrollIntoView(int index) {
  if (index < 0 || !m_Scrollable)
    return;
  const int before = m_Highlight;
  SetHighlight(index, true);
  m_Highlight = before;
}

void OptionListPopup::ScrollBy(int rows) {
  if (!m_Host || !ElementStillValid())
    return;
  const int maxFirst = (std::max)(0, FilteredCount() - m_VisibleRows);
  const int next = ClampInt(m_FirstVisible + rows, 0, maxFirst);
  if (next == m_FirstVisible)
    return;
  m_FirstVisible = next;
  InvalidateRect(m_hWnd, nullptr, FALSE);
}

int OptionListPopup::RowIndexAt(POINT clientPt) const {
  if (!m_Host || !ElementStillValid())
    return -1;
  const int row = (clientPt.y - m_Padding) / (std::max)(1, m_RowHeight);
  if (row < 0)
    return -1;
  const int index = m_FirstVisible + row;
  if (index >= FilteredCount())
    return -1;
  return index;
}

void OptionListPopup::ApplyHighlighted() {
  if (m_Highlight < 0 || !m_Host || !ElementStillValid())
    return;
  if (static_cast<size_t>(m_Highlight) >= m_OriginalIndices.size())
    return;
  const int origIndex = m_OriginalIndices[static_cast<size_t>(m_Highlight)];
  m_Host->OnOptionHighlighted(origIndex);
  m_WidgetNeedsRedraw = true;
  FlushWidgetRedraw();
}

void OptionListPopup::CommitHighlighted() {
  if (!m_Host || !ElementStillValid())
    return;
  if (m_Highlight >= 0 && static_cast<size_t>(m_Highlight) < m_OriginalIndices.size()) {
    const int origIndex = m_OriginalIndices[static_cast<size_t>(m_Highlight)];
    m_OriginalIndex = origIndex;
    m_Host->OnOptionCommitted(origIndex);
    if (m_Widget && Widget::IsValid(m_Widget)) {
      Element *elem = m_Host->GetElement();
      if (auto *dd = dynamic_cast<DropDownElement *>(elem))
        m_Widget->NotifyDropDownChange(dd);
      else if (auto *cb = dynamic_cast<ComboBoxElement *>(elem))
        m_Widget->NotifyComboBoxChange(cb);
    }
  }
  Close();
}

void OptionListPopup::SelectFromKeyboard(int index) {
  m_MouseActive = false;
  SetHighlight(index, true);
  if (m_Host && m_Host->ShouldPreviewOnHighlight()) {
    ApplyHighlighted();
  }
}

void OptionListPopup::Paint(HDC targetDc) {
  if (!m_Host || !ElementStillValid())
    return;
  const OptionListStyle style = m_Host->GetOptionListStyle();

  RECT clientRect{};
  GetClientRect(m_hWnd, &clientRect);
  const int clientWidth = clientRect.right;
  const int clientHeight = clientRect.bottom;

  HDC dc = CreateCompatibleDC(targetDc);
  HBITMAP bmp = CreateCompatibleBitmap(targetDc, clientWidth, clientHeight);
  HBITMAP oldBmp = static_cast<HBITMAP>(SelectObject(dc, bmp));
  HFONT oldFont = m_Font ? static_cast<HFONT>(SelectObject(dc, m_Font)) : nullptr;

  RECT rc{0, 0, clientWidth, clientHeight};
  HBRUSH bgBrush = CreateSolidBrush(style.background);
  FillRect(dc, &rc, bgBrush);
  DeleteObject(bgBrush);

  HBRUSH borderBrush = CreateSolidBrush(style.borderColor);
  FrameRect(dc, &rc, borderBrush);
  DeleteObject(borderBrush);

  const COLORREF textColor = style.textColor;
  const COLORREF hoverColor = style.hoverColor;
  const COLORREF selectedColor = style.selectedColor;
  const bool rtl = style.rightToLeft;

  const int oldBackMode = SetBkMode(dc, TRANSPARENT);
  const COLORREF oldTextColor = SetTextColor(dc, textColor);
  const UINT oldAlign = SetTextAlign(dc, rtl ? TA_RIGHT : TA_LEFT);

  const int count = FilteredCount();
  const int textLeft = m_Padding + 4;
  const int textRight = clientWidth - m_Padding - (m_Scrollable ? kScrollHitWidth : 4);

  for (int row = 0; row < m_VisibleRows; ++row) {
    const int index = m_FirstVisible + row;
    if (index >= count)
      break;
    const int rowTop = m_Padding + row * m_RowHeight;
    RECT rowRect{m_Padding, rowTop, clientWidth - m_Padding, rowTop + m_RowHeight};

    const int origIdx = (static_cast<size_t>(index) < m_OriginalIndices.size())
                            ? m_OriginalIndices[static_cast<size_t>(index)]
                            : -1;

    // Only the single active row (hovered or keyboard-cursor) gets the highlight background
    if (index == m_Highlight) {
      const COLORREF bg = m_MouseActive ? hoverColor : selectedColor;
      HBRUSH highlight = CreateSolidBrush(bg);
      FillRect(dc, &rowRect, highlight);
      DeleteObject(highlight);
    }

    const DropDownOption &option = m_FilteredOptions[static_cast<size_t>(index)];
    SIZE measured{};
    std::wstring label = option.label;
    GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.length()), &measured);
    if (measured.cx > textRight - textLeft) {
      while (label.length() > 1) {
        label.erase(label.length() - 1);
        label.back() = kEllipsis;
        GetTextExtentPoint32W(dc, label.c_str(), static_cast<int>(label.length()), &measured);
        if (measured.cx <= textRight - textLeft)
          break;
        label.pop_back();
      }
    }
    const int textTop = rowTop + kTextVerticalPad;
    const int textBottom = rowTop + m_RowHeight - kTextVerticalPad;
    const int textY = (measured.cy <= textBottom - textTop)
                          ? textTop + (textBottom - textTop - measured.cy) / 2
                          : rowTop;
    TextOutW(dc, rtl ? textRight : textLeft, label.empty() ? rowTop : textY,
             label.c_str(), static_cast<int>(label.length()));

    // For non-filtering dropdowns, draw a checkmark icon next to the committed choice
    if (origIdx == m_OriginalIndex && m_OriginalIndex >= 0 && !m_Host->SupportsFiltering()) {
      const COLORREF prevCol = SetTextColor(dc, style.checkColor);
      const wchar_t checkMark[] = L"\x2713";
      TextOutW(dc, rtl ? (m_Padding + 6) : (textRight - 12), textY, checkMark, 1);
      SetTextColor(dc, prevCol);
    }
  }

  // Direct2D layer: Scrollbar thumb
  Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> vectorTarget;
  if (ID2D1Factory1 *factory = Direct2D::GetFactory()) {
    const D2D1_RENDER_TARGET_PROPERTIES properties =
        D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
            0.0f, 0.0f, D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);
    if (SUCCEEDED(factory->CreateDCRenderTarget(&properties, vectorTarget.GetAddressOf())) &&
        SUCCEEDED(vectorTarget->BindDC(dc, &rc))) {
      vectorTarget->BeginDraw();
    } else {
      vectorTarget.Reset();
    }
  }

  if (vectorTarget) {
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> thumbBrush;
    Direct2D::CreateSolidBrush(vectorTarget.Get(), textColor, 0.45f, thumbBrush.GetAddressOf());
    if (thumbBrush && count > m_VisibleRows) {
      const float trackTop = static_cast<float>(m_Padding);
      const float trackHeight = static_cast<float>(m_VisibleRows * m_RowHeight);
      const float thumbHeight = (std::max)(
          static_cast<float>(kThumbMinHeight),
          trackHeight * (static_cast<float>(m_VisibleRows) / static_cast<float>(count)));
      const float maxFirst = static_cast<float>(count - m_VisibleRows);
      const float ratio = maxFirst > 0.0f ? static_cast<float>(m_FirstVisible) / maxFirst : 0.0f;
      const float thumbTop = trackTop + ratio * (trackHeight - thumbHeight);
      const float thumbLeft = static_cast<float>(clientWidth - kScrollHitWidth);
      const D2D1_RECT_F thumbRect =
          D2D1::RectF(thumbLeft + 3.0f, thumbTop, thumbLeft + 7.0f, thumbTop + thumbHeight);
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
  SelectObject(dc, oldBmp);
  DeleteObject(bmp);
  DeleteDC(dc);
}

LRESULT CALLBACK OptionListPopup::OutsideClickMouseHook(int nCode, WPARAM wParam, LPARAM lParam) {
  if (nCode >= 0 && g_OutsideClickPopup && g_OutsideClickPopup->m_hWnd) {
    if (wParam == WM_LBUTTONDOWN || wParam == WM_NCLBUTTONDOWN ||
        wParam == WM_RBUTTONDOWN || wParam == WM_NCRBUTTONDOWN) {
      const auto *hookStruct = reinterpret_cast<MSLLHOOKSTRUCT *>(lParam);
      RECT r{};
      GetWindowRect(g_OutsideClickPopup->m_hWnd, &r);
      if (!PtInRect(&r, hookStruct->pt)) {
        PostMessageW(g_OutsideClickPopup->m_hWnd, OUTSIDE_CLICK_MESSAGE, 0, 0);
      }
    }
  }
  return CallNextHookEx(g_OutsideClickHook, nCode, wParam, lParam);
}

void OptionListPopup::InstallOutsideClickHook() {
  if (!g_OutsideClickHook) {
    g_OutsideClickPopup = this;
    g_OutsideClickHook = SetWindowsHookExW(
        WH_MOUSE_LL, OutsideClickMouseHook, GetModuleHandleW(nullptr), 0);
  }
}

void OptionListPopup::RemoveOutsideClickHook() {
  if (g_OutsideClickHook && g_OutsideClickPopup == this) {
    UnhookWindowsHookEx(g_OutsideClickHook);
    g_OutsideClickHook = nullptr;
    g_OutsideClickPopup = nullptr;
  }
}

LRESULT CALLBACK OptionListPopup::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
  OptionListPopup *self = nullptr;
  if (msg == WM_NCCREATE) {
    auto *cs = reinterpret_cast<CREATESTRUCT *>(lParam);
    self = reinterpret_cast<OptionListPopup *>(cs->lpCreateParams);
    SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    self->m_hWnd = hWnd;
  } else {
    self = reinterpret_cast<OptionListPopup *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
  }
  if (self)
    return self->Handle(msg, wParam, lParam);
  return DefWindowProcW(hWnd, msg, wParam, lParam);
}

LRESULT OptionListPopup::Handle(UINT msg, WPARAM wParam, LPARAM lParam) {
  switch (msg) {
  case WM_PAINT: {
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(m_hWnd, &ps);
    Paint(dc);
    EndPaint(m_hWnd, &ps);
    return 0;
  }

  case WM_ERASEBKGND:
    return 1;

  case WM_NCPAINT:
    return 0;

  case OUTSIDE_CLICK_MESSAGE:
    Close();
    return 0;

  case WM_TIMER:
    if (wParam == SHOW_DESKTOP_TIMER) {
      const bool current = System::GetShowDesktop();
      if (current != m_ShowDesktopWasActive) {
        m_ShowDesktopWasActive = current;
        if (current) {
          m_Canceled = true;
          Close();
        }
      }
    }
    return 0;

  case WM_MOUSEMOVE: {
    const POINT pt{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
    RECT cr{};
    GetClientRect(m_hWnd, &cr);

    if (m_DraggingThumb) {
      const int count = FilteredCount();
      const float trackTop = static_cast<float>(m_Padding);
      const float trackHeight = static_cast<float>(m_VisibleRows * m_RowHeight);
      const float thumbHeight = (std::max)(
          static_cast<float>(kThumbMinHeight),
          trackHeight * (static_cast<float>(m_VisibleRows) / static_cast<float>(count)));
      const float thumbTop = static_cast<float>(pt.y) - m_ThumbDragOffset;
      const float available = trackHeight - thumbHeight;
      const float ratio = available > 0.0f ? (thumbTop - trackTop) / available : 0.0f;
      const int maxFirst = (std::max)(0, count - m_VisibleRows);
      const int newFirst = ClampInt(static_cast<int>(std::round(ratio * maxFirst)), 0, maxFirst);
      if (newFirst != m_FirstVisible) {
        m_FirstVisible = newFirst;
        InvalidateRect(m_hWnd, nullptr, FALSE);
      }
      return 0;
    }

    if (pt.x < cr.right - kScrollHitWidth) {
      const int row = RowIndexAt(pt);
      if (row >= 0 && row != m_Highlight) {
        m_MouseActive = true;
        SetHighlight(row, false);
      }
    }
    return 0;
  }

  case WM_LBUTTONDOWN: {
    const POINT pt{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
    RECT cr{};
    GetClientRect(m_hWnd, &cr);

    if (pt.x >= cr.right - kScrollHitWidth && m_Scrollable) {
      SetCapture(m_hWnd);
      m_DraggingThumb = true;
      const int count = FilteredCount();
      const float trackTop = static_cast<float>(m_Padding);
      const float trackHeight = static_cast<float>(m_VisibleRows * m_RowHeight);
      const float thumbHeight = (std::max)(
          static_cast<float>(kThumbMinHeight),
          trackHeight * (static_cast<float>(m_VisibleRows) / static_cast<float>(count)));
      const float maxFirst = static_cast<float>(count - m_VisibleRows);
      const float ratio = maxFirst > 0.0f ? static_cast<float>(m_FirstVisible) / maxFirst : 0.0f;
      const float currentThumbTop = trackTop + ratio * (trackHeight - thumbHeight);
      if (pt.y >= currentThumbTop && pt.y <= currentThumbTop + thumbHeight) {
        m_ThumbDragOffset = static_cast<float>(pt.y) - currentThumbTop;
      } else {
        m_ThumbDragOffset = thumbHeight / 2.0f;
        SendMessageW(m_hWnd, WM_MOUSEMOVE, wParam, lParam);
      }
      return 0;
    }

    const int row = RowIndexAt(pt);
    if (row >= 0) {
      m_MouseActive = true;
      SetHighlight(row, false);
    }
    return 0;
  }

  case WM_LBUTTONUP: {
    if (m_DraggingThumb) {
      ReleaseCapture();
      m_DraggingThumb = false;
      return 0;
    }
    const POINT pt{static_cast<short>(LOWORD(lParam)), static_cast<short>(HIWORD(lParam))};
    RECT cr{};
    GetClientRect(m_hWnd, &cr);
    if (pt.x < cr.right - kScrollHitWidth) {
      const int row = RowIndexAt(pt);
      if (row >= 0) {
        SetHighlight(row, false);
        CommitHighlighted();
        return 0;
      }
    }
    return 0;
  }

  case WM_MOUSEWHEEL: {
    const short delta = GET_WHEEL_DELTA_WPARAM(wParam);
    ScrollBy(delta > 0 ? -3 : 3);
    return 0;
  }

  case WM_KEYDOWN: {
    const int count = FilteredCount();
    switch (wParam) {
    case VK_DOWN:
      SelectFromKeyboard(m_Highlight + 1);
      return 0;
    case VK_UP:
      SelectFromKeyboard(m_Highlight - 1);
      return 0;
    case VK_PRIOR: // Page Up
      ScrollBy(-m_VisibleRows);
      return 0;
    case VK_NEXT: // Page Down
      ScrollBy(m_VisibleRows);
      return 0;
    case VK_HOME:
      SelectFromKeyboard(0);
      return 0;
    case VK_END:
      SelectFromKeyboard(count - 1);
      return 0;
    case VK_RETURN:
      CommitHighlighted();
      return 0;
    case VK_ESCAPE:
      m_Canceled = true;
      Close();
      return 0;
    default:
      break;
    }
    break;
  }

  case WM_DESTROY:
    RemoveOutsideClickHook();
    return 0;

  default:
    break;
  }

  return DefWindowProcW(m_hWnd, msg, wParam, lParam);
}
