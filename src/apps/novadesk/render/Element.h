/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_ELEMENT_H__
#define __NOVADESK_ELEMENT_H__

#include <windows.h>
#include <objidl.h>
#include <d2d1_1.h>
#include <string>
#include <vector>
#include <wrl/client.h>

// Helper macros for color extraction from COLORREF (0x00BBGGRR)
#ifndef GetRValue
#define GetRValue(rgb) (LOBYTE(rgb))
#endif
#ifndef GetGValue
#define GetGValue(rgb) (LOBYTE((WORD)(rgb) >> 8))
#endif
#ifndef GetBValue
#define GetBValue(rgb) (LOBYTE((rgb) >> 16))
#endif

/**
 * @brief Identifies the concrete type of a renderable element.
 */
enum ElementType {
  ELEMENT_IMAGE,       ///< Bitmap or remote image element.
  ELEMENT_TEXT,        ///< Text label element.
  ELEMENT_BAR,         ///< Horizontal/vertical bar gauge.
  ELEMENT_LINE,        ///< Straight line segment.
  ELEMENT_ROUNDLINE,   ///< Rounded line segment.
  ELEMENT_SHAPE,       ///< Geometric shape (arc, curve, ellipse, etc.).
  ELEMENT_HISTOGRAM,   ///< Histogram chart element.
  ELEMENT_BITMAP,      ///< Raw bitmap element.
  ELEMENT_BUTTON,      ///< Clickable button element.
  ELEMENT_ROTATOR,     ///< Rotating knob or dial element.
  ELEMENT_AREA_GRAPH,  ///< Area graph chart element.
  ELEMENT_LAYOUT_BOX,  ///< Flexbox container element.
  ELEMENT_INPUT_BOX,   ///< Text input field element.
  ELEMENT_COLOR_PICKER ///< Color picker popup element.
};

/**
 * @brief Axis-aligned rectangle with integer coordinates.
 */
struct GfxRect {
  int X, Y, Width, Height;

  /// Default constructor initializes to zero rect.
  GfxRect() : X(0), Y(0), Width(0), Height(0) {}

  /// Constructs a rect with specified position and dimensions.
  GfxRect(int x, int y, int w, int h) : X(x), Y(y), Width(w), Height(h) {}
};

/**
 * @brief Configuration for rendering text drop shadows.
 */
struct TextShadow {
  float offsetX = 0; ///< Horizontal offset of the shadow.
  float offsetY = 0; ///< Vertical offset of the shadow.
  float blur = 0;    ///< Blur radius of the shadow.
  COLORREF color = 0; ///< Shadow color in COLORREF format.
  BYTE alpha = 255;   ///< Shadow opacity (0-255).
};

/**
 * @brief A single color stop within a gradient definition.
 */
struct GradientStop {
  COLORREF color; ///< Color in COLORREF format.
  BYTE alpha;     ///< Opacity (0-255).
  float position; ///< Position along gradient line (0.0 to 1.0).
};

/**
 * @brief Specifies the type of gradient fill.
 */
enum GradientType { 
  GRADIENT_NONE,   ///< No gradient applied.
  GRADIENT_LINEAR, ///< Linear gradient from start to end point.
  GRADIENT_RADIAL  ///< Radial gradient from center outward.
};

/**
 * @brief Complete gradient configuration for fills and strokes.
 */
struct GradientInfo {
  GradientType type = GRADIENT_NONE;           ///< Gradient type.
  std::vector<GradientStop> stops;             ///< Color stops along the gradient.
  float angle = 0.0f;                          ///< Rotation angle (for linear gradients).
  std::wstring shape = L"circle";              ///< Shape type (for radial gradients).
};

/**
 * @brief Text case transformation mode.
 */
enum TextCase {
  TEXT_CASE_NORMAL,     ///< No transformation applied.
  TEXT_CASE_UPPER,      ///< Convert to UPPERCASE.
  TEXT_CASE_LOWER,      ///< Convert to lowercase.
  TEXT_CASE_CAPITALIZE, ///< Capitalize first letter of each word.
  TEXT_CASE_SENTENCE    ///< Capitalize first letter of each sentence.
};

/**
 * @brief CSS-like backdrop filter configuration for blur and color effects.
 */
