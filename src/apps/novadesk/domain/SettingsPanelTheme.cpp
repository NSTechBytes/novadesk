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
  p.background    = RGB(26,  26,  31);
  p.text          = RGB(235, 235, 240);
  p.label         = RGB(190, 190, 200);
  p.muted         = RGB(150, 150, 160);
  p.accent        = RGB(74,  134, 232);
  p.controlFill   = RGB(42,  42,  50);
  p.controlBorder = RGB(70,  70,  82);
  p.toggleOff     = RGB(64,  64,  74);
  p.divider       = RGB(58,  58,  66);
  p.tabActive     = RGB(235, 235, 240);
  p.tabInactive   = RGB(130, 130, 145);
  p.tabBar        = RGB(32,  32,  38);
  p.bgAlpha       = 255;
  return p;
}

ThemePalette LightPalette() {
  ThemePalette p{};
  p.background    = RGB(245, 245, 248);
  p.text          = RGB(20,  20,  24);
  p.label         = RGB(80,  80,  90);
  p.muted         = RGB(140, 140, 150);
  p.accent        = RGB(59,  130, 246);
  p.controlFill   = RGB(255, 255, 255);
  p.controlBorder = RGB(200, 200, 210);
  p.toggleOff     = RGB(180, 180, 190);
  p.divider       = RGB(210, 210, 218);
  p.tabActive     = RGB(20,  20,  24);
  p.tabInactive   = RGB(130, 130, 140);
  p.tabBar        = RGB(235, 235, 240);
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
