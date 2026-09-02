/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>
#include <oleidl.h>
#include <wrl/client.h>
#include <wrl/implements.h>
#include <vector>
#include <string>

class Widget;
class Element;

/**
 * @brief Implements OLE drag-and-drop for widgets.
 *
 * @note Handles file drops onto widget elements, routing drop events
 *       to the appropriate element's onDrop callback.
 */
class WidgetDropTarget
    : public Microsoft::WRL::RuntimeClass<
          Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
          IDropTarget> {
public:
  /**
   * @brief Constructs a drop target for a widget.
   *
   * @param widget The widget to handle drops for.
   */
  explicit WidgetDropTarget(Widget *widget);
  virtual ~WidgetDropTarget() = default;

  // IDropTarget interface
  STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt,
                       DWORD *pdwEffect) override;
  STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
  STDMETHOD(DragLeave)() override;
  STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt,
                  DWORD *pdwEffect) override;

  /// Sets or updates the associated widget.
  void SetWidget(Widget *widget) { m_Widget = widget; }

private:
  Widget *m_Widget = nullptr;               ///< Associated widget.
  Element *m_CurrentHoverElement = nullptr; ///< Element currently under cursor.
  bool m_AcceptsDrop = false; ///< Whether the current drag is accepted.
  std::vector<std::wstring> m_CachedFiles; ///< Extracted file paths from drag.

  /// Extracts file paths from the data object.
  void ExtractFiles(IDataObject *pDataObj, std::vector<std::wstring> &outFiles);

  /// Hit-tests screen coordinates against drop target elements.
  Element *HitTestDropTarget(int screenX, int screenY, int &outLocalX,
                             int &outLocalY);
};
