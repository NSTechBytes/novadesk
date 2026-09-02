/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_WIDGET_H__
#define __NOVADESK_WIDGET_H__

#include <windows.h>
#include <commctrl.h>
#include <atomic>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <mutex>
#include <d2d1_1.h>
#include <wrl/client.h>
#include "DesktopManager.h"
#include "../render/Element.h"
#include "../render/TextElement.h"
#include "../render/ImageElement.h"
#include "../render/GeneralImage.h"
#include "../render/BitmapElement.h"
#include "../render/RotatorElement.h"
#include "../render/ElementLayoutBox.h"
#include "../render/HistogramElement.h"
#include "../render/ButtonElement.h"
#include "../render/BarElement.h"
#include "../render/LineElement.h"
#include "../render/Tooltip.h"
#include "../render/CursorManager.h"
#include "../render/FlexLayoutEngine.h"
#include "../render/InputBoxElement.h"
#include "../render/ColorPickerElement.h"

#pragma comment(lib, "comctl32.lib")

#include "quickjs.h"

// Forward declarations
class WidgetLayoutHelper;
class ScrollbarRenderer;
class WidgetDropTarget;
class ColorPickerPopup;

namespace PropertyParser {
struct ImageOptions;
struct TextOptions;
struct ButtonOptions;
struct BitmapOptions;
struct RotatorOptions;
struct BarOptions;
struct LineOptions;
struct HistogramOptions;
struct RoundLineOptions;
struct ShapeOptions;
struct AreaGraphOptions;
struct InputBoxOptions;
struct ColorPickerOptions;
} // namespace PropertyParser

#include "MenuItem.h"

/**
 * @brief Specifies which edge of a widget is being targeted for resizing.
 */
enum class WidgetResizeEdge {
  None = 0,        ///< No resize edge detected.
  Left = 1,        ///< Left edge.
  Right = 2,       ///< Right edge.
  Top = 4,         ///< Top edge.
  Bottom = 8,      ///< Bottom edge.
  TopLeft = 5,     ///< Top-left corner.
  TopRight = 6,    ///< Top-right corner.
  BottomLeft = 9,  ///< Bottom-left corner.
  BottomRight = 10 ///< Bottom-right corner.
};

/**
 * @brief Axis-aligned rectangle with integer coordinates.
 */
struct WidgetRect4 {
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
};

/**
 * @brief Configuration for background image sizing behavior.
 */
struct BackgroundImageSize {
  /**
   * @brief Specifies how the background image is sized.
   */
  enum class Type { Cover, Contain, Stretch, Explicit };

  Type type = Type::Cover;
  float width = 0.0f;
  float height = 0.0f;
  bool hasWidth = false;
  bool hasHeight = false;

  bool operator==(const BackgroundImageSize &other) const {
    return type == other.type && width == other.width &&
           height == other.height && hasWidth == other.hasWidth &&
           hasHeight == other.hasHeight;
  }
  bool operator!=(const BackgroundImageSize &other) const {
    return !(*this == other);
  }
};

/**
 * @brief Configuration for background image positioning.
 */
struct BackgroundImagePosition {
  /**
   * @brief Specifies how the background image position is interpreted.
   */
  enum class Type { Keyword, Explicit };

  Type type = Type::Keyword;
  std::wstring keyword = L"center";
  float x = 0.0f;
  float y = 0.0f;

  bool operator==(const BackgroundImagePosition &other) const {
    return type == other.type && keyword == other.keyword && x == other.x &&
           y == other.y;
  }
  bool operator!=(const BackgroundImagePosition &other) const {
    return !(*this == other);
  }
};

/**
 * @brief Configuration options for creating and initializing a Widget.
 *
 * @note Populated from user-provided JSON configuration via PropertyParser.
 */
