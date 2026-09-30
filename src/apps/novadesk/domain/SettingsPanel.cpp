/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "SettingsPanel.h"

#include <algorithm>
#include <windows.h>

#include "../scripting/quickjs/parser/PropertyParser.h"
#include "../shared/ColorUtil.h"
#include "../shared/Logging.h"
#include "../shared/Utils.h"
#include "WidgetSettings.h"

extern std::vector<Widget *> widgets;

std::vector<SettingsPanel *> SettingsPanel::s_Panels;
std::vector<SettingsPanel *> SettingsPanel::s_PendingDestroy;

namespace {

constexpr int kPanelWidth = 300;
constexpr int kHeaderHeight = 44;
constexpr int kRowHeight = 44;
constexpr int kFooterHeight = 48;
constexpr int kControlX = 150;
constexpr int kControlWidth = 134;

const COLORREF kTextColor = RGB(235, 235, 240);
const COLORREF kLabelColor = RGB(190, 190, 200);
const COLORREF kMutedColor = RGB(150, 150, 160);
const COLORREF kAccentColor = RGB(74, 134, 232);
const COLORREF kControlFill = RGB(42, 42, 50);
const COLORREF kControlBorder = RGB(70, 70, 82);
const COLORREF kToggleOff = RGB(64, 64, 74);
const COLORREF kDivider = RGB(58, 58, 66);

std::wstring PanelElementId(const std::wstring &settingId,
                            const wchar_t *role) {
  return L"__settings_" + settingId + L"_" + role;
}

bool IsToggleOn(const std::wstring &value) {
  return value == L"true" || value == L"1";
}

} // namespace

Widget *SettingsPanel::OpenFor(Widget *target) {
  FlushPending();
  if (!target || !Widget::IsValid(target))
    return nullptr;
  if (!target->GetSettings().hasSchema)
    return nullptr;

  for (SettingsPanel *panel : s_Panels) {
    if (panel->m_Target == target && panel->m_Panel &&
        Widget::IsValid(panel->m_Panel)) {
      SetForegroundWindow(panel->m_Panel->GetWindow());
      return panel->m_Panel;
    }
  }

  SettingsPanel *self = new SettingsPanel();
  self->m_Target = target;
  // Set before BuildPanel(): a failed build deletes the panel, and its
  // destructor must be able to restore the target's previous sink.
  target->SetInputSink(self);
  self->BuildPanel(target);
  if (!self->m_Panel) {
    delete self;
    return nullptr;
  }
  s_Panels.push_back(self);
  return self->m_Panel;
}