struct BackdropFilter {
  float blur = 0.0f;       ///< Gaussian blur radius.
  float brightness = 1.0f; ///< Brightness multiplier (1.0 = normal).
  float contrast = 1.0f;   ///< Contrast multiplier (1.0 = normal).
  float grayscale = 0.0f;  ///< Grayscale intensity (0.0 = none, 1.0 = full).
  float saturate = 1.0f;   ///< Saturation multiplier (1.0 = normal).
  float sepia = 0.0f;      ///< Sepia tone intensity (0.0 = none, 1.0 = full).
  float hueRotate = 0.0f;  ///< Hue rotation in degrees.
  float invert = 0.0f;     ///< Color inversion intensity (0.0 = none, 1.0 = full).
  float opacity = 1.0f;    ///< Overall opacity (0.0 = transparent, 1.0 = opaque).

  /// @return True if any filter (other than default opacity) is active.
  bool IsActive() const;
};

/**
 * @brief Base class for all renderable elements in Novadesk widgets.
 *
 * @note Instances are owned by Widget and must only be created/destroyed
 *       on the main UI thread. Element handles Direct2D rendering, hit testing,
 *       scroll behavior, and event callback dispatch.
 */
class Element {
public:
  void SetBackdropFilter(const BackdropFilter &filter) {
    m_BackdropFilter = filter;
  }
  const BackdropFilter &GetBackdropFilter() const { return m_BackdropFilter; }

  /**
   * @brief Constructs an element with type, ID, and bounding rectangle.
   *
   * @param type The concrete element type.
   * @param id Unique identifier for this element within the widget.
   * @param x X-coordinate of the element's top-left corner.
   * @param y Y-coordinate of the element's top-left corner.
   * @param width Width in pixels (0 = auto-size).
   * @param height Height in pixels (0 = auto-size).
   */
  Element(ElementType type, const std::wstring &id, int x, int y, int width,
          int height);

  virtual ~Element();

  /**
   * @brief Renders the element using Direct2D.
   *
   * @param context The Direct2D device context for rendering.
   */
  virtual void Render(ID2D1DeviceContext *context) = 0;

  /// @return The concrete element type.
  ElementType GetType() const { return m_Type; }

  /// @return The unique identifier for this element.
  const std::wstring &GetId() const { return m_Id; }

  /// @return X-coordinate of the element's top-left corner.
  int GetX() const { return m_X; }

  /// @return Y-coordinate of the element's top-left corner.
  int GetY() const { return m_Y; }

  /// @return Width in pixels.
  int GetWidth();

  /// @return Height in pixels.
  int GetHeight();

  /// @return True if width was explicitly set (non-zero).
  bool IsWDefined() const { return m_WDefined; }

  /// @return True if height was explicitly set (non-zero).
  bool IsHDefined() const { return m_HDefined; }

  /// Sets the element's position.
  void SetPosition(int x, int y) {
    m_X = x;
    m_Y = y;
  }

  /// Sets the element's size. Width/height <= 0 means auto-size.
  void SetSize(int w, int h) {
    m_Width = w;
    m_Height = h;
    m_WDefined = (w > 0);
    m_HDefined = (h > 0);
  }

  /// @return Auto-calculated width based on content (0 if not supported).
  virtual int GetAutoWidth() { return 0; }

  /// @return Auto-calculated height based on content (0 if not supported).
  virtual int GetAutoHeight() { return 0; }

  /// Sets the owning widget's HWND for Win32 API calls.
  void SetOwnerHWND(HWND hWnd) {
    m_OwnerHWND = hWnd;
    OnOwnerHWNDSet();
  }

  /// @return The owning widget's HWND.
  HWND GetOwnerHWND() const { return m_OwnerHWND; }

  /// Called when the owner HWND is set; override for initialization.
  virtual void OnOwnerHWNDSet() {}

  /// Called when an image download completes for this element.
  virtual void OnImageDownloaded(const std::wstring &url,
                                 const std::vector<BYTE> &buffer) {}

  /// @return The image URL if this element loads remote images.
  virtual std::wstring GetImageUrl() const { return L""; }

  /// @return The bounding rectangle of the element.
  virtual GfxRect GetBounds();

  /// @return The background rendering bounds (may differ from content bounds).
  virtual GfxRect GetBackgroundBounds();

  /**
   * @brief Tests if a point falls within the element's hit area.
   *
   * @param x X-coordinate to test.
   * @param y Y-coordinate to test.
   *
   * @return True if the point hits this element.
   */
  virtual bool HitTest(int x, int y);

