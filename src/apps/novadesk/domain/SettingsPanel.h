/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_SETTINGSPANEL_H__
#define __NOVADESK_SETTINGSPANEL_H__

#include <string>
#include <unordered_map>
#include <vector>

#include "Widget.h"
#include "SettingsPanelTheme.h"

/**
 * @brief Built-in per-widget settings window rendered with Novadesk elements.
 *
 * Tab layout (indices):
 *   0 … N-1   custom tabs from catalog.tabs
 *   N          Window tab (if catalog.showWindowTab)
 *   last       About tab (always)
 */
class SettingsPanel : public IWidgetInputSink {
public:
  static Widget *OpenFor(Widget *target);
  static void CloseAllForTarget(Widget *target);
  static void CloseAll();

  ~SettingsPanel() override;

  void OnElementMouseUp(Widget *, Element *, int, int) override;
  void OnInputCommitted(Widget *, InputBoxElement *) override;
  void OnColorCommitted(Widget *, ColorPickerElement *) override;

private:
  // ── Control kinds ─────────────────────────────────────────────────────────
  enum class ControlKind {
    // Script-settings rows
    TogglePill, BoundSwitch, BoundSlider, PanelDropDown,
    SelectButton, SelectLabel, Input, ColorSwatch,
    // Footer
    ResetButton, ResetLabel,
    // Chrome
    CloseButton, TabButton,
    // Window tab
    WindowToggle, WindowZPos, WindowOpacity,
    // About tab
    ThemeSelector,
  };

  struct ControlInfo {
    std::wstring settingId; ///< Setting id OR repurposed key (tab index, window key)
    ControlKind  kind;
    int          tabIndex = 0; ///< Owning tab index (-1 = always-visible chrome)
  };

  SettingsPanel() = default;

  void Close();
  void Commit(const std::wstring &settingId, const std::wstring &value);
  void CommitInput(InputBoxElement *inputBox);
  void ResetAll();
  void UpdateRowVisuals(const WidgetSetting &setting);
  void UpdateAllVisuals();
  void UpdateWindowTabVisuals();
  void BuildPanel(Widget *target);
  int  BuildCustomTab(Widget *panel, Widget *target,
                      const WidgetSettingsTab &tab, int tabIndex, int startY);
  void BuildWindowTab(Widget *panel, Widget *target, int tabIndex, int startY);
  void BuildAboutTab(Widget *panel, int tabIndex, int startY);
  void SwitchTab(int tabIndex);
  void ApplyPaletteToPanel();

  static bool IsAlive(SettingsPanel *panel);
  static void FlushPending();

  static std::vector<SettingsPanel *> s_Panels;
  static std::vector<SettingsPanel *> s_PendingDestroy;

  bool m_DeferClose     = false;
  bool m_CloseRequested = false;
  int  m_ActiveTab      = 0;   ///< Current tab index
  int  m_WindowTabIndex = -1;  ///< -1 when hidden
  int  m_AboutTabIndex  = 0;
  int  m_TotalTabs      = 1;
  bool m_ShowTabBar     = true; ///< False when only one custom tab & no window tab

  ThemePalette m_Palette;

  Widget *m_Panel  = nullptr;
  Widget *m_Target = nullptr;
  std::unordered_map<std::wstring, ControlInfo> m_Controls;
};

#endif