void SettingsPanel::BuildPanel(Widget *target) {
  WidgetSettingsCatalog &catalog = target->GetSettings();

  int rows = static_cast<int>(catalog.schema.size());
  if (rows > 24)
    rows = 24;
  const int height = kHeaderHeight + rows * kRowHeight + kFooterHeight;

  // Position next to the target window, flipping sides when clipped.
  RECT tr{};
  GetWindowRect(target->GetWindow(), &tr);
  const int screenW = GetSystemMetrics(SM_CXSCREEN);
  const int screenH = GetSystemMetrics(SM_CYSCREEN);
  int x = tr.right + 12;
  if (x + kPanelWidth > screenW)
    x = (tr.left - 12 - kPanelWidth) < 0 ? 8 : tr.left - 12 - kPanelWidth;
  int y = tr.top;
  if (y + height > screenH)
    y = (std::max)(0, screenH - height - 8);

  WidgetOptions po;
  po.id = L""; // Panels never persist and are skipped by dedup/load.
  po.x = x;
  po.y = y;
  po.width = kPanelWidth;
  po.height = height;
  po.m_WDefined = true;
  po.m_HDefined = true;
  po.minWidth = 200;
  po.minHeight = 96;
  po.backgroundColor = L"rgba(26,26,31,255)";
  po.color = RGB(26, 26, 31);
  po.bgAlpha = 255;
  po.draggable = true;
  po.resizable = false;
  po.keepOnScreen = true;
  po.snapEdges = false;
  po.showInToolbar = false;
  po.zPos = ZPOSITION_ONTOPMOST;
  po.show = false;
  po.windowOpacity = 255;

  Widget *panel = new Widget(po);
  if (!panel->Create()) {
    delete panel;
    return;
  }
  m_Panel = panel;
  panel->SetInputSink(this);
  // The target's own sink was set in OpenFor() before BuildPanel(), because
  // Create() below would otherwise make this widget look like a panel to
  // CloseAllForTarget (sink already installed at Create time).
  panel->BeginUpdate();

  std::wstring title = target->GetOptions().id;
  if (title.empty() || title == L"widget")
    title = target->GetTitle();
  if (title.empty())
    title = L"Widget";

  {
    PropertyParser::TextOptions to;
    to.id = L"__settings_title";
    to.x = 14;
    to.y = 12;
    to.width = kPanelWidth - 58;
    to.height = 20;
    to.text = L"Settings - " + title;
    to.fontSize = 14;
    to.fontColor = kTextColor;
    panel->AddText(to);

    to = PropertyParser::TextOptions{};
    to.id = L"__settings_close";
    to.x = kPanelWidth - 34;
    to.y = 12;
    to.width = 22;
    to.height = 20;
    to.text = L"X";
    to.fontSize = 14;
    to.fontColor = kMutedColor;
    to.mouseEventCursorName = L"hand";
    panel->AddText(to);
    m_Controls[to.id] = {L"", ControlKind::CloseButton};

    PropertyParser::ShapeOptions so;
    so.id = L"__settings_hr";
    so.x = 10;
    so.y = kHeaderHeight - 1;
    so.width = kPanelWidth - 20;
    so.height = 1;
    so.hasSolidColor = true;
    so.solidColor = kDivider;
    so.solidAlpha = 255;
    so.fillColor = kDivider;
    so.fillAlpha = 255;
    panel->AddShape(so);
  }

  int rowTop = kHeaderHeight;
  for (int i = 0; i < rows; ++i) {
    const WidgetSetting &setting = catalog.schema[i];
    m_Order.push_back(setting.id);
    const int controlY = rowTop + 9;

    {
      PropertyParser::TextOptions lo;
      lo.id = PanelElementId(setting.id, L"label");
      lo.x = 14;
      lo.y = controlY + 2;
      lo.width = kControlX - 24;
      lo.height = 22;
      lo.text = setting.label;
      lo.fontSize = 12;
      lo.fontColor = kLabelColor;
      panel->AddText(lo);
    }

    const std::wstring value = catalog.ValueOrDefault(setting.id);

    switch (setting.type) {
    case WidgetSettingType::Color: {
      PropertyParser::ColorPickerOptions co;
      co.id = PanelElementId(setting.id, L"swatch");
      co.x = kControlX;
      co.y = controlY;
      co.width = kControlWidth;
      co.height = 26;
      COLORREF color = RGB(0, 0, 0);
      BYTE alpha = 255;
      if (!value.empty())
        ColorUtil::ParseRGBA(value, color, alpha);
      co.color = color;
      co.borderRadius = 6.0f;
      co.borderWidth = 1.0f;
      co.borderColor = kControlBorder;
      co.borderAlpha = 255;
      co.popupBackground = RGB(32, 32, 38);
      co.popupBackgroundAlpha = 255;
      co.popupAccentColor = kAccentColor;
      co.popupBorderColor = kDivider;
      co.showEyedropper = false;
      co.mouseEventCursorName = L"hand";
      panel->AddColorPicker(co);
      m_Controls[co.id] = {setting.id, ControlKind::ColorSwatch};
      break;
    }
    case WidgetSettingType::Number:
    case WidgetSettingType::Text: {
      PropertyParser::InputBoxOptions io;
      io.id = PanelElementId(setting.id, L"input");
      io.x = kControlX;
      io.y = controlY;
      io.width = kControlWidth;
      io.height = 26;
      io.text = value;
      io.fontSize = 12;
      io.fontColor = kTextColor;
      io.fontAlpha = 255;
      io.hasFillColor = true;
      io.fillColor = RGB(34, 34, 40);
      io.fillAlpha = 255;
      io.borderWidth = 1.0f;
      io.borderRadius = 6.0f;
      io.borderColor = kControlBorder;
      io.borderColorAlpha = 255;
      io.caretColor = kTextColor;
      io.selectionColor = kAccentColor;
      if (setting.type == WidgetSettingType::Number)
        io.inputType = InputType::Float;
      panel->AddInputBox(io);
      m_Controls[io.id] = {setting.id, ControlKind::Input};
      break;
    }
    case WidgetSettingType::Toggle: {
      const bool on = IsToggleOn(value);
      // When the setting is bound to a real toggleSwitch element on the
      // target widget, drive that switch instead of drawing a pill+knob.
      Element *bound = nullptr;
      if (!setting.binding.elementId.empty() &&
          setting.binding.property == L"checked" && m_Target &&
          Widget::IsValid(m_Target)) {
        bound = m_Target->FindElementById(setting.binding.elementId);
      }
      if (auto *sw = dynamic_cast<ToggleSwitchElement *>(bound)) {
        sw->SetChecked(on, false);
        m_Target->Redraw();
        m_Controls[sw->GetId()] = {setting.id, ControlKind::BoundSwitch};
        break;
      }

      PropertyParser::ShapeOptions pill;
      pill.id = PanelElementId(setting.id, L"pill");
      pill.x = kControlX;
      pill.y = controlY + 2;
      pill.width = 44;
      pill.height = 22;
      pill.solidColorRadius = 11;
      pill.hasSolidColor = true;
      pill.solidColor = on ? kAccentColor : kToggleOff;
      pill.solidAlpha = 255;
      pill.fillColor = pill.solidColor;
      pill.fillAlpha = 255;
      pill.mouseEventCursorName = L"hand";
      panel->AddShape(pill);
      m_Controls[pill.id] = {setting.id, ControlKind::TogglePill};

      PropertyParser::ShapeOptions knob;
      knob.id = PanelElementId(setting.id, L"knob");
      knob.shapeType = L"ellipse";
      knob.x = on ? kControlX + 24 : kControlX + 2;
      knob.y = controlY + 4;
      knob.width = 18;
      knob.height = 18;
      knob.hasSolidColor = true;
      knob.solidColor = RGB(245, 245, 248);
      knob.solidAlpha = 255;
      knob.fillColor = knob.solidColor;
      knob.fillAlpha = 255;
      knob.mouseEventCursorName = L"hand";
      panel->AddShape(knob);
      m_Controls[knob.id] = {setting.id, ControlKind::ToggleKnob};
      break;
    }
    case WidgetSettingType::Select: {
      PropertyParser::ShapeOptions btn;
      btn.id = PanelElementId(setting.id, L"btn");
      btn.x = kControlX;
      btn.y = controlY;
      btn.width = kControlWidth;
      btn.height = 26;
      btn.solidColorRadius = 6;
      btn.hasSolidColor = true;
      btn.solidColor = kControlFill;
      btn.solidAlpha = 255;
      btn.fillColor = kControlFill;
      btn.fillAlpha = 255;
      btn.mouseEventCursorName = L"hand";
      panel->AddShape(btn);
      m_Controls[btn.id] = {setting.id, ControlKind::SelectButton};

      PropertyParser::TextOptions vt;
      vt.id = PanelElementId(setting.id, L"btnlabel");
      vt.x = kControlX + 8;
      vt.y = controlY + 4;
      vt.width = kControlWidth - 16;
      vt.height = 18;
      vt.text = value;
      vt.fontSize = 12;
      vt.fontColor = kTextColor;
      vt.mouseEventCursorName = L"hand";
      panel->AddText(vt);
      m_Controls[vt.id] = {setting.id, ControlKind::SelectLabel};
      break;
    }
    }

    rowTop += kRowHeight;
  }

  {
    PropertyParser::ShapeOptions hr;
    hr.id = L"__settings_hr2";
    hr.x = 10;
    hr.y = height - kFooterHeight + 6;
    hr.width = kPanelWidth - 20;
    hr.height = 1;
    hr.hasSolidColor = true;
    hr.solidColor = kDivider;
    hr.solidAlpha = 255;
    hr.fillColor = kDivider;
    hr.fillAlpha = 255;
    panel->AddShape(hr);

    PropertyParser::ShapeOptions btn;
    btn.id = L"__settings_reset";
    btn.x = 14;
    btn.y = height - kFooterHeight + 14;
    btn.width = 140;
    btn.height = 26;
    btn.solidColorRadius = 6;
    btn.hasSolidColor = true;
    btn.solidColor = kControlFill;
    btn.solidAlpha = 255;
    btn.fillColor = kControlFill;
    btn.fillAlpha = 255;
    btn.mouseEventCursorName = L"hand";
    panel->AddShape(btn);
    m_Controls[btn.id] = {L"", ControlKind::ResetButton};

    PropertyParser::TextOptions bt;
    bt.id = L"__settings_reset_label";
    bt.x = 22;
    bt.y = height - kFooterHeight + 18;
    bt.width = 130;
    bt.height = 18;
    bt.text = L"Reset to defaults";
    bt.fontSize = 12;
    bt.fontColor = kTextColor;
    bt.mouseEventCursorName = L"hand";
    panel->AddText(bt);
    m_Controls[bt.id] = {L"", ControlKind::ResetLabel};
  }

  {
    std::lock_guard<std::mutex> lock(Widget::s_WidgetMutex);
    widgets.push_back(panel);
    Widget::s_WidgetSet.insert(panel);
    if (panel->GetWindow())
      Widget::s_HwndMap[panel->GetWindow()] = panel;
  }

  panel->EndUpdate();
  panel->Show();
  if (panel->GetWindow())
    SetForegroundWindow(panel->GetWindow());
}

