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
#include "../shared/Logging.h"
#include "../shared/Settings.h"
#include "../shared/Utils.h"
#include "../Version.h"
#include "SettingsPanelTheme.h"
#include "WidgetSettings.h"

extern std::vector<Widget *> widgets;

std::vector<SettingsPanel *> SettingsPanel::s_Panels;
std::vector<SettingsPanel *> SettingsPanel::s_PendingDestroy;

// ─── Layout ──────────────────────────────────────────────────────────────────
namespace {

constexpr int kPanelWidth    = 300;
constexpr int kHeaderHeight  = 44;
constexpr int kTabBarHeight  = 36;
constexpr int kRowHeight     = 44;
constexpr int kFooterHeight  = 48;
constexpr int kControlX      = 150;
constexpr int kControlWidth  = 134;
constexpr int kWindowRows    = 7;
constexpr int kAboutContentH = 160;

// Tab button x positions for up to 6 tabs (evenly spaced)
int TabButtonX(int tabIndex, int totalTabs) {
  const int margin   = 14;
  const int usable   = kPanelWidth - margin * 2;
  const int spacing  = totalTabs > 1 ? usable / totalTabs : usable;
  return margin + tabIndex * spacing;
}

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

bool ParseSettingDouble(const std::wstring &v, double &out) {
  wchar_t *end = nullptr;
  double d = std::wcstod(v.c_str(), &end);
  if (end == v.c_str()) return false;
  out = d;
  return true;
}

std::wstring PanelElemId(int tabIndex, const std::wstring &settingId,
                         const wchar_t *role) {
  return L"__t" + std::to_wstring(tabIndex) + L"_" + settingId + L"_" + role;
}

bool IsToggleOn(const std::wstring &v) {
  return v == L"true" || v == L"1";
}

int IndexOfOption(const std::vector<std::wstring> &options,
                  const std::wstring &value) {
  for (size_t i = 0; i < options.size(); ++i)
    if (options[i] == value) return static_cast<int>(i);
  return -1;
}

struct BoolRow { const wchar_t *id; const wchar_t *label; bool value; };

} // namespace

// ─── Statics ─────────────────────────────────────────────────────────────────

