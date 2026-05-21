#pragma once

#include <windows.h>
#include <vector>

#include "MenuItem.h"
#include "DesktopManager.h"

struct WidgetOptions;
class Widget;

namespace WidgetContextMenuHelper
{
    int ShowContextMenu(
        HWND hWnd,
        const std::vector<MenuItem> &customMenu,
        bool showDefaultItems,
        ZPOSITION windowZPos,
        const WidgetOptions &options);

    void HandleContextCommand(Widget &widget, int cmd);
}