  /// Sets the solid background color and alpha.
  void SetSolidColor(COLORREF color, BYTE alpha) {
    m_SolidColor = color;
    m_SolidAlpha = alpha;
    m_HasSolidColor = true;
  }

  /// Sets a gradient background fill.
  void SetSolidGradient(const GradientInfo &gradient) {
    m_SolidGradient = gradient;
    m_HasSolidColor = true;
  }

  /// Sets the corner radius for rounded rectangles.
  void SetCornerRadius(int radius) { m_CornerRadius = radius; }

  /// Configures the bevel (3D edge) effect.
  void SetBevel(int type, int width, COLORREF color, BYTE alpha,
                COLORREF color2, BYTE alpha2) {
    m_BevelType = type;
    m_BevelWidth = width;
    m_BevelColor = color;
    m_BevelAlpha = alpha;
    m_BevelColor2 = color2;
    m_BevelAlpha2 = alpha2;
  }

  /// Sets the primary bevel gradient.
  void SetBevelGradient(const GradientInfo &gradient) {
    m_BevelGradient = gradient;
  }

  /// Sets the secondary bevel gradient.
  void SetBevelGradient2(const GradientInfo &gradient) {
    m_BevelGradient2 = gradient;
  }

  /// Enables or disables anti-aliasing for this element.
  void SetAntiAlias(bool enable) { m_AntiAlias = enable; }

  /// Enables pixel-perfect hit testing (checks alpha channel).
  void SetPixelHitTest(bool enabled) { m_PixelHitTest = enabled; }

  /// @return True if pixel-perfect hit testing is enabled.
  bool GetPixelHitTest() const { return m_PixelHitTest; }

  /// Sets internal padding (space between border and content).
  void SetPadding(int left, int top, int right, int bottom);

  /// Sets the rotation angle in degrees.
  void SetRotate(float angle) { m_Rotate = angle; }

  /// @return The rotation angle in degrees.
  float GetRotate() const { return m_Rotate; }

  /// Sets a 2D affine transformation matrix (6 floats: m11, m12, m21, m22, dx, dy).
  void SetTransformMatrix(const float *matrix) {
    if (matrix) {
      memcpy(m_TransformMatrix, matrix, sizeof(float) * 6);
      m_HasTransformMatrix = true;
    } else {
      m_HasTransformMatrix = false;
    }
  }

  /// @return True if a custom transform matrix is set.
  bool HasTransformMatrix() const { return m_HasTransformMatrix; }

  /// @return Pointer to the 6-float transform matrix.
  const float *GetTransformMatrix() const { return m_TransformMatrix; }

  bool HasSolidColor() const { return m_HasSolidColor; }
  COLORREF GetSolidColor() const { return m_SolidColor; }
  BYTE GetSolidAlpha() const { return m_SolidAlpha; }
  int GetCornerRadius() const { return m_CornerRadius; }

  const GradientInfo &GetSolidGradient() const { return m_SolidGradient; }

  int GetBevelType() const { return m_BevelType; }
  int GetBevelWidth() const { return m_BevelWidth; }
  COLORREF GetBevelColor() const { return m_BevelColor; }
  BYTE GetBevelAlpha() const { return m_BevelAlpha; }
  COLORREF GetBevelColor2() const { return m_BevelColor2; }
  BYTE GetBevelAlpha2() const { return m_BevelAlpha2; }
  const GradientInfo &GetBevelGradient() const { return m_BevelGradient; }
  const GradientInfo &GetBevelGradient2() const { return m_BevelGradient2; }

  int GetPaddingLeft() const { return m_PaddingLeft; }
  int GetPaddingTop() const { return m_PaddingTop; }
  int GetPaddingRight() const { return m_PaddingRight; }
  int GetPaddingBottom() const { return m_PaddingBottom; }

  bool GetAntiAlias() const { return m_AntiAlias; }

  /// Sets the element's visibility.
  void SetShow(bool show) { m_Show = show; }

  /// @return True if the element is visible.
  bool IsVisible() const { return m_Show; }

  /// Sets the container element ID for nested layouts.
  void SetContainerId(const std::wstring &id) { m_ContainerId = id; }
  const std::wstring &GetContainerId() const { return m_ContainerId; }