struct WidgetOptions {
  std::wstring id;
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  int minWidth = 0;
  int minHeight = 0;
  std::wstring backgroundColor = L"rgba(0,0,0,0)";
  ZPOSITION zPos = ZPOSITION_NORMAL;
  BYTE bgAlpha = 0;         // Alpha component of background color (0-255)
  BYTE windowOpacity = 255; // Overall window opacity (0-255)
  COLORREF color = RGB(255, 255, 255);
  GradientInfo bgGradient;
  std::wstring backgroundImage;
  std::wstring backgroundImageFallback;
  BackgroundImageSize backgroundImageSize;
  BackgroundImagePosition backgroundImagePosition;
  bool draggable = true;
  bool resizable = false;
  bool clickThrough = false;
  bool keepOnScreen = false;
  bool snapEdges = true;
  bool showInToolbar = false;
  std::wstring toolbarIcon;
  std::wstring toolbarTitle;
  bool m_WDefined = false;
  bool m_HDefined = false;
  bool show = true;
  std::wstring scriptPath;
};

/**
 * @brief Manages a desktop widget window, its elements, and event handling.
 *
 * @note Instances are owned by DesktopManager and must only be created/destroyed
 *       on the main UI thread. Each Widget owns a set of renderable Elements
 *       and handles Win32 message dispatch, mouse interaction, drag-and-drop,
 *       scrollbar behavior, and JavaScript event callbacks via QuickJS.
 */
class Widget {
public:
  // Use FlexLayoutConfig from FlexLayoutEngine
  using LayoutConfig = FlexLayoutConfig;
  struct AnimationTarget {
    bool hasX = false;
    bool hasY = false;
    bool hasWidth = false;
    bool hasHeight = false;
    bool hasRotate = false;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float rotate = 0.0f;
    bool hasFontSize = false;
    bool hasFontWeight = false;
    bool hasLetterSpacing = false;
    bool hasFontColor = false;
    float fontSize = 12.0f;
    float fontWeight = 400.0f;
    float letterSpacing = 0.0f;
    float fontColorR = 0.0f;
    float fontColorG = 0.0f;
    float fontColorB = 0.0f;
    float fontAlpha = 255.0f;

    bool HasTransformProps() const {
      return hasX || hasY || hasWidth || hasHeight || hasRotate;
    }

    bool HasTextProps() const {
      return hasFontSize || hasFontWeight || hasLetterSpacing || hasFontColor;
    }

    bool HasAnyProps() const { return HasTransformProps() || HasTextProps(); }
  };

  struct AnimationKeyframe {
    float offset = 0.0f;
    std::wstring easing;
    AnimationTarget values;
  };

  struct WindowAnimationTarget {
    bool hasX = false;
    bool hasY = false;
    bool hasWidth = false;
    bool hasHeight = false;
    bool hasOpacity = false;
    bool hasBackgroundColor = false;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float opacity = 1.0f;
    float bgColorR = 0.0f;
    float bgColorG = 0.0f;
    float bgColorB = 0.0f;
    float bgAlpha = 255.0f;

    bool HasAnyProps() const {
      return hasX || hasY || hasWidth || hasHeight || hasOpacity ||
             hasBackgroundColor;
    }
  };

  struct WindowAnimationKeyframe {
    float offset = 0.0f;
    std::wstring easing;
    WindowAnimationTarget values;
  };

  /**
   * @brief Constructs a Widget from the given configuration options.
   *
   * @param options Widget configuration (size, position, colors, z-order, etc.).
   */
  Widget(const WidgetOptions &options);

  /// @brief Destroys the widget and releases all associated resources.
  ~Widget();

  /// @brief Deleted copy constructor to prevent accidental duplication.
  Widget(const Widget &) = delete;
  /// @brief Deleted copy assignment operator.
  Widget &operator=(const Widget &) = delete;

  /**
   * @brief Creates the widget window and initializes rendering resources.
   *
   * @return True if the window was created successfully; false otherwise.
   */
  bool Create();