SettingsPanel::~SettingsPanel() {
  auto it = std::find(s_Panels.begin(), s_Panels.end(), this);
  if (it != s_Panels.end())
    s_Panels.erase(it);
  if (m_Panel) {
    m_Panel->SetInputSink(nullptr);
    Widget::RemoveWidget(m_Panel);
    delete m_Panel;
    m_Panel = nullptr;
  }
  // Only clear the target's sink if it still points at this panel: a second
  // panel registered later owns the routing and must not be orphaned.
  if (m_Target && Widget::IsValid(m_Target) &&
      m_Target->GetInputSink() == this)
    m_Target->SetInputSink(nullptr);
}

void SettingsPanel::Close() {
  // The color popup and the focused input box live inside the panel widget.
  // When a popup drag or a blur is still on the stack above this call, the
  // widget cannot be freed: orphan the panel now and let FlushPending()
  // destroy it at the next safe opportunity.
  if (m_DeferClose) {
    if (m_Panel && Widget::IsValid(m_Panel)) {
      m_Panel->SetInputSink(nullptr);
      m_Panel->Hide();
    }
    if (m_Target && Widget::IsValid(m_Target) &&
        m_Target->GetInputSink() == this)
      m_Target->SetInputSink(nullptr);
    m_Target = nullptr;
    auto registered = std::find(s_Panels.begin(), s_Panels.end(), this);
    if (registered != s_Panels.end())
      s_Panels.erase(registered);
    if (std::find(s_PendingDestroy.begin(), s_PendingDestroy.end(), this) ==
        s_PendingDestroy.end())
      s_PendingDestroy.push_back(this);
    m_CloseRequested = true;
    return;
  }
  delete this;
}

