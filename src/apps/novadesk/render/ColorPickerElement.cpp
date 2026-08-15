#include "ColorPickerElement.h"
#include <wrl/client.h>

void ColorPickerElement::Render(ID2D1DeviceContext* context)
{
    if (!context || !m_Show) return;
    const GfxRect bounds = GetBounds();
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush;
    context->CreateSolidColorBrush(D2D1::ColorF(GetRValue(m_Color) / 255.0f, GetGValue(m_Color) / 255.0f, GetBValue(m_Color) / 255.0f, 1.0f), brush.GetAddressOf());
    if (brush) context->FillRectangle(D2D1::RectF((float)bounds.X, (float)bounds.Y, (float)(bounds.X + bounds.Width), (float)(bounds.Y + bounds.Height)), brush.Get());
}