  /// @brief Makes the widget visible and brings it to its configured z-order.
  void Show();
  /// @brief Hides the widget window.
  void Hide();
  /// @brief Reloads the widget script and recreates all elements.
  void Refresh();
  /// @brief Sets keyboard focus to the widget window.
  void SetFocus();
  /// @brief Removes keyboard focus from the widget window.
  void UnFocus();
  /// @brief Minimizes the widget window.
  void Minimize();
  /// @brief Restores the widget from minimized state.
  void UnMinimize();
  /// @brief Maximizes the widget to fill the work area.
  void Maximize();
  /// @brief Restores the widget from maximized state.
  void Restore();
  /// @brief Toggles between maximized and normal state.
  void ToggleMaximize();
  /// @return True if the widget is currently maximized.
  bool IsMaximized() const { return m_IsMaximized; }
  /// @return True if the widget is currently minimized.
  bool IsMinimized() const { return m_IsMinimized; }
  /// @return The widget's window title (toolbar title or id).
  std::wstring GetTitle() const;

  /**
   * @brief Changes the z-order position for this widget (and optionally all).
   *
   * @param zPos The target z-order position.
   * @param all If true, applies the change to all widgets with the same z-order.
   */
  void ChangeZPos(ZPOSITION zPos, bool all = false);
  /**
   * @brief Changes the z-order of this widget without affecting others.
   *
   * @param zPos The target z-order position.
   * @param all If true, delegates to ChangeZPos.
   */
  void ChangeSingleZPos(ZPOSITION zPos, bool all = false);
  /// @brief Sets the window position and size. Pass -1 to leave a dimension unchanged.
  void SetWindowPosition(int x, int y, int w, int h);
  /// @brief Sets the overall window opacity (0-255).
  void SetWindowOpacity(BYTE opacity);
  /// @brief Sets the background color or gradient from a CSS-like string.
  void SetBackgroundColor(const std::wstring &colorStr);
  /**
   * @brief Sets the background image with size and position options.
   *
   * @param path Path to the image file (absolute, relative, or URL).
   * @param size Image sizing mode (cover, contain, stretch, explicit).
   * @param position Image positioning configuration.
   */
  void SetBackgroundImage(const std::wstring &path,
                          const BackgroundImageSize &size,
                          const BackgroundImagePosition &position);
  /// @brief Sets a fallback background image path.
  void SetBackgroundImageFallback(const std::wstring &path);
  /// @brief Enables or disables window dragging.
  void SetDraggable(bool enable);
  /// @brief Enables or disables window resizing.
  void SetResizable(bool enable);
  /// @return True if the widget is resizable.
  bool IsResizable() const { return m_Options.resizable; }
  /// @brief Sets the minimum width constraint.
  void SetMinWidth(int minWidth);
  /// @return The minimum width constraint.
  int GetMinWidth() const { return m_Options.minWidth; }
  /// @brief Sets the minimum height constraint.
  void SetMinHeight(int minHeight);
  /// @return The minimum height constraint.
  int GetMinHeight() const { return m_Options.minHeight; }
  /// @brief Sets both minimum width and height constraints.
  void SetMinSize(int minWidth, int minHeight);
  /// @brief Enables or disables click-through mode.
  void SetClickThrough(bool enable);
  /// @brief Enables or disables keeping the widget on screen.
  void SetKeepOnScreen(bool enable);
  /// @brief Enables or disables snap-to-edge behavior.
  void SetSnapEdges(bool enable);
  /// @brief Enables or disables showing the widget in the taskbar.
  void SetShowInToolbar(bool enable);
  /// @brief Sets the toolbar icon image path.
  void SetToolbarIcon(const std::wstring &path);
  /// @brief Sets the toolbar display title.
  void SetToolbarTitle(const std::wstring &title);

