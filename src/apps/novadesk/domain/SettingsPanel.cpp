/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "SettingsPanel.h"
#include "../../common/Localization.h"

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <windows.h>

#include "../scripting/quickjs/parser/PropertyParser.h"
#include "../shared/ColorUtil.h"
#include "../shared/Settings.h"
#include "../shared/Utils.h"
#include "../Version.h"
#include "SettingsPanelTheme.h"
#include "WidgetSettings.h"

extern std::vector<Widget *> widgets;

std::vector<SettingsPanel *> SettingsPanel::s_Panels;
std::vector<SettingsPanel *> SettingsPanel::s_PendingDestroy;

namespace {

const std::wstring &Loc(const char *key, const wchar_t *fallback) {
  return Localization::TextOr(key, fallback);
}

constexpr int kPanelW  = 520;
constexpr int kPanelH  = 480;

constexpr int kHeaderH = 48;

constexpr int kSideW   = 140;

constexpr int kContentX = kSideW;
constexpr int kContentW = kPanelW - kSideW;
constexpr int kContentH = kPanelH - kHeaderH;

constexpr int kTabBtnH  = 40;
constexpr int kTabBtnX  = 0;
constexpr int kTabIndW  = 3;
constexpr int kTabIndH  = 16;
constexpr int kTabPadX  = 16;

constexpr int kPadX     = 20;
constexpr int kRowH     = 44;
constexpr int kCtrlH    = 28;
constexpr int kCtrlW    = kContentW - kPadX * 2;
constexpr int kInlineLabelW = 220;
constexpr int kInlineCtrlW  = 100;
// InputBoxElement's bounds include its 10px left and right padding. Keep the
// declared size 20px smaller so its visible box matches a 100px dropdown.
constexpr int kInputDeclaredW = kInlineCtrlW - 20;

constexpr int kFooterH  = 52;

constexpr int kAboutH   = 180;

constexpr int kScrollH     = kContentH - kFooterH;
constexpr int kScrollHFull = kContentH;

constexpr int   kSbWidth  = 4;
constexpr float kSbRadius = 2.0f;
constexpr float kSbInset  = 3.0f;

// Settings-panel content is authored in logical LTR coordinates.  Keeping the
// conversion here means every built-in tab (including widget custom tabs) gets
// the same mirrored layout whenever the selected catalog is RTL.
inline bool IsRtlLayout() { return Localization::IsRtl(); }
inline int MirrorX(int x, int width, int surfaceWidth) {
  return IsRtlLayout() ? surfaceWidth - x - width : x;
}
inline int ContentX(int x, int width) { return MirrorX(x, width, kContentW); }
inline int ContentOriginX() { return IsRtlLayout() ? 0 : kContentX; }
inline int SidebarX() { return IsRtlLayout() ? kContentW : 0; }
inline TextAlignment DirectionalAlign(TextAlignment align) {
  // DirectWrite's leading/trailing alignment follows the reading direction.
  // Keeping the logical alignment lets an RTL text format place "left" at the
  // visual right edge and "right" at the visual left edge.
  return align;
}

inline std::wstring ScrollId(int ti) {
  return L"__scroll_" + std::to_wstring(ti);
}

static std::wstring MakeScrollContainer(Widget *panel, int ti,
                                        int x, int y, int w, int h,
                                        COLORREF thumbColor,
                                        COLORREF bgColor) {
  const std::wstring sid = ScrollId(ti);
  PropertyParser::ShapeOptions o;
  o.id      = sid;
  o.x       = x; o.y = y; o.width = w; o.height = h;
  o.hasSolidColor = true;
  o.solidColor = bgColor; o.solidAlpha = 255;
  o.fillColor  = bgColor; o.fillAlpha  = 255;
  o.strokeWidth = 0.0f;
  o.hasOverflow  = true; o.overflow = L"auto";
  o.hasOverflowX = true; o.overflowX = L"hidden";
  o.hasShowScrollbarX = true; o.showScrollbarX = false;
  o.hasShowScrollbarY = true; o.showScrollbarY = true;
  o.hasScrollbarOnLeft = true; o.scrollbarOnLeft = IsRtlLayout();
  o.hasScrollbarWidth = true; o.scrollbarWidth = kSbWidth;
  o.hasScrollbarRadius = true; o.scrollbarRadius = kSbRadius;
  o.hasScrollbarInset  = true; o.scrollbarInset  = kSbInset;
  o.hasScrollbarColor  = true;
  o.scrollbarColor = thumbColor; o.scrollbarAlpha = 140;
  o.hasScrollbarHoverColor = true;
  o.scrollbarHoverColor = thumbColor; o.scrollbarHoverAlpha = 210;
  o.hasScrollbarActiveColor = true;
  o.scrollbarActiveColor = thumbColor; o.scrollbarActiveAlpha = 255;
  o.hasScrollbarTrackColor = true;
  o.scrollbarTrackColor = bgColor; o.scrollbarTrackAlpha = 0;
  o.hasScrollbarButtonBgColor = true;
  o.scrollbarButtonBgColor = bgColor; o.scrollbarButtonBgAlpha = 0;
  panel->AddShape(o);
  return sid;
}

std::vector<std::wstring> ZPosLabels() {
  return {L"Normal", L"On Top", L"Always on Top", L"On Bottom", L"On Desktop"};
}
const ZPOSITION kZPosValues[] = {
  ZPOSITION_NORMAL, ZPOSITION_ONTOP, ZPOSITION_ONTOPMOST,
  ZPOSITION_ONBOTTOM, ZPOSITION_ONDESKTOP
};
constexpr int kZPosCount = 5;

int ZPosToIndex(ZPOSITION z) {
  for (int i = 0; i < kZPosCount; ++i)
    if (kZPosValues[i] == z) return i;
  return 0;
}

bool ParseDbl(const std::wstring &v, double &out) {
  wchar_t *end = nullptr;
  double d = std::wcstod(v.c_str(), &end);
  if (end == v.c_str()) return false;
  out = d; return true;
}

std::wstring CEId(int ti, const std::wstring &sid, const wchar_t *role) {
  return L"__t" + std::to_wstring(ti) + L"_" + sid + L"_" + role;
}

bool IsOn(const std::wstring &v) { return v == L"true" || v == L"1"; }

int OptIndex(const std::vector<std::wstring> &opts, const std::wstring &v) {
  for (size_t i = 0; i < opts.size(); ++i) if (opts[i] == v) return (int)i;
  return -1;
}

struct BoolRow { const wchar_t *id; const wchar_t *label; bool value; };

}

Widget *SettingsPanel::OpenFor(Widget *target) {
  FlushPending();
  if (!target || !Widget::IsValid(target)) return nullptr;
  for (SettingsPanel *p : s_Panels)
    if (p->m_Target == target && p->m_Panel && Widget::IsValid(p->m_Panel))
      { SetForegroundWindow(p->m_Panel->GetWindow()); return p->m_Panel; }

  SettingsPanel *self = new SettingsPanel();
  self->m_Target = target;
  target->SetInputSink(self);
  self->BuildPanel(target);
  if (!self->m_Panel) { delete self; return nullptr; }
  s_Panels.push_back(self);
  return self->m_Panel;
}

bool SettingsPanel::IsAlive(SettingsPanel *p) {
  return std::find(s_Panels.begin(), s_Panels.end(), p) != s_Panels.end();
}
void SettingsPanel::FlushPending() {
  std::vector<SettingsPanel *> tmp; tmp.swap(s_PendingDestroy);
  for (auto *p : tmp) delete p;
}

void SettingsPanel::RefreshAllThemes() {
  const ThemePalette newPalette = ResolveTheme();
  for (SettingsPanel *p : s_Panels) {
    if (!p || !p->m_Panel || !Widget::IsValid(p->m_Panel)) continue;
    p->m_Palette = newPalette;
    p->ApplyPaletteToPanel();
    p->m_Panel->Redraw();
  }
}
void SettingsPanel::RefreshAllLocalizedText() {
  std::vector<Widget *> targets;
  targets.reserve(s_Panels.size());
  for (SettingsPanel *panel : s_Panels)
    if (panel && panel->m_Target && Widget::IsValid(panel->m_Target))
      targets.push_back(panel->m_Target);

  // Rebuilding is intentional: labels, dropdown options, and RTL-aware layout
  // are all created with the current catalog. It also avoids stale text-format
  // objects while a panel is being rendered.
  CloseAll();
  for (Widget *target : targets)
    if (Widget::IsValid(target)) OpenFor(target);
}
void SettingsPanel::CloseAllForTarget(Widget *target) {
  if (!target) return;
  std::vector<SettingsPanel *> copy = s_Panels;
  for (auto *p : copy) if (p->m_Target == target) p->Close();
  FlushPending();
}
void SettingsPanel::CloseAll() {
  std::vector<SettingsPanel *> copy = s_Panels;
  for (auto *p : copy) p->Close();
  FlushPending();
}

void SettingsPanel::SyncTargetVisuals(Widget *target) {
  if (!target) return;
  for (SettingsPanel *p : s_Panels) {
    if (p->m_Target == target && p->m_Panel && Widget::IsValid(p->m_Panel)) {
      p->UpdateWindowTabVisuals();
      break;
    }
  }
}

static void Txt(Widget *panel, const std::wstring &id,
                int x, int y, int w, int h,
                const std::wstring &text, int size, COLORREF color,
                int weight = 400, bool hand = false,
                TextAlignment align = TEXT_ALIGN_LEFT_TOP,
                const std::wstring &containerId = L"") {
  PropertyParser::TextOptions o;
  o.id = id; o.x = containerId.empty() ? x : ContentX(x, w);
  o.y = y; o.width = w; o.height = h;
  o.text = text; o.fontSize = size; o.fontColor = color;
  o.fontWeight = weight;
  o.textAlign = DirectionalAlign(align);
  o.rightToLeft = IsRtlLayout();
  o.containerId = containerId;
  if (hand) o.mouseEventCursorName = L"hand";
  panel->AddText(o);
}