  /// Sets the group ID for element grouping.
  void SetGroupId(const std::wstring &id) { m_GroupId = id; }
  const std::wstring &GetGroupId() const { return m_GroupId; }

  /// Enables or disables the custom mouse cursor for this element.
  void SetMouseEventCursor(bool enabled) { m_MouseEventCursor = enabled; }
  bool GetMouseEventCursor() const { return m_MouseEventCursor; }

  /// Sets the custom cursor name (loaded from cursors directory).
  void SetMouseEventCursorName(const std::wstring &name) {
    m_MouseEventCursorName = name;
  }
  const std::wstring &GetMouseEventCursorName() const {
    return m_MouseEventCursorName;
  }

  /// Sets the base directory for cursor resources.
  void SetCursorsDir(const std::wstring &dir) { m_CursorsDir = dir; }
  const std::wstring &GetCursorsDir() const { return m_CursorsDir; }

  /// Sets the parent container element.
  void SetContainer(Element *container) { m_ContainerElement = container; }

  /// @return The parent container element, or nullptr if none.
  Element *GetContainer() const { return m_ContainerElement; }

  /// @return True if this element is contained within another element.
  bool IsContained() const { return m_ContainerElement != nullptr; }

  /// Adds a child element to this container.
  void AddContainerItem(Element *item) { m_ContainerItems.push_back(item); }

  /// Removes a child element from this container.
  void RemoveContainerItem(Element *item);

  /// Removes all child elements from this container.
  void ClearContainerItems();

  /// @return Read-only access to the child elements.
  const std::vector<Element *> &GetContainerItems() const {
    return m_ContainerItems;
  }

  /// @return True if this element contains child elements.
  bool IsContainer() const { return !m_ContainerItems.empty(); }

  // ============================================================================
  // Scroll Properties
  // ============================================================================

  int GetScrollX() const { return m_ScrollX; }
  int GetScrollY() const { return m_ScrollY; }
  void SetScrollX(int x);
  void SetScrollY(int y);
  int GetScrollStep() const { return m_ScrollStep; }
  void SetScrollStep(int step) { m_ScrollStep = step > 0 ? step : 1; }
  int GetContentWidth() const { return m_ContentWidth; }
  int GetContentHeight() const { return m_ContentHeight; }
  int GetMaxScrollX() const;
  int GetMaxScrollY() const;
  void RecalcContentExtents();
  bool IsScrollableX() const;
  bool IsScrollableY() const;
  bool IsScrollable() const { return IsScrollableX() || IsScrollableY(); }

  /// Overflow behavior mode.
  enum class OverflowMode { Hidden, Auto, Scroll };

  OverflowMode GetOverflowX() const { return m_OverflowX; }
  OverflowMode GetOverflowY() const { return m_OverflowY; }
  void SetOverflowX(OverflowMode mode) { m_OverflowX = mode; }
  void SetOverflowY(OverflowMode mode) { m_OverflowY = mode; }
  void SetOverflow(const std::wstring &value);

  // ============================================================================
  // Scrollbar Appearance
  // ============================================================================

  bool GetShowScrollbar() const { return m_ShowScrollbar; }
  void SetShowScrollbar(bool show) { m_ShowScrollbar = show; }
  bool GetShowScrollbarX() const { return m_ShowScrollbarX; }
  void SetShowScrollbarX(bool show) { m_ShowScrollbarX = show; }
  bool GetShowScrollbarY() const { return m_ShowScrollbarY; }
  void SetShowScrollbarY(bool show) { m_ShowScrollbarY = show; }
  int GetScrollbarWidth() const { return m_ScrollbarWidth; }
  void SetScrollbarWidth(int w) { m_ScrollbarWidth = w > 0 ? w : 1; }
  int GetScrollbarHoverWidth() const {
    return m_ScrollbarHoverWidth > 0 ? m_ScrollbarHoverWidth : m_ScrollbarWidth;
  }
  void SetScrollbarHoverWidth(int w) { m_ScrollbarHoverWidth = w; }
  float GetScrollbarRadius() const { return m_ScrollbarRadius; }
  void SetScrollbarRadius(float r) { m_ScrollbarRadius = r; }
  float GetScrollbarTrackRadius() const {
    return m_ScrollbarTrackRadius >= 0.0f ? m_ScrollbarTrackRadius
                                          : m_ScrollbarRadius;
  }
  void SetScrollbarTrackRadius(float r) { m_ScrollbarTrackRadius = r; }
  float GetScrollbarInset() const { return m_ScrollbarInset; }
  void SetScrollbarInset(float inset) {
    m_ScrollbarInset = (inset >= 0.0f ? inset : 0.0f);
  }
  float GetScrollbarMinThumbLength() const { return m_ScrollbarMinThumbLength; }
  void SetScrollbarMinThumbLength(float minLen) {
    m_ScrollbarMinThumbLength = (minLen >= 4.0f ? minLen : 4.0f);
  }

