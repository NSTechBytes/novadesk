/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "ColorPickerPopup.h"
#include "DesktopManager.h"
#include "Widget.h"
#include "../render/ColorPickerElement.h"
#include "../render/Direct2DHelper.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include <algorithm>
#include <cmath>
#include <vector>

#pragma warning(push)
#pragma warning(disable : 4244)
#include "../../third_party/nanosvg/nanosvg.h"
#pragma warning(pop)

namespace
{
    // The top HSV surface intentionally spans the popup width, matching the
    // compact browser-style picker layout.
    constexpr int W = 320, H = 346, SV_WIDTH = 320, SV_HEIGHT = 190, HUEY = 212, EDITY = 253, MODEY = 303, MODEBOTTOM = 338;
    constexpr int INPUT_HEIGHT = 30, RGB_INPUT_WIDTH = 72, HEX_INPUT_WIDTH = 234;
    constexpr int MAGNIFIER_SIZE = 124, MAGNIFIER_BORDER = 0, MAGNIFIER_SOURCE_SIZE = 15;
    constexpr UINT_PTR EYEDROPPER_TIMER = 1;
    constexpr UINT_PTR SHOW_DESKTOP_TIMER = 2;
    constexpr UINT OUTSIDE_CLICK_MESSAGE = WM_APP + 0x41;
    HHOOK g_OutsideClickHook = nullptr;
    ColorPickerPopup* g_OutsideClickPopup = nullptr;
    float Clamp(float v) { return (std::max)(0.f, (std::min)(1.f, v)); }

    COLORREF HsvToColor(float hue, float saturation, float value)
    {
        const float h = hue * 6.0f;
        const float chroma = value * saturation;
        const float secondary = chroma * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
        const float match = value - chroma;
        float red = 0, green = 0, blue = 0;
        if (h < 1)
            red = chroma, green = secondary;
        else if (h < 2)
            red = secondary, green = chroma;
        else if (h < 3)
            green = chroma, blue = secondary;
        else if (h < 4)
            green = secondary, blue = chroma;
        else if (h < 5)
            red = secondary, blue = chroma;
        else
            red = chroma, blue = secondary;
        return RGB(static_cast<BYTE>((red + match) * 255), static_cast<BYTE>((green + match) * 255), static_cast<BYTE>((blue + match) * 255));
    }

    bool IsRgbTextValid(const wchar_t *text)
    {
        if (!text || !*text)
            return false;
        int value = 0;
        for (const wchar_t *ch = text; *ch; ++ch)
        {
            if (*ch < L'0' || *ch > L'9')
                return false;
            value = value * 10 + (*ch - L'0');
            if (value > 255)
                return false;
        }
        return true;
    }

    bool TryParseHexColor(const wchar_t *text, COLORREF &color)
    {
        if (!text)
            return false;
        if (*text == L'#')
            ++text;
        if (wcslen(text) != 6)
            return false;
        auto digit = [](wchar_t value) -> int
        {
            if (value >= L'0' && value <= L'9')
                return value - L'0';
            if (value >= L'a' && value <= L'f')
                return value - L'a' + 10;
            if (value >= L'A' && value <= L'F')
                return value - L'A' + 10;
            return -1;
        };
        int values[6];
        for (int i = 0; i < 6; ++i)
        {
            values[i] = digit(text[i]);
            if (values[i] < 0)
                return false;
        }
        color = RGB(values[0] * 16 + values[1], values[2] * 16 + values[3], values[4] * 16 + values[5]);
        return true;
    }

    DWORD ColorRefToDibPixel(COLORREF color)
    {
        // A 32-bit BI_RGB DIB stores pixels as B, G, R, X bytes.
        return (static_cast<DWORD>(GetRValue(color)) << 16) |
               (static_cast<DWORD>(GetGValue(color)) << 8) |
               static_cast<DWORD>(GetBValue(color));
    }

    void DrawSmoothEllipse(ID2D1RenderTarget *target, float x, float y, float width, float height, COLORREF fill, COLORREF outline, float outlineWidth = 1.0f)
    {
        if (!target)
            return;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> fillBrush;
        if (!Direct2D::CreateSolidBrush(target, fill, 1.0f, fillBrush.GetAddressOf()))
            return;
        const D2D1_ELLIPSE ellipse = D2D1::Ellipse(D2D1::Point2F(x + width / 2.0f, y + height / 2.0f), width / 2.0f, height / 2.0f);
        target->FillEllipse(ellipse, fillBrush.Get());
        if (outlineWidth > 0.0f)
        {
            Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> outlineBrush;
            if (Direct2D::CreateSolidBrush(target, outline, 1.0f, outlineBrush.GetAddressOf()))
                target->DrawEllipse(ellipse, outlineBrush.Get(), outlineWidth);
        }
    }