  /// @brief Mutex protecting the global widgets list.
  static std::mutex s_WidgetMutex;
  /// @brief Tracks whether a context menu is currently active.
  static std::atomic<bool> s_IsMenuActive;
  /// @brief Tracks the number of open color picker popups.
  static std::atomic<int> s_ActiveColorPickerCount;
  /// @return True if any menu or color picker is currently active.
  static bool IsMenuActive() {
    return s_IsMenuActive.load(std::memory_order_relaxed) ||
           s_ActiveColorPickerCount.load(std::memory_order_relaxed) > 0;
  }
  static void SetMenuActive(bool active) {
    s_IsMenuActive.store(active, std::memory_order_relaxed);
  }
  static void IncrementColorPickerCount() {
    s_ActiveColorPickerCount.fetch_add(1, std::memory_order_relaxed);
  }
  static void DecrementColorPickerCount() {
    int prev;
    do {
      prev = s_ActiveColorPickerCount.load(std::memory_order_relaxed);
    } while (prev > 0 && !s_ActiveColorPickerCount.compare_exchange_weak(
                             prev, prev - 1, std::memory_order_relaxed));
  }

  /// @return Read-only access to the widget configuration.
  const WidgetOptions &GetOptions() const { return m_Options; }
  /// @return Unique instance ID for this widget.
  uint64_t GetInstanceId() const { return m_InstanceId; }
  /// @return The underlying Win32 window handle.
  HWND GetWindow() const { return m_hWnd; }
  /// @return The current z-order position.
  ZPOSITION GetWindowZPosition() const { return m_WindowZPosition; }
  /// @return The Direct2D device context for rendering.
  ID2D1DeviceContext *GetDeviceContext() const { return m_pContext.Get(); }

  /// @brief Adds an image element to the widget.
  void AddImage(const PropertyParser::ImageOptions &options);
  /// @brief Adds a text label element to the widget.
  void AddText(const PropertyParser::TextOptions &options);
  /// @brief Adds a clickable button element to the widget.
  void AddButton(const PropertyParser::ButtonOptions &options);
  /// @brief Adds a raw bitmap element to the widget.
  void AddBitmap(const PropertyParser::BitmapOptions &options);
  /// @brief Adds a rotating knob/dial element to the widget.
  void AddRotator(const PropertyParser::RotatorOptions &options);
  /// @brief Adds a horizontal/vertical bar gauge element to the widget.
  void AddBar(const PropertyParser::BarOptions &options);
  /// @brief Adds a line segment element to the widget.
  void AddLine(const PropertyParser::LineOptions &options);
  /// @brief Adds a histogram chart element to the widget.
  void AddHistogram(const PropertyParser::HistogramOptions &options);
  /// @brief Adds a rounded line element to the widget.
  void AddRoundLine(const PropertyParser::RoundLineOptions &options);
  /// @brief Adds a geometric shape element to the widget.
  void AddShape(const PropertyParser::ShapeOptions &options);
  /// @brief Adds an area graph chart element to the widget.
  void AddAreaGraph(const PropertyParser::AreaGraphOptions &options);
  /// @brief Adds a flexbox layout container element to the widget.
  void AddLayoutBox(const PropertyParser::ShapeOptions &options);
  /// @brief Adds a text input box element to the widget.
  void AddInputBox(const PropertyParser::InputBoxOptions &options);
  /// @brief Adds a color picker popup element to the widget.
  void AddColorPicker(const PropertyParser::ColorPickerOptions &options);

