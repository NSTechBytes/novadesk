/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "MenuUtils.h"

#include "../../common/Localization.h"

namespace MenuUtils {
namespace {

void BuildMenuItems(HMENU hMenu, const std::vector<MenuItem> &items) {
  for (const auto &item : items) {
    if (item.isSeparator) {
      AppendItem(hMenu, MF_SEPARATOR, 0, nullptr);
      continue;
    }

    UINT flags = MF_STRING;
    if (item.checked)
      flags |= MF_CHECKED;
    if (item.disabled)
      flags |= MF_GRAYED;

    if (!item.children.empty()) {
      HMENU hSubMenu = CreatePopupMenu();
      BuildMenuItems(hSubMenu, item.children);
      AppendItem(hMenu, flags | MF_POPUP, reinterpret_cast<UINT_PTR>(hSubMenu),
                 item.text.c_str());
    } else {
      AppendItem(hMenu, flags, static_cast<UINT_PTR>(item.id), item.text.c_str());
    }
  }
}

} // namespace

bool AppendItem(HMENU hMenu, UINT flags, UINT_PTR idOrSubMenu,
                const wchar_t *text) {
  if (!hMenu)
    return false;
  return AppendMenuW(hMenu, flags, idOrSubMenu, text) != FALSE;
}

int TrackPopup(HMENU hMenu, UINT flags, int x, int y, HWND owner) {
  if (!hMenu)
    return 0;

  // Rainmeter (Library/ContextMenu.cpp DisplayMenu):
  //   TrackPopupMenu(..., TPM_LEFTALIGN | (IsRTL() ? TPM_LAYOUTRTL : 0), ...)
  // Rainmeter (Library/Language.cpp):
  //   SetProcessDefaultLayout(IsRTL() ? LAYOUT_RTL : 0)
  //
  // Novadesk keeps process layout LTR most of the time so DirectWrite widgets
  // are not double-mirrored. Enable LAYOUT_RTL only for the modal menu loop.
  DWORD previousLayout = 0;
  const bool rtl = Localization::IsRtl();
  if (rtl) {
    GetProcessDefaultLayout(&previousLayout);
    SetProcessDefaultLayout(LAYOUT_RTL);
    flags |= TPM_LAYOUTRTL;
  }

  if (owner && IsWindow(owner))
    SetForegroundWindow(owner);

  const int result = static_cast<int>(
      TrackPopupMenu(hMenu, flags, x, y, 0, owner, nullptr));

  if (rtl)
    SetProcessDefaultLayout(previousLayout);

  if (owner && IsWindow(owner))
    PostMessageW(owner, WM_NULL, 0, 0);

  return result;
}

void BuildMenu(HMENU hMenu, const std::vector<MenuItem> &items) {
  BuildMenuItems(hMenu, items);
}

} // namespace MenuUtils