static void Rect(Widget *panel, const std::wstring &id,
                 int x, int y, int w, int h,
                 COLORREF color, BYTE alpha = 255,
                 float radius = 0.0f, bool hand = false,
                 const std::wstring &containerId = L"") {
  PropertyParser::ShapeOptions o;
  o.id = id; o.x = containerId.empty() ? x : ContentX(x, w);
  o.y = y; o.width = w; o.height = h;
  o.hasSolidColor = true;
  o.solidColor = color; o.solidAlpha = alpha;
  o.fillColor  = color; o.fillAlpha  = alpha;
  o.strokeWidth = 0.0f;
  o.containerId = containerId;
  if (radius > 0.0f) {
    o.radiusX = radius;
    o.radiusY = radius;
    o.solidColorRadius = static_cast<int>(radius);
  }
  if (hand) o.mouseEventCursorName = L"hand";
  panel->AddShape(o);
}

static void Ellipse(Widget *panel, const std::wstring &id,
                    int x, int y, int w, int h,
                    COLORREF color, BYTE alpha = 255, bool hand = false,
                    const std::wstring &containerId = L"") {
  PropertyParser::ShapeOptions o;
  o.id = id; o.x = containerId.empty() ? x : ContentX(x, w);
  o.y = y; o.width = w; o.height = h;
  o.shapeType = L"ellipse";
  o.hasSolidColor = true;
  o.solidColor = color; o.solidAlpha = alpha;
  o.fillColor  = color; o.fillAlpha  = alpha;
  o.strokeWidth = 0.0f;
  o.containerId = containerId;
  if (hand) o.mouseEventCursorName = L"hand";
  panel->AddShape(o);
}

static void Path(Widget *panel, const std::wstring &id, int x, int y,
                 const std::wstring &data, COLORREF color, bool hand,
                 const std::wstring &containerId) {
  PropertyParser::ShapeOptions o;
  o.id = id; o.x = containerId.empty() ? x : ContentX(x, 32);
  o.y = y; o.width = 32; o.height = 32;
  o.shapeType = L"path"; o.pathData = data;
  o.hasSolidColor = false;
  o.fillColor = color; o.fillAlpha = 255; o.strokeWidth = 0.0f;
  o.containerId = containerId;
  if (hand) o.mouseEventCursorName = L"hand";
  panel->AddShape(o);
}

static bool IsSafeExternalUrl(const std::wstring &url) {
  if (url.size() < 9 || url.find_first_of(L"\r\n") != std::wstring::npos)
    return false;
  std::wstring scheme = url.substr(0, 8);
  std::transform(scheme.begin(), scheme.end(), scheme.begin(), towlower);
  return scheme == L"https://";
}

static void BrandIcon(Widget *panel, const std::wstring &id,
                      const std::wstring &service, int x, int y,
                      bool hand, const std::wstring &containerId) {
  if (service == L"PayPal") {
    Path(panel, id, x, y, L"M15.607 4.653H8.941L6.645 19.251H1.82L4.862 0h7.995c3.754 0 6.375 2.294 6.473 5.513-.648-.478-2.105-.86-3.722-.86m6.57 5.546c0 3.41-3.01 6.853-6.958 6.853h-2.493L11.595 24H6.74l1.845-11.538h3.592c4.208 0 7.346-3.634 7.153-6.949a5.24 5.24 0 0 1 2.848 4.686M9.653 5.546h6.408c.907 0 1.942.222 2.363.541-.195 2.741-2.655 5.483-6.441 5.483H8.714Z", RGB(0, 69, 124), hand, containerId);
  } else if (service == L"Patreon") {
    Path(panel, id, x, y, L"M22.957 7.21c-.004-3.064-2.391-5.576-5.191-6.482-3.478-1.125-8.064-.962-11.384.604C2.357 3.231 1.093 7.391 1.046 11.54c-.039 3.411.302 12.396 5.369 12.46 3.765.047 4.326-4.804 6.068-7.141 1.24-1.662 2.836-2.132 4.801-2.618 3.376-.836 5.678-3.501 5.673-7.031Z", RGB(255, 66, 77), hand, containerId);
  } else if (service == L"YouTube") {
    Path(panel, id, x, y, L"M23.498 6.186a3.016 3.016 0 0 0-2.122-2.136C19.505 3.545 12 3.545 12 3.545s-7.505 0-9.377.505A3.017 3.017 0 0 0 .502 6.186C0 8.07 0 12 0 12s0 3.93.502 5.814a3.016 3.016 0 0 0 2.122 2.136c1.871.505 9.376.505 9.376.505s7.505 0 9.377-.505a3.015 3.015 0 0 0 2.122-2.136C24 15.93 24 12 24 12s0-3.93-.502-5.814zM9.545 15.568V8.432L15.818 12l-6.273 3.568z", RGB(255, 0, 0), hand, containerId);
  } else if (service == L"Instagram") {
    Path(panel, id, x, y, L"M7.03.084c-1.277.06-2.149.264-2.911.563-.789.308-1.458.72-2.123 1.388C1.33 2.703.92 3.372.616 4.162c-.295.764-.496 1.637-.552 2.914C.008 8.354-.005 8.765.001 12.023c.006 3.259.021 3.667.083 4.947.061 1.277.264 2.148.564 2.911.308.789.72 1.457 1.388 2.123.668.665 1.337 1.074 2.129 1.38.763.295 1.636.496 2.913.552 1.277.056 1.688.069 4.946.063 3.258-.006 3.668-.021 4.948-.081 1.28-.061 2.147-.265 2.91-.564.789-.308 1.458-.72 2.123-1.388.665-.668 1.074-1.338 1.38-2.128.296-.763.497-1.636.552-2.912.056-1.281.069-1.69.063-4.948-.006-3.258-.021-3.667-.082-4.946-.061-1.28-.264-2.149-.563-2.912-.308-.789-.72-1.457-1.388-2.123C21.298 1.33 20.628.921 19.838.617 19.074.321 18.202.12 16.924.065 15.647.009 15.236-.005 11.977.001 8.718.008 8.31.022 7.03.084m.14 21.693c-1.17-.051-1.805-.245-2.229-.408-.56-.216-.96-.477-1.382-.895-.422-.418-.681-.819-.9-1.378-.164-.423-.362-1.058-.417-2.228-.059-1.264-.072-1.644-.079-4.848-.007-3.204.005-3.583.061-4.848.05-1.169.245-1.805.408-2.228.216-.561.476-.96.895-1.382.419-.422.818-.681 1.378-.9.423-.165 1.058-.361 2.227-.417 1.266-.06 1.645-.072 4.848-.079 3.203-.007 3.584.005 4.85.061 1.169.051 1.805.245 2.228.408.561.216.96.475 1.382.895.422.419.682.818.9 1.379.166.422.362 1.056.417 2.226.06 1.266.074 1.645.08 4.848.006 3.203-.005 3.583-.061 4.848-.051 1.17-.245 1.806-.408 2.229-.216.56-.476.96-.895 1.381-.419.422-.818.681-1.378.9-.422.165-1.058.362-2.226.417-1.266.06-1.645.072-4.849.079-3.204.007-3.583-.006-4.848-.061M16.953 5.586A1.44 1.44 0 1 0 18.39 4.144a1.44 1.44 0 0 0-1.437 1.442M5.839 12.012c.007 3.403 2.771 6.156 6.173 6.149 3.403-.006 6.157-2.77 6.151-6.173-.007-3.403-2.771-6.156-6.174-6.15-3.403.007-6.156 2.771-6.15 6.174M8 12.008a4 4 0 1 1 4.008 3.992A4 4 0 0 1 8 12.008", RGB(228, 64, 95), hand, containerId);
  } else if (service == L"DeviantArt") {
    Path(panel, id, x, y, L"M19.207 4.794l.23-.43V0H15.07l-.436.44-2.058 3.925-.646.436H4.58v5.993h4.04l.36.436-4.175 7.98-.24.43V24H8.93l.436-.44 2.07-3.925.644-.436h7.35v-5.993h-4.05l-.36-.438 4.186-7.977z", RGB(5, 204, 71), hand, containerId);
  } else if (service == L"TikTok") {
    Path(panel, id, x, y, L"M12.525.02c1.31-.02 2.61-.01 3.91-.02.08 1.53.63 3.09 1.75 4.17 1.12 1.11 2.7 1.62 4.24 1.79v4.03c-1.44-.05-2.89-.35-4.2-.97-.57-.26-1.1-.59-1.62-.93-.01 2.92.01 5.84-.02 8.75-.08 1.4-.54 2.79-1.35 3.94-1.31 1.92-3.58 3.17-5.91 3.21-1.43.08-2.86-.31-4.08-1.03-2.02-1.19-3.44-3.37-3.65-5.71-.02-.5-.03-1-.01-1.49.18-1.9 1.12-3.72 2.58-4.96 1.66-1.44 3.98-2.13 6.15-1.72.02 1.48-.04 2.96-.04 4.44-.99-.32-2.15-.23-3.02.37-.63.41-1.11 1.04-1.36 1.75-.21.51-.15 1.07-.14 1.61.24 1.64 1.82 3.02 3.5 2.87 1.12-.01 2.19-.66 2.77-1.61.19-.33.4-.67.41-1.06.1-1.79.06-3.57.07-5.36.01-4.03-.01-8.05.02-12.07z", RGB(0, 0, 0), hand, containerId);
  } else if (service == L"GitHub") {
    Path(panel, id, x, y, L"M12 .297c-6.63 0-12 5.373-12 12 0 5.303 3.438 9.8 8.205 11.385.6.113.82-.258.82-.577 0-.285-.01-1.04-.015-2.04-3.338.724-4.042-1.61-4.042-1.61C4.422 18.07 3.633 17.7 3.633 17.7c-1.087-.744.084-.729.084-.729 1.205.084 1.838 1.236 1.838 1.236 1.07 1.835 2.809 1.305 3.495.998.108-.776.417-1.305.76-1.605-2.665-.3-5.466-1.332-5.466-5.93 0-1.31.465-2.38 1.235-3.22-.135-.303-.54-1.523.105-3.176 0 0 1.005-.322 3.3 1.23.96-.267 1.98-.399 3-.405 1.02.006 2.04.138 3 .405 2.28-1.552 3.285-1.23 3.285-1.23.645 1.653.24 2.873.12 3.176.765.84 1.23 1.91 1.23 3.22 0 4.61-2.805 5.625-5.475 5.92.42.36.81 1.096.81 2.22 0 1.606-.015 2.896-.015 3.286 0 .315.21.69.825.57C20.565 22.092 24 17.592 24 12.297c0-6.627-5.373-12-12-12", RGB(24, 23, 23), hand, containerId);
  } else {
    Path(panel, id, x, y, L"M20.317 4.37a19.79 19.79 0 00-4.885-1.515.074.074 0 00-.079.037c-.211.375-.445.865-.608 1.25a20.02 20.02 0 00-5.487 0c-.164-.393-.406-.874-.618-1.25a.077.077 0 00-.078-.037A19.74 19.74 0 003.677 4.37a.07.07 0 00-.032.028C.533 9.046-.319 13.58.099 18.058a.082.082 0 00.031.056c2.053 1.508 4.041 2.423 5.993 3.029a.078.078 0 00.084-.028c.462-.63.873-1.295 1.226-1.994a.076.076 0 00-.042-.106 12.3 12.3 0 01-1.872-.892.077.077 0 01-.008-.128c.126-.094.252-.192.372-.291a.074.074 0 01.078-.011c3.928 1.793 8.18 1.793 12.061 0a.074.074 0 01.079.01c.12.099.246.198.373.292a.077.077 0 01-.007.128c-.6.35-1.221.64-1.873.891a.077.077 0 00-.041.107c.36.698.772 1.363 1.225 1.993a.076.076 0 00.084.029c1.961-.607 3.95-1.522 6.002-3.03a.077.077 0 00.031-.055c.5-5.177-.838-9.674-3.548-13.66a.061.061 0 00-.031-.029zM8.02 15.331c-1.183 0-2.157-1.086-2.157-2.419 0-1.333.956-2.419 2.157-2.419 1.211 0 2.176 1.095 2.157 2.419 0 1.333-.956 2.419-2.157 2.419zm7.975 0c-1.183 0-2.157-1.086-2.157-2.419 0-1.333.955-2.419 2.157-2.419 1.211 0 2.176 1.095 2.157 2.419 0 1.333-.946 2.419-2.157 2.419Z", RGB(88, 101, 242), hand, containerId);
  }
}