  /// @brief Applies property changes to a specific element by ID.
  void SetElementProperties(const std::wstring &id, JSContext *ctx,
                            JSValueConst options);
  /// @brief Applies property changes to all elements in a group.
  void SetGroupProperties(const std::wstring &group, JSContext *ctx,
                          JSValueConst options);
  /// @brief Removes all elements belonging to the specified group.
  void RemoveElementsByGroup(const std::wstring &group);
  /**
   * @brief Removes elements by ID. If id is empty, removes all elements.
   *
   * @param id The element ID to remove, or empty for all.
   *
   * @return True if any elements were removed.
   */
  bool RemoveElements(const std::wstring &id = L"");
  /// @brief Removes multiple elements by their IDs.
  void RemoveElements(const std::vector<std::wstring> &ids);
  /// @brief Configures the right-click context menu for this widget.
  void SetContextMenu(const std::vector<MenuItem> &menu);
  /// @brief Clears the context menu.
  void ClearContextMenu();
  /// @brief Disables or enables the context menu.
  void SetContextMenuDisabled(bool disabled) {
    m_ContextMenuDisabled = disabled;
  }
  /// @brief Shows or hides default context menu items (Open, Edit, etc.).
  void SetShowDefaultContextMenuItems(bool show) {
    m_ShowDefaultContextMenuItems = show;
  }

  /// @return The currently focused input box element, or nullptr.
  InputBoxElement *GetFocusedInputBox() const { return m_FocusedInputBox; }
  /// @brief Sets the focused input box element.
  void SetFocusedInputBox(InputBoxElement *inputElem) {
    m_FocusedInputBox = inputElem;
  }
  /// @brief Gives keyboard focus to the specified input box.
  void FocusInputBox(InputBoxElement *inputElem);
  /// @brief Removes keyboard focus from the specified (or current) input box.
  void BlurInputBox(InputBoxElement *inputElem = nullptr);

  /// @brief Opens the color picker popup for the specified element.
  void OpenColorPicker(ColorPickerElement *colorPicker);
  /// @brief Closes the currently open color picker popup.
  void CloseColorPicker();
  /// @return True if a color picker is open (optionally for a specific element).
  bool IsColorPickerOpen(const ColorPickerElement *colorPicker = nullptr) const;
  /// @return True if the eyedropper tool is active.
  bool IsColorPickerEyedropperActive() const;
  /// @brief Activates the eyedropper tool for color picking.
  void OpenColorPickerEyedropper(ColorPickerElement *colorPicker = nullptr);

  /// @brief Begins a batch update (suppresses redraws until EndUpdate).
  void BeginUpdate();
  /// @brief Ends a batch update and triggers a redraw if needed.
  void EndUpdate();

  /// @brief Forces a complete redraw of the widget.
  void Redraw();
  /// @brief Called when an async image download completes.
  void OnImageDownloaded(const std::wstring &url,
                         const std::vector<BYTE> &buffer);

  /// @brief Finds an element by its unique ID.
  Element *FindElementById(const std::wstring &id);
  /// @brief Returns a thread-safe snapshot of all live widgets.
  static std::vector<Widget *>
  GetAllWidgets();                          // returns a snapshot (thread-safe)
  /// @brief Removes a widget from the global tracking list (thread-safe).
  static void RemoveWidget(Widget *widget); // thread-safe removal
  /// @brief Clears all tracked widgets.
  static void ClearAllWidgets();
  /// @return True if the widget pointer is valid and still tracked.
  static bool IsValid(Widget *pWidget);
  /// @brief Sets the flex layout configuration for a container element.
  void SetLayoutConfig(const std::wstring &id, const LayoutConfig &config);
  /// @brief Attempts to retrieve the layout config for a container element.
  bool TryGetLayoutConfig(const std::wstring &id, LayoutConfig &config) const;
  /// @return True if the element is a layout container.
  bool IsLayoutContainer(const std::wstring &id) const;
  /// @brief Forces a layout reflow for the specified container.
  void ReflowLayout(const std::wstring &id);
  /**
   * @brief Starts a property animation on a specific element.
   *
   * @param id Element ID to animate.
   * @param to Target property values.
   * @param from Starting property values.
   * @param durationMs Animation duration in milliseconds.
   * @param easing Easing function name (e.g., "linear", "easeInOut").
   * @param iterationCount Number of iterations (-1 for infinite).
   */
  void StartElementAnimation(const std::wstring &id, const AnimationTarget &to,
                             const AnimationTarget &from, int durationMs,
                             const std::wstring &easing, int iterationCount);
  /// @brief Starts a keyframe animation on a specific element.
  void StartElementKeyframeAnimation(
      const std::wstring &id, const std::vector<AnimationKeyframe> &keyframes,
      int durationMs, const std::wstring &easing, int iterationCount);
  /// @brief Starts a window-level animation (position, size, opacity, color).
  void StartWindowAnimation(const WindowAnimationTarget &to,
                            const WindowAnimationTarget &from, int durationMs,
                            const std::wstring &easing, int iterationCount);
  /// @brief Starts a keyframe window-level animation.
  void StartWindowKeyframeAnimation(
      const std::vector<WindowAnimationKeyframe> &keyframes, int durationMs,
      const std::wstring &easing, int iterationCount);
  /// @brief Stops all running window animations.
  void StopWindowAnimations();
  /// @brief Retrieves the Widget instance from a Win32 window handle.
  static Widget *GetWidgetFromHWND(HWND hWnd);
  /// @brief Retrieves the Widget instance from its unique instance ID.
  static Widget *GetWidgetFromInstanceId(uint64_t instanceId);
  /// @brief Sets a custom font path for a specific element.
  void SetElementFontPath(const std::wstring &elementId,
                          const std::wstring &fontDir);

