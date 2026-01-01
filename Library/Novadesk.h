/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "Resource.h"

// Tray icon control functions
void ShowTrayIconDynamic();
void HideTrayIconDynamic();

#include <vector>
#include <string>

struct TrayMenuItem {
    std::wstring text;
    int id;
    bool isSeparator;
    bool checked;
    std::vector<TrayMenuItem> children;
};

void SetTrayMenu(const std::vector<TrayMenuItem>& menu);
void ClearTrayMenu();
void SetShowDefaultTrayItems(bool show);
