/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "SettingsPanel.h"

#include <algorithm>
#include <cwchar>
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

// ─── Layout constants ─────────────────────────────────────────────────────────
namespace {

// Overall panel
constexpr int kPanelW  = 520;
constexpr int kPanelH  = 480;

// Header strip (title + close button) — spans full width
constexpr int kHeaderH = 48;

// Left sidebar (tab navigation)
constexpr int kSideW   = 140;   // sidebar width

// Right content pane
constexpr int kContentX = kSideW;                    // content starts here
constexpr int kContentW = kPanelW - kSideW;          // = 380
constexpr int kContentH = kPanelH - kHeaderH;        // = 432

// Sidebar tab button geometry
constexpr int kTabBtnH  = 40;    // height of each tab button
constexpr int kTabBtnX  = 0;     // starts at sidebar left edge
constexpr int kTabIndW  = 3;     // active-tab indicator bar width
constexpr int kTabPadX  = 16;    // icon/text horizontal padding

// Content area layout
constexpr int kPadX     = 20;    // horizontal content padding
constexpr int kRowH     = 44;    // height per inline settings row (label left, control right)
constexpr int kCtrlH    = 28;    // height of individual control (input/toggle/dropdown)
constexpr int kCtrlW    = kContentW - kPadX * 2;  // = 340
constexpr int kInlineLabelW = 180;
constexpr int kInlineCtrlW  = 140;

// Footer within content pane
constexpr int kFooterH  = 52;

// Window tab
constexpr int kWinRows  = 7;

// About tab minimum height
constexpr int kAboutH   = 180;

// Z-position options
const std::vector<std::wstring> kZPosLabels = {
  L"Normal", L"On Top", L"Always on Top", L"On Bottom", L"On Desktop"
};
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

// IDs for content elements (scoped per tab to avoid collisions)
std::wstring CEId(int ti, const std::wstring &sid, const wchar_t *role) {
  return L"__t" + std::to_wstring(ti) + L"_" + sid + L"_" + role;
}

bool IsOn(const std::wstring &v) { return v == L"true" || v == L"1"; }

int OptIndex(const std::vector<std::wstring> &opts, const std::wstring &v) {
  for (size_t i = 0; i < opts.size(); ++i) if (opts[i] == v) return (int)i;
  return -1;
}

struct BoolRow { const wchar_t *id; const wchar_t *label; bool value; };

} // anonymous namespace

// ─── Statics ─────────────────────────────────────────────────────────────────

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

// ─── Element creation helpers ─────────────────────────────────────────────────

static void Txt(Widget *panel, const std::wstring &id,
                int x, int y, int w, int h,
                const std::wstring &text, int size, COLORREF color,
                int weight = 400, bool hand = false,
                TextAlignment align = TEXT_ALIGN_LEFT_TOP) {
  PropertyParser::TextOptions o;
  o.id = id; o.x = x; o.y = y; o.width = w; o.height = h;
  o.text = text; o.fontSize = size; o.fontColor = color;
  o.fontWeight = weight;
  o.textAlign = align;
  if (hand) o.mouseEventCursorName = L"hand";
  panel->AddText(o);
}

static void Rect(Widget *panel, const std::wstring &id,
                 int x, int y, int w, int h,
                 COLORREF color, BYTE alpha = 255,
                 float radius = 0.0f, bool hand = false) {
  PropertyParser::ShapeOptions o;
  o.id = id; o.x = x; o.y = y; o.width = w; o.height = h;
  o.hasSolidColor = true;
  o.solidColor = color; o.solidAlpha = alpha;
  o.fillColor  = color; o.fillAlpha  = alpha;
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
                    COLORREF color, BYTE alpha = 255, bool hand = false) {
  PropertyParser::ShapeOptions o;
  o.id = id; o.x = x; o.y = y; o.width = w; o.height = h;
  o.shapeType = L"ellipse";
  o.hasSolidColor = true;
  o.solidColor = color; o.solidAlpha = alpha;
  o.fillColor  = color; o.fillAlpha  = alpha;
  if (hand) o.mouseEventCursorName = L"hand";
  panel->AddShape(o);
}

// ─── BuildPanel ───────────────────────────────────────────────────────────────