  /// @return Read-only access to the widget's element collection.
  const std::vector<std::unique_ptr<Element>> &GetElements() const {
    return m_Elements;
  }
  /// @return The Win32 window handle (alias for GetWindow).
  HWND GetHwnd() const { return m_hWnd; }

  friend class WidgetAnimationHelper;
  friend class WidgetLayoutHelper;
  friend class ScrollbarRenderer;
  friend class WidgetDropTarget;

private:
  /// @brief Window procedure for handling widget window messages.
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);

  /// @brief Registers the widget window class (once per process).
  static bool Register();

  /// @brief Updates the layered window with the current rendered content.
  void UpdateLayeredWindowContent();

  /**
   * @brief Processes mouse messages and dispatches to element hit targets.
   *
   * @return True if the message was handled by an element.
   */
  bool HandleMouseMessage(UINT message, WPARAM wParam, LPARAM lParam);

  /// @brief Opens the context menu at the cursor position.
  void OnContextMenu();
  /// @brief Builds combined shape geometry for path-based shapes.
  bool BuildCombinedShapeGeometry(class PathShape *target,
                                  const PropertyParser::ShapeOptions &options);
  /// @brief Releases consumed resources from a combined shape.
  void ReleaseCombinedConsumes(class PathShape *target);
  /// @brief Applies parsed JavaScript properties to an element.
  void ApplyParsedPropertiesToElement(Element *element, JSContext *ctx,
                                      JSValueConst options);
  /// @brief Updates container assignment when an element's container changes.
  void UpdateContainerForElement(Element *element,
                                 const std::wstring &newContainerId);
  /// @brief Clears element references (container, tracked state) on removal.
  void ClearElementReferences(Element *element);
  /// @brief Detects if assigning a container would create a cycle.
  bool WouldCreateContainerCycle(Element *element, Element *container) const;
  /// @brief Applies flex layout to a container and its children.
  void ApplyLayoutForContainer(Element *container);
  /// @brief Renders container children in sorted order.
  void RenderContainerChildren(Element *container);
  /// @brief Hit-tests children of a container element.
  bool HitTestContainerChildren(Element *container, int x, int y,
                                Element *&outElement);
  /// @brief Detailed hit-test returning element and action targets.
  bool HitTestContainerChildrenDetailed(Element *container, int x, int y,
                                        UINT message, WPARAM wParam,
                                        Element *&outHitElement,
                                        Element *&outActionElement,
                                        Element *&outMouseActionElement,
                                        Element *&outToolTipElement);

  /// @brief Tests if an element is in the tracked elements set.
  bool IsTrackedElement(Element *el) const;
  /// @brief Stamps interactive bounds into the hit-test bitmap.
  void StampInteractiveBounds(Element *element, int offsetX, int offsetY,
                              BYTE *pvBits, int surfW, int surfH);
  /// @brief Removes an element from the tracked buttons list.
  void UntrackButton(Element *el);

