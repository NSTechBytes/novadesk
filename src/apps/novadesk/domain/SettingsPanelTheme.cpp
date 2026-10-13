/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "SettingsPanelTheme.h"
#include "../shared/Settings.h"
#include "../shared/System.h"

ThemePalette DarkPalette() {
  ThemePalette p{};
  // Win32 dark mode standard color scheme (matching win32-darkmodelib & Rainmeter)
  // Background: RGB(32, 32, 32), Page/Sidebar: RGB(37, 37, 37), Control: RGB(56, 56, 56)
  // Edge/Border: RGB(75, 75, 75), Text: RGB(224, 224, 224), Accent: RGB(96, 205, 255)
  p.background    = RGB(32,  32,  32);
  p.sidebar       = RGB(37,  37,  37);
  p.sidebarBorder = RGB(48,  48,  48);
  p.text          = RGB(224, 224, 224);
  p.label         = RGB(170, 170, 170);
  p.muted         = RGB(128, 128, 128);
  p.accent        = RGB(96,  205, 255);
  p.controlFill   = RGB(56,  56,  56);
  p.controlBorder = RGB(75,  75,  75);
  p.toggleOff     = RGB(75,  75,  75);
  p.divider       = RGB(48,  48,  48);
  p.tabActive     = RGB(224, 224, 224);
  p.tabInactive   = RGB(140, 140, 140);
  p.tabBar        = RGB(37,  37,  37);
  p.bgAlpha       = 255;
  return p;
}

ThemePalette LightPalette() {
  ThemePalette p{};
  p.background    = RGB(248, 248, 252);
  p.sidebar       = RGB(236, 237, 243);
  p.sidebarBorder = RGB(215, 216, 226);
  p.text          = RGB(18,  18,  22);
  p.label         = RGB(80,  82,  98);
  p.muted         = RGB(140, 142, 158);
  p.accent        = RGB(59,  130, 246);
  p.controlFill   = RGB(255, 255, 255);
  p.controlBorder = RGB(200, 202, 215);
  p.toggleOff     = RGB(195, 196, 210);
  p.divider       = RGB(215, 216, 226);
  p.tabActive     = RGB(18,  18,  22);
  p.tabInactive   = RGB(120, 122, 138);
  p.tabBar        = RGB(236, 237, 243);
  p.bgAlpha       = 255;
  return p;
}

ThemePalette ResolveTheme() {
  std::string pref = Settings::GetGlobalString("windowsTheme", "");
  if (pref.empty()) {
    pref = Settings::GetGlobalString("theme", "system");
  }
  if (pref == "dark")
    return DarkPalette();
  if (pref == "light")
    return LightPalette();
  // "system" or anything unrecognised — follow OS
  return novadesk::shared::system::IsSystemDarkMode() ? DarkPalette() : LightPalette();
}
