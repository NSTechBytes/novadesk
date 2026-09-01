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

class WidgetDropTarget : public Microsoft::WRL::RuntimeClass<
                             Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                             IDropTarget>
{
public:
    explicit WidgetDropTarget(Widget *widget);
    virtual ~WidgetDropTarget() = default;

    // IDropTarget
    STDMETHOD(DragEnter)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;
    STDMETHOD(DragLeave)() override;
    STDMETHOD(Drop)(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect) override;

    void SetWidget(Widget *widget) { m_Widget = widget; }

private:
    Widget *m_Widget = nullptr;
    Element *m_CurrentHoverElement = nullptr;
    bool m_AcceptsDrop = false;
    std::vector<std::wstring> m_CachedFiles;

    void ExtractFiles(IDataObject *pDataObj, std::vector<std::wstring> &outFiles);
    Element *HitTestDropTarget(int screenX, int screenY, int &outLocalX, int &outLocalY);
};
