#pragma once
#include <windows.h>
#include <vector>
class Widget;
class ColorPickerElement;
class ColorPickerPopup {
public:
    ColorPickerPopup(Widget* widget, ColorPickerElement* picker);
    ~ColorPickerPopup();
    void Show();
    void Close();
    bool IsOpen() const { return m_hWnd != nullptr; }
private:
    static LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT Handle(UINT, WPARAM, LPARAM);
    void Paint(HDC);
    void EnsureSaturationValueBitmap();
    void SetHSV(float h, float s, float v, bool notify = true);
    void SetRGB(COLORREF color, bool notify = true);
    void SyncEdits();
    void Notify();
    COLORREF HSV() const;
    Widget* m_Widget; ColorPickerElement* m_Picker; HWND m_hWnd = nullptr;
    HWND m_R = nullptr, m_G = nullptr, m_B = nullptr;
    float m_H = 0, m_S = 0, m_V = 0; bool m_dragSV = false, m_dragHue = false, m_eye = false, m_sync = false;
    float m_SaturationValueHue = -1.0f;
    std::vector<DWORD> m_SaturationValuePixels;
};