void SettingsPanel::FlushPending() {
  std::vector<SettingsPanel *> pending;
  pending.swap(s_PendingDestroy);
  for (SettingsPanel *panel : pending)
    delete panel;
}

bool SettingsPanel::IsAlive(SettingsPanel *panel) {
  return std::find(s_Panels.begin(), s_Panels.end(), panel) != s_Panels.end();
}

void SettingsPanel::CloseAllForTarget(Widget *target) {
  if (!target)
    return;
  std::vector<SettingsPanel *> copy = s_Panels;
  for (SettingsPanel *panel : copy) {
    if (panel->m_Target == target)
      panel->Close();
  }
  FlushPending();
}

void SettingsPanel::CloseAll() {
  std::vector<SettingsPanel *> copy = s_Panels;
  for (SettingsPanel *panel : copy)
    panel->Close();
  FlushPending();
}

void SettingsPanel::Commit(const std::wstring &settingId,
                           const std::wstring &value) {
  if (!m_Target || !Widget::IsValid(m_Target)) {
    Close();
    return;
  }
  CommitWidgetSetting(m_Target, settingId, value);
}

void SettingsPanel::ResetAll() {
  if (!m_Target || !Widget::IsValid(m_Target)) {
    Close();
    return;
  }
  WidgetSettingsCatalog &catalog = m_Target->GetSettings();
  for (const WidgetSetting &setting : catalog.schema) {
    Commit(setting.id, setting.defaultValue);
    // A settingchange handler can destroy the target, which closes this
    // panel; ~SettingsPanel removes it from the registry before freeing it.
    if (!IsAlive(this))
      return;
  }
  UpdateAllVisuals();
  if (m_Panel && Widget::IsValid(m_Panel))
    m_Panel->Redraw();
}

