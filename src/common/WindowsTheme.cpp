/* Copyright (C) 2026 OfficialNovadesk
 * Derived from https://github.com/ozone10/win32-darkmodelib
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#define NOMINMAX
#include "WindowsTheme.h"

#include <atomic>
#include <fstream>
#include <mutex>
#include <algorithm>
#include <shlobj.h>
#include <dwmapi.h>
#include <uxtheme.h>
#include <vsstyle.h>
#include <vssym32.h>
#include <commctrl.h>

#include "../third_party/json/json.hpp"

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

namespace WindowsTheme {

namespace {

// ---------------------------------------------------------------------------
// Color constants (matching Rainmeter palette)
// ---------------------------------------------------------------------------
constexpr COLORREF g_Background     = RGB(32, 32, 32);
constexpr COLORREF g_PageBackground = RGB(37, 37, 37);
constexpr COLORREF g_ControlBg      = RGB(56, 56, 56);
constexpr COLORREF g_HotBg          = RGB(69, 69, 69);
constexpr COLORREF g_Text           = RGB(224, 224, 224);
constexpr COLORREF g_DisabledText   = RGB(128, 128, 128);
constexpr COLORREF g_Edge           = RGB(75, 75, 75);

enum class ControlType { Button, Tab, ComboBox, Header, Edit, Spinner, ListView, ListBox, Separator };

// ---------------------------------------------------------------------------
// Uxtheme ordinal typedefs
// ---------------------------------------------------------------------------
using fnSetPreferredAppMode           = int(WINAPI*)(int);
using fnAllowDarkModeForWindow        = bool(WINAPI*)(HWND, bool);
using fnAllowDarkModeForApp           = bool(WINAPI*)(bool);
using fnRefreshImmersiveColorPolicyState = void(WINAPI*)();
using fnFlushMenuThemes               = void(WINAPI*)();

// ---------------------------------------------------------------------------
// Global theme state
// ---------------------------------------------------------------------------
struct ThemeState {
  std::atomic<bool> initialized{false};
  std::atomic<bool> isDarkMode{false};
  Mode mode{Mode::System};

  fnSetPreferredAppMode              pfnSetPreferredAppMode = nullptr;
  fnAllowDarkModeForWindow           pfnAllowDarkModeForWindow = nullptr;
  fnAllowDarkModeForApp              pfnAllowDarkModeForApp = nullptr;
  fnRefreshImmersiveColorPolicyState pfnRefreshImmersiveColorPolicyState = nullptr;
  fnFlushMenuThemes                  pfnFlushMenuThemes = nullptr;

  std::mutex     handlerMutex;
  SettingsReader settingsReader = nullptr;
  SettingsWriter settingsWriter = nullptr;

  // Brushes owned by ThemeState
  HBRUSH brBackground     = CreateSolidBrush(g_Background);
  HBRUSH brPageBackground = CreateSolidBrush(g_PageBackground);
  HBRUSH brControlBg      = CreateSolidBrush(g_ControlBg);
  HBRUSH brHotBg          = CreateSolidBrush(g_HotBg);
  HBRUSH brEdge           = CreateSolidBrush(g_Edge);

  ~ThemeState() {
    DeleteObject(brBackground);
    DeleteObject(brPageBackground);
    DeleteObject(brControlBg);
    DeleteObject(brHotBg);
    DeleteObject(brEdge);
  }
};

ThemeState& GetState() {
  static ThemeState state;
  return state;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
bool HasClass(HWND w, const WCHAR* name) {
  WCHAR cls[64] = {};
  return GetClassNameW(w, cls, 64) && wcscmp(cls, name) == 0;
}

bool IsTabSurface(HWND ctrl) {
  HWND parent = GetParent(ctrl);
  if (!parent) return false;
  if (HasClass(parent, WC_TABCONTROLW)) return true;
  for (HWND p = parent; p; p = GetParent(p)) {
    if (HasClass(p, L"#32770") && (GetWindowLongPtrW(p, GWL_STYLE) & WS_CHILD)) return true;
  }
  for (HWND sibling = GetWindow(parent, GW_CHILD); sibling; sibling = GetWindow(sibling, GW_HWNDNEXT)) {
    if (sibling == ctrl || !HasClass(sibling, WC_TABCONTROLW)) continue;
    RECT tabRc, ctrlRc;
    GetWindowRect(sibling, &tabRc);
    GetWindowRect(ctrl, &ctrlRc);
    if (ctrlRc.left >= tabRc.left && ctrlRc.right <= tabRc.right &&
        ctrlRc.top >= tabRc.top && ctrlRc.bottom <= tabRc.bottom) {
      return true;
    }
  }
  return false;
}

HBRUSH GetSurfaceBrush(HWND w) {
  return IsTabSurface(w) ? GetState().brPageBackground : GetState().brBackground;
}

void ClipSiblings(HWND window, HDC dc) {
  HWND parent = GetParent(window);
  if (!parent) return;
  for (HWND sibling = GetWindow(parent, GW_CHILD); sibling; sibling = GetWindow(sibling, GW_HWNDNEXT)) {
    if (sibling == window || !IsWindowVisible(sibling)) continue;
    if (HasClass(sibling, WC_BUTTON) && (GetWindowLongPtrW(sibling, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX) continue;

    RECT rect;
    GetWindowRect(sibling, &rect);
    MapWindowPoints(nullptr, window, (POINT*)&rect, 2);
    ExcludeClipRect(dc, rect.left, rect.top, rect.right, rect.bottom);
  }
}

bool IsHot(HWND w, const RECT& r) {
  POINT pt; GetCursorPos(&pt); ScreenToClient(w, &pt);
  return PtInRect(&r, pt) != FALSE;
}

void DrawArrow(HDC dc, RECT r, bool up, COLORREF color) {
  const int x = (r.left + r.right) / 2, y = (r.top + r.bottom) / 2, s = 3;
  const int d = up ? -1 : 1;
  const POINT pts[] = {{x-s, y-d*s/2},{x+s, y-d*s/2},{x, y+d*s}};
  HBRUSH br = CreateSolidBrush(color);
  HGDIOBJ ob = SelectObject(dc, br), op = SelectObject(dc, GetStockObject(NULL_PEN));
  Polygon(dc, pts, 3);
  SelectObject(dc, op); SelectObject(dc, ob);
  DeleteObject(br);
}

void DrawLabel(HWND w, HDC dc, RECT r, UINT flags) {
  WCHAR text[4096]; GetWindowTextW(w, text, 4096);
  if (SendMessageW(w, WM_QUERYUISTATE, 0, 0) & UISF_HIDEACCEL) flags |= DT_HIDEPREFIX;
  DrawTextW(dc, text, -1, &r, flags);
}

void DrawControlFrame(HWND w, HDC dc, const RECT& r, const WCHAR* cls, int part, int status, HBRUSH bg, RECT* content = nullptr) {
  HTHEME ht = OpenThemeData(w, cls);
  HRGN rgn = nullptr;
  if (ht && SUCCEEDED(GetThemeBackgroundRegion(ht, dc, part, status, &r, &rgn))) {
    FillRgn(dc, rgn, bg);
    FrameRgn(dc, rgn, GetState().brEdge, 1, 1);
    DeleteObject(rgn);
  } else {
    FillRect(dc, &r, bg);
    FrameRect(dc, &r, GetState().brEdge);
  }
  if (content) {
    *content = r;
    if (ht) GetThemeBackgroundContentRect(ht, dc, part, status, &r, content);
  }
  if (ht) CloseThemeData(ht);
}

// ---------------------------------------------------------------------------
// Per-control Draw routines
// ---------------------------------------------------------------------------
void DrawButton(HWND w, HDC dc, RECT r) {
  auto& st = GetState();
  const DWORD style = (DWORD)GetWindowLongPtrW(w, GWL_STYLE);
  const DWORD type  = style & BS_TYPEMASK;
  const bool enabled = IsWindowEnabled(w) != FALSE;
  const LRESULT bstate = SendMessageW(w, BM_GETSTATE, 0, 0);
  const bool pressed = (bstate & BST_PUSHED) != 0;
  const bool hot = enabled && IsHot(w, r);
  if (type == BS_GROUPBOX) {
    TEXTMETRICW tm; GetTextMetricsW(dc, &tm);
    RECT frame = r; frame.top += tm.tmHeight / 2;
    FrameRect(dc, &frame, st.brEdge);
    WCHAR text[4096]; const int len = GetWindowTextW(w, text, 4096);
    SIZE sz; GetTextExtentPoint32W(dc, text, len, &sz);
    RECT label = {8, 0, 12 + sz.cx, tm.tmHeight};
    FillRect(dc, &label, GetSurfaceBrush(w));
    DrawLabel(w, dc, label, DT_LEFT | DT_SINGLELINE);
    return;
  }

  FillRect(dc, &r, GetSurfaceBrush(w));

  const bool radio = type == BS_RADIOBUTTON || type == BS_AUTORADIOBUTTON;
  const bool check = radio || type == BS_CHECKBOX || type == BS_AUTOCHECKBOX || type == BS_3STATE || type == BS_AUTO3STATE;
  if (check) {
    const int sz = 13;
    RECT glyph = {r.left, (r.bottom - sz) / 2, r.left + sz, (r.bottom + sz) / 2};
    if (style & BS_LEFTTEXT) OffsetRect(&glyph, r.right - sz, 0);
    const int checked = (int)SendMessageW(w, BM_GETCHECK, 0, 0);
    // Open the theme against the window's current DarkMode_Explorer app ID.
    // DarkMode_Explorer::Button provides proper dark-mode glyph rendering for
    // both checkboxes and radio buttons when SetWindowTheme used DarkMode_Explorer.
    HTHEME ht = OpenThemeData(w, L"DarkMode_Explorer::Button");
    if (!ht) ht = OpenThemeData(w, L"Button");
    if (ht) {
      const int part   = radio ? BP_RADIOBUTTON : BP_CHECKBOX;
      const int status = 1 + checked * 4 + (!enabled ? 3 : pressed ? 2 : hot ? 1 : 0);
      DrawThemeBackground(ht, dc, part, status, &glyph, nullptr);
      CloseThemeData(ht);
    }
    if (style & BS_LEFTTEXT) r.right -= sz + 5; else r.left += sz + 5;
    DrawLabel(w, dc, r, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
  } else {
    const int status = !enabled ? PBS_DISABLED : pressed ? PBS_PRESSED : hot ? PBS_HOT :
        (type == BS_DEFPUSHBUTTON || GetFocus() == w) ? PBS_DEFAULTED : PBS_NORMAL;
    RECT content;
    DrawControlFrame(w, dc, r, L"Button", BP_PUSHBUTTON, status,
        pressed || hot ? st.brHotBg : st.brControlBg, &content);
    if (pressed) OffsetRect(&content, 1, 1);
    DrawLabel(w, dc, content, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }
}

void DrawTabs(HWND w, HDC dc, RECT r) {
  auto& st = GetState();
  FillRect(dc, &r, GetSurfaceBrush(w));

  RECT page = r;
  TabCtrl_AdjustRect(w, FALSE, &page);
  const int pad = 2;
  InflateRect(&page, pad, pad);
  const int count = TabCtrl_GetItemCount(w);
  RECT first;
  if (count && TabCtrl_GetItemRect(w, 0, &first)) {
    page.left = first.left - pad;
    page.top  = first.bottom - 1;
  }
  FillRect(dc, &page, st.brPageBackground);
  FrameRect(dc, &page, st.brEdge);
  const int sel = TabCtrl_GetCurSel(w);

  for (int pos = 0; pos <= count; ++pos) {
    const int i = pos == count ? sel : pos;
    if (i < 0 || i >= count || (pos != count && i == sel)) continue;
    RECT item;
    if (!TabCtrl_GetItemRect(w, i, &item)) continue;
    const RECT label = item;
    const bool active = (i == sel);
    if (active) { item.left -= pad; item.right += pad; item.top -= pad; }
    item.bottom = page.top + 1;
    FillRect(dc, &item, active ? st.brPageBackground : IsHot(w, item) ? st.brHotBg : st.brBackground);
    FrameRect(dc, &item, st.brEdge);
    if (active) {
      RECT bot = {item.left + 1, page.top, item.right - 1, page.top + 1};
      FillRect(dc, &bot, st.brPageBackground);
    }
    WCHAR text[512]; TCITEMW entry{}; entry.mask = TCIF_TEXT; entry.pszText = text; entry.cchTextMax = 512;
    RECT tr = label;
    if (TabCtrl_GetItem(w, i, &entry)) DrawTextW(dc, text, -1, &tr, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  }
}

void DrawHeader(HWND w, HDC dc, RECT r) {
  auto& st = GetState();
  FillRect(dc, &r, st.brBackground);
  const bool clickable = (GetWindowLongPtrW(w, GWL_STYLE) & HDS_BUTTONS) != 0;
  const int n = Header_GetItemCount(w);
  for (int i = 0; i < n; ++i) {
    RECT item; if (!Header_GetItemRect(w, i, &item)) continue;
    FillRect(dc, &item, clickable && IsWindowEnabled(w) && IsHot(w, item) ? st.brHotBg : st.brBackground);
    WCHAR text[512]; HDITEMW entry{}; entry.mask = HDI_TEXT | HDI_FORMAT; entry.pszText = text; entry.cchTextMax = 512;
    if (!Header_GetItem(w, i, &entry)) continue;
    const UINT fmt = entry.fmt & HDF_CENTER ? DT_CENTER : entry.fmt & HDF_RIGHT ? DT_RIGHT : DT_LEFT;
    RECT content = item;
    HTHEME ht = GetWindowTheme(w);
    if (ht) GetThemeBackgroundContentRect(ht, dc, HP_HEADERITEM, HIS_NORMAL, &item, &content);
    const int margin = (int)SendMessageW(w, HDM_GETBITMAPMARGIN, 0, 0);
    InflateRect(&content, -margin, 0); item = content;
    if (entry.fmt & (HDF_SORTUP | HDF_SORTDOWN)) {
      RECT arrow = item; arrow.left = arrow.right - 16;
      DrawArrow(dc, arrow, (entry.fmt & HDF_SORTUP) != 0, g_Text);
      item.right = arrow.left;
    }
    DrawTextW(dc, text, -1, &item, fmt | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
  }
  // Dividers
  for (int i = 0; i < n; ++i) {
    RECT item; if (!Header_GetItemRect(w, i, &item)) continue;
    RECT div = {item.right - 1, item.top, item.right, item.bottom};
    HBRUSH sep = CreateSolidBrush(RGB(48, 48, 48));
    FillRect(dc, &div, sep); DeleteObject(sep);
  }
}

void DrawComboBox(HWND w, HDC dc, RECT r) {
  auto& st = GetState();
  FillRect(dc, &r, st.brBackground);
  COMBOBOXINFO info = {sizeof(COMBOBOXINFO)};
  if (!GetComboBoxInfo(w, &info)) return;
  const DWORD style = (DWORD)GetWindowLongPtrW(w, GWL_STYLE);
  const bool list    = (style & CBS_DROPDOWNLIST) == CBS_DROPDOWNLIST;
  const bool enabled = IsWindowEnabled(w) != FALSE;
  const bool hot     = enabled && IsHot(w, info.rcButton);
  const int status   = !enabled ? CBRO_DISABLED : hot ? CBRO_HOT : CBRO_NORMAL;
  DrawControlFrame(w, dc, r, L"Combobox", CP_READONLY, status,
      enabled ? st.brControlBg : st.brBackground);
  if (list) {
    RECT label = info.rcItem;
    DrawLabel(w, dc, label, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS);
    if (GetFocus() == w && !(SendMessageW(w, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
      DrawFocusRect(dc, &info.rcItem);
  }
  if (hot) { RECT btn = info.rcButton; InflateRect(&btn, -1, -1); FillRect(dc, &btn, st.brHotBg); }
  DrawArrow(dc, info.rcButton, false, enabled ? g_Text : g_DisabledText);
}

void DrawListBox(HWND w, HDC dc, RECT r) {
  FillRect(dc, &r, GetState().brBackground);
  const int count = (int)SendMessageW(w, LB_GETCOUNT, 0, 0);
  const int sel   = (int)SendMessageW(w, LB_GETCURSEL, 0, 0);
  const int top   = (int)SendMessageW(w, LB_GETTOPINDEX, 0, 0);
  for (int i = std::max(0, top); i < count; ++i) {
    RECT item; if (SendMessageW(w, LB_GETITEMRECT, i, (LPARAM)&item) == LB_ERR) continue;
    if (item.top >= r.bottom) break;
    const bool hi = (i == sel);
    if (hi) FillRect(dc, &item, GetSysColorBrush(COLOR_HIGHLIGHT));
    SetTextColor(dc, !IsWindowEnabled(w) ? g_DisabledText : hi ? GetSysColor(COLOR_HIGHLIGHTTEXT) : g_Text);
    const int len = (int)SendMessageW(w, LB_GETTEXTLEN, i, 0);
    if (len == LB_ERR || len >= 8192) continue;
    WCHAR text[8192]; SendMessageW(w, LB_GETTEXT, i, (LPARAM)text);
    item.left += 2;
    DrawTextW(dc, text, -1, &item, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
  }
}

void DrawSpinner(HWND w, HDC dc, RECT r) {
  auto& st = GetState();
  const bool horiz   = (GetWindowLongPtrW(w, GWL_STYLE) & UDS_HORZ) != 0;
  const bool enabled = IsWindowEnabled(w) != FALSE;
  for (int i = 0; i < 2; ++i) {
    RECT item = r;
    if (horiz) { if (i == 0) item.right = (r.left+r.right)/2; else item.left = (r.left+r.right)/2; }
    else        { if (i == 0) item.bottom= (r.top+r.bottom)/2; else item.top  = (r.top+r.bottom)/2; }
    FillRect(dc, &item, enabled && IsHot(w, item) ? st.brHotBg : st.brControlBg);
    FrameRect(dc, &item, st.brEdge);
    DrawArrow(dc, item, i == 0, enabled ? g_Text : g_DisabledText);
  }
}

void DrawSeparator(HWND w, HDC dc, RECT r) {
  FillRect(dc, &r, GetState().brBackground);
  if ((GetWindowLongPtrW(w, GWL_STYLE) & SS_TYPEMASK) == SS_ETCHEDHORZ) r.bottom = r.top + 1;
  else r.right = r.left + 1;
  FillRect(dc, &r, GetState().brEdge);
}

// ---------------------------------------------------------------------------
// Buffered paint dispatcher
// ---------------------------------------------------------------------------
void PaintControl(HWND w, HDC dc, ControlType type) {
  RECT r; GetClientRect(w, &r);
  if (r.right <= 0 || r.bottom <= 0) return;
  const int saved = SaveDC(dc);
  auto font = (HFONT)SendMessageW(w, WM_GETFONT, 0, 0);
  if (font) SelectObject(dc, font);
  SetTextColor(dc, IsWindowEnabled(w) ? g_Text : g_DisabledText);
  SetBkMode(dc, TRANSPARENT);
  switch (type) {
  case ControlType::Button:    DrawButton(w, dc, r);    break;
  case ControlType::Tab:       DrawTabs(w, dc, r);      break;
  case ControlType::ComboBox:  DrawComboBox(w, dc, r);  break;
  case ControlType::Header:    DrawHeader(w, dc, r);    break;
  case ControlType::Spinner:   DrawSpinner(w, dc, r);   break;
  case ControlType::ListBox:   DrawListBox(w, dc, r);   break;
  case ControlType::Separator: DrawSeparator(w, dc, r); break;
  }
  RestoreDC(dc, saved);
}

void PaintControlMessage(HWND w, UINT msg, WPARAM wParam, ControlType type) {
  PAINTSTRUCT ps{};
  HDC dc = (msg == WM_PAINT) ? BeginPaint(w, &ps) : (HDC)wParam;
  const int saved = SaveDC(dc);
  if (type == ControlType::Button && (GetWindowLongPtrW(w, GWL_STYLE) & BS_TYPEMASK) == BS_GROUPBOX) {
    // Group boxes are siblings of the controls they frame. Clip those siblings
    // out so custom dark-mode painting cannot cover title/text/button content.
    ClipSiblings(w, dc);
    PaintControl(w, dc, type);
    RestoreDC(dc, saved);
    if (msg == WM_PAINT) EndPaint(w, &ps);
    return;
  }
  if (type == ControlType::Tab) {
    ClipSiblings(w, dc);
  }
  RECT r; GetClientRect(w, &r);
  HDC buf = CreateCompatibleDC(dc);
  HBITMAP bmp = CreateCompatibleBitmap(dc, std::max(1L, (LONG)r.right), std::max(1L, (LONG)r.bottom));
  if (buf && bmp) {
    HGDIOBJ ob = SelectObject(buf, bmp);
    PaintControl(w, buf, type);
    BitBlt(dc, 0, 0, r.right, r.bottom, buf, 0, 0, SRCCOPY);
    SelectObject(buf, ob);
  } else {
    PaintControl(w, dc, type);
  }
  if (bmp) DeleteObject(bmp);
  if (buf) DeleteDC(buf);
  RestoreDC(dc, saved);
  if (msg == WM_PAINT) EndPaint(w, &ps);
}

// ---------------------------------------------------------------------------
// Control WM_PAINT border repair for Edit boxes
// ---------------------------------------------------------------------------
void PaintEditBorder(HWND w) {
  RECT r; GetWindowRect(w, &r); OffsetRect(&r, -r.left, -r.top);
  HDC dc = GetWindowDC(w); if (!dc) return;
  FrameRect(dc, &r, GetState().brEdge);
  if (GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_CLIENTEDGE) {
    InflateRect(&r, -1, -1);
    FrameRect(dc, &r, GetFocus() == w ? GetState().brEdge : GetState().brBackground);
  }
  ReleaseDC(w, dc);
}

// ---------------------------------------------------------------------------
// Control subclass procedure
// ---------------------------------------------------------------------------

// For certain state-changing messages the native control paints itself
// immediately inside DefSubclassProc before returning, overwriting our dark
// background with a light-mode glyph.  Suppress redraws across the call so
// only our WM_PAINT handler renders the updated control.
static bool ShouldDeferPaint(UINT msg, ControlType type) {
  if (type == ControlType::Edit) return false;
  switch (msg) {
  case WM_ENABLE:
  case WM_SETTEXT:
  case WM_SETFONT:
  case BM_SETCHECK:
  case BM_SETSTATE:
  case CB_SETCURSEL:
    return true;
  default:
    return false;
  }
}

LRESULT CALLBACK ControlProc(HWND w, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
  const auto type = (ControlType)data;
  if (msg == WM_NCDESTROY) { RemoveWindowSubclass(w, ControlProc, id); return DefSubclassProc(w, msg, wp, lp); }

  // Forward non-paint if dark mode is off
  if (!WindowsTheme::IsUsingDarkMode()) return DefSubclassProc(w, msg, wp, lp);

  // Edit: intercept NC painting for border
  if (type == ControlType::Edit) {
    const LRESULT r = DefSubclassProc(w, msg, wp, lp);
    if (msg == WM_NCPAINT || msg == WM_SETFOCUS || msg == WM_KILLFOCUS) PaintEditBorder(w);
    return r;
  }

  // Erase: handle ourselves
  if (msg == WM_ERASEBKGND) return TRUE;

  // Paint / PrintClient
  if (msg == WM_PAINT || msg == WM_PRINTCLIENT) {
    PaintControlMessage(w, msg, wp, type);
    return 0;
  }

  // Mouse tracking for hover repaint
  if (msg == WM_MOUSEMOVE) {
    TRACKMOUSEEVENT tme = {sizeof(tme), TME_LEAVE, w, 0};
    TrackMouseEvent(&tme);
  }

  // For state-change messages that trigger immediate native painting, suppress
  // the native redraw and let our WM_PAINT handler draw the final state.
  LRESULT res;
  if (ShouldDeferPaint(msg, type)) {
    DefSubclassProc(w, WM_SETREDRAW, FALSE, 0);
    res = DefSubclassProc(w, msg, wp, lp);
    DefSubclassProc(w, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(w, nullptr, FALSE);
    return res;
  }

  res = DefSubclassProc(w, msg, wp, lp);

  // Invalidate after state-changing messages
  switch (msg) {
  case WM_MOUSEMOVE:
  case WM_MOUSELEAVE:
    if (type == ControlType::Tab) {
      RECT tabStrip;
      GetClientRect(w, &tabStrip);
      RECT page = tabStrip;
      TabCtrl_AdjustRect(w, FALSE, &page);
      tabStrip.bottom = page.top;
      InvalidateRect(w, &tabStrip, FALSE);
    } else {
      InvalidateRect(w, nullptr, FALSE);
    }
    break;
  case WM_LBUTTONDOWN: case WM_LBUTTONUP:
  case WM_ENABLE: case WM_SETFOCUS: case WM_KILLFOCUS: case WM_UPDATEUISTATE:
  case WM_SETTEXT: case WM_SETFONT: case BM_SETCHECK: case BM_SETSTATE:
  case TCM_SETCURSEL: case CB_SETCURSEL: case LB_SETCURSEL:
  case LB_SETTOPINDEX: case WM_KEYDOWN: case WM_MOUSEWHEEL: case WM_VSCROLL:
    InvalidateRect(w, nullptr, FALSE);
    break;
  }
  return res;
}

void SetControlSubclass(HWND w, ControlType type) {
  SetWindowSubclass(w, ControlProc, 0, (DWORD_PTR)type);
}

// ---------------------------------------------------------------------------
// Parent-window subclass (WM_ERASEBKGND, WM_CTLCOLOR*, WM_SETTINGCHANGE)
// ---------------------------------------------------------------------------
LRESULT ControlColor(UINT msg, WPARAM wp, LPARAM lp) {
  HDC dc      = (HDC)wp;
  HWND ctrl   = (HWND)lp;
  auto& st    = GetState();
  SetTextColor(dc, IsWindowEnabled(ctrl) ? g_Text : g_DisabledText);
  const DWORD style = (DWORD)GetWindowLongPtrW(ctrl, GWL_STYLE);
  const bool edit   = HasClass(ctrl, WC_EDIT);
  const bool borderlessRO = edit && (style & ES_READONLY) && !(style & WS_BORDER) && !(GetWindowLongPtrW(ctrl, GWL_EXSTYLE) & WS_EX_CLIENTEDGE);
  const bool input  = !borderlessRO && (msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX || edit);
  HBRUSH surfaceBr = GetSurfaceBrush(ctrl);
  COLORREF bgCol = (surfaceBr == st.brPageBackground) ? g_PageBackground : g_Background;
  SetBkColor(dc, input ? g_Background : bgCol);
  return (LRESULT)(input ? st.brBackground : surfaceBr);
}

LRESULT CALLBACK ParentWindowProc(HWND w, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
  if (msg == WM_NCDESTROY) { RemoveWindowSubclass(w, ParentWindowProc, id); return DefSubclassProc(w, msg, wp, lp); }
  if (msg == WM_SETTINGCHANGE) { WindowsTheme::Refresh(); }
  if (!WindowsTheme::IsUsingDarkMode()) return DefSubclassProc(w, msg, wp, lp);
  if (msg == WM_ERASEBKGND) {
    HDC dc = (HDC)wp;
    RECT r;
    // Fill with the appropriate surface brush (pageBackground for tabs, background for top-level).
    if (!GetClipBox(dc, &r)) GetClientRect(w, &r);
    FillRect(dc, &r, GetSurfaceBrush(w));
    return TRUE;
  }
  if (msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) return ControlColor(msg, wp, lp);
  return DefSubclassProc(w, msg, wp, lp);
}

// ---------------------------------------------------------------------------
// Enumerate and subclass all child controls
// ---------------------------------------------------------------------------
BOOL CALLBACK ApplyControlEnum(HWND w, LPARAM) {
  WindowsTheme::ApplyToControl(w);
  return TRUE;
}

// ---------------------------------------------------------------------------
// Settings persistence helpers
// ---------------------------------------------------------------------------
std::string FallbackReadSettings() {
  wchar_t appData[MAX_PATH]{};
  if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) return "system";
  std::wstring path = std::wstring(appData) + L"\\Novadesk\\settings.json";
  std::ifstream f(path); if (!f.is_open()) return "system";
  try {
    nlohmann::json doc; f >> doc;
    if (doc.contains("windowsTheme") && doc["windowsTheme"].is_string()) return doc["windowsTheme"].get<std::string>();
    if (doc.contains("theme") && doc["theme"].is_string()) return doc["theme"].get<std::string>();
  } catch (...) {}
  return "system";
}

void FallbackWriteSettings(const std::string& value) {
  wchar_t appData[MAX_PATH]{};
  if (FAILED(SHGetFolderPathW(nullptr, CSIDL_APPDATA, nullptr, 0, appData))) return;
  std::wstring dir = std::wstring(appData) + L"\\Novadesk";
  CreateDirectoryW(dir.c_str(), nullptr);
  std::wstring path = dir + L"\\settings.json";
  nlohmann::json doc;
  std::ifstream in(path);
  if (in.is_open()) { try { in >> doc; } catch (...) { doc = nlohmann::json::object(); } in.close(); }
  doc["windowsTheme"] = value;
  std::ofstream out(path);
  if (out.is_open()) out << doc.dump(4);
}

// CBT hook for MessageBox / FileDialog
LRESULT CALLBACK CBTDialogHookProc(int code, WPARAM wParam, LPARAM lParam) {
  if (code == HCBT_ACTIVATE || code == HCBT_CREATEWND) {
    HWND w = (HWND)wParam;
    if (w && IsWindow(w)) {
      WCHAR cls[64]{}; GetClassNameW(w, cls, 64);
      if (wcscmp(cls, L"#32770") == 0) {
        WindowsTheme::ApplyToDialog(w);
        EnumChildWindows(w, ApplyControlEnum, 0);
      }
    }
  }
  return CallNextHookEx(nullptr, code, wParam, lParam);
}

} // anonymous namespace

// ===========================================================================
// Public API
// ===========================================================================

bool IsSystemDarkMode() {
  HKEY hKey = nullptr;
  DWORD value = 1, size = sizeof(value), type = 0;
  const WCHAR* key = L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
  if (RegOpenKeyExW(HKEY_CURRENT_USER, key, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
    if (RegQueryValueExW(hKey, L"AppsUseLightTheme", nullptr, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS && type == REG_DWORD) { RegCloseKey(hKey); return value == 0; }
    if (RegQueryValueExW(hKey, L"SystemUsesLightTheme", nullptr, &type, (LPBYTE)&value, &size) == ERROR_SUCCESS && type == REG_DWORD) { RegCloseKey(hKey); return value == 0; }
    RegCloseKey(hKey);
  }
  return false;
}

void SetSettingsHandlers(SettingsReader reader, SettingsWriter writer) {
  auto& st = GetState(); std::lock_guard<std::mutex> lock(st.handlerMutex);
  st.settingsReader = reader; st.settingsWriter = writer;
}

void Initialize() {
  auto& state = GetState();
  if (state.initialized.exchange(true)) { Refresh(); return; }

  HMODULE hUx = GetModuleHandleW(L"uxtheme.dll");
  if (!hUx) hUx = LoadLibraryExW(L"uxtheme.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (hUx) {
    state.pfnSetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(hUx, MAKEINTRESOURCEA(135));
    state.pfnAllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(hUx, MAKEINTRESOURCEA(133));
    state.pfnAllowDarkModeForApp = (fnAllowDarkModeForApp)GetProcAddress(hUx, MAKEINTRESOURCEA(132));
    state.pfnRefreshImmersiveColorPolicyState = (fnRefreshImmersiveColorPolicyState)GetProcAddress(hUx, MAKEINTRESOURCEA(104));
    state.pfnFlushMenuThemes = (fnFlushMenuThemes)GetProcAddress(hUx, MAKEINTRESOURCEA(136));
  }
  Refresh();
}

void Refresh() {
  auto& state = GetState();
  // Always follow the OS system theme — user-configurable theme selection
  // has been removed. Mode is always System.
  state.mode = Mode::System;
  state.isDarkMode = IsSystemDarkMode();

  // AllowDark lets the OS apply dark mode to this process when the system is dark.
  const int appMode = 1; // AllowDark
  if (state.pfnSetPreferredAppMode) state.pfnSetPreferredAppMode(appMode);
  else if (state.pfnAllowDarkModeForApp) state.pfnAllowDarkModeForApp(state.isDarkMode);
  if (state.pfnRefreshImmersiveColorPolicyState) state.pfnRefreshImmersiveColorPolicyState();
  if (state.pfnFlushMenuThemes) state.pfnFlushMenuThemes();
}

bool IsUsingDarkMode() { return GetState().isDarkMode.load(); }
Mode GetMode()         { return GetState().mode; }

void SetMode(Mode mode) {
  auto& state = GetState(); state.mode = mode;
  std::string val = "system";
  if (mode == Mode::Dark) val = "dark"; else if (mode == Mode::Light) val = "light";
  { std::lock_guard<std::mutex> lock(state.handlerMutex);
    if (state.settingsWriter) state.settingsWriter(val); else FallbackWriteSettings(val); }
  Refresh();
}
void SetMode(const std::string& modeStr) {
  auto& state = GetState();
  { std::lock_guard<std::mutex> lock(state.handlerMutex);
    if (state.settingsWriter) state.settingsWriter(modeStr); else FallbackWriteSettings(modeStr); }
  Refresh();
}

void ApplyToControl(HWND w) {
  if (!w || !IsWindow(w)) return;
  auto& state = GetState();
  const bool dark = state.isDarkMode.load();
  if (state.pfnAllowDarkModeForWindow) state.pfnAllowDarkModeForWindow(w, dark);

  WCHAR cls[64]{}; GetClassNameW(w, cls, 64);
  const DWORD style = (DWORD)GetWindowLongPtrW(w, GWL_STYLE);
  const bool isGroupBox = (_wcsicmp(cls, L"BUTTON") == 0) &&
                          ((style & BS_TYPEMASK) == BS_GROUPBOX);
  // Keep WS_CLIPSIBLINGS off group boxes: they overlap sibling content by design,
  // and clipping those siblings makes framed controls render blank.
  if (!isGroupBox) {
    SetWindowLongPtrW(w, GWL_STYLE, style | WS_CLIPSIBLINGS);
  }

  if (_wcsicmp(cls, WC_LISTVIEWW) == 0) {
    SetWindowTheme(w, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    if (dark) {
      ListView_SetBkColor(w, g_Background);
      ListView_SetTextBkColor(w, g_Background);
      ListView_SetTextColor(w, g_Text);
    } else {
      ListView_SetBkColor(w, GetSysColor(COLOR_WINDOW));
      ListView_SetTextBkColor(w, GetSysColor(COLOR_WINDOW));
      ListView_SetTextColor(w, GetSysColor(COLOR_WINDOWTEXT));
    }
    // Subclass the header child for custom drawing
    HWND hdr = ListView_GetHeader(w);
    if (hdr) {
      if (state.pfnAllowDarkModeForWindow) state.pfnAllowDarkModeForWindow(hdr, dark);
      SetWindowTheme(hdr, dark ? L"DarkMode_ItemsView" : L"Explorer", nullptr);
      if (dark) SetControlSubclass(hdr, ControlType::Header);
    }
  } else if (_wcsicmp(cls, WC_TABCONTROLW) == 0) {
    SetWindowTheme(w, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    if (dark) SetControlSubclass(w, ControlType::Tab);
  } else if (_wcsicmp(cls, L"BUTTON") == 0) {
    const DWORD style = (DWORD)GetWindowLongPtrW(w, GWL_STYLE);
    const DWORD type  = style & BS_TYPEMASK;
    const bool isCheck = type == BS_AUTOCHECKBOX || type == BS_CHECKBOX ||
                         type == BS_AUTORADIOBUTTON || type == BS_RADIOBUTTON || type == BS_GROUPBOX;
    const bool isGroupBox = (type == BS_GROUPBOX);
    // Use DarkMode_Explorer for all button types including checkboxes and radios.
    // DarkMode_Explorer provides correct dark glyph rendering when used consistently
    // with OpenThemeData(L"DarkMode_Explorer::Button") in the custom draw path.
    SetWindowTheme(w, dark ? L"DarkMode_Explorer" : (isCheck ? L"" : L"Explorer"), nullptr);
    if (dark && (type != BS_OWNERDRAW)) SetControlSubclass(w, ControlType::Button);
  } else if (_wcsicmp(cls, L"COMBOBOX") == 0) {
    SetWindowTheme(w, dark ? L"CFD" : L"", nullptr);
    const DWORD style = (DWORD)GetWindowLongPtrW(w, GWL_STYLE);
    if (dark && !(style & (CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE)) && (style & CBS_DROPDOWNLIST) != CBS_SIMPLE) {
      SetControlSubclass(w, ControlType::ComboBox);
      COMBOBOXINFO info = {sizeof(COMBOBOXINFO)};
      if (GetComboBoxInfo(w, &info) && info.hwndList) {
        if (state.pfnAllowDarkModeForWindow) state.pfnAllowDarkModeForWindow(info.hwndList, dark);
        SetControlSubclass(info.hwndList, ControlType::ListBox);
      }
    }
  } else if (_wcsicmp(cls, L"EDIT") == 0) {
    SetWindowTheme(w, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    if (dark) {
      const DWORD style = (DWORD)GetWindowLongPtrW(w, GWL_STYLE);
      if ((style & WS_BORDER) || (GetWindowLongPtrW(w, GWL_EXSTYLE) & WS_EX_CLIENTEDGE))
        SetControlSubclass(w, ControlType::Edit);
    }
  } else if (_wcsicmp(cls, WC_LISTBOX) == 0) {
    SetWindowTheme(w, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    if (dark) SetControlSubclass(w, ControlType::ListBox);
  } else if (_wcsicmp(cls, UPDOWN_CLASS) == 0) {
    if (dark) SetControlSubclass(w, ControlType::Spinner);
  } else if (_wcsicmp(cls, WC_STATIC) == 0) {
    SetWindowTheme(w, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
    const DWORD style = (DWORD)GetWindowLongPtrW(w, GWL_STYLE) & SS_TYPEMASK;
    if (dark && (style == SS_ETCHEDHORZ || style == SS_ETCHEDVERT))
      SetControlSubclass(w, ControlType::Separator);
  } else if (_wcsicmp(cls, WC_HEADERW) == 0) {
    SetWindowTheme(w, dark ? L"DarkMode_ItemsView" : L"Explorer", nullptr);
    if (dark) SetControlSubclass(w, ControlType::Header);
  } else {
    SetWindowTheme(w, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);
  }
}

void ApplyToWindow(HWND window) {
  if (!window || !IsWindow(window)) return;
  auto& state = GetState();
  const BOOL dark = state.isDarkMode.load() ? TRUE : FALSE;

  // Titlebar
  DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &dark, sizeof(dark));

  if (state.pfnAllowDarkModeForWindow) state.pfnAllowDarkModeForWindow(window, dark != FALSE);
  SetWindowTheme(window, dark ? L"DarkMode_Explorer" : L"Explorer", nullptr);

  // Install parent-window subclass for WM_ERASEBKGND / WM_CTLCOLOR*
  SetWindowSubclass(window, ParentWindowProc, 0, 0);
  EnableDialogTexture(window, (GetWindowLongPtrW(window, GWL_STYLE) & WS_CHILD) != 0);
}

void ApplyToDialog(HWND dialog) {
  if (!dialog || !IsWindow(dialog)) return;
  ApplyToWindow(dialog);
  EnumChildWindows(dialog, ApplyControlEnum, 0);
}

void ApplyToControlTree(HWND window, HWND progressBar) {
  if (!window || !IsWindow(window)) return;
  const BOOL dark = IsUsingDarkMode() ? TRUE : FALSE;

  // Dark titlebar via DWM — must be done before theming the controls.
  DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));
  DwmSetWindowAttribute(window, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &dark, sizeof(dark));

  // Theme the window itself and every child so buttons, listview, combobox
  // etc. draw dark. Deliberately does NOT install the parent subclass so the
  // host WndProc keeps ownership of WM_CTLCOLORSTATIC / WM_ERASEBKGND.
  ApplyToControl(window);
  EnumChildWindows(window, ApplyControlEnum, 0);

  // Progress bar: PBM_SETBARCOLOR is ignored when visual styles are active.
  // Disable the theme on the progress bar first, then set explicit colors.
  if (progressBar && IsWindow(progressBar)) {
    if (dark) {
      SetWindowTheme(progressBar, L"", L""); // disable visual styles
      SendMessageW(progressBar, PBM_SETBARCOLOR, 0, (LPARAM)RGB(0, 120, 215));
      SendMessageW(progressBar, PBM_SETBKCOLOR,  0, (LPARAM)RGB(48, 48, 48));
    } else {
      SetWindowTheme(progressBar, L"Explorer", L""); // restore visual styles
      SendMessageW(progressBar, PBM_SETBARCOLOR, 0, (LPARAM)CLR_DEFAULT);
      SendMessageW(progressBar, PBM_SETBKCOLOR,  0, (LPARAM)CLR_DEFAULT);
    }
  }
}

void ApplyToTooltip(HWND w) {
  if (!w || !IsWindow(w)) return;
  auto& state = GetState();
  const bool dark = state.isDarkMode.load();
  if (state.pfnAllowDarkModeForWindow) state.pfnAllowDarkModeForWindow(w, dark);
  if (dark) {
    SetWindowTheme(w, L"DarkMode_Explorer", L"ToolTip");
    SendMessageW(w, TTM_SETTIPBKCOLOR,   (WPARAM)g_Background, 0);
    SendMessageW(w, TTM_SETTIPTEXTCOLOR, (WPARAM)g_Text, 0);
  } else {
    SetWindowTheme(w, L"Explorer", L"ToolTip");
    SendMessageW(w, TTM_SETTIPBKCOLOR,   (WPARAM)RGB(255,255,255), 0);
    SendMessageW(w, TTM_SETTIPTEXTCOLOR, (WPARAM)RGB(0,0,0), 0);
  }
}

void PrepareMenu(HWND owner) {
  Refresh();
  auto& state = GetState();
  if (owner && IsWindow(owner) && state.pfnAllowDarkModeForWindow)
    state.pfnAllowDarkModeForWindow(owner, state.isDarkMode.load());
  if (state.pfnFlushMenuThemes) state.pfnFlushMenuThemes();
}

COLORREF GetTextColor() {
  return IsUsingDarkMode() ? g_Text : GetSysColor(COLOR_WINDOWTEXT);
}

COLORREF GetDisabledTextColor() {
  return IsUsingDarkMode() ? g_DisabledText : GetSysColor(COLOR_GRAYTEXT);
}

COLORREF GetDarkBackgroundColor() {
  return g_Background;
}

HBRUSH GetDarkBackgroundBrush() {
  // Persistent brush owned by ThemeState; shared across all callers.
  return GetState().brBackground;
}

void EnableDialogTexture(HWND window, bool enable) {
  EnableThemeDialogTexture(window, enable && !IsUsingDarkMode() ? ETDT_ENABLETAB : ETDT_DISABLE);
}

DialogHookScope::DialogHookScope() {
  WindowsTheme::Initialize();
  m_Hook = SetWindowsHookExW(WH_CBT, CBTDialogHookProc, nullptr, GetCurrentThreadId());
}
DialogHookScope::~DialogHookScope() {
  if (m_Hook) { UnhookWindowsHookEx(m_Hook); m_Hook = nullptr; }
}

} // namespace WindowsTheme
