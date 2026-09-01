/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "WidgetDropTarget.h"
#include "Widget.h"
#include "WidgetLayoutHelper.h"
#include "../render/Element.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "../shared/Logging.h"

WidgetDropTarget::WidgetDropTarget(Widget* widget)
    : m_Widget(widget)
{
}

void WidgetDropTarget::ExtractFiles(IDataObject* pDataObj, std::vector<std::wstring>& outFiles)
{
    outFiles.clear();
    if (!pDataObj)
        return;

    FORMATETC fmt = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    STGMEDIUM stg = { 0 };

    if (SUCCEEDED(pDataObj->GetData(&fmt, &stg)))
    {
        HDROP hDrop = static_cast<HDROP>(GlobalLock(stg.hGlobal));
        if (hDrop)
        {
            UINT count = DragQueryFileW(hDrop, 0xFFFFFFFF, nullptr, 0);
            outFiles.reserve(count);
            for (UINT i = 0; i < count; ++i)
            {
                UINT len = DragQueryFileW(hDrop, i, nullptr, 0);
                if (len > 0)
                {
                    std::vector<wchar_t> buf(len + 1);
                    if (DragQueryFileW(hDrop, i, buf.data(), len + 1))
                    {
                        outFiles.emplace_back(buf.data());
                    }
                }
            }
            GlobalUnlock(stg.hGlobal);
        }
        ReleaseStgMedium(&stg);
    }
}

Element* WidgetDropTarget::HitTestDropTarget(int screenX, int screenY, int& outLocalX, int& outLocalY)
{
    outLocalX = 0;
    outLocalY = 0;

    if (!m_Widget)
        return nullptr;

    HWND hWnd = m_Widget->GetWindow();
    if (!hWnd || !IsWindow(hWnd))
        return nullptr;

    POINT clientPt = { screenX, screenY };
    ScreenToClient(hWnd, &clientPt);
    int x = clientPt.x;
    int y = clientPt.y;

    const auto& elements = m_Widget->GetElements();
    for (auto it = elements.rbegin(); it != elements.rend(); ++it)
    {
        Element* elem = it->get();
        if (!elem || !elem->IsVisible())
            continue;
        if (elem->IsContained())
            continue;

        if (elem->IsContainer())
        {
            Element* hitChild = nullptr;
            Element* actionElem = nullptr;
            Element* mouseActionElem = nullptr;
            Element* tooltipElem = nullptr;
            if (WidgetLayoutHelper::HitTestContainerChildrenDetailed(
                    *m_Widget, elem, x, y, WM_MOUSEMOVE, 0,
                    hitChild, actionElem, mouseActionElem, tooltipElem))
            {
                Element* candidate = hitChild ? hitChild : elem;
                while (candidate && !candidate->IsDropTarget())
                {
                    candidate = candidate->GetContainer();
                }

                if (candidate && candidate->IsDropTarget())
                {
                    GfxRect b = candidate->GetBounds();
                    outLocalX = x - b.X;
                    outLocalY = y - b.Y;
                    if (candidate->IsContained())
                    {
                        Element* parentContainer = candidate->GetContainer();
                        if (parentContainer)
                        {
                            outLocalX += parentContainer->GetScrollX();
                            outLocalY += parentContainer->GetScrollY();
                        }
                    }
                    return candidate;
                }
            }
        }

        if (elem->HitTest(x, y) && elem->IsDropTarget())
        {
            GfxRect b = elem->GetBounds();
            outLocalX = x - b.X;
            outLocalY = y - b.Y;
            return elem;
        }
    }

    return nullptr;
}