  /// Returns the scrollbar thumb color.
  COLORREF GetScrollbarColor() const { return m_ScrollbarColor; }
  BYTE GetScrollbarAlpha() const { return m_ScrollbarAlpha; }
  void SetScrollbarColor(COLORREF color, BYTE alpha) {
    m_ScrollbarColor = color;
    m_ScrollbarAlpha = alpha;
  }

  bool HasScrollbarHoverColor() const { return m_HasScrollbarHoverColor; }
  COLORREF GetScrollbarHoverColor() const {
    return m_HasScrollbarHoverColor ? m_ScrollbarHoverColor : m_ScrollbarColor;
  }
  BYTE GetScrollbarHoverAlpha() const {
    return m_HasScrollbarHoverColor
               ? m_ScrollbarHoverAlpha
               : (BYTE)(std::min)(255, (int)m_ScrollbarAlpha + 50);
  }
  void SetScrollbarHoverColor(COLORREF color, BYTE alpha) {
    m_ScrollbarHoverColor = color;
    m_ScrollbarHoverAlpha = alpha;
    m_HasScrollbarHoverColor = true;
  }

  bool HasScrollbarActiveColor() const { return m_HasScrollbarActiveColor; }
  COLORREF GetScrollbarActiveColor() const {
    return m_HasScrollbarActiveColor ? m_ScrollbarActiveColor
                                     : GetScrollbarHoverColor();
  }
  BYTE GetScrollbarActiveAlpha() const {
    return m_HasScrollbarActiveColor
               ? m_ScrollbarActiveAlpha
               : (BYTE)(std::min)(255, (int)GetScrollbarHoverAlpha() + 40);
  }
  void SetScrollbarActiveColor(COLORREF color, BYTE alpha) {
    m_ScrollbarActiveColor = color;
    m_ScrollbarActiveAlpha = alpha;
    m_HasScrollbarActiveColor = true;
  }

  /// Returns the scrollbar track background color.
  COLORREF GetScrollbarTrackColor() const { return m_ScrollbarTrackColor; }
  BYTE GetScrollbarTrackAlpha() const { return m_ScrollbarTrackAlpha; }
  void SetScrollbarTrackColor(COLORREF color, BYTE alpha) {
    m_ScrollbarTrackColor = color;
    m_ScrollbarTrackAlpha = alpha;
  }

  // ============================================================================
  // Scrollbar Arrow Buttons
  // ============================================================================

  bool GetShowScrollbarButtons() const { return m_ShowScrollbarButtons; }
  void SetShowScrollbarButtons(bool show) { m_ShowScrollbarButtons = show; }
  float GetScrollbarButtonSize() const {
    return m_ScrollbarButtonSize > 0.0f ? m_ScrollbarButtonSize
                                        : (float)GetScrollbarWidth();
  }
  void SetScrollbarButtonSize(float size) {
    m_ScrollbarButtonSize = size > 0.0f ? size : 0.0f;
  }
  float GetScrollbarButtonRadius() const {
    return m_ScrollbarButtonRadius >= 0.0f ? m_ScrollbarButtonRadius : 0.0f;
  }
  void SetScrollbarButtonRadius(float r) { m_ScrollbarButtonRadius = r; }

  /// Returns the scrollbar arrow color (defaults to scrollbar color if not set).
  COLORREF GetScrollbarArrowColor() const {
    return m_HasScrollbarArrowColor ? m_ScrollbarArrowColor
                                    : GetScrollbarColor();
  }
  BYTE GetScrollbarArrowAlpha() const {
    return m_HasScrollbarArrowColor ? m_ScrollbarArrowAlpha
                                    : GetScrollbarAlpha();
  }
  void SetScrollbarArrowColor(COLORREF color, BYTE alpha) {
    m_ScrollbarArrowColor = color;
    m_ScrollbarArrowAlpha = alpha;
    m_HasScrollbarArrowColor = true;
  }

