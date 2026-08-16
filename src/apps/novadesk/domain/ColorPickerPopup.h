/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#pragma once
#include <windows.h>
#include <vector>
class Widget;
class ColorPickerElement;
class ColorPickerPopup
{
public:
    ColorPickerPopup(Widget *widget, ColorPickerElement *picker);
    ~ColorPickerPopup();
    void Show();
    void Close();
    bool IsOpen() const { return m_hWnd != nullptr; }

private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    static LRESULT CALLBACK MagnifierWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Handle(UINT, WPARAM, LPARAM);
    
    void Paint(HDC);
    void EnsureSaturationValueBitmap();
    void SetHSV(float h, float s, float v, bool notify = true);
    void SetRGB(COLORREF color, bool notify = true);
    void SyncEdits();
    void SetHexMode(bool enabled);
    void Notify();
    void FlushWidgetRedraw();
    void ShowEyedropperMagnifier(POINT screenPosition);
    void HideEyedropperMagnifier();
    void PaintEyedropperMagnifier(HDC);
    void UpdateEyedropperSample(POINT screenPosition);
    COLORREF HSV() const;
    Widget *m_Widget;
    ColorPickerElement *m_Picker;
    HWND m_hWnd = nullptr;
    HWND m_R = nullptr, m_G = nullptr, m_B = nullptr, m_Hex = nullptr;
    HWND m_Magnifier = nullptr;
    HDC m_MagnifierFrameDc = nullptr;
    HBITMAP m_MagnifierFrameBitmap = nullptr;
    HGDIOBJ m_MagnifierFrameOldBitmap = nullptr;
    HFONT m_Font = nullptr;
    float m_H = 0, m_S = 0, m_V = 0;
    bool m_dragSV = false, m_dragHue = false, m_eye = false, m_eyeAwaitingFirstRelease = false, m_sync = false;
    float m_SaturationValueHue = -1.0f;
    std::vector<DWORD> m_SaturationValuePixels;
    bool m_WidgetNeedsRedraw = false;
    bool m_HexMode = false;
    int m_EditingControl = 0;
    bool m_FormatHover = false;
};
