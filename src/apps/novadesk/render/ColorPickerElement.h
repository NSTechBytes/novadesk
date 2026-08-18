#pragma once

#include "Element.h"

class ColorPickerElement : public Element
{
public:
    ColorPickerElement(const std::wstring& id, int x, int y, int width = 32, int height = 32)
        : Element(ELEMENT_COLOR_PICKER, id, x, y, width, height) {}

    // ── Color ──────────────────────────────────────────────────────
    void SetColor(COLORREF color) { m_Color = color; }
    COLORREF GetColor() const { return m_Color; }

    // ── Swatch styling ────────────────────────────────────────────
    float m_BorderRadius = 0.0f;
    float m_BorderWidth  = 0.0f;
    COLORREF m_BorderColor = RGB(0, 0, 0);
    BYTE  m_BorderAlpha  = 255;
    float m_Opacity      = 1.0f;
    bool  m_CircleShape  = false;   // true = ellipse swatch

    // ── Popup appearance ──────────────────────────────────────────
    COLORREF m_PopupBackground          = RGB(255, 255, 255);
    BYTE     m_PopupBackgroundAlpha     = 255;
    COLORREF m_PopupAccentColor         = RGB(0, 0, 0);
    BYTE     m_PopupAccentAlpha         = 255;
    COLORREF m_PopupBorderColor         = RGB(0, 0, 0);
    BYTE     m_PopupBorderAlpha         = 255;
    COLORREF m_PopupInputBackground      = RGB(255, 255, 255);
    BYTE     m_PopupInputBackgroundAlpha = 255;
    bool     m_HasPopupInputBackground  = false;
    COLORREF m_PopupInputColor          = RGB(0, 0, 0);
    BYTE     m_PopupInputColorAlpha     = 255;
    bool     m_HasPopupInputColor       = false;

    // ── Popup behavior ────────────────────────────────────────────
    bool m_ShowEyedropper   = true;
    bool m_ShowFormatToggle = true;
    bool m_DefaultHexMode   = false;  // true = open in HEX mode

    // ── Callbacks ─────────────────────────────────────────────────
    int m_OnChangeCallbackId       = -1;
    int m_OnOpenCallbackId         = -1;
    int m_OnCloseCallbackId        = -1;
    int m_OnCancelCallbackId       = -1;
    int m_OnEyedropperOpenCallbackId  = -1;
    int m_OnEyedropperPickCallbackId  = -1;

    void Render(ID2D1DeviceContext* context) override;
    int GetAutoWidth()  override { return 32; }
    int GetAutoHeight() override { return 32; }

private:
    COLORREF m_Color = RGB(0, 0, 0);
};
