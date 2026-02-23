#include "Widget.h"

#include <iostream>
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

std::vector<RGFW_window*> g_windows;
std::unordered_map<RGFW_window*, DragState> g_dragStates;
}

RGFW_window* CreateWidgetWindow(int width, int height, bool debug) {
    if (debug) {
        std::cerr << "[novadesk] JsCreateWindow width=" << width << " height=" << height << std::endl;
    }

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
    RGFW_window_setBorder(win, RGFW_FALSE);

    g_windows.push_back(win);
    g_dragStates[win] = DragState{};
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
        }

        if (RGFW_window_shouldClose(win) == RGFW_TRUE) {
            g_dragStates.erase(win);
            RGFW_window_close(win);
            it = g_windows.erase(it);
        } else {
            ++it;
        }
    }
}
}  // namespace novadesk::domain::widget
