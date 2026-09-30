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

/**
 * @brief Built-in per-widget settings window rendered with Novadesk elements.
 *
 * @note The panel is itself a scriptless Widget (empty ID, so it never
 *       persists) whose elements carry no JS callbacks; their input arrives
 *       through the IWidgetInputSink installed on the panel widget. Values are
 *       applied to the target widget declaratively and persisted via
 *       CommitWidgetSetting().
 */
class SettingsPanel : public IWidgetInputSink {
public:
  /// @brief Opens (or focuses) the settings panel for a target widget.
  /// @return The panel widget, or nullptr on failure.
  static Widget *OpenFor(Widget *target);
  /// @brief Closes every panel whose target is the given widget.
  static void CloseAllForTarget(Widget *target);
  /// @brief Closes all open panels (application shutdown).
  static void CloseAll();

  ~SettingsPanel() override;

  void OnElementMouseUp(Widget *widget, Element *element, int x,
                        int y) override;
  void OnInputCommitted(Widget *widget, InputBoxElement *inputBox) override;
  void OnColorCommitted(Widget *widget, ColorPickerElement *colorPicker) override;

private:
  enum class ControlKind {
    TogglePill,
    ToggleKnob,
    BoundSwitch, ///< A real toggleSwitch element on the target widget.
    BoundSlider, ///< A real slider element on the target widget.
    SelectButton,
    SelectLabel,
    ResetButton,
    ResetLabel,
    CloseButton,
    Input,
    ColorSwatch,
  };

  struct ControlInfo {
    std::wstring settingId;
    ControlKind kind;
  };

  SettingsPanel() = default;

  void Close();
  void Commit(const std::wstring &settingId, const std::wstring &value);
  void CommitInput(InputBoxElement *inputBox);
  void CommitColor(ColorPickerElement *colorPicker);
  void ResetAll();
  void UpdateRowVisuals(const WidgetSetting &setting);
  void UpdateAllVisuals();
  void BuildPanel(Widget *target);

  /// @brief False once a nested callback has deleted this panel.
  static bool IsAlive(SettingsPanel *panel);
  /// @brief Frees panels orphaned during a popup or blur dispatch.
  static void FlushPending();

  static std::vector<SettingsPanel *> s_Panels;
  static std::vector<SettingsPanel *> s_PendingDestroy;

  bool m_DeferClose = false;    ///< Inside a popup/blur dispatch.
  bool m_CloseRequested = false; ///< Close() called while deferring.

  Widget *m_Panel = nullptr;  ///< Owned panel widget.
  Widget *m_Target = nullptr; ///< Target widget (guarded by IsValid).
  std::unordered_map<std::wstring, ControlInfo>
      m_Controls;                    ///< Panel element ID -> control info.
  std::vector<std::wstring> m_Order; ///< Setting IDs in schema order.
};

#endif // __NOVADESK_SETTINGSPANEL_H__