void SettingsPanel::BuildPanel(Widget *target) {  m_Palette = ResolveTheme();

  const WidgetSettingsCatalog &cat = target->GetSettings();
  const int nCustom    = static_cast<int>(cat.tabs.size());
  const bool showWin   = cat.showWindowTab;

  m_WindowTabIndex = showWin ? nCustom : -1;
  m_AboutTabIndex  = nCustom + (showWin ? 1 : 0);
  m_TotalTabs      = m_AboutTabIndex + 1;
  m_ActiveTab      = (nCustom > 0) ? 0
                   : (showWin     ? m_WindowTabIndex
                                  : m_AboutTabIndex);

  RECT tr{};
  GetWindowRect(target->GetWindow(), &tr);
  const int screenW = GetSystemMetrics(SM_CXSCREEN);
  const int screenH = GetSystemMetrics(SM_CYSCREEN);
  int px = tr.right + 12;
  if (px + kPanelW > screenW) px = std::max(8, (int)tr.left - 12 - kPanelW);
  int py = tr.top;
  if (py + kPanelH > screenH) py = std::max(0, screenH - kPanelH - 8);

  WidgetOptions po;
  po.id = L""; po.x = px; po.y = py; po.width = kPanelW; po.height = kPanelH;
  po.m_WDefined = true; po.m_HDefined = true;
  po.minWidth = kPanelW; po.minHeight = kPanelH;
  wchar_t bg[64];
  swprintf_s(bg, L"rgba(%d,%d,%d,255)",
             GetRValue(m_Palette.background),
             GetGValue(m_Palette.background),
             GetBValue(m_Palette.background));
  po.backgroundColor = bg;
  po.color = m_Palette.background; po.bgAlpha = 255;
  po.draggable = true; po.resizable = false;
  po.keepOnScreen = true; po.snapEdges = false;
  po.showInToolbar = false; po.zPos = ZPOSITION_ONTOPMOST;
  po.show = false; po.windowOpacity = 255;
  po.scriptPath = L"__settings_panel__";

  Widget *panel = new Widget(po);
  if (!panel->Create()) { delete panel; return; }
  // Localization intentionally mirrors native title bars process-wide.  This
  // panel owns a custom header and mirrors its elements explicitly, so opt it
  // out of the inherited GDI mirror to avoid applying RTL twice.
  if (HWND hwnd = panel->GetWindow()) {
    const LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    SetWindowLongPtrW(hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_LAYOUTRTL);
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE |
                 SWP_FRAMECHANGED);
  }
  m_Panel = panel;
  panel->SetInputSink(this);
  panel->BeginUpdate();

  Rect(panel, L"__hdr_bg", 0, 0, kPanelW, kHeaderH, m_Palette.sidebar);

  std::wstring hdrTitle = cat.panelTitle;
  if (hdrTitle.empty()) {
    hdrTitle = target->GetOptions().id;
    if (hdrTitle.empty() || hdrTitle == L"widget") hdrTitle = target->GetTitle();
    if (hdrTitle.empty()) hdrTitle = Loc("settingspanel.widget", L"Widget");
    hdrTitle = Loc("settingspanel.titlePrefix", L"Settings \u2014 ") + hdrTitle;
  }
  Txt(panel, L"__hdr_title", MirrorX(kPadX, kPanelW - kPadX - 48, kPanelW), 14, kPanelW - kPadX - 48,
      22, hdrTitle, 13, m_Palette.text, 600, false, TEXT_ALIGN_LEFT_TOP);

  {
    constexpr int kCloseSize = 16;
    const int closeX = MirrorX(kPanelW - kCloseSize - 16, kCloseSize, kPanelW);
    const int closeY = (kHeaderH - kCloseSize) / 2;
    Rect(panel, L"__hdr_close", closeX, closeY, kCloseSize, kCloseSize,
         RGB(239, 68, 68), 255, static_cast<float>(kCloseSize) / 2.0f, true);
    m_Controls[L"__hdr_close"] = {L"", ControlKind::CloseButton, -1};
  }

  Rect(panel, L"__hdr_border", 0, kHeaderH - 1, kPanelW, 1, m_Palette.divider);

  Rect(panel, L"__sb_bg", SidebarX(), kHeaderH, kSideW, kContentH, m_Palette.sidebar);

  Rect(panel, L"__sb_border", IsRtlLayout() ? kContentW : kSideW - 1, kHeaderH, 1, kContentH,
       m_Palette.sidebarBorder);

  std::vector<std::wstring> tabLabels, tabIcons;
  for (const auto &t : cat.tabs) {
    tabLabels.push_back(t.label);
    tabIcons.push_back(t.icon);
  }
  if (showWin) { tabLabels.push_back(Loc("settingspanel.tab.window", L"Window"));  tabIcons.push_back(L"\uE770"); }
  tabLabels.push_back(Loc("settingspanel.tab.about", L"About")); tabIcons.push_back(L"\uE946");

  const int sideContentY = kHeaderH + 12;

  for (int ti = 0; ti < m_TotalTabs; ++ti) {
    const int btnY = sideContentY + ti * kTabBtnH;
    const bool active = (ti == m_ActiveTab);

    const int midY = btnY + kTabBtnH / 2;

    {
      const int tabPadX = 6;
      const int tabBgH = 32;
      const int tabBgY = midY - tabBgH / 2 -2;
      PropertyParser::ShapeOptions bg;
      bg.id = L"__tab_bg_" + std::to_wstring(ti);
      bg.x = SidebarX() + tabPadX;
      bg.y = tabBgY;
      bg.width = kSideW - tabPadX * 2;
      bg.height = tabBgH;
      bg.radiusX = 6.0f;
      bg.radiusY = 6.0f;
      bg.solidColorRadius = 6;
      bg.strokeWidth = 0.0f;
      bg.hasSolidColor = true;
      COLORREF initColor = active ? m_Palette.controlFill : m_Palette.sidebar;
      bg.solidColor = initColor; bg.solidAlpha = 255;
      bg.fillColor  = initColor; bg.fillAlpha  = 255;
      bg.mouseEventCursorName = L"hand";
      panel->AddShape(bg);
      m_Controls[bg.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }

    {
      PropertyParser::ShapeOptions ind;
      ind.id = L"__tab_ind_" + std::to_wstring(ti);
      ind.x = SidebarX() + (IsRtlLayout() ? kSideW - kTabIndW - 2 : 2);
      ind.y = midY - kTabIndH / 2 - 3;
      ind.width = kTabIndW;
      ind.height = kTabIndH;
      ind.radiusX = 1.5f;
      ind.radiusY = 1.5f;
      ind.solidColorRadius = 2;
      ind.strokeWidth = 0.0f;
      ind.hasSolidColor = true;
      ind.solidColor = m_Palette.accent; ind.solidAlpha = 255;
      ind.fillColor  = m_Palette.accent; ind.fillAlpha  = 255;
      panel->AddShape(ind);
      m_Controls[ind.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }

    if (!tabIcons[ti].empty()) {
      constexpr int kIconSize = 16;
      PropertyParser::TextOptions ico;
      ico.id = L"__tab_ico_" + std::to_wstring(ti);
      ico.x = SidebarX() + (IsRtlLayout() ? kSideW - kTabPadX - kIconSize : kTabPadX);
      ico.y = midY - kIconSize / 2 - 1;
      ico.width = kIconSize;
      ico.height = kIconSize;
      ico.text = tabIcons[ti];
      ico.fontFace = L"Segoe MDL2 Assets";
      ico.fontSize = 12;
      ico.fontColor = active ? m_Palette.accent : m_Palette.tabInactive;
      ico.mouseEventCursorName = L"hand";
      panel->AddText(ico);
      m_Controls[ico.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }

    {
      constexpr int kTabLabelFontSize = 12;
      constexpr int kLabelH = 14;
      PropertyParser::TextOptions lbl;
      lbl.id = L"__tab_lbl_" + std::to_wstring(ti);
      lbl.x = SidebarX() + (IsRtlLayout()
          ? 8
          : kTabPadX + (!tabIcons[ti].empty() ? 22 : 0));
      // TextElement's paragraph-center mode has a visible baseline offset in
      // a SettingsPanel tab. Position a top-aligned glyph line directly at
      // the card's midpoint instead.
      lbl.y = midY - kLabelH / 2 - 2;
      // In RTL the icon occupies the visual right edge, so the label must
      // stop before it rather than drawing underneath it.
      lbl.width = kSideW - kTabPadX - 8 -
          (IsRtlLayout() && !tabIcons[ti].empty() ? 22 : 0);
      lbl.height = kLabelH;
      lbl.text = tabLabels[ti];
      lbl.fontSize = kTabLabelFontSize;
      lbl.fontWeight = active ? 600 : 400;
      lbl.fontColor = active ? m_Palette.tabActive : m_Palette.tabInactive;
      lbl.textAlign = TEXT_ALIGN_LEFT_TOP;
      lbl.rightToLeft = IsRtlLayout();
      lbl.mouseEventCursorName = L"hand";
      panel->AddText(lbl);
      m_Controls[lbl.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }
  }

  Txt(panel, L"__sb_ver",
      SidebarX() + (IsRtlLayout() ? 8 : kTabPadX), kHeaderH + kContentH - 22,
      kSideW - kTabPadX, 18, Utils::ToWString(NOVADESK_VERSION), 9, m_Palette.muted,
      400, false, TEXT_ALIGN_LEFT_TOP);

  Rect(panel, L"__content_bg",
       ContentOriginX(), kHeaderH, kContentW, kContentH, m_Palette.background);

  const int contentY0 = kHeaderH + 16;

  for (int ti = 0; ti < nCustom; ++ti)
    BuildCustomTab(panel, target, cat.tabs[ti], ti, contentY0);

  if (showWin) BuildWindowTab(panel, target, m_WindowTabIndex, contentY0);
  BuildAboutTab(panel, m_AboutTabIndex, contentY0);

  if (nCustom > 0) {
    const int footerY = kHeaderH + kContentH - kFooterH;
    const int resetW  = 150;
    const int resetH  = 28;
    const int resetX  = ContentOriginX() + ContentX(kContentW - kPadX - resetW, resetW);
    const int resetY  = footerY + (kFooterH - resetH) / 2;

    Rect(panel, L"__settings_hr2",
         ContentOriginX(), footerY, kContentW, 1, m_Palette.divider);
    m_Controls[L"__settings_hr2"] = {L"", ControlKind::ResetLabel, 0};

    Rect(panel, L"__settings_reset",
         resetX, resetY, resetW, resetH,
         m_Palette.controlFill, 255, 6.0f, true);
    m_Controls[L"__settings_reset"] = {L"", ControlKind::ResetButton, 0};

    Txt(panel, L"__settings_reset_label",
        resetX + resetW / 2, resetY + resetH / 2, resetW, resetH,
        Loc("settingspanel.resetToDefaults", L"Reset to defaults"),
        12, m_Palette.text, 400, true, TEXT_ALIGN_CENTER_CENTER);
    m_Controls[L"__settings_reset_label"] = {L"", ControlKind::ResetLabel, 0};
  }

  {
    std::lock_guard<std::mutex> lock(Widget::s_WidgetMutex);
    widgets.push_back(panel);
    Widget::s_WidgetSet.insert(panel);
    if (panel->GetWindow()) Widget::s_HwndMap[panel->GetWindow()] = panel;
  }

  panel->EndUpdate();
  SwitchTab(m_ActiveTab);
  panel->Show();
  if (panel->GetWindow()) SetForegroundWindow(panel->GetWindow());
}

void SettingsPanel::BuildCustomTab(Widget *panel, Widget *target,
                                   const WidgetSettingsTab &tab,
                                   int ti, int startY) {
  const std::wstring scrollId = MakeScrollContainer(
      panel, ti,
      ContentOriginX(), kHeaderH,
      kContentW, kScrollH,
      m_Palette.label,
      m_Palette.background);
  m_Controls[scrollId] = {L"", ControlKind::SelectLabel, ti};

  int y = startY - kHeaderH;

  Txt(panel, CEId(ti, L"", L"heading"),
      kPadX, y, kCtrlW, 22,
      tab.label, 14, m_Palette.text, 700, false, TEXT_ALIGN_LEFT_TOP, scrollId);
  m_Controls[CEId(ti, L"", L"heading")] = {L"", ControlKind::SelectLabel, ti};
  y += 30;

  Rect(panel, CEId(ti, L"", L"hdivider"),
       kPadX, y, kCtrlW, 1, m_Palette.divider, 255, 0.0f, false, scrollId);
  m_Controls[CEId(ti, L"", L"hdivider")] = {L"", ControlKind::SelectLabel, ti};
  y += 12;

  for (const WidgetSetting &setting : tab.settings) {
    const std::wstring value = target->GetSettings().ValueOrDefault(setting.id);

    const int rowOff = (kRowH - kCtrlH) / 2;
    Txt(panel, CEId(ti, setting.id, L"label"),
        kPadX, y + rowOff + kCtrlH / 2, kInlineLabelW, kCtrlH,
        setting.label, 12, m_Palette.label, 400, false, TEXT_ALIGN_LEFT_CENTER, scrollId);
    m_Controls[CEId(ti, setting.id, L"label")] =
        {setting.id, ControlKind::SelectLabel, ti};

    const int ctrlX = kPadX + kCtrlW - kInlineCtrlW;
    const int ctrlY = y + rowOff;

    switch (setting.type) {
    case WidgetSettingType::Color: {
      PropertyParser::ColorPickerOptions co;
      co.id = CEId(ti, setting.id, L"swatch");
      co.containerId = scrollId;
      co.x = ContentX(kPadX + kCtrlW - kCtrlH, kCtrlH); co.y = ctrlY;
      co.width = kCtrlH; co.height = kCtrlH;
      COLORREF col = RGB(0,0,0); BYTE alp = 255;
      if (!value.empty()) ColorUtil::ParseRGBA(value, col, alp);
      co.color = col; co.borderRadius = 7.0f; co.borderWidth = 1.0f;
      co.borderColor = m_Palette.controlBorder; co.borderAlpha = 255;
      co.popupBackground = m_Palette.controlFill; co.popupBackgroundAlpha = 255;
      co.popupAccentColor = m_Palette.accent;
      co.popupBorderColor = m_Palette.divider;
      co.showEyedropper = false; co.mouseEventCursorName = L"hand";
      panel->AddColorPicker(co);
      m_Controls[co.id] = {setting.id, ControlKind::ColorSwatch, ti};
      break;
    }
    case WidgetSettingType::Number:
    case WidgetSettingType::Text: {
      Element *bs = nullptr;
      if (setting.type == WidgetSettingType::Number &&
          !setting.binding.elementId.empty() &&
          setting.binding.property == L"value" && m_Target &&
          Widget::IsValid(m_Target))
        bs = m_Target->FindElementById(setting.binding.elementId);
      if (auto *sl = dynamic_cast<SliderElement *>(bs)) {
        double init = sl->m_MinValue; ParseDbl(value, init);
        sl->SetValue(init); m_Target->Redraw();
        m_Controls[sl->GetId()] = {setting.id, ControlKind::BoundSlider, ti};
        break;
      }
      PropertyParser::InputBoxOptions io;
      io.id = CEId(ti, setting.id, L"input");
      io.containerId = scrollId;
      io.x = ContentX(ctrlX, kInlineCtrlW); io.y = ctrlY;
      io.width = kInputDeclaredW; io.height = kCtrlH;
      io.text = value; io.fontSize = 12;
      io.fontColor = m_Palette.text; io.fontAlpha = 255;
      io.hasFillColor = true;
      io.fillColor = m_Palette.controlFill; io.fillAlpha = 255;
      io.borderWidth = 1.0f; io.borderRadius = 7.0f;
      io.borderColor = m_Palette.controlBorder; io.borderColorAlpha = 255;
      io.caretColor = m_Palette.text; io.selectionColor = m_Palette.accent;
      io.paddingLeft = 10; io.paddingRight = 10;
      io.textAlign = DirectionalAlign(TEXT_ALIGN_LEFT_CENTER);
      if (setting.type == WidgetSettingType::Number) io.inputType = InputType::Float;
      panel->AddInputBox(io);
      m_Controls[io.id] = {setting.id, ControlKind::Input, ti};
      break;
    }
    case WidgetSettingType::Toggle: {
      const bool on = IsOn(value);
      Element *bound = nullptr;
      if (!setting.binding.elementId.empty() &&
          setting.binding.property == L"checked" && m_Target &&
          Widget::IsValid(m_Target))
        bound = m_Target->FindElementById(setting.binding.elementId);
      if (auto *sw = dynamic_cast<ToggleSwitchElement *>(bound)) {
        sw->SetChecked(on); m_Target->Redraw();
        m_Controls[sw->GetId()] = {setting.id, ControlKind::BoundSwitch, ti};
        break;
      }
      if (auto *cb = dynamic_cast<CheckBoxElement *>(bound)) {
        cb->SetChecked(on); m_Target->Redraw();
        m_Controls[cb->GetId()] = {setting.id, ControlKind::BoundSwitch, ti};
        break;
      }
      PropertyParser::ToggleSwitchOptions ts;
      ts.id = CEId(ti, setting.id, L"pill");
      ts.containerId = scrollId;
      ts.x = ContentX(kCtrlW - 46 + kPadX, 46); ts.y = ctrlY;
      ts.width = 46; ts.height = 24;
      ts.checked = on; ts.onColor = m_Palette.accent; ts.onAlpha = 255;
      ts.offColor = m_Palette.toggleOff; ts.offAlpha = 255;
      ts.borderRadius = -1.0f;
      ts.knobColor = RGB(252,252,255); ts.knobAlpha = 255;
      ts.knobPadding = 3.0f; ts.mouseEventCursorName = L"hand";
      panel->AddToggleSwitch(ts);
      m_Controls[ts.id] = {setting.id, ControlKind::TogglePill, ti};
      break;
    }
    case WidgetSettingType::Select: {
      if (!setting.options.empty()) {
        PropertyParser::DropDownOptions dd;
        dd.id = CEId(ti, setting.id, L"dropdown");
        dd.containerId = scrollId;
        dd.x = ContentX(ctrlX, kInlineCtrlW); dd.y = ctrlY;
        dd.width = kInlineCtrlW; dd.height = kCtrlH;
        for (const auto &opt : setting.options) dd.options.push_back({opt, opt});
        dd.hasOptions = true;
        dd.selectedIndex = OptIndex(setting.options, value);
        dd.rightToLeft = IsRtlLayout();
        dd.backgroundColor    = m_Palette.controlFill;
        dd.borderColor        = m_Palette.controlBorder;
        dd.borderWidth = 1.0f; dd.borderRadius = 7.0f;
        dd.fontColor          = m_Palette.text;
        dd.placeholderColor   = m_Palette.muted;
        dd.popupBackground    = m_Palette.controlFill;
        dd.popupBorderColor   = m_Palette.controlBorder;
        dd.popupHoverColor    = m_Palette.controlBorder;
        dd.popupSelectedColor = m_Palette.accent;
        dd.popupTextColor     = m_Palette.text;
        dd.mouseEventCursorName = L"hand";
        panel->AddDropDown(dd);
        m_Controls[dd.id] = {setting.id, ControlKind::PanelDropDown, ti};
        break;
      }
      Rect(panel, CEId(ti, setting.id, L"btn"),
           ctrlX, ctrlY, kInlineCtrlW, kCtrlH,
           m_Palette.controlFill, 255, 7.0f, true, scrollId);
      m_Controls[CEId(ti, setting.id, L"btn")] =
          {setting.id, ControlKind::SelectButton, ti};
      Txt(panel, CEId(ti, setting.id, L"btnlabel"),
          ctrlX + 8, ctrlY, kInlineCtrlW - 16, kCtrlH,
          value, 12, m_Palette.text, 400, true, TEXT_ALIGN_CENTER_CENTER, scrollId);
      m_Controls[CEId(ti, setting.id, L"btnlabel")] =
          {setting.id, ControlKind::SelectLabel, ti};
      break;
    }
    }
    y += kRowH;
  }
}

void SettingsPanel::BuildWindowTab(Widget *panel, Widget *target,
                                   int ti, int startY) {
  if (!target || !Widget::IsValid(target)) return;
  const WidgetOptions &opts = target->GetOptions();

  const std::wstring scrollId = MakeScrollContainer(
      panel, ti,
      ContentOriginX(), kHeaderH,
      kContentW, kScrollHFull,
      m_Palette.label,
      m_Palette.background);
  m_Controls[scrollId] = {L"", ControlKind::SelectLabel, ti};

  int y = startY - kHeaderH;

  Txt(panel, CEId(ti, L"", L"heading"),
      kPadX, y, kCtrlW, 22,
      Loc("settingspanel.window.title", L"Window Settings"), 14, m_Palette.text, 700, false, TEXT_ALIGN_LEFT_TOP, scrollId);
  m_Controls[CEId(ti, L"", L"heading")] = {L"", ControlKind::SelectLabel, ti};
  y += 30;

  Rect(panel, CEId(ti, L"", L"hdivider"),
       kPadX, y, kCtrlW, 1, m_Palette.divider, 255, 0.0f, false, scrollId);
  m_Controls[CEId(ti, L"", L"hdivider")] = {L"", ControlKind::SelectLabel, ti};
  y += 16;

  const BoolRow toggleRows[] = {
    { L"__win_draggable",    Loc("settingspanel.window.draggable", L"Draggable").c_str(),       opts.draggable    },
    { L"__win_clickthrough", Loc("settingspanel.window.clickThrough", L"Click-through").c_str(), opts.clickThrough },
    { L"__win_keeponscreen", Loc("settingspanel.window.keepOnScreen", L"Keep on screen").c_str(), opts.keepOnScreen },
    { L"__win_snapedges",    Loc("settingspanel.window.snapEdges", L"Snap edges").c_str(),      opts.snapEdges    },
  };

  for (const auto &r : toggleRows) {
    Txt(panel, std::wstring(r.id) + L"_lbl",
        kPadX, y + 4, kCtrlW - 56, 24,
        r.label, 12, m_Palette.label, 400, false, TEXT_ALIGN_LEFT_TOP, scrollId);
    m_Controls[std::wstring(r.id) + L"_lbl"] = {r.id, ControlKind::SelectLabel, ti};

    PropertyParser::ToggleSwitchOptions ts;
    ts.id = r.id;
    ts.containerId = scrollId;
    ts.x = ContentX(kCtrlW - 46 + kPadX, 46); ts.y = y + 4;
    ts.width = 46; ts.height = 24;
    ts.checked = r.value;
    ts.onColor = m_Palette.accent; ts.onAlpha = 255;
    ts.offColor = m_Palette.toggleOff; ts.offAlpha = 255;
    ts.borderRadius = -1.0f;
    ts.knobColor = RGB(252,252,255); ts.knobAlpha = 255;
    ts.knobPadding = 3.0f; ts.mouseEventCursorName = L"hand";
    panel->AddToggleSwitch(ts);
    m_Controls[ts.id] = {r.id, ControlKind::WindowToggle, ti};

    y += 36;
    Rect(panel, std::wstring(r.id) + L"_sep",
         kPadX, y, kCtrlW, 1, m_Palette.divider, 255, 0.0f, false, scrollId);
    m_Controls[std::wstring(r.id) + L"_sep"] = {L"", ControlKind::SelectLabel, ti};
    y += 8;
  }

  const int ctrlX  = kPadX + kCtrlW - kInlineCtrlW;
  const int rowOff = (kRowH - kCtrlH) / 2;

  Txt(panel, L"__win_zpos_lbl",
      kPadX, y + rowOff + kCtrlH / 2, kInlineLabelW, kCtrlH,
      Loc("settingspanel.window.zPosition", L"Z-position"), 12, m_Palette.label, 400, false, TEXT_ALIGN_LEFT_CENTER, scrollId);
  m_Controls[L"__win_zpos_lbl"] = {L"", ControlKind::SelectLabel, ti};
  {
    PropertyParser::DropDownOptions dd;
    dd.id = L"__win_zpos";
    dd.containerId = scrollId;
    dd.x = ContentX(ctrlX, kInlineCtrlW); dd.y = y + rowOff;
    dd.width = kInlineCtrlW; dd.height = kCtrlH;
    dd.popupMinWidth = 150;
    for (const auto &label : ZPosLabels()) dd.options.push_back({label, label});
    dd.hasOptions = true; dd.selectedIndex = ZPosToIndex(opts.zPos);
    dd.rightToLeft = IsRtlLayout();
    dd.backgroundColor = m_Palette.controlFill;
    dd.borderColor = m_Palette.controlBorder;
    dd.borderWidth = 1.0f; dd.borderRadius = 7.0f;
    dd.fontColor = m_Palette.text; dd.placeholderColor = m_Palette.muted;
    dd.popupBackground = m_Palette.controlFill;
    dd.popupBorderColor = m_Palette.controlBorder;
    dd.popupHoverColor = m_Palette.controlBorder;
    dd.popupSelectedColor = m_Palette.accent;
    dd.popupTextColor = m_Palette.text; dd.mouseEventCursorName = L"hand";
    panel->AddDropDown(dd);
    m_Controls[dd.id] = {L"__win_zpos", ControlKind::WindowZPos, ti};
  }
  y += kRowH;

  Txt(panel, L"__win_opacity_lbl",
      kPadX, y + rowOff + kCtrlH / 2, kInlineLabelW, kCtrlH,
      Loc("settingspanel.window.opacity", L"Opacity (0 \u2013 100)"), 12, m_Palette.label, 400, false, TEXT_ALIGN_LEFT_CENTER, scrollId);
  m_Controls[L"__win_opacity_lbl"] = {L"", ControlKind::SelectLabel, ti};
  {
    const int pct = static_cast<int>(std::round(opts.windowOpacity / 255.0 * 100.0));
    wchar_t buf[8]; swprintf_s(buf, L"%d", pct);
    PropertyParser::InputBoxOptions io;
    io.id = L"__win_opacity";
    io.containerId = scrollId;
    io.x = ContentX(ctrlX, kInlineCtrlW); io.y = y + rowOff;
    io.width = kInputDeclaredW; io.height = kCtrlH;
    io.text = buf; io.fontSize = 12;
    io.fontColor = m_Palette.text; io.fontAlpha = 255;
    io.hasFillColor = true; io.fillColor = m_Palette.controlFill; io.fillAlpha = 255;
    io.borderWidth = 1.0f; io.borderRadius = 7.0f;
    io.borderColor = m_Palette.controlBorder; io.borderColorAlpha = 255;
    io.caretColor = m_Palette.text; io.selectionColor = m_Palette.accent;
    io.paddingLeft = 10; io.paddingRight = 10;
    io.textAlign = DirectionalAlign(TEXT_ALIGN_LEFT_CENTER);
    io.inputType = InputType::Float;
    panel->AddInputBox(io);
    m_Controls[io.id] = {L"__win_opacity", ControlKind::WindowOpacity, ti};
  }
}

void SettingsPanel::BuildAboutTab(Widget *panel, int ti, int startY) {
  const WidgetSettingsCatalog &cat =
      m_Target ? m_Target->GetSettings() : WidgetSettingsCatalog{};
  const auto &about = cat.about;

  const std::wstring scrollId = MakeScrollContainer(
      panel, ti,
      ContentOriginX(), kHeaderH,
      kContentW, kScrollHFull,
      m_Palette.label,
      m_Palette.background);
  m_Controls[scrollId] = {L"", ControlKind::SelectLabel, ti};

  int y = startY - kHeaderH;

  const std::wstring nameText =
      about.hasName ? about.name
      : (m_Target ? m_Target->GetOptions().id : Loc("settingspanel.appName", L"Novadesk"));
  Txt(panel, L"__about_name",
      kPadX, y, kCtrlW, 32,
      nameText.empty() ? Loc("settingspanel.appName", L"Novadesk") : nameText, 20, m_Palette.accent, 700,
      false, TEXT_ALIGN_LEFT_TOP, scrollId);
  m_Controls[L"__about_name"] = {L"", ControlKind::SelectLabel, ti};
  y += 36;

  const std::wstring verText = about.hasVersion
      ? (Loc("settingspanel.versionPrefix", L"Version ") + about.version)
      : (Loc("settingspanel.versionPrefix", L"Version ") + Utils::ToWString(NOVADESK_VERSION));
  Txt(panel, L"__about_ver",
      kPadX, y, kCtrlW, 18, verText, 12, m_Palette.label, 400,
      false, TEXT_ALIGN_LEFT_TOP, scrollId);
  m_Controls[L"__about_ver"] = {L"", ControlKind::SelectLabel, ti};
  y += 22;

  if (about.hasDescription) {
    Txt(panel, L"__about_desc",
        kPadX, y, kCtrlW, 18,
        about.description, 11, m_Palette.muted, 400,
        false, TEXT_ALIGN_LEFT_TOP, scrollId);
    m_Controls[L"__about_desc"] = {L"", ControlKind::SelectLabel, ti};
    y += 24;
  }

  y += 8;
  Rect(panel, L"__about_hr",
       kPadX, y, kCtrlW, 1, m_Palette.divider, 255, 0.0f, false, scrollId);
  m_Controls[L"__about_hr"] = {L"", ControlKind::SelectLabel, ti};
  y += 18;

  const auto addLinkCard = [&](const WidgetSettingsCatalog::AboutInfo::Link &link,
                               int x, int cardY, int width) {
    const std::wstring id = L"__about_link_" + link.service;
    Rect(panel, id, x, cardY, width, 38, m_Palette.controlFill, 255, 6.0f,
         true, scrollId);
    const int iconY = cardY +
        ((link.service == L"YouTube" || link.service == L"Discord") ? 10 : 7);
    const int iconX = x + (link.service == L"DeviantArt" ? 11 : 9);
    BrandIcon(panel, id + L"_icon", link.service, iconX, iconY, true, scrollId);
    // The icon moves to the visual right in RTL. Keep its 32px area plus a
    // gap out of the text layout, otherwise a right-to-left label can paint
    // underneath the icon.
    const int labelX = x + (IsRtlLayout() ? 49 : 39);
    const int labelW = width - (IsRtlLayout() ? 54 : 45);
    Txt(panel, id + L"_label", labelX, cardY + 12, labelW, 18,
        link.service, 11, m_Palette.text, 600, true, TEXT_ALIGN_LEFT_TOP, scrollId);
    const ControlInfo info{link.url, ControlKind::ExternalLink, ti};
    m_Controls[id] = info;
    m_Controls[id + L"_icon"] = info;
    m_Controls[id + L"_icon_p"] = info;
    m_Controls[id + L"_icon_circle"] = info;
    m_Controls[id + L"_icon_play"] = info;
    m_Controls[id + L"_label"] = info;
  };

  const auto addSection = [&](const wchar_t *title,
                              const std::vector<WidgetSettingsCatalog::AboutInfo::Link> &links) {
    std::vector<WidgetSettingsCatalog::AboutInfo::Link> valid;
    for (const auto &link : links)
      if (IsSafeExternalUrl(link.url)) valid.push_back(link);
    if (valid.empty()) return;
    Txt(panel, L"__about_" + std::wstring(title) + L"_heading", kPadX, y,
        kCtrlW, 20, title, 13, m_Palette.text, 700, false, TEXT_ALIGN_LEFT_TOP, scrollId);
    m_Controls[L"__about_" + std::wstring(title) + L"_heading"] =
        {L"", ControlKind::SelectLabel, ti};
    y += 26;
    for (size_t index = 0; index < valid.size(); ++index) {
      const int column = static_cast<int>(index % 2);
      const int row = static_cast<int>(index / 2);
      addLinkCard(valid[index], kPadX + column * 172, y + row * 46, 160);
    }
    y += static_cast<int>((valid.size() + 1) / 2) * 46 + 10;
  };

  addSection(Loc("settingspanel.about.support", L"Support").c_str(), about.supportLinks);
  addSection(Loc("settingspanel.about.social", L"Social").c_str(), about.socialLinks);
}

void SettingsPanel::SwitchTab(int tabIndex) {
  if (!m_Panel || !Widget::IsValid(m_Panel)) return;
  m_ActiveTab = tabIndex;

  for (auto &[id, info] : m_Controls) {
    if (info.tabIndex == -1) continue;
    Element *el = m_Panel->FindElementById(id);
    if (!el) continue;
    if (id == L"__settings_reset" || id == L"__settings_reset_label" ||
        id == L"__settings_hr2") {
      el->SetShow(tabIndex == 0);
      continue;
    }
    el->SetShow(info.tabIndex == tabIndex);
  }

  for (int ti = 0; ti < m_TotalTabs; ++ti) {
    const bool active = (ti == tabIndex);

    const std::wstring bgId = L"__tab_bg_" + std::to_wstring(ti);
    if (Element *el = m_Panel->FindElementById(bgId)) {
      COLORREF bgCol = active ? m_Palette.controlFill : m_Palette.sidebar;
      el->SetSolidColor(bgCol, 255);
      if (auto *shape = dynamic_cast<ShapeElement *>(el)) {
        shape->SetFill(bgCol, 255);
      }
    }

    const std::wstring indId = L"__tab_ind_" + std::to_wstring(ti);
    if (Element *el = m_Panel->FindElementById(indId))
      el->SetShow(active);

    const std::wstring icoId = L"__tab_ico_" + std::to_wstring(ti);
    if (Element *el = m_Panel->FindElementById(icoId))
      if (auto *te = dynamic_cast<TextElement *>(el))
        te->SetFontColor(active ? m_Palette.accent : m_Palette.tabInactive, 255);

    const std::wstring lblId = L"__tab_lbl_" + std::to_wstring(ti);
    if (Element *el = m_Panel->FindElementById(lblId))
      if (auto *te = dynamic_cast<TextElement *>(el)) {
        te->SetFontColor(active ? m_Palette.tabActive : m_Palette.tabInactive, 255);
        te->SetFontWeight(active ? 600 : 400);
      }
  }

  for (const wchar_t *id : {L"__hdr_bg",L"__hdr_title",L"__hdr_border",
                              L"__hdr_close",L"__sb_bg",L"__sb_border",
                              L"__sb_ver",L"__content_bg"})
    if (Element *el = m_Panel->FindElementById(id)) el->SetShow(true);

  if (tabIndex == m_WindowTabIndex) UpdateWindowTabVisuals();
  m_Panel->Redraw();
}

void SettingsPanel::ApplyPaletteToPanel() {
  if (!m_Panel || !Widget::IsValid(m_Panel)) return;

  wchar_t bgBuf[64];
  swprintf_s(bgBuf, L"rgba(%d,%d,%d,255)",
             GetRValue(m_Palette.background),
             GetGValue(m_Palette.background),
             GetBValue(m_Palette.background));
  {
    WidgetOptions &opts = const_cast<WidgetOptions &>(m_Panel->GetOptions());
    opts.backgroundColor = bgBuf;
    opts.color           = m_Palette.background;
    opts.bgAlpha         = 255;
  }
  m_Panel->Redraw();

  auto T = [&](const wchar_t *id, COLORREF c, int w = -1) {
    if (Element *el = m_Panel->FindElementById(id))
      if (auto *te = dynamic_cast<TextElement *>(el)) {
        te->SetFontColor(c, 255);
        if (w >= 0) te->SetFontWeight(w);
      }
  };

  auto S = [&](const wchar_t *id, COLORREF c) {
    if (Element *el = m_Panel->FindElementById(id)) {
      el->SetSolidColor(c, 255);
      if (auto *sh = dynamic_cast<ShapeElement *>(el))
        sh->SetFill(c, 255);
    }
  };

  T(L"__hdr_title",      m_Palette.text);
  T(L"__about_name",     m_Palette.accent);
  T(L"__about_ver",      m_Palette.label);
  T(L"__about_desc",     m_Palette.muted);
  T(L"__sb_ver",         m_Palette.muted);
  S(L"__hdr_bg",         m_Palette.sidebar);
  S(L"__hdr_border",     m_Palette.divider);
  S(L"__sb_bg",          m_Palette.sidebar);
  S(L"__sb_border",      m_Palette.sidebarBorder);
  S(L"__content_bg",     m_Palette.background);
  S(L"__about_hr",       m_Palette.divider);
  S(L"__settings_hr2",   m_Palette.divider);

  for (auto &[id, info] : m_Controls) {
    Element *el = m_Panel->FindElementById(id);
    if (!el) continue;

    const std::wstring &eid = id;
    auto lastUs = eid.rfind(L'_');
    const std::wstring role = (lastUs != std::wstring::npos) ? eid.substr(lastUs + 1) : eid;

    switch (info.kind) {
    case ControlKind::SelectLabel:
      if (id.rfind(L"__scroll_", 0) == 0) break;
      if (auto *te = dynamic_cast<TextElement *>(el)) {
        if (role == L"label")        te->SetFontColor(m_Palette.label, 255);
        else if (role == L"heading") te->SetFontColor(m_Palette.text,  255);
        else                         te->SetFontColor(m_Palette.text,  255);
      } else {
        el->SetSolidColor(m_Palette.divider, 255);
        if (auto *sh = dynamic_cast<ShapeElement *>(el)) sh->SetFill(m_Palette.divider, 255);
      }
      break;
    case ControlKind::ResetLabel:
      if (auto *te = dynamic_cast<TextElement *>(el)) te->SetFontColor(m_Palette.text, 255);
      break;
    case ControlKind::ResetButton:
      el->SetSolidColor(m_Palette.controlFill, 255);
      if (auto *sh = dynamic_cast<ShapeElement *>(el)) sh->SetFill(m_Palette.controlFill, 255);
      break;
    case ControlKind::Input:
      if (auto *ib = dynamic_cast<InputBoxElement *>(el)) {
        ib->SetFontColor(m_Palette.text, 255);
        ib->SetFillColor(m_Palette.controlFill, 255);
        ib->SetBorderColor(m_Palette.controlBorder, 255);
        ib->SetCaretColor(m_Palette.text, 255);
        ib->SetSelectionColor(m_Palette.accent, 128);
      }
      break;
    case ControlKind::PanelDropDown:
    case ControlKind::WindowZPos:
      if (auto *dd = dynamic_cast<DropDownElement *>(el)) {
        dd->m_FontColor        = m_Palette.text;
        dd->m_BackgroundColor  = m_Palette.controlFill;
        dd->m_BorderColor      = m_Palette.controlBorder;
        dd->m_PopupBackground  = m_Palette.controlFill;
        dd->m_PopupBorderColor = m_Palette.controlBorder;
        dd->m_PopupTextColor   = m_Palette.text;
      }
      break;
    case ControlKind::TogglePill:
    case ControlKind::WindowToggle:
      if (auto *ts = dynamic_cast<ToggleSwitchElement *>(el)) {
        ts->m_OnColor  = m_Palette.accent;
        ts->m_OffColor = m_Palette.toggleOff;
      }
      break;
    case ControlKind::SelectButton:
      el->SetSolidColor(m_Palette.controlFill, 255);
      if (auto *sh = dynamic_cast<ShapeElement *>(el)) sh->SetFill(m_Palette.controlFill, 255);
      break;
    case ControlKind::ExternalLink:
      if (id.find(L"_label") != std::wstring::npos) {
        if (auto *te = dynamic_cast<TextElement *>(el)) te->SetFontColor(m_Palette.text, 255);
      } else if (id.find(L"_icon") == std::wstring::npos) {
        el->SetSolidColor(m_Palette.controlFill, 255);
        if (auto *sh = dynamic_cast<ShapeElement *>(el)) sh->SetFill(m_Palette.controlFill, 255);
      }
      break;
    case ControlKind::BoundSwitch:
    case ControlKind::BoundSlider:
    default:
      if (auto *te = dynamic_cast<TextElement *>(el)) te->SetFontColor(m_Palette.text, 255);
      break;
    }
  }

  for (const wchar_t *lid : {L"__win_zpos_lbl", L"__win_opacity_lbl"})
    if (Element *el = m_Panel->FindElementById(lid))
      if (auto *te = dynamic_cast<TextElement *>(el)) te->SetFontColor(m_Palette.label, 255);

  if (Element *el = m_Panel->FindElementById(L"__win_opacity"))
    if (auto *ib = dynamic_cast<InputBoxElement *>(el)) {
      ib->SetFontColor(m_Palette.text, 255);
      ib->SetFillColor(m_Palette.controlFill, 255);
      ib->SetBorderColor(m_Palette.controlBorder, 255);
      ib->SetCaretColor(m_Palette.text, 255);
      ib->SetSelectionColor(m_Palette.accent, 128);
    }

  for (auto &[id, info] : m_Controls) {
    if (info.kind != ControlKind::Input && info.kind != ControlKind::WindowOpacity)
      continue;
    Element *el = m_Panel->FindElementById(id);
    if (!el) continue;
    if (auto *ib = dynamic_cast<InputBoxElement *>(el)) {
      ib->SetCaretColor(m_Palette.text, 255);
      ib->SetSelectionColor(m_Palette.accent, 128);
    }
  }

  for (auto &[id, info] : m_Controls) {
    if (info.kind != ControlKind::ColorSwatch)
      continue;
    Element *el = m_Panel->FindElementById(id);
    if (!el) continue;
    if (auto *cp = dynamic_cast<ColorPickerElement *>(el)) {
      cp->m_PopupBackground = m_Palette.controlFill;
      cp->m_PopupBackgroundAlpha = 255;
      cp->m_PopupAccentColor = m_Palette.accent;
      cp->m_PopupAccentAlpha = 255;
      cp->m_PopupBorderColor = m_Palette.divider;
      cp->m_PopupBorderAlpha = 255;
      cp->m_PopupInputBackground = m_Palette.controlFill;
      cp->m_PopupInputBackgroundAlpha = 255;
      cp->m_HasPopupInputBackground = true;
      cp->m_PopupInputColor = m_Palette.text;
      cp->m_PopupInputColorAlpha = 255;
      cp->m_HasPopupInputColor = true;
    }
  }

  for (int ti = 0; ti < m_TotalTabs; ++ti) {
    if (Element *el = m_Panel->FindElementById(ScrollId(ti))) {
      el->SetScrollbarColor(m_Palette.label, 140);
      el->SetScrollbarHoverColor(m_Palette.label, 210);
      el->SetScrollbarActiveColor(m_Palette.label, 255);
      el->SetSolidColor(m_Palette.background, 255);
      if (auto *sh = dynamic_cast<ShapeElement *>(el))
        sh->SetFill(m_Palette.background, 255);
    }
  }

  SwitchTab(m_ActiveTab);
}

void SettingsPanel::UpdateWindowTabVisuals() {
  if (!m_Target || !Widget::IsValid(m_Target) ||
      !m_Panel  || !Widget::IsValid(m_Panel)) return;
  const WidgetOptions &opts = m_Target->GetOptions();
  const BoolRow bools[] = {
    {L"__win_draggable",    L"", opts.draggable   },
    {L"__win_clickthrough", L"", opts.clickThrough},
    {L"__win_keeponscreen", L"", opts.keepOnScreen},
    {L"__win_snapedges",    L"", opts.snapEdges   },
  };
  for (const auto &b : bools)
    if (Element *el = m_Panel->FindElementById(b.id))
      if (auto *ts = dynamic_cast<ToggleSwitchElement *>(el))
        ts->SetChecked(b.value);
  if (Element *el = m_Panel->FindElementById(L"__win_zpos"))
    if (auto *dd = dynamic_cast<DropDownElement *>(el))
      dd->SetSelectedIndex(ZPosToIndex(opts.zPos));
  if (Element *el = m_Panel->FindElementById(L"__win_opacity"))
    if (auto *ib = dynamic_cast<InputBoxElement *>(el)) {
      const int pct = static_cast<int>(std::round(opts.windowOpacity / 255.0 * 100.0));
      wchar_t buf[8]; swprintf_s(buf, L"%d", pct); ib->SetText(buf);
    }
  m_Panel->Redraw();
}

void SettingsPanel::OnBeforeOpenDropDown(Widget *widget,
                                         DropDownElement *dropDown) {
  if (widget == m_Panel && dropDown && dropDown->GetId() == L"__win_zpos")
    UpdateWindowTabVisuals();
}

void SettingsPanel::OnElementHoverChanged(Widget *widget, Element *element,
                                         bool isHovered) {
  if (widget != m_Panel || !element) return;
  const std::wstring &id = element->GetId();
  if (id == L"__hdr_close") {
    COLORREF color = isHovered ? RGB(255, 90, 90) : RGB(239, 68, 68);
    element->SetSolidColor(color, 255);
    if (auto *shape = dynamic_cast<ShapeElement *>(element)) {
      shape->SetFill(color, 255);
    }
    m_Panel->Redraw();
  } else if (id == L"__settings_reset" || id == L"__settings_reset_label") {
    Element *btnEl = m_Panel->FindElementById(L"__settings_reset");
    Element *lblEl = m_Panel->FindElementById(L"__settings_reset_label");
    COLORREF bgCol = isHovered ? m_Palette.controlBorder : m_Palette.controlFill;
    COLORREF txtCol = isHovered ? m_Palette.tabActive : m_Palette.text;

    if (btnEl) {
      btnEl->SetSolidColor(bgCol, 255);
      if (auto *shape = dynamic_cast<ShapeElement *>(btnEl)) {
        shape->SetFill(bgCol, 255);
      }
    }
    if (lblEl) {
      if (auto *txt = dynamic_cast<TextElement *>(lblEl)) {
        txt->SetFontColor(txtCol, 255);
      }
    }
    m_Panel->Redraw();
  } else if (id.rfind(L"__about_link_", 0) == 0) {
    std::wstring cardId = id;
    const size_t suffix = cardId.find(L"_icon", 13);
    if (suffix != std::wstring::npos) cardId.resize(suffix);
    const size_t labelSuffix = cardId.find(L"_label", 13);
    if (labelSuffix != std::wstring::npos) cardId.resize(labelSuffix);
    if (Element *card = m_Panel->FindElementById(cardId)) {
      const COLORREF color = isHovered ? m_Palette.controlBorder : m_Palette.controlFill;
      card->SetSolidColor(color, 255);
      if (auto *shape = dynamic_cast<ShapeElement *>(card)) shape->SetFill(color, 255);
    }
    if (Element *label = m_Panel->FindElementById(cardId + L"_label"))
      if (auto *text = dynamic_cast<TextElement *>(label))
        text->SetFontColor(isHovered ? m_Palette.accent : m_Palette.text, 255);
    m_Panel->Redraw();
  } else if (id.rfind(L"__tab_", 0) == 0) {
    int tabIdx = -1;
    size_t lastUnderscore = id.rfind(L'_');
    if (lastUnderscore != std::wstring::npos) {
      try {
        tabIdx = std::stoi(id.substr(lastUnderscore + 1));
      } catch (...) {}
    }
    if (tabIdx >= 0 && tabIdx < m_TotalTabs && tabIdx != m_ActiveTab) {
      Element *bgEl  = m_Panel->FindElementById(L"__tab_bg_" + std::to_wstring(tabIdx));
      Element *lblEl = m_Panel->FindElementById(L"__tab_lbl_" + std::to_wstring(tabIdx));
      Element *icoEl = m_Panel->FindElementById(L"__tab_ico_" + std::to_wstring(tabIdx));

      COLORREF bgCol  = isHovered ? m_Palette.controlFill : m_Palette.sidebar;
      COLORREF lblCol = isHovered ? m_Palette.text        : m_Palette.tabInactive;
      COLORREF icoCol = isHovered ? m_Palette.text        : m_Palette.tabInactive;

      if (bgEl) {
        bgEl->SetSolidColor(bgCol, 255);
        if (auto *shape = dynamic_cast<ShapeElement *>(bgEl)) {
          shape->SetFill(bgCol, 255);
        }
      }
      if (lblEl) {
        if (auto *txt = dynamic_cast<TextElement *>(lblEl)) {
          txt->SetFontColor(lblCol, 255);
        }
      }
      if (icoEl) {
        if (auto *txt = dynamic_cast<TextElement *>(icoEl)) {
          txt->SetFontColor(icoCol, 255);
        }
      }
      m_Panel->Redraw();
    }
  }
}

SettingsPanel::~SettingsPanel() {
  auto it = std::find(s_Panels.begin(), s_Panels.end(), this);
  if (it != s_Panels.end()) s_Panels.erase(it);
  if (m_Panel) {
    m_Panel->SetInputSink(nullptr);
    Widget::RemoveWidget(m_Panel); delete m_Panel; m_Panel = nullptr;
  }
  if (m_Target && Widget::IsValid(m_Target) &&
      m_Target->GetInputSink() == this)
    m_Target->SetInputSink(nullptr);
}

void SettingsPanel::Close() {
  if (m_DeferClose) {
    if (m_Panel && Widget::IsValid(m_Panel)) {
      m_Panel->SetInputSink(nullptr); m_Panel->Hide();
    }
    if (m_Target && Widget::IsValid(m_Target) &&
        m_Target->GetInputSink() == this)
      m_Target->SetInputSink(nullptr);
    m_Target = nullptr;
    auto r = std::find(s_Panels.begin(), s_Panels.end(), this);
    if (r != s_Panels.end()) s_Panels.erase(r);
    if (std::find(s_PendingDestroy.begin(), s_PendingDestroy.end(), this) ==
        s_PendingDestroy.end())
      s_PendingDestroy.push_back(this);
    m_CloseRequested = true;
    return;
  }
  delete this;
}

void SettingsPanel::Commit(const std::wstring &sid, const std::wstring &val) {
  if (!m_Target || !Widget::IsValid(m_Target)) { Close(); return; }
  CommitWidgetSetting(m_Target, sid, val);
}
void SettingsPanel::ResetAll() {
  if (!m_Target || !Widget::IsValid(m_Target)) { Close(); return; }
  for (const WidgetSetting &s : m_Target->GetSettings().schema) {
    Commit(s.id, s.defaultValue);
    if (!IsAlive(this)) return;
  }
  UpdateAllVisuals();
  if (m_Panel && Widget::IsValid(m_Panel)) m_Panel->Redraw();
}

void SettingsPanel::UpdateRowVisuals(const WidgetSetting &setting) {
  if (!m_Panel || !Widget::IsValid(m_Panel) ||
      !m_Target || !Widget::IsValid(m_Target)) return;
  const std::wstring value = m_Target->GetSettings().ValueOrDefault(setting.id);

  int ti = 0;
  for (auto &[id, info] : m_Controls)
    if (info.settingId == setting.id && info.tabIndex >= 0)
      { ti = info.tabIndex; break; }

  if (setting.type == WidgetSettingType::Toggle) {
    const bool on = IsOn(value);
    if (!setting.binding.elementId.empty() && m_Target && Widget::IsValid(m_Target)) {
      if (auto *sw = dynamic_cast<ToggleSwitchElement *>(
              m_Target->FindElementById(setting.binding.elementId)))
        { sw->SetChecked(on); m_Target->Redraw(); return; }
      if (auto *cb = dynamic_cast<CheckBoxElement *>(
              m_Target->FindElementById(setting.binding.elementId)))
        { cb->SetChecked(on); m_Target->Redraw(); return; }
    }
    if (Element *el = m_Panel->FindElementById(CEId(ti, setting.id, L"pill")))
      if (auto *ts = dynamic_cast<ToggleSwitchElement *>(el))
        ts->SetChecked(on);
  } else if (setting.type == WidgetSettingType::Select) {
    if (Element *el = m_Panel->FindElementById(CEId(ti, setting.id, L"dropdown")))
      if (auto *dd = dynamic_cast<DropDownElement *>(el))
        { dd->SetSelectedIndex(OptIndex(setting.options, value)); return; }
    if (Element *el = m_Panel->FindElementById(CEId(ti, setting.id, L"btnlabel")))
      if (auto *te = dynamic_cast<TextElement *>(el)) te->SetText(value);
  } else if (setting.type == WidgetSettingType::Number ||
             setting.type == WidgetSettingType::Text) {
    if (!setting.binding.elementId.empty() && m_Target && Widget::IsValid(m_Target))
      if (auto *sl = dynamic_cast<SliderElement *>(
              m_Target->FindElementById(setting.binding.elementId)))
        { double p = sl->GetValue(); ParseDbl(value, p); sl->SetValue(p);
          m_Target->Redraw(); return; }
    if (Element *el = m_Panel->FindElementById(CEId(ti, setting.id, L"input")))
      if (auto *box = dynamic_cast<InputBoxElement *>(el)) box->SetText(value);
  } else if (setting.type == WidgetSettingType::Color) {
    if (Element *el = m_Panel->FindElementById(CEId(ti, setting.id, L"swatch")))
      if (auto *cp = dynamic_cast<ColorPickerElement *>(el)) {
        COLORREF c = RGB(0,0,0); BYTE a = 255;
        if (!value.empty() && ColorUtil::ParseRGBA(value, c, a)) cp->SetColor(c);
      }
  }
}

void SettingsPanel::UpdateAllVisuals() {
  if (!m_Target || !Widget::IsValid(m_Target)) return;
  for (const WidgetSetting &s : m_Target->GetSettings().schema)
    UpdateRowVisuals(s);
}

void SettingsPanel::OnElementMouseUp(Widget *widget, Element *element, int, int) {
  if (!widget || widget != m_Panel || !element) return;
  auto it = m_Controls.find(element->GetId());
  if (it == m_Controls.end()) return;
  const ControlInfo &info = it->second;
  if (info.kind == ControlKind::TabButton) {
    int idx = 0;
    try { idx = std::stoi(Utils::ToString(info.settingId)); } catch (...) {}
    SwitchTab(idx); return;
  }
  if (info.kind == ControlKind::CloseButton) { Close(); return; }
  if (info.kind == ControlKind::ResetButton ||
      info.kind == ControlKind::ResetLabel)  { ResetAll(); return; }
  if (info.kind == ControlKind::ExternalLink) {
    if (IsSafeExternalUrl(info.settingId))
      ShellExecuteW(nullptr, L"open", info.settingId.c_str(), nullptr, nullptr,
                    SW_SHOWNORMAL);
    return;
  }
  if (!m_Target || !Widget::IsValid(m_Target)) { Close(); return; }

  if (info.kind == ControlKind::WindowToggle) {
    if (auto *ts = dynamic_cast<ToggleSwitchElement *>(element)) {
      ts->Toggle();
      m_Panel->Redraw();
      const bool on = ts->IsChecked(); const std::wstring &sid = info.settingId;
      if      (sid == L"__win_draggable")    m_Target->SetDraggable(on);
      else if (sid == L"__win_clickthrough") m_Target->SetClickThrough(on);
      else if (sid == L"__win_keeponscreen") m_Target->SetKeepOnScreen(on);
      else if (sid == L"__win_snapedges")    m_Target->SetSnapEdges(on);
      Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
    }
    return;
  }
  if (info.kind == ControlKind::WindowZPos) {
    if (auto *dd = dynamic_cast<DropDownElement *>(element)) {
      const int index = dd->GetSelectedIndex();
      if (index >= 0 && index < kZPosCount) {
        m_Target->ChangeZPos(kZPosValues[index], false);
        Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
        UpdateWindowTabVisuals();
      }
    }
    return;
  }

  const WidgetSettingsCatalog &catalog = m_Target->GetSettings();
  const WidgetSetting *setting = catalog.Find(info.settingId);
  if (!setting) return;
  const std::wstring current = catalog.ValueOrDefault(setting->id);

  if (info.kind == ControlKind::BoundSwitch) {
    bool checked = false;
    if (auto *sw = dynamic_cast<ToggleSwitchElement *>(element)) {
      sw->Toggle();
      m_Panel->Redraw();
      checked = sw->IsChecked();
    } else if (auto *cb = dynamic_cast<CheckBoxElement *>(element)) {
      cb->Toggle();
      m_Panel->Redraw();
      checked = cb->GetState() == CheckBoxElement::State::Checked;
    }
    Commit(setting->id, checked ? L"true" : L"false");
  } else if (info.kind == ControlKind::BoundSlider) {
    if (auto *sl = dynamic_cast<SliderElement *>(element)) {
      Commit(setting->id, Utils::ToWString(
                 SliderElement::FormatValue(sl->GetSnappedValue())));
      if (!IsAlive(this)) return;
      UpdateRowVisuals(*setting);
    }
  } else if (info.kind == ControlKind::PanelDropDown) {
    if (auto *dd = dynamic_cast<DropDownElement *>(element)) {
      Commit(setting->id, dd->SelectedValue());
      if (!IsAlive(this)) return; UpdateRowVisuals(*setting);
    }
  } else if (info.kind == ControlKind::TogglePill) {
    Commit(setting->id, IsOn(current) ? L"false" : L"true");
    if (!IsAlive(this)) return; UpdateRowVisuals(*setting);
  } else if (info.kind == ControlKind::SelectButton ||
             info.kind == ControlKind::SelectLabel) {
    if (!setting || setting->options.empty()) return;
    size_t idx = 0;
    for (size_t i = 0; i < setting->options.size(); ++i)
      if (setting->options[i] == current) { idx = i; break; }
    Commit(setting->id, setting->options[(idx + 1) % setting->options.size()]);
    if (!IsAlive(this)) return; UpdateRowVisuals(*setting);
  } else { return; }

  if (m_Panel && Widget::IsValid(m_Panel)) m_Panel->Redraw();
}

void SettingsPanel::OnInputCommitted(Widget *widget, InputBoxElement *inputBox) {
  if (!widget || widget != m_Panel || !inputBox) return;
  m_DeferClose = true;
  auto it = m_Controls.find(inputBox->GetId());
  if (it != m_Controls.end()) {
    if (it->second.kind == ControlKind::WindowOpacity) {
      if (m_Target && Widget::IsValid(m_Target)) {
        double pct = 100.0; ParseDbl(inputBox->GetText(), pct);
        pct = std::max(0.0, std::min(100.0, pct));
        const BYTE b = static_cast<BYTE>(std::round(pct / 100.0 * 255.0));
        m_Target->SetWindowOpacity(b);
        Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
      }
    } else if (it->second.kind == ControlKind::Input) {
      if (m_Target && Widget::IsValid(m_Target)) {
        Commit(it->second.settingId, inputBox->GetText());
        if (!m_CloseRequested && Widget::IsValid(m_Target)) {
          const WidgetSetting *s =
              m_Target->GetSettings().Find(it->second.settingId);
          if (s) UpdateRowVisuals(*s);
          if (m_Panel && Widget::IsValid(m_Panel)) m_Panel->Redraw();
        }
      }
    }
  }
  m_DeferClose = false;
  FlushPending();
}

void SettingsPanel::OnColorCommitted(Widget *widget,
                                     ColorPickerElement *colorPicker) {
  if (!widget || widget != m_Panel || !colorPicker) return;
  auto it = m_Controls.find(colorPicker->GetId());
  if (it == m_Controls.end() || it->second.kind != ControlKind::ColorSwatch) return;
  m_DeferClose = true;
  if (m_Target && Widget::IsValid(m_Target))
    Commit(it->second.settingId, WidgetSettingColorToHex(colorPicker->GetColor()));
  m_DeferClose = false;
}