void SettingsPanel::UpdateRowVisuals(const WidgetSetting &setting) {
  if (!m_Panel || !Widget::IsValid(m_Panel) || !m_Target ||
      !Widget::IsValid(m_Target))
    return;

  const std::wstring value = m_Target->GetSettings().ValueOrDefault(setting.id);

  if (setting.type == WidgetSettingType::Toggle) {
    const bool on = IsToggleOn(value);
    auto bound = m_Controls.find(setting.binding.elementId);
    if (bound != m_Controls.end() &&
        bound->second.kind == ControlKind::BoundSwitch &&
        bound->second.settingId == setting.id && m_Target &&
        Widget::IsValid(m_Target)) {
      if (auto *sw = dynamic_cast<ToggleSwitchElement *>(
              m_Target->FindElementById(setting.binding.elementId))) {
        sw->SetChecked(on, false);
        m_Target->Redraw();
      }
      return;
    }
    if (Element *pill =
            m_Panel->FindElementById(PanelElementId(setting.id, L"pill"))) {
      pill->SetSolidColor(on ? kAccentColor : kToggleOff, 255);
    }
    if (Element *knob =
            m_Panel->FindElementById(PanelElementId(setting.id, L"knob"))) {
      knob->SetPosition(on ? kControlX + 24 : kControlX + 2, knob->GetY());
    }
  } else if (setting.type == WidgetSettingType::Select) {
    if (Element *label =
            m_Panel->FindElementById(PanelElementId(setting.id, L"btnlabel"))) {
      if (TextElement *text = dynamic_cast<TextElement *>(label))
        text->SetText(value);
    }
  } else if (setting.type == WidgetSettingType::Number ||
             setting.type == WidgetSettingType::Text) {
    if (Element *input =
            m_Panel->FindElementById(PanelElementId(setting.id, L"input"))) {
      if (InputBoxElement *box = dynamic_cast<InputBoxElement *>(input))
        box->SetText(value);
    }
  } else if (setting.type == WidgetSettingType::Color) {
    if (Element *swatch = m_Panel->FindElementById(
            PanelElementId(setting.id, L"swatch"))) {
      if (ColorPickerElement *picker =
              dynamic_cast<ColorPickerElement *>(swatch)) {
        COLORREF color = RGB(0, 0, 0);
        BYTE alpha = 255;
        if (!value.empty() && ColorUtil::ParseRGBA(value, color, alpha))
          picker->SetColor(color);
      }
    }
  }
}

void SettingsPanel::UpdateAllVisuals() {
  if (!m_Target || !Widget::IsValid(m_Target))
    return;
  WidgetSettingsCatalog &catalog = m_Target->GetSettings();
  for (const WidgetSetting &setting : catalog.schema)
    UpdateRowVisuals(setting);
}