  COLORREF GetScrollbarArrowHoverColor() const {
    return m_HasScrollbarArrowHoverColor ? m_ScrollbarArrowHoverColor
                                         : GetScrollbarHoverColor();
  }
  BYTE GetScrollbarArrowHoverAlpha() const {
    return m_HasScrollbarArrowHoverColor ? m_ScrollbarArrowHoverAlpha
                                         : GetScrollbarHoverAlpha();
  }
  void SetScrollbarArrowHoverColor(COLORREF color, BYTE alpha) {
    m_ScrollbarArrowHoverColor = color;
    m_ScrollbarArrowHoverAlpha = alpha;
    m_HasScrollbarArrowHoverColor = true;
  }

  COLORREF GetScrollbarArrowActiveColor() const {
    return m_HasScrollbarArrowActiveColor ? m_ScrollbarArrowActiveColor
                                          : GetScrollbarActiveColor();
  }
  BYTE GetScrollbarArrowActiveAlpha() const {
    return m_HasScrollbarArrowActiveColor ? m_ScrollbarArrowActiveAlpha
                                          : GetScrollbarActiveAlpha();
  }
  void SetScrollbarArrowActiveColor(COLORREF color, BYTE alpha) {
    m_ScrollbarArrowActiveColor = color;
    m_ScrollbarArrowActiveAlpha = alpha;
    m_HasScrollbarArrowActiveColor = true;
  }

  /// Returns the scrollbar button background color.
  COLORREF GetScrollbarButtonBgColor() const {
    return m_ScrollbarButtonBgColor;
  }
  BYTE GetScrollbarButtonBgAlpha() const { return m_ScrollbarButtonBgAlpha; }
  void SetScrollbarButtonBgColor(COLORREF color, BYTE alpha) {
    m_ScrollbarButtonBgColor = color;
    m_ScrollbarButtonBgAlpha = alpha;
  }

  /// Returns the scrollbar button hover background color.
  COLORREF GetScrollbarButtonHoverBgColor() const {
    return m_ScrollbarButtonHoverBgColor;
  }
  BYTE GetScrollbarButtonHoverBgAlpha() const {
    return m_ScrollbarButtonHoverBgAlpha;
  }
  void SetScrollbarButtonHoverBgColor(COLORREF color, BYTE alpha) {
    m_ScrollbarButtonHoverBgColor = color;
    m_ScrollbarButtonHoverBgAlpha = alpha;
  }

  /// Returns true if the element allows transparent (alpha=0) hit testing.
  virtual bool IsTransparentHit() const { return false; }

  /// Checks if this element handles a specific Windows message.
  bool HasAction(UINT message, WPARAM wParam) const;

  /// @return True if any mouse event callbacks are registered.
  bool HasMouseAction() const;

  /// @return True if drag-start or drag callbacks are registered.
  bool HasDragAction() const;

  /// @return True if drop callbacks are registered.
  bool HasDropAction() const;

  /// @return True if this element is a drop target.
  bool IsDropTarget() const { return m_IsDropTarget || HasDropAction(); }
  void SetDropTarget(bool enable) { m_IsDropTarget = enable; }

  /// @return True if this element is a drag source area.
  bool IsDragArea() const { return m_IsDragArea; }
  void SetDragArea(bool enable) { m_IsDragArea = enable; }

  // ============================================================================
  // Tooltip Properties
  // ============================================================================

  /// Configures the tooltip displayed on hover.
  void SetToolTip(const std::wstring &text, const std::wstring &title = L"",
                  const std::wstring &icon = L"", int maxWidth = 0,
                  int maxHeight = 0, bool balloon = false) {
    m_ToolTipText = text;
    m_ToolTipTitle = title;
    m_ToolTipIcon = icon;
    m_ToolTipMaxWidth = maxWidth;
    m_ToolTipMaxHeight = maxHeight;
    m_ToolTipBalloon = balloon;
  }

