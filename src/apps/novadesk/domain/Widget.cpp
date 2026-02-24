#include "Widget.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#define RGFW_IMPLEMENTATION
#include "RGFW.h"

namespace novadesk::domain::widget {
namespace {
struct DragState {
    bool active = false;
    int startMouseX = 0;
    int startMouseY = 0;
    int startWindowX = 0;
    int startWindowY = 0;
};

struct BackgroundState {
    std::vector<u8> pixels;
    RGFW_surface* surface = nullptr;
    int width = 0;
    int height = 0;
};

std::vector<RGFW_window*> g_windows;
std::unordered_map<RGFW_window*, DragState> g_dragStates;
std::unordered_map<RGFW_window*, BackgroundState> g_backgroundStates;

void ReleaseBackground(RGFW_window* win) {
    auto it = g_backgroundStates.find(win);
    if (it == g_backgroundStates.end()) {
        return;
    }
    if (it->second.surface) {
        RGFW_surface_free(it->second.surface);
        it->second.surface = nullptr;
    }
    g_backgroundStates.erase(it);
}

void EnsureOpaqueBackground(RGFW_window* win, int width, int height) {
    if (width <= 0 || height <= 0) {
        return;
    }

    BackgroundState& bg = g_backgroundStates[win];
    if (bg.surface && bg.width == width && bg.height == height) {
        return;
    }

    if (bg.surface) {
        RGFW_surface_free(bg.surface);
        bg.surface = nullptr;
    }

    bg.width = width;
    bg.height = height;
    bg.pixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 3U);

    // BGR8 (no alpha channel) to force opaque content on Linux compositors.
    for (size_t i = 0; i < bg.pixels.size(); i += 3) {
        bg.pixels[i + 0] = 18;   // B
        bg.pixels[i + 1] = 18;   // G
        bg.pixels[i + 2] = 18;   // R
    }

    bg.surface = RGFW_window_createSurface(win, bg.pixels.data(), width, height, RGFW_formatBGR8);
}

void BlitOpaqueBackground(RGFW_window* win) {
    auto it = g_backgroundStates.find(win);
    if (it != g_backgroundStates.end() && it->second.surface) {
        RGFW_window_blitSurface(win, it->second.surface);
    }
}
}

RGFW_window* CreateWidgetWindow(int width, int height, bool debug) {
    if (debug) {
        std::cerr << "[novadesk] JsCreateWindow width=" << width << " height=" << height << std::endl;
    }

#if defined(__linux__)
    // Prefer X11 backend to avoid XWayland/Wayland compositor alpha artifacts
    // with frameless windows in some environments.
    RGFW_useWayland(RGFW_FALSE);
#endif

    const RGFW_windowFlags flags =
        RGFW_windowCenter |
        RGFW_windowFocusOnShow;

    RGFW_window* win = RGFW_createWindow("Novadesk", 100, 100, width, height, flags);
    if (!win) {
        return nullptr;
    }

    RGFW_window_show(win);
    RGFW_window_restore(win);
    RGFW_window_focus(win);
    RGFW_window_raise(win);
    RGFW_pollEvents();
    bool frameless = true;
#if defined(__linux__)
    // Linux compositors/VMs can artifact badly with borderless windows.
    // Default to framed; allow opt-in via env NOVADESK_LINUX_FRAMELESS=1.
    frameless = false;
    const char* forceFrameless = std::getenv("NOVADESK_LINUX_FRAMELESS");
    if (forceFrameless && std::string(forceFrameless) == "1") {
        frameless = true;
    }
    if (debug) {
        std::cerr << "[novadesk] linux backend: X11 (RGFW_useWayland=false)" << std::endl;
        std::cerr << "[novadesk] linux frameless: " << (frameless ? "enabled" : "disabled") << std::endl;
        const char* skipArgb = std::getenv("XLIB_SKIP_ARGB_VISUALS");
        std::cerr << "[novadesk] XLIB_SKIP_ARGB_VISUALS="
                  << (skipArgb ? skipArgb : "<unset>") << std::endl;
    }
#endif
    if (frameless) {
        RGFW_window_setBorder(win, RGFW_FALSE);
    }
    RGFW_window_setOpacity(win, 255);

    g_windows.push_back(win);
    g_dragStates[win] = DragState{};
    EnsureOpaqueBackground(win, width, height);
    BlitOpaqueBackground(win);
    return win;
}

bool HasOpenWindows() {
    return !g_windows.empty();
}

void PollAndUpdateWindows() {
    RGFW_pollEvents();

    for (auto it = g_windows.begin(); it != g_windows.end();) {
        RGFW_window* win = *it;
        RGFW_event event;
        while (RGFW_window_checkEvent(win, &event)) {
            auto dragIt = g_dragStates.find(win);
            if (dragIt != g_dragStates.end()) {
                DragState& drag = dragIt->second;

                if (event.type == RGFW_mouseButtonPressed && event.button.value == RGFW_mouseLeft) {
                    drag.active = true;
                    RGFW_getGlobalMouse(&drag.startMouseX, &drag.startMouseY);
                    RGFW_window_getPosition(win, &drag.startWindowX, &drag.startWindowY);
                } else if (event.type == RGFW_mouseButtonReleased && event.button.value == RGFW_mouseLeft) {
                    drag.active = false;
                } else if (event.type == RGFW_mousePosChanged && drag.active) {
                    int currentMouseX = 0;
                    int currentMouseY = 0;
                    RGFW_getGlobalMouse(&currentMouseX, &currentMouseY);
                    RGFW_window_move(
                        win,
                        drag.startWindowX + (currentMouseX - drag.startMouseX),
                        drag.startWindowY + (currentMouseY - drag.startMouseY)
                    );
                }
            }

            if (event.type == RGFW_windowResized) {
                EnsureOpaqueBackground(win, win->w, win->h);
                BlitOpaqueBackground(win);
            } else if (event.type == RGFW_windowRefresh) {
                BlitOpaqueBackground(win);
            }
        }

        if (RGFW_window_shouldClose(win) == RGFW_TRUE) {
            g_dragStates.erase(win);
            ReleaseBackground(win);
            RGFW_window_close(win);
            it = g_windows.erase(it);
        } else {
            // Keep repainting opaque background to prevent alpha bleed-through artifacts.
            BlitOpaqueBackground(win);
            ++it;
        }
    }
}
}  // namespace novadesk::domain::widget