void SettingsPanel::BuildPanel(Widget *target) {
  m_Palette = ResolveTheme();

  const WidgetSettingsCatalog &cat = target->GetSettings();
  const int nCustom    = static_cast<int>(cat.tabs.size());
  const bool showWin   = cat.showWindowTab;

  m_WindowTabIndex = showWin ? nCustom : -1;
  m_AboutTabIndex  = nCustom + (showWin ? 1 : 0);
  m_TotalTabs      = m_AboutTabIndex + 1;
  m_ActiveTab      = (nCustom > 0) ? 0
                   : (showWin     ? m_WindowTabIndex
                                  : m_AboutTabIndex);

  // Position panel beside target
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
  // A non-empty scriptPath makes the widget treat every element as an action
  // target (sinkHandlesClicks=true), routing WM_LBUTTONUP to OnElementMouseUp
  // even for elements that have no JS callbacks (all settings panel controls).
  po.scriptPath = L"__settings_panel__";

  Widget *panel = new Widget(po);
  if (!panel->Create()) { delete panel; return; }
  m_Panel = panel;
  panel->SetInputSink(this);
  panel->BeginUpdate();

  // ── Header (full width) ───────────────────────────────────────────────────
  // Header background
  Rect(panel, L"__hdr_bg", 0, 0, kPanelW, kHeaderH, m_Palette.sidebar);

  // Title — left-aligned starting at kPadX, vertically centered with TEXT_ALIGN_LEFT_TOP
  std::wstring hdrTitle = cat.panelTitle;
  if (hdrTitle.empty()) {
    hdrTitle = target->GetOptions().id;
    if (hdrTitle.empty() || hdrTitle == L"widget") hdrTitle = target->GetTitle();
    if (hdrTitle.empty()) hdrTitle = L"Widget";
    hdrTitle = L"Settings \u2014 " + hdrTitle; // em dash
  }
  // FluentWidgets style: fixed y = 14 with standard TEXT_ALIGN_LEFT_TOP
  Txt(panel, L"__hdr_title", kPadX, 14, kPanelW - kPadX - 48,
      22, hdrTitle, 13, m_Palette.text, 600, false, TEXT_ALIGN_LEFT_TOP);

  // Close button — circular button like FluentWidgets (radius = size/2)
  {
    constexpr int kCloseSize = 16;
    const int closeX = kPanelW - kCloseSize - 16;
    const int closeY = (kHeaderH - kCloseSize) / 2;
    Rect(panel, L"__hdr_close", closeX, closeY, kCloseSize, kCloseSize,
         RGB(239, 68, 68), 255, static_cast<float>(kCloseSize) / 2.0f, true);
    m_Controls[L"__hdr_close"] = {L"", ControlKind::CloseButton, -1};
  }

  // Header bottom border
  Rect(panel, L"__hdr_border", 0, kHeaderH - 1, kPanelW, 1, m_Palette.divider);

  // ── Left sidebar ──────────────────────────────────────────────────────────
  // Sidebar background
  Rect(panel, L"__sb_bg", 0, kHeaderH, kSideW, kContentH, m_Palette.sidebar);

  // Sidebar right border (1px)
  Rect(panel, L"__sb_border", kSideW - 1, kHeaderH, 1, kContentH,
       m_Palette.sidebarBorder);

  // Build tab button labels
  std::vector<std::wstring> tabLabels, tabIcons;
  for (const auto &t : cat.tabs) {
    tabLabels.push_back(t.label);
    tabIcons.push_back(t.icon);
  }
  if (showWin) { tabLabels.push_back(L"Window");  tabIcons.push_back(L"\uE770"); }
  tabLabels.push_back(L"About"); tabIcons.push_back(L"\uE946");

  const int sideContentY = kHeaderH + 12; // top padding in sidebar

  for (int ti = 0; ti < m_TotalTabs; ++ti) {
    const int btnY = sideContentY + ti * kTabBtnH;
    const bool active = (ti == m_ActiveTab);

    // Active indicator bar (left edge, always built, hidden when inactive)
    {
      PropertyParser::ShapeOptions ind;
      ind.id = L"__tab_ind_" + std::to_wstring(ti);
      ind.x = 0; ind.y = btnY; ind.width = kTabIndW; ind.height = kTabBtnH;
      ind.solidColorRadius = 0;
      ind.hasSolidColor = true;
      ind.solidColor = m_Palette.accent; ind.solidAlpha = 255;
      ind.fillColor  = m_Palette.accent; ind.fillAlpha  = 255;
      panel->AddShape(ind);
      // indicator is always registered at tabIndex=-1 but we manage show manually
      m_Controls[ind.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }

    // Icon (Segoe MDL2) — shown at left side of button
    if (!tabIcons[ti].empty()) {
      PropertyParser::TextOptions ico;
      ico.id = L"__tab_ico_" + std::to_wstring(ti);
      ico.x = kTabPadX; ico.y = btnY; ico.width = 20; ico.height = kTabBtnH;
      ico.text = tabIcons[ti];
      ico.fontFace = L"Segoe MDL2 Assets";
      ico.fontSize = 11;
      ico.fontColor = active ? m_Palette.accent : m_Palette.tabInactive;
      ico.mouseEventCursorName = L"hand";
      panel->AddText(ico);
      m_Controls[ico.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }

    // Label text
    {
      PropertyParser::TextOptions lbl;
      lbl.id = L"__tab_lbl_" + std::to_wstring(ti);
      lbl.x = kTabPadX + (!tabIcons[ti].empty() ? 22 : 0);
      lbl.y = btnY; lbl.width = kSideW - kTabPadX - 8; lbl.height = kTabBtnH;
      lbl.text = tabLabels[ti]; lbl.fontSize = 12;
      lbl.fontWeight = active ? 600 : 400;
      lbl.fontColor = active ? m_Palette.tabActive : m_Palette.tabInactive;
      lbl.mouseEventCursorName = L"hand";
      panel->AddText(lbl);
      m_Controls[lbl.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }
  }

  // Sidebar bottom — version micro-text
  Txt(panel, L"__sb_ver",
      kTabPadX, kHeaderH + kContentH - 22, kSideW - kTabPadX, 18,
      Utils::ToWString(NOVADESK_VERSION), 9, m_Palette.muted);

  // ── Content area background ───────────────────────────────────────────────
  Rect(panel, L"__content_bg",
       kContentX, kHeaderH, kContentW, kContentH, m_Palette.background);

  // ── Build each tab content ────────────────────────────────────────────────
  const int contentY0 = kHeaderH + 16; // top padding in content area

  for (int ti = 0; ti < nCustom; ++ti)
    BuildCustomTab(panel, target, cat.tabs[ti], ti, contentY0);

  if (showWin) BuildWindowTab(panel, target, m_WindowTabIndex, contentY0);
  BuildAboutTab(panel, m_AboutTabIndex, contentY0);

  // ── Footer (Reset to defaults) — belongs to tab 0 ────────────────────────
  if (nCustom > 0) {
    const int footerY = kHeaderH + kContentH - kFooterH;
    const int resetW  = 150;
    const int resetH  = 28;
    // Center button horizontally within the content pane
    const int resetX  = kContentX + (kContentW - resetW) / 2;
    const int resetY  = footerY + (kFooterH - resetH) / 2;

    Rect(panel, L"__settings_hr2",
         kContentX, footerY, kContentW, 1, m_Palette.divider);
    m_Controls[L"__settings_hr2"] = {L"", ControlKind::ResetLabel, 0};

    Rect(panel, L"__settings_reset",
         resetX, resetY, resetW, resetH,
         m_Palette.controlFill, 255, 6.0f, true);
    m_Controls[L"__settings_reset"] = {L"", ControlKind::ResetButton, 0};

    // Use TEXT_ALIGN_LEFT_TOP with calculated offsets so text position is strictly deterministic
    // and does not get distorted by GetBounds offsets
    Txt(panel, L"__settings_reset_label",
        resetX + 22, resetY + 6, resetW - 44, resetH - 12,
        L"Reset to defaults", 12, m_Palette.text, 400, true, TEXT_ALIGN_LEFT_TOP);
    m_Controls[L"__settings_reset_label"] = {L"", ControlKind::ResetLabel, 0};
  }

  // Register widget
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

// ─── BuildCustomTab ──────────────────────────────────────────────────────────

void SettingsPanel::BuildCustomTab(Widget *panel, Widget *target,
                                   const WidgetSettingsTab &tab,
                                   int ti, int startY) {
  int y = startY;
  // Tab heading
  Txt(panel, CEId(ti, L"", L"heading"),
      kContentX + kPadX, y, kCtrlW, 22,
      tab.label, 14, m_Palette.text, 700);
  m_Controls[CEId(ti, L"", L"heading")] = {L"", ControlKind::SelectLabel, ti};
  y += 30;

  // Thin heading underline
  Rect(panel, CEId(ti, L"", L"hdivider"),
       kContentX + kPadX, y, kCtrlW, 1, m_Palette.divider);
  m_Controls[CEId(ti, L"", L"hdivider")] = {L"", ControlKind::SelectLabel, ti};
  y += 12;

  for (const WidgetSetting &setting : tab.settings) {
    const std::wstring value = target->GetSettings().ValueOrDefault(setting.id);

    // Row label (left-aligned, vertically centered with control)
    const int rowOff = (kRowH - kCtrlH) / 2; // = 8 when kRowH=44, kCtrlH=28
    Txt(panel, CEId(ti, setting.id, L"label"),
        kContentX + kPadX, y + rowOff, kInlineLabelW, kCtrlH,
        setting.label, 12, m_Palette.label, 400, false, TEXT_ALIGN_LEFT_CENTER);
    m_Controls[CEId(ti, setting.id, L"label")] =
        {setting.id, ControlKind::SelectLabel, ti};

    const int ctrlX = kContentX + kPadX + kCtrlW - kInlineCtrlW;
    const int ctrlY = y + rowOff;

    switch (setting.type) {
    case WidgetSettingType::Color: {
      PropertyParser::ColorPickerOptions co;
      co.id = CEId(ti, setting.id, L"swatch");
      co.x = kContentX + kPadX + kCtrlW - 60; co.y = ctrlY;
      co.width = 60; co.height = kCtrlH;
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
      io.x = ctrlX; io.y = ctrlY;
      io.width = kInlineCtrlW; io.height = kCtrlH;
      io.text = value; io.fontSize = 12;
      io.fontColor = m_Palette.text; io.fontAlpha = 255;
      io.hasFillColor = true;
      io.fillColor = m_Palette.controlFill; io.fillAlpha = 255;
      io.borderWidth = 1.0f; io.borderRadius = 7.0f;
      io.borderColor = m_Palette.controlBorder; io.borderColorAlpha = 255;
      io.caretColor = m_Palette.text; io.selectionColor = m_Palette.accent;
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
      ts.x = kContentX + kCtrlW - 46 + kPadX; ts.y = ctrlY;
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
        dd.x = ctrlX; dd.y = ctrlY;
        dd.width = kInlineCtrlW; dd.height = kCtrlH;
        for (const auto &opt : setting.options) dd.options.push_back({opt, opt});
        dd.hasOptions = true;
        dd.selectedIndex = OptIndex(setting.options, value);
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
      // Fallback button
      Rect(panel, CEId(ti, setting.id, L"btn"),
           ctrlX, ctrlY, kInlineCtrlW, kCtrlH,
           m_Palette.controlFill, 255, 7.0f, true);
      m_Controls[CEId(ti, setting.id, L"btn")] =
          {setting.id, ControlKind::SelectButton, ti};
      Txt(panel, CEId(ti, setting.id, L"btnlabel"),
          ctrlX + 8, ctrlY, kInlineCtrlW - 16, kCtrlH,
          value, 12, m_Palette.text, 400, true, TEXT_ALIGN_CENTER_CENTER);
      m_Controls[CEId(ti, setting.id, L"btnlabel")] =
          {setting.id, ControlKind::SelectLabel, ti};
      break;
    }
    }
    y += kRowH;
  }
}

// ─── BuildWindowTab ───────────────────────────────────────────────────────────

void SettingsPanel::BuildWindowTab(Widget *panel, Widget *target,
                                   int ti, int startY) {
  if (!target || !Widget::IsValid(target)) return;
  const WidgetOptions &opts = target->GetOptions();

  int y = startY;
  Txt(panel, CEId(ti, L"", L"heading"),
      kContentX + kPadX, y, kCtrlW, 22,
      L"Window Settings", 14, m_Palette.text, 700);
  m_Controls[CEId(ti, L"", L"heading")] = {L"", ControlKind::SelectLabel, ti};
  y += 30;
  Rect(panel, CEId(ti, L"", L"hdivider"),
       kContentX + kPadX, y, kCtrlW, 1, m_Palette.divider);
  m_Controls[CEId(ti, L"", L"hdivider")] = {L"", ControlKind::SelectLabel, ti};
  y += 16;

  const BoolRow toggleRows[] = {
    { L"__win_draggable",    L"Draggable",      opts.draggable    },
    { L"__win_clickthrough", L"Click-through",  opts.clickThrough },
    { L"__win_keeponscreen", L"Keep on screen", opts.keepOnScreen },
    { L"__win_snapedges",    L"Snap edges",     opts.snapEdges    },
    { L"__win_resizable",    L"Resizable",      opts.resizable    },
  };

  for (const auto &r : toggleRows) {
    // Label + toggle on the same row (toggle right-aligned)
    Txt(panel, std::wstring(r.id) + L"_lbl",
        kContentX + kPadX, y + 4, kCtrlW - 56, 24,
        r.label, 12, m_Palette.label, 400);
    m_Controls[std::wstring(r.id) + L"_lbl"] =
        {r.id, ControlKind::SelectLabel, ti};

    PropertyParser::ToggleSwitchOptions ts;
    ts.id = r.id;
    ts.x = kContentX + kCtrlW - 46 + kPadX; ts.y = y + 4;
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
    // Light separator between rows
    Rect(panel, std::wstring(r.id) + L"_sep",
         kContentX + kPadX, y, kCtrlW, 1, m_Palette.divider);
    m_Controls[std::wstring(r.id) + L"_sep"] = {L"", ControlKind::SelectLabel, ti};
    y += 8;
  }

  const int ctrlX  = kContentX + kPadX + kCtrlW - kInlineCtrlW;
  const int rowOff = (kRowH - kCtrlH) / 2;

  // Z-position (same row: label left, dropdown right)
  Txt(panel, L"__win_zpos_lbl",
      kContentX + kPadX, y + rowOff, kInlineLabelW, kCtrlH, L"Z-position", 12, m_Palette.label, 400, false, TEXT_ALIGN_LEFT_CENTER);
  m_Controls[L"__win_zpos_lbl"] = {L"", ControlKind::SelectLabel, ti};
  {
    PropertyParser::DropDownOptions dd;
    dd.id = L"__win_zpos";
    dd.x = ctrlX; dd.y = y + rowOff; dd.width = kInlineCtrlW; dd.height = kCtrlH;
    for (const auto &l : kZPosLabels) dd.options.push_back({l, l});
    dd.hasOptions = true; dd.selectedIndex = ZPosToIndex(opts.zPos);
    dd.backgroundColor    = m_Palette.controlFill;
    dd.borderColor        = m_Palette.controlBorder;
    dd.borderWidth = 1.0f; dd.borderRadius = 7.0f;
    dd.fontColor          = m_Palette.text; dd.placeholderColor = m_Palette.muted;
    dd.popupBackground    = m_Palette.controlFill;
    dd.popupBorderColor   = m_Palette.controlBorder;
    dd.popupHoverColor    = m_Palette.controlBorder;
    dd.popupSelectedColor = m_Palette.accent;
    dd.popupTextColor     = m_Palette.text; dd.mouseEventCursorName = L"hand";
    panel->AddDropDown(dd);
    m_Controls[dd.id] = {L"__win_zpos", ControlKind::WindowZPos, ti};
  }
  y += kRowH;

  // Opacity (same row: label left, input box right)
  Txt(panel, L"__win_opacity_lbl",
      kContentX + kPadX, y + rowOff, kInlineLabelW, kCtrlH, L"Opacity (0 \u2013 100)", 12, m_Palette.label, 400, false, TEXT_ALIGN_LEFT_CENTER);
  m_Controls[L"__win_opacity_lbl"] = {L"", ControlKind::SelectLabel, ti};
  {
    const int pct = static_cast<int>(std::round(opts.windowOpacity / 255.0 * 100.0));
    wchar_t buf[8]; swprintf_s(buf, L"%d", pct);
    PropertyParser::InputBoxOptions io;
    io.id = L"__win_opacity";
    io.x = ctrlX; io.y = y + rowOff; io.width = kInlineCtrlW; io.height = kCtrlH;
    io.text = buf; io.fontSize = 12;
    io.fontColor = m_Palette.text; io.fontAlpha = 255;
    io.hasFillColor = true; io.fillColor = m_Palette.controlFill; io.fillAlpha = 255;
    io.borderWidth = 1.0f; io.borderRadius = 7.0f;
    io.borderColor = m_Palette.controlBorder; io.borderColorAlpha = 255;
    io.caretColor = m_Palette.text; io.selectionColor = m_Palette.accent;
    io.inputType = InputType::Float;
    panel->AddInputBox(io);
    m_Controls[io.id] = {L"__win_opacity", ControlKind::WindowOpacity, ti};
  }
}

// ─── BuildAboutTab ────────────────────────────────────────────────────────────

void SettingsPanel::BuildAboutTab(Widget *panel, int ti, int startY) {
  const WidgetSettingsCatalog &cat =
      m_Target ? m_Target->GetSettings() : WidgetSettingsCatalog{};
  const auto &about = cat.about;

  int y = startY;

  // App / widget name (large accent)
  const std::wstring nameText =
      about.hasName ? about.name
      : (m_Target ? m_Target->GetOptions().id : L"Novadesk");
  Txt(panel, L"__about_name",
      kContentX + kPadX, y, kCtrlW, 32,
      nameText.empty() ? L"Novadesk" : nameText, 20, m_Palette.accent, 700);
  m_Controls[L"__about_name"] = {L"", ControlKind::SelectLabel, ti};
  y += 36;

  // Version
  const std::wstring verText = about.hasVersion
      ? (L"Version " + about.version)
      : (L"Version " NOVADESK_VERSION);
  Txt(panel, L"__about_ver",
      kContentX + kPadX, y, kCtrlW, 18, verText, 12, m_Palette.label);
  m_Controls[L"__about_ver"] = {L"", ControlKind::SelectLabel, ti};
  y += 22;

  // Description
  if (about.hasDescription) {
    Txt(panel, L"__about_desc",
        kContentX + kPadX, y, kCtrlW, 18,
        about.description, 11, m_Palette.muted);
    m_Controls[L"__about_desc"] = {L"", ControlKind::SelectLabel, ti};
    y += 24;
  }

  // Divider
  y += 8;
  Rect(panel, L"__about_hr",
       kContentX + kPadX, y, kCtrlW, 1, m_Palette.divider);
  m_Controls[L"__about_hr"] = {L"", ControlKind::SelectLabel, ti};
}

// ─── SwitchTab ────────────────────────────────────────────────────────────────

void SettingsPanel::SwitchTab(int tabIndex) {
  if (!m_Panel || !Widget::IsValid(m_Panel)) return;
  m_ActiveTab = tabIndex;

  // Show/hide content elements
  for (auto &[id, info] : m_Controls) {
    if (info.tabIndex == -1) continue; // sidebar chrome: manage separately
    Element *el = m_Panel->FindElementById(id);
    if (!el) continue;
    if (id == L"__settings_reset" || id == L"__settings_reset_label" ||
        id == L"__settings_hr2") {
      el->SetShow(tabIndex == 0);
      continue;
    }
    el->SetShow(info.tabIndex == tabIndex);
  }

  // Update sidebar tab buttons (active indicator, label weight/color, icon color)
  for (int ti = 0; ti < m_TotalTabs; ++ti) {
    const bool active = (ti == tabIndex);

    // Indicator bar
    const std::wstring indId = L"__tab_ind_" + std::to_wstring(ti);
    if (Element *el = m_Panel->FindElementById(indId))
      el->SetShow(active);

    // Icon
    const std::wstring icoId = L"__tab_ico_" + std::to_wstring(ti);
    if (Element *el = m_Panel->FindElementById(icoId))
      if (auto *te = dynamic_cast<TextElement *>(el))
        te->SetFontColor(active ? m_Palette.accent : m_Palette.tabInactive, 255);

    // Label
    const std::wstring lblId = L"__tab_lbl_" + std::to_wstring(ti);
    if (Element *el = m_Panel->FindElementById(lblId))
      if (auto *te = dynamic_cast<TextElement *>(el)) {
        te->SetFontColor(active ? m_Palette.tabActive : m_Palette.tabInactive, 255);
        te->SetFontWeight(active ? 600 : 400);
      }
  }

  // Always-visible structural elements
  for (const wchar_t *id : {L"__hdr_bg",L"__hdr_title",L"__hdr_border",
                              L"__hdr_close",L"__sb_bg",L"__sb_border",
                              L"__sb_ver",L"__content_bg"})
    if (Element *el = m_Panel->FindElementById(id)) el->SetShow(true);

  if (tabIndex == m_WindowTabIndex) UpdateWindowTabVisuals();
  m_Panel->Redraw();
}

// ─── ApplyPaletteToPanel ──────────────────────────────────────────────────────

void SettingsPanel::ApplyPaletteToPanel() {
  if (!m_Panel || !Widget::IsValid(m_Panel)) return;

  wchar_t bgBuf[64];
  swprintf_s(bgBuf, L"rgba(%d,%d,%d,255)",
             GetRValue(m_Palette.background),
             GetGValue(m_Palette.background),
             GetBValue(m_Palette.background));
  m_Panel->SetBackgroundColor(bgBuf);

  auto T = [&](const wchar_t *id, COLORREF c, int w = -1) {
    if (Element *el = m_Panel->FindElementById(id))
      if (auto *te = dynamic_cast<TextElement *>(el)) {
        te->SetFontColor(c, 255);
        if (w >= 0) te->SetFontWeight(w);
      }
  };
  auto S = [&](const wchar_t *id, COLORREF c) {
    if (Element *el = m_Panel->FindElementById(id)) el->SetSolidColor(c, 255);
  };

  T(L"__hdr_title",      m_Palette.text);
  T(L"__about_name",     m_Palette.accent);
  T(L"__about_ver",      m_Palette.label);
  T(L"__about_desc",     m_Palette.muted);
  S(L"__hdr_bg",         m_Palette.sidebar);
  S(L"__hdr_border",     m_Palette.divider);
  S(L"__sb_bg",          m_Palette.sidebar);
  S(L"__sb_border",      m_Palette.sidebarBorder);
  S(L"__content_bg",     m_Palette.background);
  S(L"__about_hr",       m_Palette.divider);
  S(L"__settings_hr2",   m_Palette.divider);

  SwitchTab(m_ActiveTab);
}

// ─── UpdateWindowTabVisuals ───────────────────────────────────────────────────

void SettingsPanel::UpdateWindowTabVisuals() {
  if (!m_Target || !Widget::IsValid(m_Target) ||
      !m_Panel  || !Widget::IsValid(m_Panel)) return;
  const WidgetOptions &opts = m_Target->GetOptions();
  const BoolRow bools[] = {
    {L"__win_draggable",    L"", opts.draggable   },
    {L"__win_clickthrough", L"", opts.clickThrough},
    {L"__win_keeponscreen", L"", opts.keepOnScreen},
    {L"__win_snapedges",    L"", opts.snapEdges   },
    {L"__win_resizable",    L"", opts.resizable   },
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
  if (widget == m_Panel && dropDown && dropDown->GetId() == L"__win_zpos") {
    UpdateWindowTabVisuals();
  }
}

// ─── Destructor / Close ───────────────────────────────────────────────────────

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

// ─── Commit helpers ───────────────────────────────────────────────────────────

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

// ─── Visual updates ───────────────────────────────────────────────────────────

void SettingsPanel::UpdateRowVisuals(const WidgetSetting &setting) {
  if (!m_Panel || !Widget::IsValid(m_Panel) ||
      !m_Target || !Widget::IsValid(m_Target)) return;
  const std::wstring value = m_Target->GetSettings().ValueOrDefault(setting.id);

  // Determine which tab index owns this setting
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

// ─── Input sink ───────────────────────────────────────────────────────────────

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
  if (!m_Target || !Widget::IsValid(m_Target)) { Close(); return; }

  if (info.kind == ControlKind::WindowToggle) {
    if (auto *ts = dynamic_cast<ToggleSwitchElement *>(element)) {
      // When scriptPath is non-empty (our sentinel), the widget skips
      // ToggleToggleSwitch() and routes straight here — the element has NOT
      // been flipped yet. Flip it manually, then apply to the target.
      ts->Toggle();
      m_Panel->Redraw();
      const bool on = ts->IsChecked(); const std::wstring &sid = info.settingId;
      if      (sid == L"__win_draggable")    m_Target->SetDraggable(on);
      else if (sid == L"__win_clickthrough") m_Target->SetClickThrough(on);
      else if (sid == L"__win_keeponscreen") m_Target->SetKeepOnScreen(on);
      else if (sid == L"__win_snapedges")    m_Target->SetSnapEdges(on);
      else if (sid == L"__win_resizable")    m_Target->SetResizable(on);
      Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
    }
    return;
  }
  if (info.kind == ControlKind::WindowZPos) {
    if (auto *dd = dynamic_cast<DropDownElement *>(element)) {
      const int idx = dd->GetSelectedIndex();
      if (idx >= 0 && idx < kZPosCount) {
        m_Target->ChangeZPos(kZPosValues[idx], false);
        if (m_Target && Widget::IsValid(m_Target)) {
          Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
          UpdateWindowTabVisuals();
        }
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
      // consumedBySink path: element was not flipped by the widget — flip it now
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
