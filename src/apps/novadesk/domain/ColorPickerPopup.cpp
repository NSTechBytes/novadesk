#include "ColorPickerPopup.h"
#include "Widget.h"
#include "../render/ColorPickerElement.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include <algorithm>
#include <cmath>
#include <vector>

#pragma warning(push)
#pragma warning(disable: 4244)
#include "../../third_party/nanosvg/nanosvg.h"
#pragma warning(pop)

namespace
{
    // The top HSV surface intentionally spans the popup width, matching the
    // compact browser-style picker layout.
    constexpr int W = 320, H = 346, SV_WIDTH = 320, SV_HEIGHT = 190, HUEY = 212, EDITY = 253;
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
    DWORD ColorRefToDibPixel(COLORREF color)
    {
        // A 32-bit BI_RGB DIB stores pixels as B, G, R, X bytes.
        return (static_cast<DWORD>(GetRValue(color)) << 16) |
               (static_cast<DWORD>(GetGValue(color)) << 8) |
               static_cast<DWORD>(GetBValue(color));
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

        const float scale = size / 16.0f;
        const int originalFillMode = SetPolyFillMode(dc, WINDING);
        HGDIOBJ oldBrush = SelectObject(dc, GetStockObject(DC_BRUSH));
        SetDCBrushColor(dc, RGB(68, 68, 68));
        BeginPath(dc);
        for (NSVGshape* shape = image->shapes; shape; shape = shape->next)
        {
            for (NSVGpath* path = shape->paths; path; path = path->next)
            {
                if (path->npts < 2) continue;
                float* points = path->pts;
                MoveToEx(dc, left + static_cast<int>(std::lround(points[0] * scale)), top + static_cast<int>(std::lround(points[1] * scale)), nullptr);
                for (int i = 0; i < path->npts - 1; i += 3)
                {
                    float* point = &points[i * 2];
                    POINT bezier[3] = {
                        {left + static_cast<int>(std::lround(point[2] * scale)), top + static_cast<int>(std::lround(point[3] * scale))},
                        {left + static_cast<int>(std::lround(point[4] * scale)), top + static_cast<int>(std::lround(point[5] * scale))},
                        {left + static_cast<int>(std::lround(point[6] * scale)), top + static_cast<int>(std::lround(point[7] * scale))}
                    };
                    PolyBezierTo(dc, bezier, 3);
                }
                if (path->closed) CloseFigure(dc);
            }
        }
        EndPath(dc);
        FillPath(dc);
        SelectObject(dc, oldBrush);
        SetPolyFillMode(dc, originalFillMode);
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
    m_hWnd = CreateWindowExW(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, L"NovadeskColorPickerPopup", L"", WS_POPUP | WS_BORDER, x, y, W, H, m_Widget->GetWindow(), nullptr, GetModuleHandleW(nullptr), this);
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
        InvalidateRect(m_hWnd, nullptr, FALSE);
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
        InvalidateRect(m_hWnd, nullptr, FALSE);
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
    swprintf_s(t, L"%u", GetRValue(c));
    SetWindowTextW(m_R, t);
    swprintf_s(t, L"%u", GetGValue(c));
    SetWindowTextW(m_G, t);
    swprintf_s(t, L"%u", GetBValue(c));
    SetWindowTextW(m_B, t);
    m_sync = false;
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
    COLORREF c = HSV();
    HBRUSH br = CreateSolidBrush(c);
    RECT sw{58, 201, 102, 245};
    HGDIOBJ oldBrush = SelectObject(dc, br);
    Ellipse(dc, sw.left, sw.top, sw.right, sw.bottom);
    SelectObject(dc, oldBrush);
    DeleteObject(br);
    SetDCPenColor(dc, RGB(0, 0, 0));
    Ellipse(dc, static_cast<int>(m_S * (SV_WIDTH - 1)) - 8, static_cast<int>((1 - m_V) * (SV_HEIGHT - 1)) - 8,
            static_cast<int>(m_S * (SV_WIDTH - 1)) + 8, static_cast<int>((1 - m_V) * (SV_HEIGHT - 1)) + 8);
    DrawEyedropperSvg(dc, 19, 213, 20);
    TextOutW(dc, 65, 314, L"R", 1);
    TextOutW(dc, 150, 314, L"G", 1);
    TextOutW(dc, 235, 314, L"B", 1);

    BitBlt(targetDc, 0, 0, clientWidth, clientHeight, dc, 0, 0, SRCCOPY);
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
        m_R = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 33, EDITY, 72, 44, m_hWnd, (HMENU)1, 0, 0);
        m_G = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 115, EDITY, 72, 44, m_hWnd, (HMENU)2, 0, 0);
        m_B = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 197, EDITY, 72, 44, m_hWnd, (HMENU)3, 0, 0);
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
        if (x >= 0 && x < SV_WIDTH && y >= 0 && y < SV_HEIGHT)
        {
            m_dragSV = true;
            SetCapture(m_hWnd);
        }
        else if (x >= 120 && x < 300 && y >= HUEY && y < HUEY + 20)
        {
            m_dragHue = true;
            SetCapture(m_hWnd);
        }
        else if (x < 45 && y >= 200 && y < 245)
        {
            m_eye = true;
            SetCapture(m_hWnd);
        }
        return 0;
    }
    if (m == WM_MOUSEMOVE && (m_dragSV || m_dragHue || m_eye))
    {
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
    if (m == WM_LBUTTONUP)
    {
        m_dragSV = m_dragHue = m_eye = false;
        ReleaseCapture();
        FlushWidgetRedraw();
        return 0;
    }
    if (m == WM_COMMAND && !m_sync && HIWORD(w) == EN_CHANGE)
    {
        wchar_t a[8], b[8], c[8];
        GetWindowTextW(m_R, a, 8);
        GetWindowTextW(m_G, b, 8);
        GetWindowTextW(m_B, c, 8);
        int r = _wtoi(a), g = _wtoi(b), bb = _wtoi(c);
        if (IsRgbTextValid(a) && IsRgbTextValid(b) && IsRgbTextValid(c))
            SetRGB(RGB(r, g, bb));
        return 0;
    }
    if (m == WM_KEYDOWN && w == VK_ESCAPE)
    {
        Close();
        return 0;
    }
    if (m == WM_KILLFOCUS)
    {
        Close();
        return 0;
    }
    return DefWindowProcW(m_hWnd, m, w, l);
}
