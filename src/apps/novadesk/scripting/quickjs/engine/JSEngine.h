/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>
#include <windows.h>

#include "quickjs.h"



class Widget;

/**
 * @brief Core JavaScript engine integration for Novadesk scripting.
 *
 * @note Manages script loading, execution, event dispatching, and IPC
 *       between the C++ widget system and QuickJS JavaScript runtime.
 */
namespace JSEngine {

/**
 * @brief Mouse event data passed to JavaScript event handlers.
 */
struct MouseEventData {
  int clientX = 0;        ///< Client-area X coordinate.
  int clientY = 0;        ///< Client-area Y coordinate.
  int screenX = 0;        ///< Screen X coordinate.
  int screenY = 0;        ///< Screen Y coordinate.
  int offsetX = 0;        ///< Element-local X coordinate.
  int offsetY = 0;        ///< Element-local Y coordinate.
  int offsetXPercent = 0; ///< X as percentage of element width.
  int offsetYPercent = 0; ///< Y as percentage of element height.
};

/**
 * @brief Drop event data passed to JavaScript onDrop handlers.
 */
struct DropEventData {
  std::vector<std::wstring> files; ///< Dropped file paths.
  int clientX = 0;                 ///< Client-area X coordinate.
  int clientY = 0;                 ///< Client-area Y coordinate.
  int screenX = 0;                 ///< Screen X coordinate.
  int screenY = 0;                 ///< Screen Y coordinate.
  int offsetX = 0;                 ///< Element-local X coordinate.
  int offsetY = 0;                 ///< Element-local Y coordinate.
  int offsetXPercent = 0;          ///< X as percentage of element width.
  int offsetYPercent = 0;          ///< Y as percentage of element height.
  std::string effect = "copy";     ///< Drop effect ("copy", "move", "link").
};

/**
 * @brief Toast notification event data.
 */
struct ToastEventData {
  int64_t toastId = 0;         ///< Toast notification ID.
  std::string type;            ///< Event type (e.g., "clicked", "dismissed").
  int actionIndex = -1;        ///< Index of clicked action button.
  std::wstring input;          ///< User input text (for input toasts).
  std::string dismissalReason; ///< Reason for dismissal.
};

// ============================================================================
// Script Loading & Execution
// ============================================================================

/// Initializes the JavaScript API bindings.
void InitializeJavaScriptAPI();

/// Loads and executes a single script file.
bool LoadAndExecuteScript(const std::wstring &scriptPath = L"");

/// Loads and executes multiple script files in order.
bool LoadAndExecuteScripts(const std::vector<std::wstring> &scriptPaths);

/// @return The directory of the entry script.
std::wstring GetEntryScriptDir();

/// @return The directory of the currently executing script.
std::wstring GetCurrentScriptDir();

/// @return The path of the currently executing script.
std::wstring GetCurrentScriptPath();

/// Associates a widget with its script path for event routing.
void RegisterWidgetOwner(Widget *widget, const std::wstring &scriptPath);

/// Removes a widget's script association.
void UnregisterWidgetOwner(Widget *widget);

/// Associates a tray icon with its script path.
void RegisterTrayOwner(int trayId, const std::wstring &scriptPath);

/// Removes a tray icon's script association.
void UnregisterTrayOwner(int trayId);

/// Reloads all scripts.
void Reload();

/// Frees the QuickJS runtime and context at application shutdown.
/// Must be called after all widgets are destroyed and no JS calls remain.
void Shutdown();

/// Adds a script to the runtime.
bool AddScript(const std::wstring &scriptPath);

/// Removes a script from the runtime.
bool RemoveScript(const std::wstring &scriptPath);

/// Refreshes (reloads) a specific script.
bool RefreshScript(const std::wstring &scriptPath);

/// @return List of currently loaded script paths.
std::vector<std::wstring> GetLoadedScripts();

// ============================================================================
// Message & Timer Handling
// ============================================================================

/// Handles timer events for script scheduling.
void OnTimer(UINT_PTR id);

/// Dispatches Windows messages to the JavaScript engine.
void OnMessage(UINT message, WPARAM wParam, LPARAM lParam);

/// Sets the message window handle for dispatching.
void SetMessageWindow(HWND hWnd);

/// @return The message window handle.
HWND GetMessageWindow();

// ============================================================================
// Event Dispatching
// ============================================================================

/// Handles tray icon command events.
void OnTrayCommand(int commandId);

/// Dispatches a tray event to JavaScript.
void DispatchTrayEvent(int trayId, const std::string &eventName);

/// Handles widget context menu command selection.
void OnWidgetContextCommand(const std::wstring &widgetId, int commandId);

/// Triggers a mouse event on a widget (e.g., "mousedown", "mouseup").
void TriggerWidgetEvent(Widget *widget, const char *eventName,
                        const MouseEventData *data = nullptr);

/// Clears all event listeners for a widget.
void ClearWidgetEventListeners(Widget *widget);

/// Calls a registered event callback by ID.
void CallEventCallback(int callbackId, Widget *widget = nullptr,
                       const MouseEventData *data = nullptr);

/// Calls an event callback with text data.
void CallEventCallbackWithText(int callbackId, Widget *widget,
                               const std::wstring &text);

/// Calls a drop event callback.
void CallDropCallback(int callbackId, Widget *widget,
                      const DropEventData *data);

/// Registers a JavaScript function as an event callback.
int RegisterEventCallback(JSContext *ctx, JSValueConst fn);

/// Registers a widget event listener (e.g., "onMouseDown").
bool RegisterWidgetEventListener(JSContext *ctx, Widget *widget,
                                 const std::string &eventName, JSValueConst fn);

/// Registers a widget context menu callback.
bool RegisterWidgetContextMenuCallback(JSContext *ctx,
                                       const std::wstring &widgetId,
                                       int commandId, JSValueConst fn);

/// Clears widget context menu callbacks.
void ClearWidgetContextMenuCallbacks(const std::wstring &widgetId);

/// Registers a tray command callback.
bool RegisterTrayCommandCallback(JSContext *ctx, int trayId, int commandId,
                                 JSValueConst fn);

/// Clears tray command callbacks.
void ClearTrayCommandCallbacks(int trayId);

/// Clears all tray command callbacks.
void ClearAllTrayCommandCallbacks();

/// Registers a tray event callback (e.g., "onClick").
bool RegisterTrayEventCallback(JSContext *ctx, int trayId,
                               const std::string &eventName, JSValueConst fn);

/// Clears tray event callbacks.
void ClearTrayEventCallbacks(int trayId);

/// Clears all tray event callbacks.
void ClearAllTrayEventCallbacks();

/// Registers a toast notification callback.
int RegisterToastCallback(JSContext *ctx, JSValueConst fn);

/// Dispatches a toast event asynchronously to JavaScript.
void DispatchToastEventAsync(int callbackId, const ToastEventData &data);

/// Clears UI IPC registrations for a specific script.
void ClearUiIpcForScript(const std::wstring &scriptPath);

/// Executes a widget's script.
bool ExecuteWidgetScript(Widget *widget);

/// Creates the UI IPC object for JavaScript.
JSValue CreateUiIpcObject(JSContext *ctx);

// ============================================================================
// Constants
// ============================================================================

/// Custom Windows message for dispatching events to the main thread.
static const UINT WM_NOVADESK_DISPATCH = WM_USER + 101;

/**
 * @brief Dispatch types for WM_NOVADESK_DISPATCH.
 */
enum DispatchType : WPARAM {
  DISPATCH_TOAST = 1,      ///< Toast notification event.
  DISPATCH_WEBFETCH = 2,   ///< WebFetch completion event.
  DISPATCH_FONT_READY = 3, ///< Font download completion event.
};

} // namespace JSEngine
