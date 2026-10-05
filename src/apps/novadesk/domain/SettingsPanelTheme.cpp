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
  p.background    = RGB(22,  22,  28);
  p.sidebar       = RGB(16,  16,  21);
  p.sidebarBorder = RGB(38,  38,  48);
  p.text          = RGB(236, 236, 241);
  p.label         = RGB(160, 160, 175);
  p.muted         = RGB(110, 110, 125);
  p.accent        = RGB(99,  155, 255);
  p.controlFill   = RGB(32,  32,  40);
  p.controlBorder = RGB(55,  55,  68);
  p.toggleOff     = RGB(55,  55,  68);
  p.divider       = RGB(38,  38,  48);
  p.tabActive     = RGB(236, 236, 241);
  p.tabInactive   = RGB(100, 100, 118);
  p.tabBar        = RGB(16,  16,  21);
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
  const std::string pref = Settings::GetGlobalString("theme", "system");
  if (pref == "dark")
    return DarkPalette();
  if (pref == "light")
    return LightPalette();
  // "system" or anything unrecognised — follow OS
  return novadesk::shared::system::IsSystemDarkMode() ? DarkPalette() : LightPalette();
}