  const std::wstring &GetToolTipText() const { return m_ToolTipText; }
  const std::wstring &GetToolTipTitle() const { return m_ToolTipTitle; }
  const std::wstring &GetToolTipIcon() const { return m_ToolTipIcon; }
  int GetToolTipMaxWidth() const { return m_ToolTipMaxWidth; }
  int GetToolTipMaxHeight() const { return m_ToolTipMaxHeight; }
  bool GetToolTipBalloon() const { return m_ToolTipBalloon; }
  bool GetToolTipDisabled() const { return m_ToolTipDisabled; }
  void SetToolTipDisabled(bool disabled) { m_ToolTipDisabled = disabled; }

  /// @return True if tooltip text is set and not disabled.
  bool HasToolTip() const {
    return !m_ToolTipText.empty() && !m_ToolTipDisabled;
  }

  // ============================================================================
  // Mouse Event Callback IDs (initialized to -1 when unregistered)
  // ============================================================================

  int m_OnLeftMouseUpCallbackId = -1;
  int m_OnLeftMouseDownCallbackId = -1;
  int m_OnLeftDoubleClickCallbackId = -1;
  int m_OnRightMouseUpCallbackId = -1;
  int m_OnRightMouseDownCallbackId = -1;
  int m_OnRightDoubleClickCallbackId = -1;
  int m_OnMiddleMouseUpCallbackId = -1;
  int m_OnMiddleMouseDownCallbackId = -1;
  int m_OnMiddleDoubleClickCallbackId = -1;
  int m_OnX1MouseUpCallbackId = -1;
  int m_OnX1MouseDownCallbackId = -1;
  int m_OnX1DoubleClickCallbackId = -1;
  int m_OnX2MouseUpCallbackId = -1;
  int m_OnX2MouseDownCallbackId = -1;
  int m_OnX2DoubleClickCallbackId = -1;
  int m_OnScrollUpCallbackId = -1;
  int m_OnScrollDownCallbackId = -1;
  int m_OnScrollLeftCallbackId = -1;
  int m_OnScrollRightCallbackId = -1;
  int m_OnMouseOverCallbackId = -1;
  int m_OnMouseLeaveCallbackId = -1;
  int m_OnDragStartCallbackId = -1;
  int m_OnDragCallbackId = -1;
  int m_OnDragEndCallbackId = -1;

  // ============================================================================
  // Drop Target Callback IDs
  // ============================================================================

  int m_OnDropCallbackId = -1;
  int m_OnDragEnterCallbackId = -1;
  int m_OnDragOverCallbackId = -1;
  int m_OnDragLeaveCallbackId = -1;
  bool m_IsDropTarget = false;
  bool m_IsDragArea = false;

  bool m_IsMouseOver = false;

protected:
  // ============================================================================
  // Core Properties
  // ============================================================================

  BackdropFilter m_BackdropFilter;
  ElementType m_Type;
  std::wstring m_Id;
  int m_X, m_Y;
  int m_Width, m_Height;
  bool m_WDefined, m_HDefined;

  // ============================================================================
  // Background & Fill Properties
  // ============================================================================

  bool m_HasSolidColor = false;
  COLORREF m_SolidColor = 0;
  BYTE m_SolidAlpha = 0;
  int m_CornerRadius = 0;

  /// Gradient fill configuration.
  GradientInfo m_SolidGradient;

  // ============================================================================
  // Bevel (3D Edge) Properties
  // ============================================================================

  int m_BevelType = 0;
  int m_BevelWidth = 0;
  COLORREF m_BevelColor = RGB(255, 255, 255);
  BYTE m_BevelAlpha = 200;
  COLORREF m_BevelColor2 = RGB(0, 0, 0);
  BYTE m_BevelAlpha2 = 150;
  GradientInfo m_BevelGradient;
  GradientInfo m_BevelGradient2;

  // ============================================================================
  // Rendering Properties
  // ============================================================================

  bool m_AntiAlias = true;
  bool m_PixelHitTest = false;
  bool m_Show = true;

  // ============================================================================
  // Container & Grouping
  // ============================================================================

  std::wstring m_ContainerId;
  std::wstring m_GroupId;
  bool m_MouseEventCursor = true;
  std::wstring m_MouseEventCursorName;
  std::wstring m_CursorsDir;
  Element *m_ContainerElement = nullptr;
  std::vector<Element *> m_ContainerItems;

  // ============================================================================
  // Scroll State
  // ============================================================================

  int m_ScrollX = 0;
  int m_ScrollY = 0;
  int m_ScrollStep = 24;
  int m_ContentWidth = 0;
  int m_ContentHeight = 0;
  OverflowMode m_OverflowX = OverflowMode::Hidden;
  OverflowMode m_OverflowY = OverflowMode::Hidden;