    void DrawEyedropperSvg(ID2D1RenderTarget *target, int left, int top, int size)
    {
        // This is the supplied Vaadin SVG path, parsed by the same NanoSVG
        // library used by Novadesk's PathShape renderer.
        static const char svg[] =
            "<svg viewBox=\"0 0 16 16\"><path fill=\"#444\" d=\"M15 1c-1.8-1.8-3.7-0.7-4.6 0.1-0.4 0.4-0.7 0.9-0.7 1.5v0c0 1.1-1.1 1.8-2.1 1.5l-0.1-0.1-0.7 0.8 0.7 0.7-6 6-0.8 2.3-0.7 0.7 1.5 1.5 0.8-0.8 2.3-0.8 6-6 0.7 0.7 0.7-0.6-0.1-0.2c-0.3-1 0.4-2.1 1.5-2.1v0c0.6 0 1.1-0.2 1.4-0.6 0.9-0.9 2-2.8 0.2-4.6zM3.9 13.6l-2 0.7-0.2 0.1 0.1-0.2 0.7-2 5.8-5.8 1.5 1.5-5.9 5.7z\"/></svg>";
        static NSVGimage *image = []()
        {
            std::vector<char> source(svg, svg + sizeof(svg));
            return nsvgParse(source.data(), "px", 96.0f);
        }();
        if (!image)
            return;

        ID2D1Factory1 *factory = Direct2D::GetFactory();
        if (!target || !factory)
            return;
        const float scale = size / 16.0f;
        Microsoft::WRL::ComPtr<ID2D1PathGeometry> iconPath;
        if (FAILED(factory->CreatePathGeometry(iconPath.GetAddressOf())))
            return;
        Microsoft::WRL::ComPtr<ID2D1GeometrySink> sink;
        if (FAILED(iconPath->Open(sink.GetAddressOf())))
            return;
        sink->SetFillMode(D2D1_FILL_MODE_WINDING);
        for (NSVGshape *shape = image->shapes; shape; shape = shape->next)
        {
            for (NSVGpath *path = shape->paths; path; path = path->next)
            {
                if (path->npts < 2)
                    continue;
                float *points = path->pts;
                float currentX = left + points[0] * scale;
                float currentY = top + points[1] * scale;
                sink->BeginFigure(D2D1::Point2F(currentX, currentY), D2D1_FIGURE_BEGIN_FILLED);
                for (int i = 0; i < path->npts - 1; i += 3)
                {
                    float *point = &points[i * 2];
                    const float control1X = left + point[2] * scale;
                    const float control1Y = top + point[3] * scale;
                    const float control2X = left + point[4] * scale;
                    const float control2Y = top + point[5] * scale;
                    const float endX = left + point[6] * scale;
                    const float endY = top + point[7] * scale;
                    sink->AddBezier(D2D1::BezierSegment(D2D1::Point2F(control1X, control1Y), D2D1::Point2F(control2X, control2Y), D2D1::Point2F(endX, endY)));
                    currentX = endX;
                    currentY = endY;
                }
                sink->EndFigure(path->closed ? D2D1_FIGURE_END_CLOSED : D2D1_FIGURE_END_OPEN);
            }
        }
        if (FAILED(sink->Close()))
            return;
        Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> iconBrush;
        if (Direct2D::CreateSolidBrush(target, RGB(68, 68, 68), 1.0f, iconBrush.GetAddressOf()))
            target->FillGeometry(iconPath.Get(), iconBrush.Get());
    }
}

ColorPickerPopup::ColorPickerPopup(Widget *w, ColorPickerElement *p) : m_Widget(w), m_Picker(p) { SetRGB(p->GetColor(), false); }
ColorPickerPopup::~ColorPickerPopup() { Close(); }

void ColorPickerPopup::Show()
{
    static ATOM atom = 0;
    if (!atom)
    {
        WNDCLASSW wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandleW(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"NovadeskColorPickerPopup";
        // WM_PAINT presents a fully rendered back buffer, so Windows must not
        // erase the client area with a separate white frame first.
        wc.hbrBackground = nullptr;
        atom = RegisterClassW(&wc);
    }
    if (m_hWnd)
    {
        BringWindowToTop(m_hWnd);
        SetForegroundWindow(m_hWnd);
        return;
    }
    RECT r{};
    GetWindowRect(m_Widget->GetWindow(), &r);
    GfxRect b = m_Picker->GetBounds();
    int x = r.left + b.X, y = r.top + b.Y + b.Height;
    HMONITOR mon = MonitorFromPoint({x, y}, MONITOR_DEFAULTTONEAREST);
    MONITORINFO mi{sizeof(mi)};
    GetMonitorInfoW(mon, &mi);
    if (y + H > mi.rcWork.bottom)
        y = r.top + b.Y - H;
    const int workLeft = static_cast<int>(mi.rcWork.left);
    const int workRight = static_cast<int>(mi.rcWork.right);
    x = (std::max)(workLeft, (std::min)(x, workRight - W));
    Widget::IncrementColorPickerCount();
    m_hWnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"NovadeskColorPickerPopup", L"", WS_POPUP | WS_BORDER | WS_CLIPCHILDREN, x, y, W, H, m_Widget->GetWindow(), nullptr, GetModuleHandleW(nullptr), this);
    if (m_hWnd)
    {
        SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, W, H, SWP_SHOWWINDOW);
        BringWindowToTop(m_hWnd);
        SetForegroundWindow(m_hWnd);
        InstallOutsideClickHook();
        m_ShowDesktopWasActive = System::GetShowDesktop();
        SetTimer(m_hWnd, SHOW_DESKTOP_TIMER, 100, nullptr);
    }
    else
    {
        Widget::DecrementColorPickerCount();
    }
}

