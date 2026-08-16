#pragma once

#include "Element.h"

class ColorPickerElement : public Element
{
public:
    ColorPickerElement(const std::wstring& id, int x, int y, int width = 32, int height = 32)
        : Element(ELEMENT_COLOR_PICKER, id, x, y, width, height) {}

    void SetColor(COLORREF color) { m_Color = color; }
    COLORREF GetColor() const { return m_Color; }
    int m_OnChangeCallbackId = -1;
    int m_OnOpenCallbackId = -1;
    int m_OnCloseCallbackId = -1;
    int m_OnCancelCallbackId = -1;
    int m_OnEyedropperOpenCallbackId = -1;
    int m_OnEyedropperPickCallbackId = -1;
    void Render(ID2D1DeviceContext* context) override;
    int GetAutoWidth() override { return 32; }
    int GetAutoHeight() override { return 32; }

private:
    COLORREF m_Color = RGB(0, 0, 0);
};