void SettingsPanel::OnElementMouseUp(Widget *widget, Element *element, int,
                                     int) {
  if (!widget || widget != m_Panel || !element)
    return;

  auto it = m_Controls.find(element->GetId());
  if (it == m_Controls.end())
    return;

  if (it->second.kind == ControlKind::CloseButton) {
    Close();
    return;
  }
  if (it->second.kind == ControlKind::ResetButton ||
      it->second.kind == ControlKind::ResetLabel) {
    ResetAll();
    return;
  }

  if (!m_Target || !Widget::IsValid(m_Target)) {
    Close();
    return;
  }

  const WidgetSettingsCatalog &catalog = m_Target->GetSettings();
  const WidgetSetting *setting = catalog.Find(it->second.settingId);
  if (!setting)
    return;

  const std::wstring current = catalog.ValueOrDefault(setting->id);

  if (it->second.kind == ControlKind::BoundSwitch) {
    // Widget::ToggleToggleSwitch already flipped the switch and routed here;
    // commit the state it now shows.
    auto *sw = dynamic_cast<ToggleSwitchElement *>(element);
    Commit(setting->id, (sw && sw->IsChecked()) ? L"true" : L"false");
  } else if (it->second.kind == ControlKind::TogglePill ||
             it->second.kind == ControlKind::ToggleKnob) {
    Commit(setting->id, IsToggleOn(current) ? L"false" : L"true");
    if (!IsAlive(this))
      return;
    UpdateRowVisuals(*setting);
  } else if (it->second.kind == ControlKind::SelectButton ||
             it->second.kind == ControlKind::SelectLabel) {
    if (setting->options.empty())
      return;
    size_t index = 0;
    for (size_t i = 0; i < setting->options.size(); ++i) {
      if (setting->options[i] == current) {
        index = i;
        break;
      }
    }
    const std::wstring &next =
        setting->options[(index + 1) % setting->options.size()];
    Commit(setting->id, next);
    if (!IsAlive(this))
      return;
    UpdateRowVisuals(*setting);
  } else {
    return;
  }

  if (m_Panel && Widget::IsValid(m_Panel))
    m_Panel->Redraw();
}

void SettingsPanel::OnInputCommitted(Widget *widget,
                                     InputBoxElement *inputBox) {
  if (!widget || widget != m_Panel || !inputBox)
    return;
  m_DeferClose = true;
  CommitInput(inputBox);
  m_DeferClose = false;
  FlushPending();
}

void SettingsPanel::CommitInput(InputBoxElement *inputBox) {
  auto it = m_Controls.find(inputBox->GetId());
  if (it == m_Controls.end() || it->second.kind != ControlKind::Input)
    return;
  // The blur/Enter dispatch runs inside the panel window's own message
  // handling, so Close() is deferred until the sink callback returns.
  if (!m_Target || !Widget::IsValid(m_Target)) {
    Close();
    return;
  }

  const std::wstring settingId = it->second.settingId;
  Commit(settingId, inputBox->GetText());
  if (m_CloseRequested)
    return;
  if (Widget::IsValid(m_Target)) {
    const WidgetSetting *setting = m_Target->GetSettings().Find(settingId);
    if (setting)
      UpdateRowVisuals(*setting);
  }
  if (m_Panel && Widget::IsValid(m_Panel))
    m_Panel->Redraw();
}

void SettingsPanel::OnColorCommitted(Widget *widget,
                                     ColorPickerElement *colorPicker) {
  if (!widget || widget != m_Panel || !colorPicker)
    return;
  auto it = m_Controls.find(colorPicker->GetId());
  if (it == m_Controls.end() || it->second.kind != ControlKind::ColorSwatch)
    return;

  m_DeferClose = true;
  // The popup is owned by the panel widget; it cannot be freed while the
  // popup's own dispatch is on the stack, so an orphaned panel stays queued
  // until FlushPending() runs from a safe context.
  if (m_Target && Widget::IsValid(m_Target))
    Commit(it->second.settingId,
           WidgetSettingColorToHex(colorPicker->GetColor()));
  m_DeferClose = false;
}