void ColorPickerPopup::Close()
{
    FlushWidgetRedraw();
    RemoveOutsideClickHook();
    if (m_hWnd)
    {
        KillTimer(m_hWnd, EYEDROPPER_TIMER);
        KillTimer(m_hWnd, SHOW_DESKTOP_TIMER);
    }
    HideEyedropperMagnifier();
    if (m_hWnd)
    {
        Widget::DecrementColorPickerCount();
        ReleaseCapture();
        DestroyWindow(m_hWnd);
        m_hWnd = nullptr;
    }
    if (m_Font)
    {
        DeleteObject(m_Font);
        m_Font = nullptr;
    }
}

void ColorPickerPopup::InstallOutsideClickHook()
{
    if (g_OutsideClickPopup == this && g_OutsideClickHook) return;
    if (g_OutsideClickHook)
    {
        UnhookWindowsHookEx(g_OutsideClickHook);
        g_OutsideClickHook = nullptr;
        g_OutsideClickPopup = nullptr;
    }
    g_OutsideClickPopup = this;
    g_OutsideClickHook = SetWindowsHookExW(WH_MOUSE_LL, OutsideClickMouseHook, GetModuleHandleW(nullptr), 0);
    if (!g_OutsideClickHook)
        g_OutsideClickPopup = nullptr;
}

void ColorPickerPopup::RemoveOutsideClickHook()
{
    if (g_OutsideClickPopup != this) return;
    if (g_OutsideClickHook) UnhookWindowsHookEx(g_OutsideClickHook);
    g_OutsideClickHook = nullptr;
    g_OutsideClickPopup = nullptr;
}

void ColorPickerPopup::ShowEyedropperMagnifier(POINT screenPosition)
{
    static ATOM atom = 0;
    if (!atom)
    {
        WNDCLASSW windowClass{};
        windowClass.lpfnWndProc = MagnifierWndProc;
        windowClass.hInstance = GetModuleHandleW(nullptr);
        windowClass.lpszClassName = L"NovadeskColorPickerMagnifier";
        windowClass.hbrBackground = nullptr;
        atom = RegisterClassW(&windowClass);
    }
    if (!m_Magnifier)
    {
        m_Magnifier = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
                                      L"NovadeskColorPickerMagnifier", L"", WS_POPUP,
                                      0, 0, MAGNIFIER_SIZE, MAGNIFIER_SIZE, m_hWnd, nullptr, GetModuleHandleW(nullptr), this);
    }
    if (!m_Magnifier)
        return;

    if (!m_MagnifierFrameDc)
    {
        HDC screenDc = GetDC(nullptr);
        if (screenDc)
        {
            m_MagnifierFrameDc = CreateCompatibleDC(screenDc);
            m_MagnifierFrameBitmap = CreateCompatibleBitmap(screenDc, MAGNIFIER_SIZE, MAGNIFIER_SIZE);
            if (m_MagnifierFrameDc && m_MagnifierFrameBitmap)
                m_MagnifierFrameOldBitmap = SelectObject(m_MagnifierFrameDc, m_MagnifierFrameBitmap);
            else
            {
                if (m_MagnifierFrameBitmap)
                    DeleteObject(m_MagnifierFrameBitmap);
                if (m_MagnifierFrameDc)
                    DeleteDC(m_MagnifierFrameDc);
                m_MagnifierFrameBitmap = nullptr;
                m_MagnifierFrameDc = nullptr;
            }
            ReleaseDC(nullptr, screenDc);
        }
    }

    MONITORINFO monitorInfo{sizeof(monitorInfo)};
    GetMonitorInfoW(MonitorFromPoint(screenPosition, MONITOR_DEFAULTTONEAREST), &monitorInfo);
    const int workLeft = static_cast<int>(monitorInfo.rcWork.left);
    const int workTop = static_cast<int>(monitorInfo.rcWork.top);
    const int workRight = static_cast<int>(monitorInfo.rcWork.right);
    const int workBottom = static_cast<int>(monitorInfo.rcWork.bottom);

    int left = screenPosition.x + 20;
    int top = screenPosition.y + 20;

    if (left + MAGNIFIER_SIZE > workRight)
        left = screenPosition.x - MAGNIFIER_SIZE - 20;
    if (top + MAGNIFIER_SIZE > workBottom)
        top = screenPosition.y - MAGNIFIER_SIZE - 20;

    left = (std::max)(workLeft, (std::min)(left, workRight - MAGNIFIER_SIZE));
    top = (std::max)(workTop, (std::min)(top, workBottom - MAGNIFIER_SIZE));

    UINT positionFlags = SWP_NOACTIVATE;
    if (!IsWindowVisible(m_Magnifier))
        positionFlags |= SWP_SHOWWINDOW;
    SetWindowPos(m_Magnifier, HWND_TOPMOST, left, top, MAGNIFIER_SIZE, MAGNIFIER_SIZE, positionFlags);
}

