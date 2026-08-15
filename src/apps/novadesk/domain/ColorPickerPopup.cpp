#include "ColorPickerPopup.h"
#include "Widget.h"
#include "../render/ColorPickerElement.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <gdiplus.h>

#pragma comment(lib, "gdiplus.lib")

#pragma warning(push)
#pragma warning(disable: 4244)
#include "../../third_party/nanosvg/nanosvg.h"
#pragma warning(pop)

namespace
{
    // The top HSV surface intentionally spans the popup width, matching the
    // compact browser-style picker layout.
    constexpr int W = 320, H = 346, SV_WIDTH = 320, SV_HEIGHT = 190, HUEY = 212, EDITY = 253, MODEY = 303, MODEBOTTOM = 338;
    constexpr int INPUT_HEIGHT = 30, RGB_INPUT_WIDTH = 72, HEX_INPUT_WIDTH = 234;
    float Clamp(float v) { return (std::max)(0.f, (std::min)(1.f, v)); }
    COLORREF HsvToColor(float hue, float saturation, float value)
    {
        const float h = hue * 6.0f;
        const float chroma = value * saturation;
        const float secondary = chroma * (1.0f - std::fabs(std::fmod(h, 2.0f) - 1.0f));
        const float match = value - chroma;
        float red = 0, green = 0, blue = 0;
        if (h < 1) red = chroma, green = secondary;
        else if (h < 2) red = secondary, green = chroma;
        else if (h < 3) green = chroma, blue = secondary;
        else if (h < 4) green = secondary, blue = chroma;
        else if (h < 5) red = secondary, blue = chroma;
        else red = chroma, blue = secondary;
        return RGB(static_cast<BYTE>((red + match) * 255), static_cast<BYTE>((green + match) * 255), static_cast<BYTE>((blue + match) * 255));
    }
    bool IsRgbTextValid(const wchar_t* text)
    {
        if (!text || !*text) return false;
        int value = 0;
        for (const wchar_t* ch = text; *ch; ++ch)
        {
            if (*ch < L'0' || *ch > L'9') return false;
            value = value * 10 + (*ch - L'0');
            if (value > 255) return false;
        }
        return true;
    }
    bool TryParseHexColor(const wchar_t* text, COLORREF& color)
    {
        if (!text) return false;
        if (*text == L'#') ++text;
        if (wcslen(text) != 6) return false;
        auto digit = [](wchar_t value) -> int
        {
            if (value >= L'0' && value <= L'9') return value - L'0';
            if (value >= L'a' && value <= L'f') return value - L'a' + 10;
            if (value >= L'A' && value <= L'F') return value - L'A' + 10;
            return -1;
        };
        int values[6];
        for (int i = 0; i < 6; ++i)
        {
            values[i] = digit(text[i]);
            if (values[i] < 0) return false;
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
    bool EnsureGdiplus()
    {
        static const ULONG_PTR token = []()
        {
            Gdiplus::GdiplusStartupInput startupInput;
            ULONG_PTR startupToken = 0;
            return Gdiplus::GdiplusStartup(&startupToken, &startupInput, nullptr) == Gdiplus::Ok ? startupToken : ULONG_PTR{};
        }();
        return token != 0;
    }
    Gdiplus::Color ToGdiplusColor(COLORREF color)
    {
        return Gdiplus::Color(255, GetRValue(color), GetGValue(color), GetBValue(color));
    }
    void DrawSmoothEllipse(HDC dc, float x, float y, float width, float height, COLORREF fill, COLORREF outline, float outlineWidth = 1.0f)
    {
        if (!EnsureGdiplus()) return;
        Gdiplus::Graphics graphics(dc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Gdiplus::SolidBrush fillBrush(ToGdiplusColor(fill));
        graphics.FillEllipse(&fillBrush, x, y, width, height);
        if (outlineWidth > 0.0f)
        {
            Gdiplus::Pen outlinePen(ToGdiplusColor(outline), outlineWidth);
            graphics.DrawEllipse(&outlinePen, x, y, width, height);
        }
    }
    void DrawEyedropperSvg(HDC dc, int left, int top, int size)
    {
        // This is the supplied Vaadin SVG path, parsed by the same NanoSVG
        // library used by Novadesk's PathShape renderer.
        static const char svg[] =
            "<svg viewBox=\"0 0 16 16\"><path fill=\"#444\" d=\"M15 1c-1.8-1.8-3.7-0.7-4.6 0.1-0.4 0.4-0.7 0.9-0.7 1.5v0c0 1.1-1.1 1.8-2.1 1.5l-0.1-0.1-0.7 0.8 0.7 0.7-6 6-0.8 2.3-0.7 0.7 1.5 1.5 0.8-0.8 2.3-0.8 6-6 0.7 0.7 0.7-0.6-0.1-0.2c-0.3-1 0.4-2.1 1.5-2.1v0c0.6 0 1.1-0.2 1.4-0.6 0.9-0.9 2-2.8 0.2-4.6zM3.9 13.6l-2 0.7-0.2 0.1 0.1-0.2 0.7-2 5.8-5.8 1.5 1.5-5.9 5.7z\"/></svg>";
        static NSVGimage* image = []()
        {
            std::vector<char> source(svg, svg + sizeof(svg));
            return nsvgParse(source.data(), "px", 96.0f);
        }();
        if (!image) return;

        if (!EnsureGdiplus()) return;
        const float scale = size / 16.0f;
        Gdiplus::Graphics graphics(dc);
        graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
        Gdiplus::GraphicsPath iconPath(Gdiplus::FillModeWinding);
        for (NSVGshape* shape = image->shapes; shape; shape = shape->next)
        {
            for (NSVGpath* path = shape->paths; path; path = path->next)
            {
                if (path->npts < 2) continue;
                float* points = path->pts;
                iconPath.StartFigure();
                float currentX = left + points[0] * scale;
                float currentY = top + points[1] * scale;
                for (int i = 0; i < path->npts - 1; i += 3)
                {
                    float* point = &points[i * 2];
                    const float control1X = left + point[2] * scale;
                    const float control1Y = top + point[3] * scale;
                    const float control2X = left + point[4] * scale;
                    const float control2Y = top + point[5] * scale;
                    const float endX = left + point[6] * scale;
                    const float endY = top + point[7] * scale;
                    iconPath.AddBezier(currentX, currentY, control1X, control1Y, control2X, control2Y, endX, endY);
                    currentX = endX;
                    currentY = endY;
                }
                if (path->closed) iconPath.CloseFigure();
            }
        }
        Gdiplus::SolidBrush iconBrush(Gdiplus::Color(255, 68, 68, 68));
        graphics.FillPath(&iconBrush, &iconPath);
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
    m_hWnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"NovadeskColorPickerPopup", L"", WS_POPUP | WS_BORDER | WS_CLIPCHILDREN, x, y, W, H, m_Widget->GetWindow(), nullptr, GetModuleHandleW(nullptr), this);
    ShowWindow(m_hWnd, SW_SHOWNOACTIVATE);
}
void ColorPickerPopup::Close()
{
    FlushWidgetRedraw();
    if (m_hWnd)
    {
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
COLORREF ColorPickerPopup::HSV() const
{
    return HsvToColor(m_H, m_S, m_V);
}
void ColorPickerPopup::SetRGB(COLORREF c, bool n)
{
    float r = GetRValue(c) / 255.f, g = GetGValue(c) / 255.f, b = GetBValue(c) / 255.f, mx = (std::max)(r, (std::max)(g, b)), mn = (std::min)(r, (std::min)(g, b)), d = mx - mn;
    m_V = mx;
    m_S = mx ? d / mx : 0;
    if (!d)
        m_H = 0;
    else if (mx == r)
        m_H = std::fmod((g - b) / d + 6, 6) / 6;
    else if (mx == g)
        m_H = ((b - r) / d + 2) / 6;
    else
        m_H = ((r - g) / d + 4) / 6;
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
    if (m_HexMode == enabled) return;
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
    if (!m_WidgetNeedsRedraw) return;
    m_Widget->Redraw();
    m_WidgetNeedsRedraw = false;
}
void ColorPickerPopup::Paint(HDC targetDc)
{
    RECT clientRect{};
    GetClientRect(m_hWnd, &clientRect);
    const int clientWidth = clientRect.right - clientRect.left;
    const int clientHeight = clientRect.bottom - clientRect.top;
    if (clientWidth <= 0 || clientHeight <= 0) return;

    // Render the complete popup into memory, then copy one finished frame to
    // the screen. Direct GDI drawing first clears white and then draws each
    // control, which is visible as flicker during high-frequency color input.
    HDC dc = CreateCompatibleDC(targetDc);
    if (!dc) return;
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
    // Hue selector: a high-contrast ring with the selected hue at its center.
    const int hueSelectorX = 120 + static_cast<int>(m_H * 179.0f);
    const int hueSelectorY = HUEY + 9;
    DrawSmoothEllipse(dc, hueSelectorX - 10.0f, hueSelectorY - 10.0f, 20.0f, 20.0f, RGB(255, 255, 255), RGB(0, 0, 0));
    DrawSmoothEllipse(dc, hueSelectorX - 8.0f, hueSelectorY - 8.0f, 16.0f, 16.0f, HsvToColor(m_H, 1.0f, 1.0f), RGB(255, 255, 255));
    COLORREF c = HSV();
    RECT sw{58, 201, 102, 245};
    DrawSmoothEllipse(dc, static_cast<float>(sw.left), static_cast<float>(sw.top), static_cast<float>(sw.right - sw.left), static_cast<float>(sw.bottom - sw.top), c, c, 0.0f);
    const float saturationValueX = m_S * (SV_WIDTH - 1.0f);
    const float saturationValueY = (1.0f - m_V) * (SV_HEIGHT - 1.0f);
    DrawSmoothEllipse(dc, saturationValueX - 8.0f, saturationValueY - 8.0f, 16.0f, 16.0f, RGB(255, 255, 255), RGB(0, 0, 0));
    DrawEyedropperSvg(dc, 19, 213, 20);
    // Keep the format selector visually lightweight: it is not a blue or
    // permanently highlighted button. Hover only darkens its neutral text.
    const COLORREF modeColor = m_FormatHover ? RGB(68, 68, 68) : RGB(0, 0, 0);
    const COLORREF oldTextColor = SetTextColor(dc, modeColor);
    const int oldBackgroundMode = SetBkMode(dc, TRANSPARENT);
    const int modeTextLeft = m_HexMode ? 134 : 132;
    TextOutW(dc, modeTextLeft, 313, m_HexMode ? L"HEX" : L"RGB", 3);
    SetDCPenColor(dc, modeColor);
    MoveToEx(dc, 272, 317, nullptr); LineTo(dc, 275, 314);
    MoveToEx(dc, 275, 314, nullptr); LineTo(dc, 278, 317);
    MoveToEx(dc, 272, 324, nullptr); LineTo(dc, 275, 327);
    MoveToEx(dc, 275, 327, nullptr); LineTo(dc, 278, 324);
    SetBkMode(dc, oldBackgroundMode);
    SetTextColor(dc, oldTextColor);

    BitBlt(targetDc, 0, 0, clientWidth, clientHeight, dc, 0, 0, SRCCOPY);
    if (oldFont) SelectObject(dc, oldFont);
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
LRESULT ColorPickerPopup::Handle(UINT m, WPARAM w, LPARAM l)
{
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
            SetCapture(m_hWnd);
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
        if (!(m_dragSV || m_dragHue || m_eye)) return 0;

        POINT p;
        GetCursorPos(&p);
        if (m_eye)
        {
            HDC s = GetDC(nullptr);
            SetRGB(GetPixel(s, p.x, p.y));
            ReleaseDC(nullptr, s);
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
        m_dragSV = m_dragHue = m_eye = false;
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
