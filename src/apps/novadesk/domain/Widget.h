#pragma once

struct RGFW_window;

namespace novadesk::domain::widget {
RGFW_window* CreateWidgetWindow(int width, int height, bool debug);
bool HasOpenWindows();
void PollAndUpdateWindows();
}  // namespace novadesk::domain::widget
