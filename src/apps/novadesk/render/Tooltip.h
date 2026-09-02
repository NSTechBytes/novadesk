/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef TOOLTIP_H
#define TOOLTIP_H

#include <Windows.h>
#include <CommCtrl.h>
#include <string>

class Element;

/**
 * @brief Manages Win32 tooltip windows for element hover display.
 *
 * @note Supports both standard and balloon-style tooltips with hybrid
 *       tracking (delayed move to prevent flicker on rapid hover changes).
 */
class Tooltip {
public:
  Tooltip();
  ~Tooltip();

  /**
   * @brief Initializes the tooltip system.
   *
   * @param parentHWnd Parent window handle.
   * @param hInstance Application instance handle.
   *
   * @return True if initialization succeeded.
   */
  bool Initialize(HWND parentHWnd, HINSTANCE hInstance);

  /**
   * @brief Updates the tooltip content for the hovered element.
   *
   * @param element The element to show tooltip for.
   */
  void Update(Element *element);

  /// Repositions the tooltip to follow the cursor.
  void Move();

  /// Destroys the active tooltip window.
  void Destroy();

  /// @return True if a tooltip is currently displayed.
  bool IsActive() const { return m_ActiveToolTipHWnd != nullptr; }

  /// @return Handle to the active tooltip window.
  HWND GetActiveHWnd() const { return m_ActiveToolTipHWnd; }

private:
  HWND m_ParentHWnd = nullptr;       ///< Parent window.
  HWND m_ToolTipHWnd = nullptr;      ///< Standard tooltip window.
  HWND m_ToolTipBalloonHWnd = nullptr; ///< Balloon tooltip window.
  HWND m_ActiveToolTipHWnd = nullptr;  ///< Currently active tooltip.
  DWORD m_LastMoveTime = 0;         ///< Last cursor move timestamp.
  UINT m_ToolInfoSize = 0;          ///< Size of TOOLINFO structure.
  POINT m_LastPos = {-1, -1};      ///< Last cursor position.

  // Hybrid tracking state (delayed move to prevent flicker)
  POINT m_PendingPos = {-1, -1};   ///< Pending cursor position.
  DWORD m_PendingMoveTime = 0;     ///< Pending move timestamp.
  bool m_IsMovePending = false;    ///< True if a move is pending.

  void InitializeToolTip(HWND hwnd);
};

#endif