private:
  // ============================================================================
  // Window & Identity
  // ============================================================================

  std::wstring m_Id;               ///< Unique widget identifier.
  std::wstring m_Name;             ///< Display name.
  WidgetOptions m_Options;         ///< Configuration options.
  HWND m_hWnd = nullptr;           ///< Win32 window handle.
  Tooltip m_Tooltip;               ///< Tooltip manager.
  ZPOSITION m_WindowZPosition;     ///< Current z-order position.
  // ============================================================================
  // Elements
  // ============================================================================

  std::vector<std::unique_ptr<Element>> m_Elements; ///< Owned element instances.
  std::unordered_set<Element *>
      m_TrackedElements; // Flat set of all live element pointers for O(1)
                         // IsTrackedElement
  std::vector<ButtonElement *>
      m_Buttons; // Cached button pointers for O(1) mouse-move iteration
  std::unordered_map<std::wstring, Element *>
      m_ElementIndex; // ID→pointer lookup; kept in sync with m_Elements
  std::unordered_map<std::wstring, LayoutConfig> m_LayoutConfigs;

  // Spatial grid for O(1) hit-testing instead of O(n) linear scan.
  // Cell size chosen so typical widget (200-600px) spans 3-10 cells.
  static const int GRID_CELL_SIZE = 64;
  static const int GRID_THRESHOLD = 32; // Use grid only above this count
  std::unordered_map<int64_t, std::vector<Element *>> m_SpatialGrid;
  void RebuildSpatialGrid();
  struct ElementAnimation {
    std::wstring id;
    std::wstring easing = L"linear";
    DWORD startTick = 0;
    int durationMs = 250;
    int iterationCount = 1;
    int completedIterations = 0;
    bool useKeyframes = false;
    std::vector<float> keyframeOffsets;
    std::vector<std::wstring> keyframeEasings;
    std::vector<AnimationTarget> resolvedStops;
    AnimationTarget from;
    AnimationTarget to;
  };

  struct WindowAnimation {
    std::wstring easing = L"linear";
    DWORD startTick = 0;
    int durationMs = 250;
    int iterationCount = 1;
    int completedIterations = 0;
    bool useKeyframes = false;
    std::vector<float> keyframeOffsets;
    std::vector<std::wstring> keyframeEasings;
    std::vector<WindowAnimationTarget> resolvedStops;
    WindowAnimationTarget from;
    WindowAnimationTarget to;
  };

  std::vector<ElementAnimation> m_Animations;
  std::vector<WindowAnimation> m_WindowAnimations;
  Element *m_MouseOverElement = nullptr;
  Element *m_CursorElement = nullptr;
  Element *m_TooltipElement = nullptr;
  int m_IsBatchUpdating = 0;

  // Context Menu
  std::vector<MenuItem> m_ContextMenu;
  bool m_ShowDefaultContextMenuItems = true;
  bool m_ContextMenuDisabled = false;

  // Dragging State
  bool m_IsDragging = false;
  bool m_DragThresholdMet = false;
  int m_DragThresholdX = 0; // cached SM_CXDRAG at drag start
  int m_DragThresholdY = 0; // cached SM_CYDRAG at drag start
  POINT m_DragStartCursor = {0, 0};
  POINT m_DragStartWindow = {0, 0};
  bool m_IsElementDragging = false;
  Element *m_DragElement = nullptr;

  // Scrollbar Dragging & Hover State
  enum class ScrollbarHitPart {
    None,
    VerticalTopButton,
    VerticalBottomButton,
    VerticalThumb,
    VerticalTrack,
    HorizontalLeftButton,
    HorizontalRightButton,
    HorizontalThumb,
    HorizontalTrack
  };
  struct ScrollbarHitResult {
    Element *container = nullptr;
    ScrollbarHitPart part = ScrollbarHitPart::None;
    int trackLength = 0;
    int thumbLength = 0;
    int thumbOffset = 0;
    int maxScroll = 0;
  };
  bool HitTestContainerScrollbar(int x, int y, ScrollbarHitResult &result);
  bool m_IsScrollbarDragging = false;
  Element *m_ScrollbarDragContainer = nullptr;
  Element *m_ScrollbarHoverContainer = nullptr;
  ScrollbarHitPart m_ScrollbarHoverPart = ScrollbarHitPart::None;
  ScrollbarHitPart m_ScrollbarActivePart = ScrollbarHitPart::None;
  bool m_ScrollbarDragIsVertical = true;
  int m_ScrollbarDragStartMouse = 0;
  int m_ScrollbarDragStartScroll = 0;
  int m_ScrollbarDragTrackLength = 0;
  int m_ScrollbarDragThumbLength = 0;
  int m_ScrollbarDragMaxScroll = 0;
  int m_ScrollbarDragGrabOffset = 0;

  // Container Swiping / Pan Dragging State
  bool m_IsContainerSwiping = false;
  Element *m_SwipeContainer = nullptr;
  Element *m_SwipeTargetElement = nullptr;
  POINT m_SwipeStartPos = {0, 0};
  DWORD m_SwipeStartTime = 0;
  int m_SwipeStartScrollX = 0;
  int m_SwipeStartScrollY = 0;

  bool m_IsMouseOverWidget = false;
  bool m_IsMinimized = false;
  bool m_IsMaximized = false;
  WidgetRect4 m_PreMaximizeBounds = {0, 0, 0, 0};

  // Resizing State
  bool m_IsResizing = false;
  WidgetResizeEdge m_ResizeEdge = WidgetResizeEdge::None;
  POINT m_ResizeStartCursor = {0, 0};
  WidgetRect4 m_ResizeStartWindow = {0, 0, 0, 0};
  static WidgetResizeEdge GetResizeEdgeAt(int x, int y, int w, int h);
  static LPCWSTR GetCursorForResizeEdge(WidgetResizeEdge edge);

  CursorManager m_CursorManager;
  HICON m_ToolbarIconHandle = nullptr;
  bool m_ToolbarIconOwned = false;

  // Text Selection State
  TextElement *m_TextSelectionElement = nullptr;

  uint64_t m_InstanceId = 0;

  // Input box focus state
  InputBoxElement *m_FocusedInputBox = nullptr;
  std::unique_ptr<ColorPickerPopup> m_ColorPickerPopup;
  Microsoft::WRL::ComPtr<WidgetDropTarget> m_DropTarget;

  void ApplyToolbarStyle();
  void ApplyToolbarIcon();
  void ApplyToolbarTitle();
  void DestroyToolbarIcon();
  void ReleaseRenderSurface();

  // Rendering
  Microsoft::WRL::ComPtr<ID2D1DeviceContext> m_pContext;
  GeneralImage m_BackgroundImage;
  HDC m_hRenderMemDc = nullptr;
  HBITMAP m_hRenderBitmap = nullptr;
  HBITMAP m_hRenderOldBitmap = nullptr;
  void *m_pRenderBitmapBits = nullptr;
  int m_RenderBitmapW = 0;
  int m_RenderBitmapH = 0;

  static const UINT_PTR TIMER_TOPMOST = 2;
  static const UINT_PTR TIMER_TOOLTIP = 3;
  static const UINT_PTR TIMER_CTRL_OVERRIDE = 4;
  static const UINT_PTR TIMER_CARET = 5;
  static const UINT_PTR TIMER_ANIMATION = 6;
  static const UINT_PTR TIMER_SCROLLBAR_BUTTON = 7;
};

#endif