Widget *SettingsPanel::OpenFor(Widget *target) {
  FlushPending();
  if (!target || !Widget::IsValid(target)) return nullptr;
  for (SettingsPanel *p : s_Panels) {
    if (p->m_Target == target && p->m_Panel && Widget::IsValid(p->m_Panel)) {
      SetForegroundWindow(p->m_Panel->GetWindow());
      return p->m_Panel;
    }
  }
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

// ─── Small helpers ────────────────────────────────────────────────────────────

static void AddLabel(Widget *panel, const std::wstring &id,
                     int x, int y, int w, int h,
                     const std::wstring &text, int fontSize,
                     COLORREF color, bool hand = false) {
  PropertyParser::TextOptions lo;
  lo.id = id; lo.x = x; lo.y = y; lo.width = w; lo.height = h;
  lo.text = text; lo.fontSize = fontSize; lo.fontColor = color;
  if (hand) lo.mouseEventCursorName = L"hand";
  panel->AddText(lo);
}

static void AddDivider(Widget *panel, const std::wstring &id,
                       int y, COLORREF color) {
  PropertyParser::ShapeOptions so;
  so.id = id; so.x = 10; so.y = y;
  so.width = kPanelWidth - 20; so.height = 1;
  so.hasSolidColor = true;
  so.solidColor = color; so.solidAlpha = 255;
  so.fillColor  = color; so.fillAlpha  = 255;
  panel->AddShape(so);
}

// ─── BuildPanel ──────────────────────────────────────────────────────────────

void SettingsPanel::BuildPanel(Widget *target) {
  m_Palette = ResolveTheme();

  const WidgetSettingsCatalog &catalog = target->GetSettings();
  const int numCustomTabs = static_cast<int>(catalog.tabs.size());
  const bool showWindow   = catalog.showWindowTab;

  // Compute tab indices
  m_WindowTabIndex = showWindow ? numCustomTabs : -1;
  m_AboutTabIndex  = numCustomTabs + (showWindow ? 1 : 0);
  m_TotalTabs      = m_AboutTabIndex + 1;

  // Hide tab bar when there is exactly 1 custom tab and no Window tab
  m_ShowTabBar = !(numCustomTabs <= 1 && !showWindow);

  // Default active tab
  m_ActiveTab = 0;
  if (numCustomTabs == 0 && showWindow)
    m_ActiveTab = m_WindowTabIndex;

  // Heights
  const int maxCustomRows = [&]() {
    int mx = 0;
    for (const auto &t : catalog.tabs)
      mx = std::max(mx, static_cast<int>(t.settings.size()));
    return mx;
  }();
  const int contentH = std::max({
    maxCustomRows  * kRowHeight,
    kWindowRows    * kRowHeight,
    kAboutContentH
  });
  const int tabBarH  = m_ShowTabBar ? kTabBarHeight : 0;
  const int startY   = kHeaderHeight + tabBarH;
  const int panelH   = kHeaderHeight + tabBarH + contentH + kFooterHeight;

  // Position
  RECT tr{};
  GetWindowRect(target->GetWindow(), &tr);
  const int screenW = GetSystemMetrics(SM_CXSCREEN);
  const int screenH = GetSystemMetrics(SM_CYSCREEN);
  int x = tr.right + 12;
  if (x + kPanelWidth > screenW)
    x = std::max(8, (int)(tr.left) - 12 - kPanelWidth);
  int y = tr.top;
  if (y + panelH > screenH) y = std::max(0, screenH - panelH - 8);

  WidgetOptions po;
  po.id = L""; po.x = x; po.y = y; po.width = kPanelWidth; po.height = panelH;
  po.m_WDefined = true; po.m_HDefined = true;
  po.minWidth = 200; po.minHeight = 96;
  wchar_t bgBuf[64];
  swprintf_s(bgBuf, L"rgba(%d,%d,%d,%d)",
             GetRValue(m_Palette.background), GetGValue(m_Palette.background),
             GetBValue(m_Palette.background), (int)m_Palette.bgAlpha);
  po.backgroundColor = bgBuf;
  po.color = m_Palette.background; po.bgAlpha = m_Palette.bgAlpha;
  po.draggable = true; po.resizable = false; po.keepOnScreen = true;
  po.snapEdges = false; po.showInToolbar = false;
  po.zPos = ZPOSITION_ONTOPMOST; po.show = false; po.windowOpacity = 255;

  Widget *panel = new Widget(po);
  if (!panel->Create()) { delete panel; return; }
  m_Panel = panel;
  panel->SetInputSink(this);
  panel->BeginUpdate();

  // ── Header ────────────────────────────────────────────────────────────────
  std::wstring headerTitle = catalog.panelTitle;
  if (headerTitle.empty()) {
    headerTitle = target->GetOptions().id;
    if (headerTitle.empty() || headerTitle == L"widget")
      headerTitle = target->GetTitle();
    if (headerTitle.empty()) headerTitle = L"Widget";
    headerTitle = L"Settings - " + headerTitle;
  }
  AddLabel(panel, L"__settings_title", 14, 12, kPanelWidth - 58, 20,
           headerTitle, 14, m_Palette.text);

  {
    PropertyParser::TextOptions to;
    to.id = L"__settings_close";
    to.x = kPanelWidth - 34; to.y = 10; to.width = 22; to.height = 22;
    to.text = L"\uE711"; to.fontFace = L"Segoe MDL2 Assets";
    to.fontSize = 10; to.fontColor = m_Palette.muted;
    to.mouseEventCursorName = L"hand";
    panel->AddText(to);
    m_Controls[to.id] = {L"", ControlKind::CloseButton, -1};
  }
  AddDivider(panel, L"__settings_hr", kHeaderHeight - 1, m_Palette.divider);

  // ── Tab strip (optional) ──────────────────────────────────────────────────
  if (m_ShowTabBar) {
    // Background
    {
      PropertyParser::ShapeOptions bg;
      bg.id = L"__tab_bar";
      bg.x = 0; bg.y = kHeaderHeight; bg.width = kPanelWidth; bg.height = kTabBarHeight;
      bg.hasSolidColor = true; bg.solidColor = m_Palette.tabBar; bg.solidAlpha = 255;
      bg.fillColor = m_Palette.tabBar; bg.fillAlpha = 255;
      panel->AddShape(bg);
    }

    // Build tab labels list
    std::vector<std::wstring> tabLabels;
    for (const auto &t : catalog.tabs)
      tabLabels.push_back(t.icon.empty() ? t.label : t.icon + L" " + t.label);
    if (showWindow) tabLabels.push_back(L"Window");
    tabLabels.push_back(L"About");

    for (int ti = 0; ti < m_TotalTabs; ++ti) {
      bool active = (ti == m_ActiveTab);
      PropertyParser::TextOptions to;
      to.id = L"__tab_" + std::to_wstring(ti);
      to.x = TabButtonX(ti, m_TotalTabs);
      to.y = kHeaderHeight + 8; to.width = 80; to.height = 20;
      to.text = tabLabels[ti]; to.fontSize = 11;
      to.fontColor = active ? m_Palette.tabActive : m_Palette.tabInactive;
      to.mouseEventCursorName = L"hand";
      panel->AddText(to);
      m_Controls[to.id] = {std::to_wstring(ti), ControlKind::TabButton, -1};
    }

    // Indicator
    {
      PropertyParser::ShapeOptions ind;
      ind.id = L"__tab_indicator";
      ind.x = TabButtonX(m_ActiveTab, m_TotalTabs);
      ind.y = kHeaderHeight + kTabBarHeight - 2;
      ind.width = 50; ind.height = 2;
      ind.hasSolidColor = true;
      ind.solidColor = m_Palette.accent; ind.solidAlpha = 255;
      ind.fillColor  = m_Palette.accent; ind.fillAlpha  = 255;
      panel->AddShape(ind);
    }
    AddDivider(panel, L"__tab_divider",
               kHeaderHeight + kTabBarHeight - 1, m_Palette.divider);
  }

  // ── Custom tabs ───────────────────────────────────────────────────────────
  for (int ti = 0; ti < numCustomTabs; ++ti)
    BuildCustomTab(panel, target, catalog.tabs[ti], ti, startY);

  // ── Window tab ────────────────────────────────────────────────────────────
  if (showWindow) BuildWindowTab(panel, target, m_WindowTabIndex, startY);

  // ── About tab ─────────────────────────────────────────────────────────────
  BuildAboutTab(panel, m_AboutTabIndex, startY);

  // ── Footer ────────────────────────────────────────────────────────────────
  const int footerY = panelH - kFooterHeight + 6;
  AddDivider(panel, L"__settings_hr2", footerY, m_Palette.divider);
  {
    PropertyParser::ShapeOptions btn;
    btn.id = L"__settings_reset";
    btn.x = 14; btn.y = footerY + 8; btn.width = 140; btn.height = 26;
    btn.solidColorRadius = 6; btn.hasSolidColor = true;
    btn.solidColor = m_Palette.controlFill; btn.solidAlpha = 255;
    btn.fillColor  = m_Palette.controlFill; btn.fillAlpha  = 255;
    btn.mouseEventCursorName = L"hand";
    panel->AddShape(btn);
    m_Controls[btn.id] = {L"", ControlKind::ResetButton, 0};
  }
  {
    PropertyParser::TextOptions bt;
    bt.id = L"__settings_reset_label";
    bt.x = 22; bt.y = footerY + 12; bt.width = 130; bt.height = 18;
    bt.text = L"Reset to defaults"; bt.fontSize = 12;
    bt.fontColor = m_Palette.text; bt.mouseEventCursorName = L"hand";
    panel->AddText(bt);
    m_Controls[bt.id] = {L"", ControlKind::ResetLabel, 0};
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

// ─── BuildCustomTab ───────────────────────────────────────────────────────────

int SettingsPanel::BuildCustomTab(Widget *panel, Widget *target,
                                  const WidgetSettingsTab &tab,
                                  int tabIndex, int startY) {
  int rowTop = startY;
  for (const WidgetSetting &setting : tab.settings) {
    const int controlY = rowTop + 9;
    const std::wstring value =
        target->GetSettings().ValueOrDefault(setting.id);

    // Row label
    {
      PropertyParser::TextOptions lo;
      lo.id = PanelElemId(tabIndex, setting.id, L"label");
      lo.x = 14; lo.y = controlY + 2;
      lo.width = kControlX - 24; lo.height = 22;
      lo.text = setting.label; lo.fontSize = 12; lo.fontColor = m_Palette.label;
      panel->AddText(lo);
      m_Controls[lo.id] = {setting.id, ControlKind::SelectLabel, tabIndex};
    }

    switch (setting.type) {
    case WidgetSettingType::Color: {
      PropertyParser::ColorPickerOptions co;
      co.id = PanelElemId(tabIndex, setting.id, L"swatch");
      co.x = kControlX; co.y = controlY; co.width = kControlWidth; co.height = 26;
      COLORREF color = RGB(0,0,0); BYTE alpha = 255;
      if (!value.empty()) ColorUtil::ParseRGBA(value, color, alpha);
      co.color = color; co.borderRadius = 6.0f; co.borderWidth = 1.0f;
      co.borderColor = m_Palette.controlBorder; co.borderAlpha = 255;
      co.popupBackground = m_Palette.controlFill; co.popupBackgroundAlpha = 255;
      co.popupAccentColor = m_Palette.accent;
      co.popupBorderColor = m_Palette.divider;
      co.showEyedropper = false; co.mouseEventCursorName = L"hand";
      panel->AddColorPicker(co);
      m_Controls[co.id] = {setting.id, ControlKind::ColorSwatch, tabIndex};
      break;
    }
    case WidgetSettingType::Number:
    case WidgetSettingType::Text: {
      Element *boundSlider = nullptr;
      if (setting.type == WidgetSettingType::Number &&
          !setting.binding.elementId.empty() &&
          setting.binding.property == L"value" &&
          m_Target && Widget::IsValid(m_Target))
        boundSlider = m_Target->FindElementById(setting.binding.elementId);

      if (auto *sl = dynamic_cast<SliderElement *>(boundSlider)) {
        double init = sl->m_MinValue;
        ParseSettingDouble(value, init); sl->SetValue(init); m_Target->Redraw();
        m_Controls[sl->GetId()] = {setting.id, ControlKind::BoundSlider, tabIndex};
        break;
      }
      PropertyParser::InputBoxOptions io;
      io.id = PanelElemId(tabIndex, setting.id, L"input");
      io.x = kControlX; io.y = controlY; io.width = kControlWidth; io.height = 26;
      io.text = value; io.fontSize = 12;
      io.fontColor = m_Palette.text; io.fontAlpha = 255;
      io.hasFillColor = true;
      io.fillColor = m_Palette.controlFill; io.fillAlpha = 255;
      io.borderWidth = 1.0f; io.borderRadius = 6.0f;
      io.borderColor = m_Palette.controlBorder; io.borderColorAlpha = 255;
      io.caretColor = m_Palette.text; io.selectionColor = m_Palette.accent;
      if (setting.type == WidgetSettingType::Number) io.inputType = InputType::Float;
      panel->AddInputBox(io);
      m_Controls[io.id] = {setting.id, ControlKind::Input, tabIndex};
      break;
    }
    case WidgetSettingType::Toggle: {
      const bool on = IsToggleOn(value);
      Element *bound = nullptr;
      if (!setting.binding.elementId.empty() &&
          setting.binding.property == L"checked" &&
          m_Target && Widget::IsValid(m_Target))
        bound = m_Target->FindElementById(setting.binding.elementId);

      if (auto *sw = dynamic_cast<ToggleSwitchElement *>(bound)) {
        sw->SetChecked(on); m_Target->Redraw();
        m_Controls[sw->GetId()] = {setting.id, ControlKind::BoundSwitch, tabIndex};
        break;
      }
      if (auto *cb = dynamic_cast<CheckBoxElement *>(bound)) {
        cb->SetChecked(on); m_Target->Redraw();
        m_Controls[cb->GetId()] = {setting.id, ControlKind::BoundSwitch, tabIndex};
        break;
      }
      PropertyParser::ToggleSwitchOptions ts;
      ts.id = PanelElemId(tabIndex, setting.id, L"pill");
      ts.x = kControlX; ts.y = controlY + 2; ts.width = 44; ts.height = 22;
      ts.checked = on; ts.onColor = m_Palette.accent; ts.onAlpha = 255;
      ts.offColor = m_Palette.toggleOff; ts.offAlpha = 255;
      ts.borderRadius = -1.0f; ts.knobColor = RGB(245,245,248); ts.knobAlpha = 255;
      ts.knobPadding = 2.0f; ts.mouseEventCursorName = L"hand";
      panel->AddToggleSwitch(ts);
      m_Controls[ts.id] = {setting.id, ControlKind::TogglePill, tabIndex};
      break;
    }
    case WidgetSettingType::Select: {
      if (!setting.options.empty()) {
        PropertyParser::DropDownOptions dd;
        dd.id = PanelElemId(tabIndex, setting.id, L"dropdown");
        dd.x = kControlX; dd.y = controlY; dd.width = kControlWidth; dd.height = 26;
        for (const auto &opt : setting.options) dd.options.push_back({opt, opt});
        dd.hasOptions = true;
        dd.selectedIndex = IndexOfOption(setting.options, value);
        dd.backgroundColor = m_Palette.controlFill;
        dd.borderColor = m_Palette.controlBorder; dd.borderWidth = 1.0f;
        dd.borderRadius = 6.0f; dd.fontColor = m_Palette.text;
        dd.placeholderColor = m_Palette.muted;
        dd.popupBackground = m_Palette.controlFill;
        dd.popupBorderColor = m_Palette.controlBorder;
        dd.popupHoverColor  = m_Palette.controlBorder;
        dd.popupSelectedColor = m_Palette.accent;
        dd.popupTextColor = m_Palette.text; dd.mouseEventCursorName = L"hand";
        panel->AddDropDown(dd);
        m_Controls[dd.id] = {setting.id, ControlKind::PanelDropDown, tabIndex};
        break;
      }
      PropertyParser::ShapeOptions btn;
      btn.id = PanelElemId(tabIndex, setting.id, L"btn");
      btn.x = kControlX; btn.y = controlY; btn.width = kControlWidth; btn.height = 26;
      btn.solidColorRadius = 6; btn.hasSolidColor = true;
      btn.solidColor = m_Palette.controlFill; btn.solidAlpha = 255;
      btn.fillColor  = m_Palette.controlFill; btn.fillAlpha  = 255;
      btn.mouseEventCursorName = L"hand";
      panel->AddShape(btn);
      m_Controls[btn.id] = {setting.id, ControlKind::SelectButton, tabIndex};

      PropertyParser::TextOptions vt;
      vt.id = PanelElemId(tabIndex, setting.id, L"btnlabel");
      vt.x = kControlX + 8; vt.y = controlY + 4;
      vt.width = kControlWidth - 16; vt.height = 18;
      vt.text = value; vt.fontSize = 12; vt.fontColor = m_Palette.text;
      vt.mouseEventCursorName = L"hand";
      panel->AddText(vt);
      m_Controls[vt.id] = {setting.id, ControlKind::SelectLabel, tabIndex};
      break;
    }
    }
    rowTop += kRowHeight;
  }
  return rowTop;
}

// ─── BuildWindowTab ───────────────────────────────────────────────────────────

void SettingsPanel::BuildWindowTab(Widget *panel, Widget *target,
                                   int tabIndex, int startY) {
  if (!target || !Widget::IsValid(target)) return;
  const WidgetOptions &opts = target->GetOptions();

  const BoolRow toggleRows[] = {
    { L"__win_draggable",    L"Draggable",      opts.draggable    },
    { L"__win_clickthrough", L"Click-through",  opts.clickThrough },
    { L"__win_keeponscreen", L"Keep on screen", opts.keepOnScreen },
    { L"__win_snapedges",    L"Snap edges",     opts.snapEdges    },
    { L"__win_resizable",    L"Resizable",      opts.resizable    },
  };

  int rowTop = startY;
  for (const auto &r : toggleRows) {
    const int cy = rowTop + 9;
    AddLabel(panel, std::wstring(r.id) + L"_lbl",
             14, cy + 2, kControlX - 24, 22, r.label, 12, m_Palette.label);
    m_Controls[std::wstring(r.id) + L"_lbl"] = {r.id, ControlKind::SelectLabel, tabIndex};

    PropertyParser::ToggleSwitchOptions ts;
    ts.id = r.id;
    ts.x = kControlX; ts.y = cy + 2; ts.width = 44; ts.height = 22;
    ts.checked = r.value; ts.onColor = m_Palette.accent; ts.onAlpha = 255;
    ts.offColor = m_Palette.toggleOff; ts.offAlpha = 255;
    ts.borderRadius = -1.0f; ts.knobColor = RGB(245,245,248); ts.knobAlpha = 255;
    ts.knobPadding = 2.0f; ts.mouseEventCursorName = L"hand";
    panel->AddToggleSwitch(ts);
    m_Controls[ts.id] = {r.id, ControlKind::WindowToggle, tabIndex};
    rowTop += kRowHeight;
  }

  // Z-position
  {
    const int cy = rowTop + 9;
    AddLabel(panel, L"__win_zpos_lbl", 14, cy + 2, kControlX - 24, 22,
             L"Z-position", 12, m_Palette.label);
    m_Controls[L"__win_zpos_lbl"] = {L"__win_zpos", ControlKind::SelectLabel, tabIndex};

    PropertyParser::DropDownOptions dd;
    dd.id = L"__win_zpos";
    dd.x = kControlX; dd.y = cy; dd.width = kControlWidth; dd.height = 26;
    for (const auto &l : kZPosLabels) dd.options.push_back({l, l});
    dd.hasOptions = true; dd.selectedIndex = ZPosToIndex(opts.zPos);
    dd.backgroundColor = m_Palette.controlFill;
    dd.borderColor = m_Palette.controlBorder; dd.borderWidth = 1.0f; dd.borderRadius = 6.0f;
    dd.fontColor = m_Palette.text; dd.placeholderColor = m_Palette.muted;
    dd.popupBackground = m_Palette.controlFill; dd.popupBorderColor = m_Palette.controlBorder;
    dd.popupHoverColor = m_Palette.controlBorder; dd.popupSelectedColor = m_Palette.accent;
    dd.popupTextColor = m_Palette.text; dd.mouseEventCursorName = L"hand";
    panel->AddDropDown(dd);
    m_Controls[dd.id] = {L"__win_zpos", ControlKind::WindowZPos, tabIndex};
    rowTop += kRowHeight;
  }

  // Opacity
  {
    const int cy = rowTop + 9;
    AddLabel(panel, L"__win_opacity_lbl", 14, cy + 2, kControlX - 24, 22,
             L"Opacity (0-100)", 12, m_Palette.label);
    m_Controls[L"__win_opacity_lbl"] = {L"__win_opacity", ControlKind::SelectLabel, tabIndex};

    const int pct = static_cast<int>(std::round(opts.windowOpacity / 255.0 * 100.0));
    wchar_t buf[8]; swprintf_s(buf, L"%d", pct);

    PropertyParser::InputBoxOptions io;
    io.id = L"__win_opacity";
    io.x = kControlX; io.y = cy; io.width = kControlWidth; io.height = 26;
    io.text = buf; io.fontSize = 12;
    io.fontColor = m_Palette.text; io.fontAlpha = 255;
    io.hasFillColor = true; io.fillColor = m_Palette.controlFill; io.fillAlpha = 255;
    io.borderWidth = 1.0f; io.borderRadius = 6.0f;
    io.borderColor = m_Palette.controlBorder; io.borderColorAlpha = 255;
    io.caretColor = m_Palette.text; io.selectionColor = m_Palette.accent;
    io.inputType = InputType::Float;
    panel->AddInputBox(io);
    m_Controls[io.id] = {L"__win_opacity", ControlKind::WindowOpacity, tabIndex};
  }
}

// ─── BuildAboutTab ────────────────────────────────────────────────────────────

void SettingsPanel::BuildAboutTab(Widget *panel, int tabIndex, int startY) {
  const WidgetSettingsCatalog &catalog =
      m_Target ? m_Target->GetSettings() : WidgetSettingsCatalog{};
  const auto &about = catalog.about;

  int y = startY;

  // App / widget name
  std::wstring nameText = about.hasName ? about.name
      : (m_Target ? m_Target->GetOptions().id : L"Novadesk");
  if (nameText.empty()) nameText = L"Novadesk";
  AddLabel(panel, L"__about_name", 14, y + 12, 260, 26,
           nameText, 18, m_Palette.accent);
  m_Controls[L"__about_name"] = {L"", ControlKind::SelectLabel, tabIndex};

  // Version
  if (about.hasVersion) {
    AddLabel(panel, L"__about_ver", 14, y + 46, 260, 18,
             L"Version " + about.version, 12, m_Palette.label);
    m_Controls[L"__about_ver"] = {L"", ControlKind::SelectLabel, tabIndex};
  } else {
    AddLabel(panel, L"__about_ver", 14, y + 46, 260, 18,
             L"Version " NOVADESK_VERSION, 12, m_Palette.label);
    m_Controls[L"__about_ver"] = {L"", ControlKind::SelectLabel, tabIndex};
  }

  // Description
  if (about.hasDescription) {
    AddLabel(panel, L"__about_desc", 14, y + 68, 260, 18,
             about.description, 11, m_Palette.muted);
    m_Controls[L"__about_desc"] = {L"", ControlKind::SelectLabel, tabIndex};
  }

  AddDivider(panel, L"__about_hr", y + 96, m_Palette.divider);
  m_Controls[L"__about_hr"] = {L"", ControlKind::SelectLabel, tabIndex};

  // Theme
  AddLabel(panel, L"__about_theme_lbl", 14, y + 110, kControlX - 24, 22,
           L"Theme", 12, m_Palette.label);
  m_Controls[L"__about_theme_lbl"] = {L"", ControlKind::SelectLabel, tabIndex};

  {
    const std::string cur = Settings::GetGlobalString("theme", "system");
    int sel = 2;
    if (cur == "dark") sel = 0; else if (cur == "light") sel = 1;

    PropertyParser::DropDownOptions dd;
    dd.id = L"__about_theme";
    dd.x = kControlX; dd.y = y + 106; dd.width = kControlWidth; dd.height = 26;
    dd.options = {{L"Dark",L"dark"},{L"Light",L"light"},{L"System",L"system"}};
    dd.hasOptions = true; dd.selectedIndex = sel;
    dd.backgroundColor = m_Palette.controlFill;
    dd.borderColor = m_Palette.controlBorder; dd.borderWidth = 1.0f; dd.borderRadius = 6.0f;
    dd.fontColor = m_Palette.text; dd.placeholderColor = m_Palette.muted;
    dd.popupBackground = m_Palette.controlFill; dd.popupBorderColor = m_Palette.controlBorder;
    dd.popupHoverColor = m_Palette.controlBorder; dd.popupSelectedColor = m_Palette.accent;
    dd.popupTextColor = m_Palette.text; dd.mouseEventCursorName = L"hand";
    panel->AddDropDown(dd);
    m_Controls[dd.id] = {L"__about_theme", ControlKind::ThemeSelector, tabIndex};
  }
}

// ─── SwitchTab ────────────────────────────────────────────────────────────────

void SettingsPanel::SwitchTab(int tabIndex) {
  if (!m_Panel || !Widget::IsValid(m_Panel)) return;
  m_ActiveTab = tabIndex;

  for (auto &[id, info] : m_Controls) {
    if (info.tabIndex == -1) continue; // chrome always visible
    Element *el = m_Panel->FindElementById(id);
    if (!el) continue;

    // Footer: only on first custom tab (or window tab if no custom tabs)
    if (id == L"__settings_reset" || id == L"__settings_reset_label" ||
        id == L"__settings_hr2") {
      el->SetShow(tabIndex == 0 ||
                  (m_Target && m_Target->GetSettings().tabs.empty() &&
                   tabIndex == m_WindowTabIndex));
      continue;
    }
    el->SetShow(info.tabIndex == tabIndex);
  }

  // Always show chrome
  for (const wchar_t *id : {L"__settings_hr", L"__settings_title",
                              L"__tab_bar", L"__tab_divider"}) {
    if (Element *el = m_Panel->FindElementById(id)) el->SetShow(true);
  }

  // Move indicator
  if (m_ShowTabBar) {
    if (Element *ind = m_Panel->FindElementById(L"__tab_indicator"))
      ind->SetPosition(TabButtonX(tabIndex, m_TotalTabs), ind->GetY());

    // Update tab label colors
    for (int ti = 0; ti < m_TotalTabs; ++ti) {
      const std::wstring tabId = L"__tab_" + std::to_wstring(ti);
      if (Element *el = m_Panel->FindElementById(tabId))
        if (auto *te = dynamic_cast<TextElement *>(el))
          te->SetFontColor(ti == tabIndex ? m_Palette.tabActive
                                          : m_Palette.tabInactive, 255);
    }
  }

  if (tabIndex == m_WindowTabIndex) UpdateWindowTabVisuals();
  m_Panel->Redraw();
}

// ─── Palette application ──────────────────────────────────────────────────────

void SettingsPanel::ApplyPaletteToPanel() {
  if (!m_Panel || !Widget::IsValid(m_Panel)) return;
  wchar_t bgBuf[64];
  swprintf_s(bgBuf, L"rgba(%d,%d,%d,%d)",
             GetRValue(m_Palette.background), GetGValue(m_Palette.background),
             GetBValue(m_Palette.background), (int)m_Palette.bgAlpha);
  m_Panel->SetBackgroundColor(bgBuf);

  auto Txt = [&](const wchar_t *id, COLORREF c) {
    if (Element *el = m_Panel->FindElementById(id))
      if (auto *te = dynamic_cast<TextElement *>(el))
        te->SetFontColor(c, 255);
  };
  Txt(L"__settings_title", m_Palette.text);
  Txt(L"__settings_close", m_Palette.muted);
  Txt(L"__about_name",     m_Palette.accent);
  Txt(L"__about_ver",      m_Palette.label);
  Txt(L"__about_desc",     m_Palette.muted);

  auto Shp = [&](const wchar_t *id, COLORREF c) {
    if (Element *el = m_Panel->FindElementById(id))
      el->SetSolidColor(c, 255);
  };
  Shp(L"__settings_hr",   m_Palette.divider);
  Shp(L"__tab_bar",       m_Palette.tabBar);
  Shp(L"__tab_divider",   m_Palette.divider);
  Shp(L"__tab_indicator", m_Palette.accent);
  Shp(L"__settings_hr2",  m_Palette.divider);
  Shp(L"__about_hr",      m_Palette.divider);

  SwitchTab(m_ActiveTab);
}

// ─── UpdateWindowTabVisuals ───────────────────────────────────────────────────

void SettingsPanel::UpdateWindowTabVisuals() {
  if (!m_Target || !Widget::IsValid(m_Target) ||
      !m_Panel  || !Widget::IsValid(m_Panel)) return;

  const WidgetOptions &opts = m_Target->GetOptions();
  const BoolRow bools[] = {
    { L"__win_draggable",    L"", opts.draggable    },
    { L"__win_clickthrough", L"", opts.clickThrough },
    { L"__win_keeponscreen", L"", opts.keepOnScreen },
    { L"__win_snapedges",    L"", opts.snapEdges    },
    { L"__win_resizable",    L"", opts.resizable    },
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

  // Find which tab owns this setting
  int tabIndex = 0;
  for (auto &[id, info] : m_Controls) {
    if (info.settingId == setting.id && info.tabIndex >= 0) {
      tabIndex = info.tabIndex; break;
    }
  }

  if (setting.type == WidgetSettingType::Toggle) {
    const bool on = IsToggleOn(value);
    if (!setting.binding.elementId.empty() && m_Target && Widget::IsValid(m_Target)) {
      if (auto *sw = dynamic_cast<ToggleSwitchElement *>(
              m_Target->FindElementById(setting.binding.elementId)))
        { sw->SetChecked(on); m_Target->Redraw(); return; }
      if (auto *cb = dynamic_cast<CheckBoxElement *>(
              m_Target->FindElementById(setting.binding.elementId)))
        { cb->SetChecked(on); m_Target->Redraw(); return; }
    }
    if (Element *el = m_Panel->FindElementById(
            PanelElemId(tabIndex, setting.id, L"pill")))
      if (auto *ts = dynamic_cast<ToggleSwitchElement *>(el))
        ts->SetChecked(on);

  } else if (setting.type == WidgetSettingType::Select) {
    if (Element *el = m_Panel->FindElementById(
            PanelElemId(tabIndex, setting.id, L"dropdown")))
      if (auto *dd = dynamic_cast<DropDownElement *>(el))
        { dd->SetSelectedIndex(IndexOfOption(setting.options, value)); return; }
    if (Element *el = m_Panel->FindElementById(
            PanelElemId(tabIndex, setting.id, L"btnlabel")))
      if (auto *te = dynamic_cast<TextElement *>(el)) te->SetText(value);

  } else if (setting.type == WidgetSettingType::Number ||
             setting.type == WidgetSettingType::Text) {
    if (!setting.binding.elementId.empty() && m_Target && Widget::IsValid(m_Target))
      if (auto *sl = dynamic_cast<SliderElement *>(
              m_Target->FindElementById(setting.binding.elementId))) {
        double p = sl->GetValue(); ParseSettingDouble(value, p);
        sl->SetValue(p); m_Target->Redraw(); return;
      }
    if (Element *el = m_Panel->FindElementById(
            PanelElemId(tabIndex, setting.id, L"input")))
      if (auto *box = dynamic_cast<InputBoxElement *>(el)) box->SetText(value);

  } else if (setting.type == WidgetSettingType::Color) {
    if (Element *el = m_Panel->FindElementById(
            PanelElemId(tabIndex, setting.id, L"swatch")))
      if (auto *picker = dynamic_cast<ColorPickerElement *>(el)) {
        COLORREF c = RGB(0,0,0); BYTE a = 255;
        if (!value.empty() && ColorUtil::ParseRGBA(value, c, a)) picker->SetColor(c);
      }
  }
}

void SettingsPanel::UpdateAllVisuals() {
  if (!m_Target || !Widget::IsValid(m_Target)) return;
  for (const WidgetSetting &s : m_Target->GetSettings().schema)
    UpdateRowVisuals(s);
}

// ─── Input sink ───────────────────────────────────────────────────────────────

void SettingsPanel::OnElementMouseUp(Widget *widget, Element *element,
                                     int, int) {
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

  // Window toggles
  if (info.kind == ControlKind::WindowToggle) {
    if (auto *ts = dynamic_cast<ToggleSwitchElement *>(element)) {
      const bool on = ts->IsChecked();
      const std::wstring &sid = info.settingId;
      if      (sid == L"__win_draggable")    m_Target->SetDraggable(on);
      else if (sid == L"__win_clickthrough") m_Target->SetClickThrough(on);
      else if (sid == L"__win_keeponscreen") m_Target->SetKeepOnScreen(on);
      else if (sid == L"__win_snapedges")    m_Target->SetSnapEdges(on);
      else if (sid == L"__win_resizable")    m_Target->SetResizable(on);
      Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
    }
    return;
  }

  // Window z-pos
  if (info.kind == ControlKind::WindowZPos) {
    if (auto *dd = dynamic_cast<DropDownElement *>(element)) {
      const int idx = dd->GetSelectedIndex();
      if (idx >= 0 && idx < kZPosCount) {
        m_Target->ChangeZPos(kZPosValues[idx]);
        Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
      }
    }
    return;
  }

  // Theme selector
  if (info.kind == ControlKind::ThemeSelector) {
    if (auto *dd = dynamic_cast<DropDownElement *>(element)) {
      Settings::SetGlobalString("theme", Utils::ToString(dd->SelectedValue()));
      Settings::Flush();
      m_Palette = ResolveTheme();
      ApplyPaletteToPanel();
    }
    return;
  }

  // Script-settings rows
  const WidgetSettingsCatalog &catalog = m_Target->GetSettings();
  const WidgetSetting *setting = catalog.Find(info.settingId);
  if (!setting) return;
  const std::wstring current = catalog.ValueOrDefault(setting->id);

  if (info.kind == ControlKind::BoundSwitch) {
    bool checked = false;
    if (auto *sw = dynamic_cast<ToggleSwitchElement *>(element))
      checked = sw->IsChecked();
    else if (auto *cb = dynamic_cast<CheckBoxElement *>(element))
      checked = cb->GetState() == CheckBoxElement::State::Checked;
    Commit(setting->id, checked ? L"true" : L"false");

  } else if (info.kind == ControlKind::BoundSlider) {
    if (auto *sl = dynamic_cast<SliderElement *>(element)) {
      Commit(setting->id,
             Utils::ToWString(SliderElement::FormatValue(sl->GetSnappedValue())));
      if (!IsAlive(this)) return;
      UpdateRowVisuals(*setting);
    }
  } else if (info.kind == ControlKind::PanelDropDown) {
    if (auto *dd = dynamic_cast<DropDownElement *>(element)) {
      Commit(setting->id, dd->SelectedValue());
      if (!IsAlive(this)) return;
      UpdateRowVisuals(*setting);
    }
  } else if (info.kind == ControlKind::TogglePill) {
    Commit(setting->id, IsToggleOn(current) ? L"false" : L"true");
    if (!IsAlive(this)) return;
    UpdateRowVisuals(*setting);
  } else if (info.kind == ControlKind::SelectButton ||
             info.kind == ControlKind::SelectLabel) {
    if (!setting || setting->options.empty()) return;
    size_t idx = 0;
    for (size_t i = 0; i < setting->options.size(); ++i)
      if (setting->options[i] == current) { idx = i; break; }
    Commit(setting->id,
           setting->options[(idx + 1) % setting->options.size()]);
    if (!IsAlive(this)) return;
    UpdateRowVisuals(*setting);
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
        double pct = 100.0;
        ParseSettingDouble(inputBox->GetText(), pct);
        pct = std::max(0.0, std::min(100.0, pct));
        const BYTE b = static_cast<BYTE>(std::round(pct / 100.0 * 255.0));
        m_Target->SetWindowOpacity(b);
        Settings::SaveWidget(m_Target->GetOptions().id, m_Target->GetOptions());
      }
    } else if (it->second.kind == ControlKind::Input) {
      auto jt = m_Controls.find(inputBox->GetId());
      if (jt != m_Controls.end() && m_Target && Widget::IsValid(m_Target)) {
        Commit(jt->second.settingId, inputBox->GetText());
        if (!m_CloseRequested) {
          if (Widget::IsValid(m_Target)) {
            const WidgetSetting *s =
                m_Target->GetSettings().Find(jt->second.settingId);
            if (s) UpdateRowVisuals(*s);
          }
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
    Commit(it->second.settingId,
           WidgetSettingColorToHex(colorPicker->GetColor()));
  m_DeferClose = false;
}
