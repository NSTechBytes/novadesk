/* Copyright (C) 2026 OfficialNovadesk
 * Derived from https://github.com/rainmeter/rainmeter/tree/master/Common
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <Windows.h>
#include <vector>
#include <cstdint>
#include "ControlTemplate.h"

// Shared base class for Dialog and Tab.
class BaseDialog
{
public:
	HWND GetControl(WORD id) { return GetDlgItem(m_Window, id); }

protected:
	BaseDialog();
	virtual ~BaseDialog() {}

	void Show(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless);

	void CreateControls(const Control* cts, UINT ctCount, ControlTemplate::GetStringFunc getString);
	void RelayoutControls();

	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) { return FALSE; }

	// Called once the derived dialog has handled WM_INITDIALOG and created its controls.
	virtual void HandleInitDialog() {}

	// Dialogs that draw their own list items must keep receiving NM_CUSTOMDRAW.

	HWND m_Window;
	UINT m_Dpi;
	SIZE m_DesignSize;  // Client area in dialog units.
	ControlTemplate m_ControlTemplate;

private:
	BaseDialog(const BaseDialog& r);

	static INT_PTR CALLBACK InitialDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	static INT_PTR CALLBACK MainDlgProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
};

class Dialog : public BaseDialog
{
public:
	HWND GetWindow() { return m_Window; }

	static void Initialize(HACCEL accelerator) { c_Accelerator = accelerator; }
	static bool HandleMessage(MSG& msg);
	static void DrawMenuButtonArrow(HWND window, HDC dc);

protected:
	class Tab : public BaseDialog
	{
	public:
		HWND GetWindow() { return m_Window; }
		bool IsInitialized() { return m_Initialized; }
		virtual void Create(HWND owner) = 0;
		void Activate();
		// Destroys the native page so it can be created again after a live
		// layout-direction change. The Tab object and its state remain valid.
		void ResetWindow();
		void UpdateDpi(UINT dpi) { m_Dpi = dpi; }
		void SetParentDesignSize(const SIZE& size) { m_ParentDesignSize = size; }
		RECT GetLayoutRect(UINT dpi);

		virtual void Initialize() {}
		// Called after Initialize + theme apply, before the tab is shown/painted.
		virtual void OnInitialized() {}
		virtual void Relayout(int w, int h) { RelayoutControls(); }
		virtual void HandleDpiChange() {}
		void SetPosition(int x, int y, int width, int height)
		{
			if (m_Window)
			{
				SetWindowPos(m_Window, nullptr, x, y, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
			}
		}

	protected:
		Tab();
		virtual ~Tab();

		void CreateTabWindow(short x, short y, short w, short h, HWND parent);
		void CreateTabWindowPixel(const RECT& pixelRect, HWND parent);

		bool m_Initialized;
		SIZE m_DesignOffset;  // Offset from the top left of the parent client area in dialog units.
		SIZE m_ParentDesignSize;
	};

	Dialog();
	Dialog(WINDOWPLACEMENT* placement);
	virtual ~Dialog();

	void ShowDialogWindow(const WCHAR* title, short x, short y, short w, short h, DWORD style, DWORD exStyle, HWND parent, bool modeless = true);

	// Set to true inside HandleInitDialog() to prevent ShowDialogWindow from
	// calling ShowWindow after creation (e.g. for tray-only apps that start hidden).
	bool m_StartHidden = false;

	SIZE GetWindowSizeForDesignSize(const SIZE& designSize) const;

	virtual INT_PTR HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	virtual void HandleInitDialog();
	virtual void Relayout();
	virtual void HandleDpiChange() {}

	void AddTab(WORD controlId, Tab& tab, const WCHAR* text);
	void AddPage(Tab& tab);
	void ResetTabs();
	void SelectTab(int index);
	Tab& GetActiveTab();
	virtual void ActivateTab();

	static void SetMenuButton(HWND button);
	static UINT ShowMenuButtonPopupMenu(HMENU menu, HWND button, HWND window, UINT extraFlags = 0);

protected:
	WINDOWPLACEMENT* m_WindowPlacement;
	HWND m_TabControl;
	std::vector<Tab*> m_Pages;
	std::vector<Tab*> m_Tabs;

private:
	Dialog(const Dialog& r);

	static LRESULT CALLBACK MenuButtonProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
	INT_PTR HandleDpiChanged(WPARAM wParam, LPARAM lParam);
	bool ResizeToDesignSize(const RECT& bounds);

	static HWND c_ActiveDialogWindow;
	static HACCEL c_Accelerator;
	static HHOOK c_PopupMenuFilterHook;
};