STDMETHODIMP WidgetDropTarget::DragEnter(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (!pdwEffect)
        return E_INVALIDARG;

    ExtractFiles(pDataObj, m_CachedFiles);
    m_AcceptsDrop = !m_CachedFiles.empty();

    int localX = 0, localY = 0;
    Element* target = HitTestDropTarget(pt.x, pt.y, localX, localY);
    m_CurrentHoverElement = target;

    if (target && m_AcceptsDrop)
    {
        *pdwEffect &= (DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK);
        if (*pdwEffect == 0)
            *pdwEffect = DROPEFFECT_COPY;

        if (target->m_OnDragEnterCallbackId != -1)
        {
            JSEngine::DropEventData data;
            data.files = m_CachedFiles;
            data.screenX = pt.x;
            data.screenY = pt.y;
            POINT clientPt = { pt.x, pt.y };
            if (m_Widget && m_Widget->GetWindow())
                ScreenToClient(m_Widget->GetWindow(), &clientPt);
            data.clientX = clientPt.x;
            data.clientY = clientPt.y;
            data.offsetX = localX;
            data.offsetY = localY;
            GfxRect b = target->GetBounds();
            if (b.Width > 0)
                data.offsetXPercent = (int)((data.offsetX / (double)b.Width) * 100.0);
            if (b.Height > 0)
                data.offsetYPercent = (int)((data.offsetY / (double)b.Height) * 100.0);
            data.effect = (*pdwEffect & DROPEFFECT_COPY) ? "copy" : ((*pdwEffect & DROPEFFECT_MOVE) ? "move" : "link");
            JSEngine::CallDropCallback(target->m_OnDragEnterCallbackId, m_Widget, &data);
        }
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}

STDMETHODIMP WidgetDropTarget::DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (!pdwEffect)
        return E_INVALIDARG;

    int localX = 0, localY = 0;
    Element* target = HitTestDropTarget(pt.x, pt.y, localX, localY);

    if (target != m_CurrentHoverElement)
    {
        // Fired leave on previously hovered element
        if (m_CurrentHoverElement && m_CurrentHoverElement->m_OnDragLeaveCallbackId != -1)
        {
            JSEngine::DropEventData data;
            data.files = m_CachedFiles;
            data.screenX = pt.x;
            data.screenY = pt.y;
            POINT clientPt = { pt.x, pt.y };
            if (m_Widget && m_Widget->GetWindow())
                ScreenToClient(m_Widget->GetWindow(), &clientPt);
            data.clientX = clientPt.x;
            data.clientY = clientPt.y;
            JSEngine::CallDropCallback(m_CurrentHoverElement->m_OnDragLeaveCallbackId, m_Widget, &data);
        }

        // Fired enter on newly hovered element
        if (target && target->m_OnDragEnterCallbackId != -1 && m_AcceptsDrop)
        {
            JSEngine::DropEventData data;
            data.files = m_CachedFiles;
            data.screenX = pt.x;
            data.screenY = pt.y;
            POINT clientPt = { pt.x, pt.y };
            if (m_Widget && m_Widget->GetWindow())
                ScreenToClient(m_Widget->GetWindow(), &clientPt);
            data.clientX = clientPt.x;
            data.clientY = clientPt.y;
            data.offsetX = localX;
            data.offsetY = localY;
            GfxRect b = target->GetBounds();
            if (b.Width > 0)
                data.offsetXPercent = (int)((data.offsetX / (double)b.Width) * 100.0);
            if (b.Height > 0)
                data.offsetYPercent = (int)((data.offsetY / (double)b.Height) * 100.0);
            data.effect = "copy";
            JSEngine::CallDropCallback(target->m_OnDragEnterCallbackId, m_Widget, &data);
        }

        m_CurrentHoverElement = target;
    }

    if (target && m_AcceptsDrop)
    {
        *pdwEffect &= (DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK);
        if (*pdwEffect == 0)
            *pdwEffect = DROPEFFECT_COPY;

        if (target->m_OnDragOverCallbackId != -1)
        {
            JSEngine::DropEventData data;
            data.files = m_CachedFiles;
            data.screenX = pt.x;
            data.screenY = pt.y;
            POINT clientPt = { pt.x, pt.y };
            if (m_Widget && m_Widget->GetWindow())
                ScreenToClient(m_Widget->GetWindow(), &clientPt);
            data.clientX = clientPt.x;
            data.clientY = clientPt.y;
            data.offsetX = localX;
            data.offsetY = localY;
            GfxRect b = target->GetBounds();
            if (b.Width > 0)
                data.offsetXPercent = (int)((data.offsetX / (double)b.Width) * 100.0);
            if (b.Height > 0)
                data.offsetYPercent = (int)((data.offsetY / (double)b.Height) * 100.0);
            data.effect = (*pdwEffect & DROPEFFECT_COPY) ? "copy" : ((*pdwEffect & DROPEFFECT_MOVE) ? "move" : "link");
            JSEngine::CallDropCallback(target->m_OnDragOverCallbackId, m_Widget, &data);
        }
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    return S_OK;
}

STDMETHODIMP WidgetDropTarget::DragLeave()
{
    if (m_CurrentHoverElement && m_CurrentHoverElement->m_OnDragLeaveCallbackId != -1)
    {
        JSEngine::DropEventData data;
        data.files = m_CachedFiles;
        JSEngine::CallDropCallback(m_CurrentHoverElement->m_OnDragLeaveCallbackId, m_Widget, &data);
    }

    m_CurrentHoverElement = nullptr;
    m_CachedFiles.clear();
    m_AcceptsDrop = false;
    return S_OK;
}

STDMETHODIMP WidgetDropTarget::Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect)
{
    if (!pdwEffect)
        return E_INVALIDARG;

    ExtractFiles(pDataObj, m_CachedFiles);

    int localX = 0, localY = 0;
    Element* target = HitTestDropTarget(pt.x, pt.y, localX, localY);

    if (target && !m_CachedFiles.empty())
    {
        *pdwEffect &= (DROPEFFECT_COPY | DROPEFFECT_MOVE | DROPEFFECT_LINK);
        if (*pdwEffect == 0)
            *pdwEffect = DROPEFFECT_COPY;

        if (target->m_OnDropCallbackId != -1)
        {
            JSEngine::DropEventData data;
            data.files = m_CachedFiles;
            data.screenX = pt.x;
            data.screenY = pt.y;
            POINT clientPt = { pt.x, pt.y };
            if (m_Widget && m_Widget->GetWindow())
                ScreenToClient(m_Widget->GetWindow(), &clientPt);
            data.clientX = clientPt.x;
            data.clientY = clientPt.y;
            data.offsetX = localX;
            data.offsetY = localY;
            GfxRect b = target->GetBounds();
            if (b.Width > 0)
                data.offsetXPercent = (int)((data.offsetX / (double)b.Width) * 100.0);
            if (b.Height > 0)
                data.offsetYPercent = (int)((data.offsetY / (double)b.Height) * 100.0);
            data.effect = (*pdwEffect & DROPEFFECT_COPY) ? "copy" : ((*pdwEffect & DROPEFFECT_MOVE) ? "move" : "link");
            JSEngine::CallDropCallback(target->m_OnDropCallbackId, m_Widget, &data);
        }
    }
    else
    {
        *pdwEffect = DROPEFFECT_NONE;
    }

    m_CurrentHoverElement = nullptr;
    m_CachedFiles.clear();
    m_AcceptsDrop = false;
    return S_OK;
}