  // ============================================================================
  // Scrollbar Appearance State
  // ============================================================================

  bool m_ShowScrollbar = true;
  bool m_ShowScrollbarX = true;
  bool m_ShowScrollbarY = true;
  int m_ScrollbarWidth = 6;
  int m_ScrollbarHoverWidth = -1;
  float m_ScrollbarRadius = 3.0f;
  float m_ScrollbarTrackRadius = -1.0f;
  float m_ScrollbarInset = 2.0f;
  float m_ScrollbarMinThumbLength = 20.0f;
  COLORREF m_ScrollbarColor = RGB(255, 255, 255);
  BYTE m_ScrollbarAlpha = 100;
  bool m_HasScrollbarHoverColor = false;
  COLORREF m_ScrollbarHoverColor = RGB(255, 255, 255);
  BYTE m_ScrollbarHoverAlpha = 180;
  bool m_HasScrollbarActiveColor = false;
  COLORREF m_ScrollbarActiveColor = RGB(255, 255, 255);
  BYTE m_ScrollbarActiveAlpha = 240;
  COLORREF m_ScrollbarTrackColor = RGB(0, 0, 0);
  BYTE m_ScrollbarTrackAlpha = 0;
  bool m_ShowScrollbarButtons = false;
  float m_ScrollbarButtonSize = 14.0f;
  float m_ScrollbarButtonRadius = 2.0f;
  bool m_HasScrollbarArrowColor = false;
  COLORREF m_ScrollbarArrowColor = RGB(255, 255, 255);
  BYTE m_ScrollbarArrowAlpha = 150;
  bool m_HasScrollbarArrowHoverColor = false;
  COLORREF m_ScrollbarArrowHoverColor = RGB(255, 255, 255);
  BYTE m_ScrollbarArrowHoverAlpha = 220;
  bool m_HasScrollbarArrowActiveColor = false;
  COLORREF m_ScrollbarArrowActiveColor = RGB(255, 255, 255);
  BYTE m_ScrollbarArrowActiveAlpha = 255;
  COLORREF m_ScrollbarButtonBgColor = RGB(0, 0, 0);
  BYTE m_ScrollbarButtonBgAlpha = 0;
  COLORREF m_ScrollbarButtonHoverBgColor = RGB(255, 255, 255);
  BYTE m_ScrollbarButtonHoverBgAlpha = 30;

  // ============================================================================
  // Padding
  // ============================================================================

  int m_PaddingLeft = 0;
  int m_PaddingTop = 0;
  int m_PaddingRight = 0;
  int m_PaddingBottom = 0;

  // ============================================================================
  // Transform Properties
  // ============================================================================

  float m_Rotate = 0.0f;
  bool m_HasTransformMatrix = false;
  float m_TransformMatrix[6] = {1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f};

  // ============================================================================
  // Backdrop Filter Cache (avoids per-frame GPU surface reallocation)
  // ============================================================================

  BackdropFilter m_BackdropFilterCache;
  GfxRect m_BackdropFilterBounds{};
  Microsoft::WRL::ComPtr<ID2D1BitmapRenderTarget> m_BackdropFilterTarget;
  Microsoft::WRL::ComPtr<ID2D1Bitmap> m_BackdropFilterBitmap;

  // ============================================================================
  // Tooltip
  // ============================================================================

  std::wstring m_ToolTipText;
  std::wstring m_ToolTipTitle;
  std::wstring m_ToolTipIcon;
  int m_ToolTipMaxWidth = 0;
  int m_ToolTipMaxHeight = 0;
  bool m_ToolTipBalloon = false;
  bool m_ToolTipDisabled = false;

  // ============================================================================
  // Internal Rendering Methods
  // ============================================================================

  void RenderBackground(ID2D1DeviceContext *context);
  void RenderBackdropFilter(ID2D1DeviceContext *context);
  void RenderBevel(ID2D1DeviceContext *context);
  void ApplyRenderTransform(ID2D1DeviceContext *context,
                            D2D1_MATRIX_3X2_F &originalTransform);
  void RestoreRenderTransform(ID2D1DeviceContext *context,
                              const D2D1_MATRIX_3X2_F &originalTransform);

protected:
  HWND m_OwnerHWND = nullptr;
};

#endif