void ColorPickerPopup::HideEyedropperMagnifier()
{
    if (m_Magnifier)
    {
        DestroyWindow(m_Magnifier);
        m_Magnifier = nullptr;
    }
    if (m_MagnifierFrameDc)
    {
        SelectObject(m_MagnifierFrameDc, m_MagnifierFrameOldBitmap);
        DeleteObject(m_MagnifierFrameBitmap);
        DeleteDC(m_MagnifierFrameDc);
        m_MagnifierFrameDc = nullptr;
        m_MagnifierFrameBitmap = nullptr;
        m_MagnifierFrameOldBitmap = nullptr;
    }
}

void ColorPickerPopup::UpdateEyedropperSample(POINT screenPosition)
{
    ShowEyedropperMagnifier(screenPosition);

    m_LastSampledPos = screenPosition;

    HDC screenDc = GetDC(nullptr);
    if (screenDc)
    {
        if (m_MagnifierFrameDc)
        {
            RECT frame{0, 0, MAGNIFIER_SIZE, MAGNIFIER_SIZE};
            FillRect(m_MagnifierFrameDc, &frame, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            const int previousStretchMode = SetStretchBltMode(m_MagnifierFrameDc, COLORONCOLOR);
            StretchBlt(m_MagnifierFrameDc, MAGNIFIER_BORDER, MAGNIFIER_BORDER,
                       MAGNIFIER_SIZE - MAGNIFIER_BORDER * 2, MAGNIFIER_SIZE - MAGNIFIER_BORDER * 2,
                       screenDc, screenPosition.x - MAGNIFIER_SOURCE_SIZE / 2, screenPosition.y - MAGNIFIER_SOURCE_SIZE / 2,
                       MAGNIFIER_SOURCE_SIZE, MAGNIFIER_SOURCE_SIZE, SRCCOPY | CAPTUREBLT);
            SetStretchBltMode(m_MagnifierFrameDc, previousStretchMode);
            const int center = MAGNIFIER_SIZE / 2;
            SelectObject(m_MagnifierFrameDc, GetStockObject(DC_PEN));
            SetDCPenColor(m_MagnifierFrameDc, RGB(255, 255, 255));
            MoveToEx(m_MagnifierFrameDc, center - 9, center, nullptr);
            LineTo(m_MagnifierFrameDc, center + 10, center);
            MoveToEx(m_MagnifierFrameDc, center, center - 9, nullptr);
            LineTo(m_MagnifierFrameDc, center, center + 10);
            SetDCPenColor(m_MagnifierFrameDc, RGB(0, 0, 0));
            Rectangle(m_MagnifierFrameDc, center - 4, center - 4, center + 5, center + 5);
        }
        ReleaseDC(nullptr, screenDc);
    }
    if (m_Magnifier)
        InvalidateRect(m_Magnifier, nullptr, FALSE);
}

void ColorPickerPopup::ApplyEyedropperSelection(POINT screenPosition)
{
    // If the cursor is directly over the popup's own SV gradient, map mathematically
    if (m_hWnd)
    {
        POINT localPt = screenPosition;
        ScreenToClient(m_hWnd, &localPt);
        if (localPt.x >= 0 && localPt.x < SV_WIDTH && localPt.y >= 0 && localPt.y < SV_HEIGHT)
        {
            const float s = Clamp(localPt.x / static_cast<float>(SV_WIDTH - 1));
            const float v = Clamp(1.0f - localPt.y / static_cast<float>(SV_HEIGHT - 1));
            SetHSV(m_H, s, v, true);
            return;
        }
        else if (localPt.x >= 120 && localPt.x < 300 && localPt.y >= HUEY && localPt.y < HUEY + 20)
        {
            const float h = Clamp((localPt.x - 120) / 179.0f);
            SetHSV(h, m_S, m_V, true);
            return;
        }
    }

    HDC screenDc = GetDC(nullptr);
    if (screenDc)
    {
        COLORREF sampledColor = GetPixel(screenDc, screenPosition.x, screenPosition.y);
        ReleaseDC(nullptr, screenDc);
        if (sampledColor != CLR_INVALID)
        {
            SetRGB(sampledColor, true);
        }
    }
}

void ColorPickerPopup::PaintEyedropperMagnifier(HDC dc)
{
    RECT client{};
    GetClientRect(m_Magnifier, &client);
    if (m_MagnifierFrameDc)
        BitBlt(dc, 0, 0, client.right, client.bottom, m_MagnifierFrameDc, 0, 0, SRCCOPY);
    else
        FillRect(dc, &client, static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
}

COLORREF ColorPickerPopup::HSV() const
{
    return HsvToColor(m_H, m_S, m_V);
}

void ColorPickerPopup::SetRGB(COLORREF c, bool n)
{
    float r = GetRValue(c) / 255.f, g = GetGValue(c) / 255.f, b = GetBValue(c) / 255.f, mx = (std::max)(r, (std::max)(g, b)), mn = (std::min)(r, (std::min)(g, b)), d = mx - mn;
    m_V = mx;
    m_S = mx ? d / mx : 0;
    if (d >= 0.015f)
    {
        if (mx == r)
            m_H = std::fmod((g - b) / d + 6, 6) / 6;
        else if (mx == g)
            m_H = ((b - r) / d + 2) / 6;
        else
            m_H = ((r - g) / d + 4) / 6;
    }
    if (m_hWnd)
    {
        SyncEdits();
        RECT colorRegion{0, 0, W, 250};
        InvalidateRect(m_hWnd, &colorRegion, FALSE);
    }
    if (n)
        Notify();
}

void ColorPickerPopup::SetHSV(float h, float s, float v, bool n)
{
    m_H = Clamp(h);
    m_S = Clamp(s);
    m_V = Clamp(v);
    if (m_hWnd)
    {
        SyncEdits();
        RECT colorRegion{0, 0, W, 250};
        InvalidateRect(m_hWnd, &colorRegion, FALSE);
    }
    if (n)
        Notify();
}

void ColorPickerPopup::SyncEdits()
{
    if (!m_R)
        return;
    m_sync = true;
    COLORREF c = HSV();
    wchar_t t[8];
    if (m_EditingControl != 1)
    {
        swprintf_s(t, L"%u", GetRValue(c));
        SetWindowTextW(m_R, t);
    }
    if (m_EditingControl != 2)
    {
        swprintf_s(t, L"%u", GetGValue(c));
        SetWindowTextW(m_G, t);
    }
    if (m_EditingControl != 3)
    {
        swprintf_s(t, L"%u", GetBValue(c));
        SetWindowTextW(m_B, t);
    }
    if (m_EditingControl != 4)
    {
        swprintf_s(t, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
        SetWindowTextW(m_Hex, t);
    }
    m_sync = false;
}

void ColorPickerPopup::SetHexMode(bool enabled)
{
    if (m_HexMode == enabled)
        return;
    m_HexMode = enabled;
    ShowWindow(m_R, enabled ? SW_HIDE : SW_SHOW);
    ShowWindow(m_G, enabled ? SW_HIDE : SW_SHOW);
    ShowWindow(m_B, enabled ? SW_HIDE : SW_SHOW);
    ShowWindow(m_Hex, enabled ? SW_SHOW : SW_HIDE);
    SyncEdits();
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void ColorPickerPopup::Notify()
{
    m_Picker->SetColor(HSV());
    // UpdateLayeredWindowContent redraws the complete layered widget. Calling
    // it for every mouse-move sample makes the widget visibly flicker, so the
    // swatch is committed once an interaction is complete or the popup closes.
    m_WidgetNeedsRedraw = true;
    if (m_Picker->m_OnChangeCallbackId != -1)
    {
        wchar_t s[8];
        COLORREF c = HSV();
        swprintf_s(s, L"#%02X%02X%02X", GetRValue(c), GetGValue(c), GetBValue(c));
        JSEngine::CallEventCallbackWithText(m_Picker->m_OnChangeCallbackId, m_Widget, s);
    }
}

void ColorPickerPopup::FlushWidgetRedraw()
{
    if (!m_WidgetNeedsRedraw)
        return;
    m_Widget->Redraw();
    m_WidgetNeedsRedraw = false;
}

void ColorPickerPopup::Paint(HDC targetDc)
{
    RECT clientRect{};
    GetClientRect(m_hWnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientWidth <= 0 || clientHeight <= 0)
        return;

    // Render the complete popup into memory, then copy one finished frame to
    // the screen. Direct GDI drawing first clears white and then draws each
    // control, which is visible as flicker during high-frequency color input.
    HDC dc = CreateCompatibleDC(targetDc);
    if (!dc)
        return;
    HBITMAP bitmap = CreateCompatibleBitmap(targetDc, clientWidth, clientHeight);
    if (!bitmap)
    {
        DeleteDC(dc);
        return;
    }
    HGDIOBJ oldBitmap = SelectObject(dc, bitmap);
    HGDIOBJ oldFont = m_Font ? SelectObject(dc, m_Font) : nullptr;

    RECT rc{0, 0, clientWidth, clientHeight};
    FillRect(dc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
    EnsureSaturationValueBitmap();
    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = SV_WIDTH;
    bitmapInfo.bmiHeader.biHeight = -SV_HEIGHT;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(dc, 0, 0, SV_WIDTH, SV_HEIGHT, 0, 0, SV_WIDTH, SV_HEIGHT, m_SaturationValuePixels.data(), &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);

    SelectObject(dc, GetStockObject(DC_PEN));
    for (int x = 0; x < 180; x++)
    {
        SetDCPenColor(dc, HsvToColor(x / 179.f, 1.0f, 1.0f));
        MoveToEx(dc, 120 + x, HUEY, nullptr);
        LineTo(dc, 120 + x, HUEY + 18);
    }

    // Bind a Direct2D DC render target to the same back buffer. It provides
    // antialiased vector rendering without changing the popup's GDI controls.
    Microsoft::WRL::ComPtr<ID2D1DCRenderTarget> vectorTarget;
    if (ID2D1Factory1 *factory = Direct2D::GetFactory())
    {
        const D2D1_RENDER_TARGET_PROPERTIES properties = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE),
            0.0f, 0.0f, D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE);
        if (SUCCEEDED(factory->CreateDCRenderTarget(&properties, vectorTarget.GetAddressOf())) &&
            SUCCEEDED(vectorTarget->BindDC(dc, &rc)))
        {
            vectorTarget->BeginDraw();
        }
        else
        {
            vectorTarget.Reset();
        }
    }
    // Hue selector: a high-contrast ring with the selected hue at its center.
    const int hueSelectorX = 120 + static_cast<int>(m_H * 179.0f);
    const int hueSelectorY = HUEY + 9;
    DrawSmoothEllipse(vectorTarget.Get(), hueSelectorX - 10.0f, hueSelectorY - 10.0f, 20.0f, 20.0f, RGB(255, 255, 255), RGB(0, 0, 0));
    DrawSmoothEllipse(vectorTarget.Get(), hueSelectorX - 8.0f, hueSelectorY - 8.0f, 16.0f, 16.0f, HsvToColor(m_H, 1.0f, 1.0f), RGB(255, 255, 255));
    COLORREF c = HSV();
    RECT sw{58, 201, 102, 245};
    DrawSmoothEllipse(vectorTarget.Get(), static_cast<float>(sw.left), static_cast<float>(sw.top), static_cast<float>(sw.right - sw.left), static_cast<float>(sw.bottom - sw.top), c, c, 0.0f);
    const float saturationValueX = m_S * (SV_WIDTH - 1.0f);
    const float saturationValueY = (1.0f - m_V) * (SV_HEIGHT - 1.0f);
    DrawSmoothEllipse(vectorTarget.Get(), saturationValueX - 8.0f, saturationValueY - 8.0f, 16.0f, 16.0f, RGB(255, 255, 255), RGB(0, 0, 0));
    DrawEyedropperSvg(vectorTarget.Get(), 19, 213, 20);
    if (vectorTarget)
        vectorTarget->EndDraw();
    // Keep the format selector visually lightweight: it is not a blue or
    // permanently highlighted button. Hover only darkens its neutral text.
    const COLORREF modeColor = m_FormatHover ? RGB(68, 68, 68) : RGB(0, 0, 0);
    const COLORREF oldTextColor = SetTextColor(dc, modeColor);
    const int oldBackgroundMode = SetBkMode(dc, TRANSPARENT);
    const int modeTextLeft = m_HexMode ? 134 : 132;
    TextOutW(dc, modeTextLeft, 313, m_HexMode ? L"HEX" : L"RGB", 3);
    SetDCPenColor(dc, modeColor);
    MoveToEx(dc, 272, 317, nullptr);
    LineTo(dc, 275, 314);
    MoveToEx(dc, 275, 314, nullptr);
    LineTo(dc, 278, 317);
    MoveToEx(dc, 272, 324, nullptr);
    LineTo(dc, 275, 327);
    MoveToEx(dc, 275, 327, nullptr);
    LineTo(dc, 278, 324);
    SetBkMode(dc, oldBackgroundMode);
    SetTextColor(dc, oldTextColor);

    BitBlt(targetDc, 0, 0, clientWidth, clientHeight, dc, 0, 0, SRCCOPY);
    if (oldFont)
        SelectObject(dc, oldFont);
    SelectObject(dc, oldBitmap);
    DeleteObject(bitmap);
    DeleteDC(dc);
}

void ColorPickerPopup::EnsureSaturationValueBitmap()
{
    if (m_SaturationValuePixels.size() == static_cast<size_t>(SV_WIDTH * SV_HEIGHT) && std::fabs(m_SaturationValueHue - m_H) < 0.0001f)
        return;
    m_SaturationValuePixels.resize(SV_WIDTH * SV_HEIGHT);
    for (int y = 0; y < SV_HEIGHT; ++y)
    {
        const float value = 1.0f - y / static_cast<float>(SV_HEIGHT - 1);
        for (int x = 0; x < SV_WIDTH; ++x)
            m_SaturationValuePixels[y * SV_WIDTH + x] = ColorRefToDibPixel(HsvToColor(m_H, x / static_cast<float>(SV_WIDTH - 1), value));
    }
    m_SaturationValueHue = m_H;
}

LRESULT CALLBACK ColorPickerPopup::OutsideClickMouseHook(int code, WPARAM message, LPARAM data)
{
    if (code == HC_ACTION && message == WM_LBUTTONDOWN && g_OutsideClickPopup && !g_OutsideClickPopup->m_eye)
    {
        const auto* mouse = reinterpret_cast<const MSLLHOOKSTRUCT*>(data);
        HWND clickedWindow = WindowFromPoint(mouse->pt);
        bool isInsidePicker = false;
        for (HWND window = clickedWindow; window; window = GetParent(window))
        {
            if (window == g_OutsideClickPopup->m_hWnd || window == g_OutsideClickPopup->m_Magnifier)
            {
                isInsidePicker = true;
                break;
            }
        }
        if (!isInsidePicker && g_OutsideClickPopup->m_hWnd)
            PostMessageW(g_OutsideClickPopup->m_hWnd, OUTSIDE_CLICK_MESSAGE, 0, 0);
    }
    return CallNextHookEx(g_OutsideClickHook, code, message, data);
}

LRESULT CALLBACK ColorPickerPopup::WndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    auto *p = (ColorPickerPopup *)GetWindowLongPtrW(h, GWLP_USERDATA);
    if (m == WM_NCCREATE)
    {
        p = (ColorPickerPopup *)((CREATESTRUCTW *)l)->lpCreateParams;
        SetWindowLongPtrW(h, GWLP_USERDATA, (LONG_PTR)p);
        p->m_hWnd = h;
    }
    return p ? p->Handle(m, w, l) : DefWindowProcW(h, m, w, l);
}

LRESULT CALLBACK ColorPickerPopup::MagnifierWndProc(HWND h, UINT m, WPARAM w, LPARAM l)
{
    auto *popup = reinterpret_cast<ColorPickerPopup *>(GetWindowLongPtrW(h, GWLP_USERDATA));
    if (m == WM_NCCREATE)
    {
        popup = static_cast<ColorPickerPopup *>(reinterpret_cast<CREATESTRUCTW *>(l)->lpCreateParams);
        SetWindowLongPtrW(h, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(popup));
    }
    if (!popup)
        return DefWindowProcW(h, m, w, l);
    if (m == WM_ERASEBKGND)
        return 1;
    if (m == WM_MOUSEACTIVATE)
        return MA_NOACTIVATE;
    if (m == WM_PAINT)
    {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(h, &paint);
        popup->PaintEyedropperMagnifier(dc);
        EndPaint(h, &paint);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

LRESULT ColorPickerPopup::Handle(UINT m, WPARAM w, LPARAM l)
{
    if (m == OUTSIDE_CLICK_MESSAGE && !m_eye)
    {
        Close();
        return 0;
    }
    if (m == WM_CREATE)
    {
        m_Font = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                             OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        const DWORD rgbInputStyle = WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER | ES_CENTER;
        const DWORD hexInputStyle = WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | ES_CENTER;
        m_R = CreateWindowW(L"EDIT", L"", rgbInputStyle, 33, EDITY, RGB_INPUT_WIDTH, INPUT_HEIGHT, m_hWnd, (HMENU)1, 0, 0);
        m_G = CreateWindowW(L"EDIT", L"", rgbInputStyle, 115, EDITY, RGB_INPUT_WIDTH, INPUT_HEIGHT, m_hWnd, (HMENU)2, 0, 0);
        m_B = CreateWindowW(L"EDIT", L"", rgbInputStyle, 197, EDITY, RGB_INPUT_WIDTH, INPUT_HEIGHT, m_hWnd, (HMENU)3, 0, 0);
        m_Hex = CreateWindowW(L"EDIT", L"", hexInputStyle, 33, EDITY, HEX_INPUT_WIDTH, INPUT_HEIGHT, m_hWnd, (HMENU)4, 0, 0);
        if (m_Font)
        {
            SendMessageW(m_R, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);
            SendMessageW(m_G, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);
            SendMessageW(m_B, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);
            SendMessageW(m_Hex, WM_SETFONT, reinterpret_cast<WPARAM>(m_Font), TRUE);
        }
        SendMessageW(m_R, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
        SendMessageW(m_G, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
        SendMessageW(m_B, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
        SendMessageW(m_Hex, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(0, 0));
        ShowWindow(m_Hex, SW_HIDE);
        SyncEdits();
        return 0;
    }
    if (m == WM_ERASEBKGND)
    {
        return 1;
    }
    if (m == WM_PAINT)
    {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(m_hWnd, &ps);
        Paint(dc);
        EndPaint(m_hWnd, &ps);
        return 0;
    }
    if (m == WM_LBUTTONDOWN)
    {
        // Eyedropper mode stays active after the button click that opened it.
        // A later click anywhere confirms the pixel under the cursor.
        if (m_eye)
        {
            POINT cursor{};
            GetCursorPos(&cursor);
            ApplyEyedropperSelection(cursor);
            m_eye = false;
            m_eyeAwaitingFirstRelease = false;
            m_IgnoreEyedropperFocusLoss = true;
            KillTimer(m_hWnd, EYEDROPPER_TIMER);
            HideEyedropperMagnifier();
            ReleaseCapture();
            SetForegroundWindow(m_hWnd);
            SetFocus(m_hWnd);
            FlushWidgetRedraw();
            return 0;
        }
        int x = LOWORD(l), y = HIWORD(l);
        if (x >= 32 && x <= 308 && y >= MODEY && y <= MODEBOTTOM)
        {
            SetHexMode(!m_HexMode);
        }
        else if (x >= 0 && x < SV_WIDTH && y >= 0 && y < SV_HEIGHT)
        {
            m_dragSV = true;
            SetCapture(m_hWnd);
            SetHSV(m_H, x / static_cast<float>(SV_WIDTH - 1), 1 - y / static_cast<float>(SV_HEIGHT - 1));
        }
        else if (x >= 120 && x < 300 && y >= HUEY && y < HUEY + 20)
        {
            m_dragHue = true;
            SetCapture(m_hWnd);
            SetHSV((x - 120) / 179.0f, m_S, m_V);
        }
        else if (x < 45 && y >= 200 && y < 245)
        {
            m_eye = true;
            m_eyeAwaitingFirstRelease = true;
            m_IgnoreEyedropperFocusLoss = false;
            m_LastSampledPos = {-1, -1};
            m_LastSampledColor = CLR_INVALID;
            SetCapture(m_hWnd);
            POINT cursor{};
            GetCursorPos(&cursor);
            UpdateEyedropperSample(cursor);
            SetTimer(m_hWnd, EYEDROPPER_TIMER, 16, nullptr);
        }
        return 0;
    }
    if (m == WM_MOUSEMOVE)
    {
        const int x = LOWORD(l), y = HIWORD(l);
        const bool formatHover = x >= 32 && x <= 308 && y >= MODEY && y <= MODEBOTTOM;
        if (formatHover != m_FormatHover)
        {
            m_FormatHover = formatHover;
            InvalidateRect(m_hWnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tracking{sizeof(TRACKMOUSEEVENT), TME_LEAVE, m_hWnd, 0};
        TrackMouseEvent(&tracking);
        if (!(m_dragSV || m_dragHue || m_eye))
            return 0;

        POINT p;
        GetCursorPos(&p);
        if (m_eye)
        {
            UpdateEyedropperSample(p);
        }
        else
        {
            ScreenToClient(m_hWnd, &p);
            if (m_dragSV)
                SetHSV(m_H, p.x / static_cast<float>(SV_WIDTH - 1), 1 - p.y / static_cast<float>(SV_HEIGHT - 1));
            else
                SetHSV((p.x - 120) / 180.f, m_S, m_V);
        }
        return 0;
    }
    if (m == WM_TIMER && w == SHOW_DESKTOP_TIMER)
    {
        const bool isShowingDesktop = System::GetShowDesktop();
        if (isShowingDesktop && !m_ShowDesktopWasActive)
        {
            Close();
            return 0;
        }
        m_ShowDesktopWasActive = isShowingDesktop;
        return 0;
    }
    if (m == WM_TIMER && w == EYEDROPPER_TIMER && m_eye)
    {
        POINT cursor{};
        GetCursorPos(&cursor);
        UpdateEyedropperSample(cursor);
        return 0;
    }
    if (m == WM_MOUSELEAVE)
    {
        if (m_FormatHover)
        {
            m_FormatHover = false;
            InvalidateRect(m_hWnd, nullptr, FALSE);
        }
        return 0;
    }
    if (m == WM_SETCURSOR && LOWORD(l) == HTCLIENT)
    {
        POINT cursor{};
        GetCursorPos(&cursor);
        ScreenToClient(m_hWnd, &cursor);
        if (cursor.x >= 32 && cursor.x <= 308 && cursor.y >= MODEY && cursor.y <= MODEBOTTOM)
        {
            SetCursor(LoadCursor(nullptr, IDC_HAND));
            return TRUE;
        }
    }
    if (m == WM_LBUTTONUP)
    {
        if (m_eye && m_eyeAwaitingFirstRelease)
        {
            m_eyeAwaitingFirstRelease = false;
            return 0;
        }
        if (m_eye)
        {
            POINT cursor{};
            GetCursorPos(&cursor);
            ApplyEyedropperSelection(cursor);
            m_eye = false;
            m_IgnoreEyedropperFocusLoss = true;
            KillTimer(m_hWnd, EYEDROPPER_TIMER);
            HideEyedropperMagnifier();
            ReleaseCapture();
            SetForegroundWindow(m_hWnd);
            SetFocus(m_hWnd);
            FlushWidgetRedraw();
            return 0;
        }
        m_dragSV = m_dragHue = false;
        ReleaseCapture();
        FlushWidgetRedraw();
        return 0;
    }
    if (m == WM_COMMAND && !m_sync && HIWORD(w) == EN_CHANGE)
    {
        if (LOWORD(w) == 4)
        {
            wchar_t hex[16];
            GetWindowTextW(m_Hex, hex, 16);
            COLORREF color;
            if (TryParseHexColor(hex, color))
            {
                m_EditingControl = 4;
                SetRGB(color);
                m_EditingControl = 0;
            }
            return 0;
        }
        wchar_t a[8], b[8], c[8];
        GetWindowTextW(m_R, a, 8);
        GetWindowTextW(m_G, b, 8);
        GetWindowTextW(m_B, c, 8);
        int r = _wtoi(a), g = _wtoi(b), bb = _wtoi(c);
        if (IsRgbTextValid(a) && IsRgbTextValid(b) && IsRgbTextValid(c))
        {
            m_EditingControl = LOWORD(w);
            SetRGB(RGB(r, g, bb));
            m_EditingControl = 0;
        }
        return 0;
    }
    if (m == WM_KEYDOWN && w == VK_ESCAPE)
    {
        Close();
        return 0;
    }
    if (m == WM_KILLFOCUS)
    {
        // A confirmation click happens outside the popup. Keep the picker
        // visible instead of treating that eyedropper click as a close action.
        if (m_eye || m_IgnoreEyedropperFocusLoss)
        {
            m_IgnoreEyedropperFocusLoss = false;
            return 0;
        }
        // Clicking an EDIT child transfers focus away from the popup HWND, but
        // it is still interaction inside this popup and must not close it.
        HWND nextFocus = reinterpret_cast<HWND>(w);
        if (nextFocus == m_hWnd || (nextFocus && IsChild(m_hWnd, nextFocus)))
            return 0;
        Close();
        return 0;
    }
    return DefWindowProcW(m_hWnd, m, w, l);
}
