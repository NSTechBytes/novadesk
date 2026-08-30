/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "WidgetWindowEventBindings.h"
#include "WidgetUiBindings.h"

#include <algorithm>
#include <cwctype>
#include <cstdint>
#include <string>
#include <vector>

#include "../../domain/Widget.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include "../parser/PropertyParser.h"

extern std::vector<Widget *> widgets;

namespace novadesk::scripting::quickjs
{
    namespace
    {
        JSClassID g_widgetWindowClassId = 0;
        int g_nextContextMenuId = 1;

        JSValue ThrowTypeError(JSContext *ctx, const char *method, const char *usage)
        {
            return JS_ThrowTypeError(ctx, "%s: %s", method, usage);
        }

        Widget *GetWidget(JSContext *ctx, JSValueConst thisVal)
        {
            (void)ctx;
            WidgetWrapper *wrapper = static_cast<WidgetWrapper *>(JS_GetOpaque(thisVal, g_widgetWindowClassId));
            if (!wrapper || !wrapper->widget || wrapper->widget->GetInstanceId() != wrapper->instanceId)
                return nullptr;
            return Widget::IsValid(wrapper->widget) ? wrapper->widget : nullptr;
        }

        Widget *GetWidgetRaw(JSValueConst thisVal)
        {
            WidgetWrapper *wrapper = static_cast<WidgetWrapper *>(JS_GetOpaque(thisVal, g_widgetWindowClassId));
            if (!wrapper || !wrapper->widget || wrapper->widget->GetInstanceId() != wrapper->instanceId)
                return nullptr;
            return wrapper->widget;
        }

        bool DestroyWidgetInstance(Widget *widget, bool skipCloseEvent)
        {
            if (!widget)
                return false;

            if (!skipCloseEvent)
            {
                JSEngine::TriggerWidgetEvent(widget, "close");
                if (!Widget::IsValid(widget))
                    return true;
            }

            {
                std::lock_guard<std::mutex> lock(Widget::s_WidgetMutex);
                auto it = std::find(widgets.begin(), widgets.end(), widget);
                if (it == widgets.end())
                    return false;
                widgets.erase(it);
            }
            // Lock released before delete: the destructor calls DestroyWindow
            // which dispatches WM_DESTROY synchronously; holding the lock there
            // would deadlock.
            delete widget;
            return true;
        }

        JSValue JsWidgetWindowOn(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;

            if (argc < 2 || !JS_IsFunction(ctx, argv[1]))
            {
                return ThrowTypeError(ctx, "on", "expected (eventName, callback)");
            }

            const char *eventName = JS_ToCString(ctx, argv[0]);
            if (!eventName || !*eventName)
            {
                if (eventName)
                    JS_FreeCString(ctx, eventName);
                return ThrowTypeError(ctx, "on", "eventName must be non-empty string");
            }

            const std::string event(eventName);
            JS_FreeCString(ctx, eventName);

            if (!JSEngine::RegisterWidgetEventListener(ctx, widget, event, argv[1]))
            {
                return JS_ThrowInternalError(ctx, "failed to register widget event listener");
            }

            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowSetProperties(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1 || !JS_IsObject(argv[0]))
            {
                return ThrowTypeError(ctx, "setProperties", "expected options object");
            }

            parser::WidgetWindowOptions parsed;
            parser::ParseWidgetWindowOptions(ctx, argv[0], parsed);

            if (parsed.hasMinWidth)
                widget->SetMinWidth(parsed.minWidth);
            if (parsed.hasMinHeight)
                widget->SetMinHeight(parsed.minHeight);

            if (parsed.hasX || parsed.hasY || parsed.hasWidth || parsed.hasHeight)
            {
                const int x = parsed.hasX ? parsed.x : CW_USEDEFAULT;
                const int y = parsed.hasY ? parsed.y : CW_USEDEFAULT;
                const int w = parsed.hasWidth ? parsed.width : -1;
                const int h = parsed.hasHeight ? parsed.height : -1;
                widget->SetWindowPosition(x, y, w, h);
            }

            if (parsed.hasBackgroundColor)
                widget->SetBackgroundColor(parsed.backgroundColor);
            if (parsed.hasBackgroundImageFallback)
                widget->SetBackgroundImageFallback(parsed.backgroundImageFallback);
            if (parsed.hasBackgroundImage || parsed.hasBackgroundImageSize || parsed.hasBackgroundImagePosition)
            {
                const WidgetOptions &current = widget->GetOptions();
                BackgroundImageSize size = current.backgroundImageSize;
                BackgroundImagePosition position = current.backgroundImagePosition;
                if (parsed.hasBackgroundImageSize)
                {
                    if (parsed.backgroundImageSizeIsExplicit)
                    {
                        size.type = BackgroundImageSize::Type::Explicit;
                        size.width = parsed.backgroundImageSizeWidth;
                        size.height = parsed.backgroundImageSizeHeight;
                        size.hasWidth = parsed.backgroundImageSizeHasWidth;
                        size.hasHeight = parsed.backgroundImageSizeHasHeight;
                    }
                    else
                    {
                        size.type = parsed.backgroundImageSize == L"contain" ? BackgroundImageSize::Type::Contain :
                            parsed.backgroundImageSize == L"stretch" ? BackgroundImageSize::Type::Stretch : BackgroundImageSize::Type::Cover;
                    }
                }
                if (parsed.hasBackgroundImagePosition)
                {
                    if (parsed.backgroundImagePositionIsExplicit)
                    {
                        position.type = BackgroundImagePosition::Type::Explicit;
                        position.x = parsed.backgroundImagePositionX;
                        position.y = parsed.backgroundImagePositionY;
                    }
                    else
                    {
                        position.type = BackgroundImagePosition::Type::Keyword;
                        position.keyword = parsed.backgroundImagePosition;
                    }
                }
                widget->SetBackgroundImage(
                    parsed.hasBackgroundImage ? parsed.backgroundImage : current.backgroundImage,
                    size,
                    position);
            }
            if (parsed.hasWindowOpacity)
                widget->SetWindowOpacity(parsed.windowOpacity);
            if (parsed.hasDraggable)
                widget->SetDraggable(parsed.draggable);
            if (parsed.hasResizable)
                widget->SetResizable(parsed.resizable);
            if (parsed.hasClickThrough)
                widget->SetClickThrough(parsed.clickThrough);
            if (parsed.hasKeepOnScreen)
                widget->SetKeepOnScreen(parsed.keepOnScreen);
            if (parsed.hasSnapEdges)
                widget->SetSnapEdges(parsed.snapEdges);
            if (parsed.hasShowInToolbar)
                widget->SetShowInToolbar(parsed.showInToolbar);
            if (parsed.hasToolbarTitle)
                widget->SetToolbarTitle(parsed.toolbarTitle);
            if (parsed.hasToolbarIcon)
            {
                std::wstring iconPath = parsed.toolbarIcon;
                if (!iconPath.empty())
                {
                    if (PathUtils::IsPathRelative(iconPath))
                    {
                        std::wstring base = JSEngine::GetCurrentScriptDir();
                        if (base.empty())
                            base = JSEngine::GetEntryScriptDir();
                        if (!base.empty())
                            iconPath = PathUtils::ResolvePath(iconPath, base);
                        else
                            iconPath = PathUtils::ResolvePath(iconPath, PathUtils::GetWidgetsDir());
                    }
                    else
                    {
                        iconPath = PathUtils::NormalizePath(iconPath);
                    }
                }
                widget->SetToolbarIcon(iconPath);
            }
            if (parsed.hasShow)
            {
                if (parsed.show)
                    widget->Show();
                else
                    widget->Hide();
            }
            if (parsed.hasZPos)
                widget->ChangeZPos(static_cast<ZPOSITION>(parsed.zPos));

            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetProperties(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;

            const WidgetOptions &o = widget->GetOptions();
            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "id", JS_NewString(ctx, Utils::ToString(o.id).c_str()));
            JS_SetPropertyStr(ctx, out, "x", JS_NewInt32(ctx, o.x));
            JS_SetPropertyStr(ctx, out, "y", JS_NewInt32(ctx, o.y));
            JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, o.width));
            JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, o.height));
            JS_SetPropertyStr(ctx, out, "minWidth", JS_NewInt32(ctx, o.minWidth));
            JS_SetPropertyStr(ctx, out, "minHeight", JS_NewInt32(ctx, o.minHeight));
            JS_SetPropertyStr(ctx, out, "draggable", JS_NewBool(ctx, o.draggable ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "resizable", JS_NewBool(ctx, o.resizable ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "clickThrough", JS_NewBool(ctx, o.clickThrough ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "keepOnScreen", JS_NewBool(ctx, o.keepOnScreen ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "snapEdges", JS_NewBool(ctx, o.snapEdges ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "showInToolbar", JS_NewBool(ctx, o.showInToolbar ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "toolbarIcon", JS_NewString(ctx, Utils::ToString(o.toolbarIcon).c_str()));
            JS_SetPropertyStr(ctx, out, "toolbarTitle", JS_NewString(ctx, Utils::ToString(o.toolbarTitle).c_str()));
            JS_SetPropertyStr(ctx, out, "show", JS_NewBool(ctx, IsWindowVisible(widget->GetWindow()) ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "windowOpacity", JS_NewInt32(ctx, static_cast<int>(o.windowOpacity)));
            JS_SetPropertyStr(ctx, out, "backgroundColor", JS_NewString(ctx, Utils::ToString(o.backgroundColor).c_str()));
            JS_SetPropertyStr(ctx, out, "backgroundImage", JS_NewString(ctx, Utils::ToString(o.backgroundImage).c_str()));
            JS_SetPropertyStr(ctx, out, "backgroundImageFallback", JS_NewString(ctx, Utils::ToString(o.backgroundImageFallback).c_str()));
            if (o.backgroundImageSize.type == BackgroundImageSize::Type::Explicit)
            {
                JSValue size = JS_NewObject(ctx);
                if (o.backgroundImageSize.hasWidth)
                    JS_SetPropertyStr(ctx, size, "width", JS_NewFloat64(ctx, o.backgroundImageSize.width));
                if (o.backgroundImageSize.hasHeight)
                    JS_SetPropertyStr(ctx, size, "height", JS_NewFloat64(ctx, o.backgroundImageSize.height));
                JS_SetPropertyStr(ctx, out, "backgroundImageSize", size);
            }
            else
            {
                const char *size = o.backgroundImageSize.type == BackgroundImageSize::Type::Contain ? "contain" :
                    o.backgroundImageSize.type == BackgroundImageSize::Type::Stretch ? "stretch" : "cover";
                JS_SetPropertyStr(ctx, out, "backgroundImageSize", JS_NewString(ctx, size));
            }
            if (o.backgroundImagePosition.type == BackgroundImagePosition::Type::Explicit)
            {
                JSValue position = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, position, "x", JS_NewFloat64(ctx, o.backgroundImagePosition.x));
                JS_SetPropertyStr(ctx, position, "y", JS_NewFloat64(ctx, o.backgroundImagePosition.y));
                JS_SetPropertyStr(ctx, out, "backgroundImagePosition", position);
            }
            else
            {
                JS_SetPropertyStr(ctx, out, "backgroundImagePosition", JS_NewString(ctx, Utils::ToString(o.backgroundImagePosition.keyword).c_str()));
            }
            JS_SetPropertyStr(ctx, out, "zPos", JS_NewInt32(ctx, static_cast<int>(o.zPos)));
            JS_SetPropertyStr(ctx, out, "script", JS_NewString(ctx, Utils::ToString(o.scriptPath).c_str()));
            return out;
        }

        JSValue JsWidgetWindowClose(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;

            DestroyWidgetInstance(widget, false);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowDestroy(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            DestroyWidgetInstance(widget, true);
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowShow(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Show();
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowHide(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Hide();
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowIsFocused(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewBool(ctx, 0);
            HWND hWnd = widget->GetWindow();
            return JS_NewBool(ctx, (hWnd && GetFocus() == hWnd) ? 1 : 0);
        }

        JSValue JsWidgetWindowIsVisible(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewBool(ctx, 0);
            HWND hWnd = widget->GetWindow();
            return JS_NewBool(ctx, (hWnd && IsWindowVisible(hWnd)) ? 1 : 0);
        }

        JSValue JsWidgetWindowIsDestroyed(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *raw = GetWidgetRaw(thisVal);
            const bool destroyed = !(raw && Widget::IsValid(raw));
            return JS_NewBool(ctx, destroyed ? 1 : 0);
        }

        JSValue JsWidgetWindowSetBounds(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1 || !JS_IsObject(argv[0]))
            {
                return ThrowTypeError(ctx, "setBounds", "expected bounds object");
            }

            int x = CW_USEDEFAULT;
            int y = CW_USEDEFAULT;
            int w = -1;
            int h = -1;

            int32_t v = 0;
            JSValue xv = JS_GetPropertyStr(ctx, argv[0], "x");
            if (!JS_IsUndefined(xv) && !JS_IsNull(xv) && JS_ToInt32(ctx, &v, xv) == 0) x = static_cast<int>(v);
            JS_FreeValue(ctx, xv);

            JSValue yv = JS_GetPropertyStr(ctx, argv[0], "y");
            if (!JS_IsUndefined(yv) && !JS_IsNull(yv) && JS_ToInt32(ctx, &v, yv) == 0) y = static_cast<int>(v);
            JS_FreeValue(ctx, yv);

            JSValue wv = JS_GetPropertyStr(ctx, argv[0], "width");
            if (!JS_IsUndefined(wv) && !JS_IsNull(wv) && JS_ToInt32(ctx, &v, wv) == 0) w = static_cast<int>(v);
            JS_FreeValue(ctx, wv);

            JSValue hv = JS_GetPropertyStr(ctx, argv[0], "height");
            if (!JS_IsUndefined(hv) && !JS_IsNull(hv) && JS_ToInt32(ctx, &v, hv) == 0) h = static_cast<int>(v);
            JS_FreeValue(ctx, hv);

            widget->SetWindowPosition(x, y, w, h);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetBounds(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            HWND hWnd = widget->GetWindow();
            if (!hWnd)
                return JS_NULL;

            RECT rc{};
            if (!GetWindowRect(hWnd, &rc))
                return JS_NULL;

            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "x", JS_NewInt32(ctx, rc.left));
            JS_SetPropertyStr(ctx, out, "y", JS_NewInt32(ctx, rc.top));
            JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, rc.right - rc.left));
            JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, rc.bottom - rc.top));
            return out;
        }

        JSValue JsWidgetWindowSetSize(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 2)
            {
                return ThrowTypeError(ctx, "setSize", "expected (width, height)");
            }

            int32_t w = 0;
            int32_t h = 0;
            if (JS_ToInt32(ctx, &w, argv[0]) != 0 || JS_ToInt32(ctx, &h, argv[1]) != 0)
            {
                return ThrowTypeError(ctx, "setSize", "width/height must be numbers");
            }

            widget->SetWindowPosition(CW_USEDEFAULT, CW_USEDEFAULT, static_cast<int>(w), static_cast<int>(h));
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetSize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            HWND hWnd = widget->GetWindow();
            if (!hWnd)
                return JS_NULL;

            RECT rc{};
            if (!GetWindowRect(hWnd, &rc))
                return JS_NULL;

            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, rc.right - rc.left));
            JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, rc.bottom - rc.top));
            return out;
        }

        JSValue JsWidgetWindowGetBackgroundColor(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            return JS_NewString(ctx, Utils::ToString(widget->GetOptions().backgroundColor).c_str());
        }

        JSValue JsWidgetWindowSetBackgroundColor(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1)
            {
                return ThrowTypeError(ctx, "setBackgroundColor", "expected color string");
            }
            const char *colorUtf8 = JS_ToCString(ctx, argv[0]);
            if (!colorUtf8)
                return JS_EXCEPTION;
            std::wstring color = Utils::ToWString(colorUtf8);
            JS_FreeCString(ctx, colorUtf8);
            widget->SetBackgroundColor(color);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowSetOpacity(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1)
            {
                return ThrowTypeError(ctx, "setOpacity", "expected value");
            }

            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, argv[0]) != 0)
            {
                return ThrowTypeError(ctx, "setOpacity", "value must be number");
            }

            int opacity = 255;
            if (d <= 1.0)
            {
                opacity = static_cast<int>(d * 255.0);
            }
            else if (d <= 100.0)
            {
                opacity = static_cast<int>((d / 100.0) * 255.0);
            }
            else
            {
                opacity = static_cast<int>(d);
            }

            opacity = std::clamp(opacity, 0, 255);
            widget->SetWindowOpacity(static_cast<BYTE>(opacity));
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowSetResizable(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1)
            {
                return ThrowTypeError(ctx, "setResizable", "expected boolean");
            }
            const bool resizable = JS_ToBool(ctx, argv[0]) != 0;
            widget->SetResizable(resizable);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowIsResizable(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewBool(ctx, 0);
            return JS_NewBool(ctx, widget->IsResizable() ? 1 : 0);
        }

        JSValue JsWidgetWindowSetMinWidth(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1)
            {
                return ThrowTypeError(ctx, "setMinWidth", "expected width number");
            }
            int32_t minW = 0;
            if (JS_ToInt32(ctx, &minW, argv[0]) != 0)
            {
                return ThrowTypeError(ctx, "setMinWidth", "width must be number");
            }
            widget->SetMinWidth(static_cast<int>(minW));
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetMinWidth(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewInt32(ctx, 0);
            return JS_NewInt32(ctx, widget->GetMinWidth());
        }

        JSValue JsWidgetWindowSetMinHeight(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1)
            {
                return ThrowTypeError(ctx, "setMinHeight", "expected height number");
            }
            int32_t minH = 0;
            if (JS_ToInt32(ctx, &minH, argv[0]) != 0)
            {
                return ThrowTypeError(ctx, "setMinHeight", "height must be number");
            }
            widget->SetMinHeight(static_cast<int>(minH));
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetMinHeight(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewInt32(ctx, 0);
            return JS_NewInt32(ctx, widget->GetMinHeight());
        }

        JSValue JsWidgetWindowSetMinSize(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 2)
            {
                return ThrowTypeError(ctx, "setMinSize", "expected (minWidth, minHeight)");
            }
            int32_t minW = 0;
            int32_t minH = 0;
            if (JS_ToInt32(ctx, &minW, argv[0]) != 0 || JS_ToInt32(ctx, &minH, argv[1]) != 0)
            {
                return ThrowTypeError(ctx, "setMinSize", "minWidth/minHeight must be numbers");
            }
            widget->SetMinSize(static_cast<int>(minW), static_cast<int>(minH));
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowGetMinSize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, widget->GetMinWidth()));
            JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, widget->GetMinHeight()));
            return out;
        }

        JSValue JsWidgetWindowRefresh(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Refresh();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowSetFocus(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->SetFocus();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowUnFocus(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->UnFocus();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowMinimize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Minimize();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowUnMinimize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->UnMinimize();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowIsMinimized(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewBool(ctx, 0);
            return JS_NewBool(ctx, widget->IsMinimized() ? 1 : 0);
        }

        JSValue JsWidgetWindowMaximize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Maximize();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowRestore(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->Restore();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowToggleMaximize(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->ToggleMaximize();
            return JS_UNDEFINED;
        }

        JSValue JsWidgetWindowIsMaximized(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewBool(ctx, 0);
            return JS_NewBool(ctx, widget->IsMaximized() ? 1 : 0);
        }

        JSValue JsWidgetWindowGetHandle(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            return JS_NewInt64(ctx, static_cast<int64_t>(reinterpret_cast<uintptr_t>(widget->GetWindow())));
        }

        JSValue JsWidgetWindowGetInternalPointer(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NULL;
            return JS_NewInt64(ctx, static_cast<int64_t>(reinterpret_cast<uintptr_t>(widget)));
        }

        JSValue JsWidgetWindowGetTitle(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_NewString(ctx, "");
            return JS_NewString(ctx, Utils::ToString(widget->GetTitle()).c_str());
        }

        bool ParseContextMenuItems(JSContext *ctx, JSValueConst arr, const std::wstring &widgetId, std::vector<MenuItem> &out)
        {
            if (!JS_IsArray(arr))
                return false;

            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
            if (JS_ToUint32(ctx, &len, lenV) != 0)
            {
                JS_FreeValue(ctx, lenV);
                return false;
            }
            JS_FreeValue(ctx, lenV);

            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue itemV = JS_GetPropertyUint32(ctx, arr, i);
                if (!JS_IsObject(itemV))
                {
                    JS_FreeValue(ctx, itemV);
                    continue;
                }

                MenuItem item{};
                item.id = 0;

                JSValue typeV = JS_GetPropertyStr(ctx, itemV, "type");
                const char *typeS = JS_ToCString(ctx, typeV);
                if (typeS && std::string(typeS) == "separator")
                    item.isSeparator = true;
                if (typeS)
                    JS_FreeCString(ctx, typeS);
                JS_FreeValue(ctx, typeV);

                if (!item.isSeparator)
                {
                    JSValue textV = JS_GetPropertyStr(ctx, itemV, "text");
                    const char *textS = JS_ToCString(ctx, textV);
                    if (textS)
                    {
                        item.text = Utils::ToWString(textS);
                        JS_FreeCString(ctx, textS);
                    }
                    JS_FreeValue(ctx, textV);

                    JSValue checkedV = JS_GetPropertyStr(ctx, itemV, "checked");
                    int checked = JS_ToBool(ctx, checkedV);
                    if (checked >= 0)
                        item.checked = (checked != 0);
                    JS_FreeValue(ctx, checkedV);

                    JSValue actionV = JS_GetPropertyStr(ctx, itemV, "action");
                    if (JS_IsFunction(ctx, actionV))
                    {
                        item.id = 2000 + g_nextContextMenuId++;
                        JSEngine::RegisterWidgetContextMenuCallback(ctx, widgetId, item.id, actionV);
                    }
                    JS_FreeValue(ctx, actionV);

                    JSValue childV = JS_GetPropertyStr(ctx, itemV, "items");
                    if (JS_IsArray(childV))
                        ParseContextMenuItems(ctx, childV, widgetId, item.children);
                    JS_FreeValue(ctx, childV);
                }

                out.push_back(std::move(item));
                JS_FreeValue(ctx, itemV);
            }

            return true;
        }

        JSValue JsWidgetWindowSetContextMenu(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1 || !JS_IsArray(argv[0]))
                return ThrowTypeError(ctx, "setContextMenu", "expected items array");

            const std::wstring widgetId = widget->GetOptions().id;
            JSEngine::ClearWidgetContextMenuCallbacks(widgetId);

            std::vector<MenuItem> menu;
            if (!ParseContextMenuItems(ctx, argv[0], widgetId, menu))
            {
                return JS_ThrowTypeError(ctx, "setContextMenu: invalid items");
            }

            widget->SetContextMenu(menu);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowClearContextMenu(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->ClearContextMenu();
            JSEngine::ClearWidgetContextMenuCallbacks(widget->GetOptions().id);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowDisableContextMenu(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            bool disable = true;
            if (argc > 0)
            {
                int b = JS_ToBool(ctx, argv[0]);
                if (b >= 0)
                    disable = (b != 0);
            }
            widget->SetContextMenuDisabled(disable);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowShowDefaultContextMenuItems(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc > 0)
            {
                int b = JS_ToBool(ctx, argv[0]);
                if (b >= 0)
                    widget->SetShowDefaultContextMenuItems(b != 0);
            }
            return JS_DupValue(ctx, thisVal);
        }

        // ── Screen Position Resolver ─────────────────────────────────────────

        struct ScreenArea
        {
            int left = 0;
            int top = 0;
            int right = 0;
            int bottom = 0;
            int width() const { return right - left; }
            int height() const { return bottom - top; }
        };

        static ScreenArea GetWorkAreaForWidget(HWND hWnd)
        {
            ScreenArea area{};
            RECT wa{};
            if (hWnd)
            {
                HMONITOR hMon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                if (hMon)
                {
                    MONITORINFO mi{};
                    mi.cbSize = sizeof(mi);
                    if (GetMonitorInfo(hMon, &mi))
                    {
                        area.left   = mi.rcWork.left;
                        area.top    = mi.rcWork.top;
                        area.right  = mi.rcWork.right;
                        area.bottom = mi.rcWork.bottom;
                        return area;
                    }
                }
            }
            // Fallback to primary work area
            SystemParametersInfo(SPI_GETWORKAREA, 0, &wa, 0);
            area.left   = wa.left;
            area.top    = wa.top;
            area.right  = wa.right;
            area.bottom = wa.bottom;
            return area;
        }

        // Parse optional trailing offset: "keyword + N" or "keyword - N"
        // Returns the base keyword (lowercased, trimmed) and the numeric offset.
        static std::wstring ParseKeywordAndOffset(const std::wstring &expr, float &outOffset)
        {
            outOffset = 0.0f;
            std::wstring trimmed = expr;
            // trim leading/trailing spaces
            auto ltrim = [](std::wstring &s) { s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](wchar_t c) { return !::iswspace(c); })); };
            auto rtrim = [](std::wstring &s) { s.erase(std::find_if(s.rbegin(), s.rend(), [](wchar_t c) { return !::iswspace(c); }).base(), s.end()); };
            ltrim(trimmed);
            rtrim(trimmed);

            // Convert to lowercase
            std::wstring lower = trimmed;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

            // Look for last '+' or '-' that isn't part of the keyword
            // Scan from right for a trailing numeric offset
            size_t plusPos  = lower.rfind(L'+');
            size_t minusPos = lower.rfind(L'-');
            size_t opPos = std::wstring::npos;
            bool negate = false;

            if (plusPos != std::wstring::npos && (minusPos == std::wstring::npos || plusPos > minusPos))
                opPos = plusPos;
            else if (minusPos != std::wstring::npos && minusPos > 0)
            { opPos = minusPos; negate = true; }

            std::wstring keyword = lower;
            if (opPos != std::wstring::npos && opPos > 0)
            {
                std::wstring numPart = lower.substr(opPos + 1);
                ltrim(numPart);
                rtrim(numPart);
                bool isNum = !numPart.empty() && std::all_of(numPart.begin(), numPart.end(), [](wchar_t c) { return ::iswdigit(c) || c == L'.' || c == L'-'; });
                if (isNum)
                {
                    try { outOffset = std::stof(numPart) * (negate ? -1.0f : 1.0f); } catch (...) {}
                    keyword = lower.substr(0, opPos);
                    rtrim(keyword);
                }
            }
            return keyword;
        }

        // Resolve composite position preset (e.g. "bottom-right") into x/y keywords.
        static bool ResolvePositionPreset(const std::wstring &pos, std::wstring &xKeyword, std::wstring &yKeyword)
        {
            std::wstring lower = pos;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

            if (lower == L"top-left"     || lower == L"topleft")     { yKeyword = L"top";    xKeyword = L"left";   return true; }
            if (lower == L"top-center"   || lower == L"top")         { yKeyword = L"top";    xKeyword = L"center"; return true; }
            if (lower == L"top-right"    || lower == L"topright")    { yKeyword = L"top";    xKeyword = L"right";  return true; }
            if (lower == L"center-left"  || lower == L"left")        { yKeyword = L"center"; xKeyword = L"left";   return true; }
            if (lower == L"center"       || lower == L"middle")      { yKeyword = L"center"; xKeyword = L"center"; return true; }
            if (lower == L"center-right" || lower == L"right")       { yKeyword = L"center"; xKeyword = L"right";  return true; }
            if (lower == L"bottom-left"  || lower == L"bottomleft")  { yKeyword = L"bottom"; xKeyword = L"left";   return true; }
            if (lower == L"bottom-center"|| lower == L"bottom")      { yKeyword = L"bottom"; xKeyword = L"center"; return true; }
            if (lower == L"bottom-right" || lower == L"bottomright") { yKeyword = L"bottom"; xKeyword = L"right";  return true; }
            return false;
        }

        static float ResolveXKeyword(const std::wstring &keyword, const ScreenArea &wa, int winW, float offset)
        {
            if (keyword == L"left")            return static_cast<float>(wa.left) + offset;
            if (keyword == L"right")           return static_cast<float>(wa.right - winW) + offset;
            if (keyword == L"center" || keyword == L"middle")
                return static_cast<float>(wa.left + (wa.width() - winW) / 2) + offset;
            if (keyword == L"offscreen-left")  return static_cast<float>(wa.left - winW) + offset;
            if (keyword == L"offscreen-right") return static_cast<float>(wa.right) + offset;
            return offset; // unknown keyword: use offset as-is
        }

        static float ResolveYKeyword(const std::wstring &keyword, const ScreenArea &wa, int winH, float offset)
        {
            if (keyword == L"top")             return static_cast<float>(wa.top) + offset;
            if (keyword == L"bottom")          return static_cast<float>(wa.bottom - winH) + offset;
            if (keyword == L"center" || keyword == L"middle")
                return static_cast<float>(wa.top + (wa.height() - winH) / 2) + offset;
            if (keyword == L"offscreen-top")   return static_cast<float>(wa.top - winH) + offset;
            if (keyword == L"offscreen-bottom")return static_cast<float>(wa.bottom) + offset;
            return offset;
        }

        // Resolve string expressions and position presets into concrete x/y on a target.
        static void ResolveWindowTargetExpressions(
            Widget::WindowAnimationTarget &target,
            const ScreenArea &wa,
            int winW, int winH,
            bool hasXExpr, const std::wstring &xExpr,
            bool hasYExpr, const std::wstring &yExpr,
            bool hasPosition, const std::wstring &position,
            float offsetX, float offsetY)
        {
            if (hasPosition && !position.empty())
            {
                std::wstring xKw, yKw;
                if (ResolvePositionPreset(position, xKw, yKw))
                {
                    target.hasX = true;
                    target.x = ResolveXKeyword(xKw, wa, winW, offsetX);
                    target.hasY = true;
                    target.y = ResolveYKeyword(yKw, wa, winH, offsetY);
                }
            }

            if (hasXExpr && !xExpr.empty())
            {
                float exprOffset = 0.0f;
                const std::wstring kw = ParseKeywordAndOffset(xExpr, exprOffset);
                target.hasX = true;
                target.x = ResolveXKeyword(kw, wa, winW, exprOffset + offsetX);
            }

            if (hasYExpr && !yExpr.empty())
            {
                float exprOffset = 0.0f;
                const std::wstring kw = ParseKeywordAndOffset(yExpr, exprOffset);
                target.hasY = true;
                target.y = ResolveYKeyword(kw, wa, winH, exprOffset + offsetY);
            }
        }

        // ── Target Builders ─────────────────────────────────────────────────

        static Widget::WindowAnimationTarget BuildWindowAnimationTarget(
            const PropertyParser::WindowAnimationOptions &options,
            bool from,
            const Widget *widget)
        {
            Widget::WindowAnimationTarget target{};
            if (from)
            {
                target.hasX = options.fromHasX;
                target.x = options.fromX;
                target.hasY = options.fromHasY;
                target.y = options.fromY;
                target.hasWidth = options.fromHasWidth;
                target.width = options.fromWidth;
                target.hasHeight = options.fromHasHeight;
                target.height = options.fromHeight;
                target.hasOpacity = options.fromHasOpacity;
                target.opacity = options.fromOpacity;
                target.hasBackgroundColor = options.fromHasBackgroundColor;
                target.bgColorR = options.fromBgColorR;
                target.bgColorG = options.fromBgColorG;
                target.bgColorB = options.fromBgColorB;
                target.bgAlpha = options.fromBgAlpha;
            }
            else
            {
                target.hasX = options.hasX;
                target.x = options.x;
                target.hasY = options.hasY;
                target.y = options.y;
                target.hasWidth = options.hasWidth;
                target.width = options.width;
                target.hasHeight = options.hasHeight;
                target.height = options.height;
                target.hasOpacity = options.hasOpacity;
                target.opacity = options.opacity;
                target.hasBackgroundColor = options.hasBackgroundColor;
                target.bgColorR = options.bgColorR;
                target.bgColorG = options.bgColorG;
                target.bgColorB = options.bgColorB;
                target.bgAlpha = options.bgAlpha;
            }

            // Resolve string expressions
            if (widget)
            {
                const ScreenArea wa = GetWorkAreaForWidget(widget->GetWindow());
                const int winW = options.hasWidth  ? static_cast<int>(from ? options.fromWidth  : options.width)  : widget->GetOptions().width;
                const int winH = options.hasHeight ? static_cast<int>(from ? options.fromHeight : options.height) : widget->GetOptions().height;

                if (from)
                {
                    ResolveWindowTargetExpressions(
                        target, wa, winW, winH,
                        options.fromHasXExpr, options.fromXExpr,
                        options.fromHasYExpr, options.fromYExpr,
                        options.fromHasPosition, options.fromPosition,
                        options.fromOffsetX, options.fromOffsetY);
                }
                else
                {
                    ResolveWindowTargetExpressions(
                        target, wa, winW, winH,
                        options.hasXExpr, options.xExpr,
                        options.hasYExpr, options.yExpr,
                        options.hasPosition, options.position,
                        options.offsetX, options.offsetY);
                }
            }

            return target;
        }

        static std::vector<Widget::WindowAnimationKeyframe> BuildWindowKeyframes(
            const PropertyParser::WindowAnimationOptions &options,
            const Widget *widget)
        {
            ScreenArea wa{};
            if (widget)
                wa = GetWorkAreaForWidget(widget->GetWindow());

            std::vector<Widget::WindowAnimationKeyframe> keyframes;
            keyframes.reserve(options.keyframes.size());
            for (const auto &kf : options.keyframes)
            {
                Widget::WindowAnimationKeyframe item{};
                item.offset = kf.offset;
                item.easing = kf.easing;
                item.values.hasX = kf.hasX;
                item.values.x = kf.x;
                item.values.hasY = kf.hasY;
                item.values.y = kf.y;
                item.values.hasWidth = kf.hasWidth;
                item.values.width = kf.width;
                item.values.hasHeight = kf.hasHeight;
                item.values.height = kf.height;
                item.values.hasOpacity = kf.hasOpacity;
                item.values.opacity = kf.opacity;
                item.values.hasBackgroundColor = kf.hasBackgroundColor;
                item.values.bgColorR = kf.bgColorR;
                item.values.bgColorG = kf.bgColorG;
                item.values.bgColorB = kf.bgColorB;
                item.values.bgAlpha = kf.bgAlpha;

                // Resolve string expressions for this keyframe
                if (widget)
                {
                    const int winW = kf.hasWidth  ? static_cast<int>(kf.width)  : widget->GetOptions().width;
                    const int winH = kf.hasHeight ? static_cast<int>(kf.height) : widget->GetOptions().height;
                    ResolveWindowTargetExpressions(
                        item.values, wa, winW, winH,
                        kf.hasXExpr, kf.xExpr,
                        kf.hasYExpr, kf.yExpr,
                        kf.hasPosition, kf.position,
                        kf.offsetX, kf.offsetY);
                }

                keyframes.push_back(std::move(item));
            }
            return keyframes;
        }


        JSValue JsWidgetWindowAnimate(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            if (argc < 1 || !JS_IsObject(argv[0]))
                return ThrowTypeError(ctx, "animate", "expected options object");

            PropertyParser::WindowAnimationOptions options;
            PropertyParser::ParseWindowAnimationOptions(ctx, argv[0], options);

            if (!options.HasAnyToProps())
                return ThrowTypeError(ctx, "animate", "to or keyframes must include at least one supported property");

            if (options.iterationCountInvalid)
                return ThrowTypeError(ctx, "animate", "iterationCount must be at least 1 or 'infinite'");

            if (options.keyframesInvalid)
            {
                const std::string msg = Utils::ToString(options.keyframesError.empty() ? L"invalid keyframes" : options.keyframesError);
                return ThrowTypeError(ctx, "animate", msg.c_str());
            }

            if (options.tweenInvalid)
            {
                const std::string msg = Utils::ToString(options.tweenError.empty() ? L"invalid from/to" : options.tweenError);
                return ThrowTypeError(ctx, "animate", msg.c_str());
            }

            int iterationCount = options.iterationCount;
            if (options.iterationInfinite)
                iterationCount = -1;

            if (options.hasKeyframes)
            {
                const std::vector<Widget::WindowAnimationKeyframe> keyframes = BuildWindowKeyframes(options, widget);
                widget->StartWindowKeyframeAnimation(keyframes, options.duration, options.easing, iterationCount);
                return JS_DupValue(ctx, thisVal);
            }

            const Widget::WindowAnimationTarget to = BuildWindowAnimationTarget(options, false, widget);
            const Widget::WindowAnimationTarget from = BuildWindowAnimationTarget(options, true, widget);
            widget->StartWindowAnimation(to, from, options.duration, options.easing, iterationCount);
            return JS_DupValue(ctx, thisVal);
        }

        JSValue JsWidgetWindowStopAnimation(JSContext *ctx, JSValueConst thisVal, int, JSValueConst *)
        {
            Widget *widget = GetWidget(ctx, thisVal);
            if (!widget)
                return JS_UNDEFINED;
            widget->StopWindowAnimations();
            return JS_DupValue(ctx, thisVal);
        }

        const JSCFunctionListEntry kWidgetWindowEventFuncs[] = {
            JS_CFUNC_DEF("animate", 1, JsWidgetWindowAnimate),
            JS_CFUNC_DEF("stopAnimation", 0, JsWidgetWindowStopAnimation),
            JS_CFUNC_DEF("setProperties", 1, JsWidgetWindowSetProperties),
            JS_CFUNC_DEF("getProperties", 0, JsWidgetWindowGetProperties),
            JS_CFUNC_DEF("close", 0, JsWidgetWindowClose),
            JS_CFUNC_DEF("destroy", 0, JsWidgetWindowDestroy),
            JS_CFUNC_DEF("show", 0, JsWidgetWindowShow),
            JS_CFUNC_DEF("hide", 0, JsWidgetWindowHide),
            JS_CFUNC_DEF("isFocused", 0, JsWidgetWindowIsFocused),
            JS_CFUNC_DEF("isVisible", 0, JsWidgetWindowIsVisible),
            JS_CFUNC_DEF("isDestroyed", 0, JsWidgetWindowIsDestroyed),
            JS_CFUNC_DEF("setBounds", 1, JsWidgetWindowSetBounds),
            JS_CFUNC_DEF("getBounds", 0, JsWidgetWindowGetBounds),
            JS_CFUNC_DEF("setSize", 2, JsWidgetWindowSetSize),
            JS_CFUNC_DEF("getSize", 0, JsWidgetWindowGetSize),
            JS_CFUNC_DEF("getBackgroundColor", 0, JsWidgetWindowGetBackgroundColor),
            JS_CFUNC_DEF("setBackgroundColor", 1, JsWidgetWindowSetBackgroundColor),
            JS_CFUNC_DEF("setOpacity", 1, JsWidgetWindowSetOpacity),
            JS_CFUNC_DEF("refresh", 0, JsWidgetWindowRefresh),
            JS_CFUNC_DEF("setResizable", 1, JsWidgetWindowSetResizable),
            JS_CFUNC_DEF("isResizable", 0, JsWidgetWindowIsResizable),
            JS_CFUNC_DEF("setMinWidth", 1, JsWidgetWindowSetMinWidth),
            JS_CFUNC_DEF("getMinWidth", 0, JsWidgetWindowGetMinWidth),
            JS_CFUNC_DEF("setMinHeight", 1, JsWidgetWindowSetMinHeight),
            JS_CFUNC_DEF("getMinHeight", 0, JsWidgetWindowGetMinHeight),
            JS_CFUNC_DEF("setMinSize", 2, JsWidgetWindowSetMinSize),
            JS_CFUNC_DEF("getMinSize", 0, JsWidgetWindowGetMinSize),
            JS_CFUNC_DEF("setFocus", 0, JsWidgetWindowSetFocus),
            JS_CFUNC_DEF("unFocus", 0, JsWidgetWindowUnFocus),
            JS_CFUNC_DEF("minimize", 0, JsWidgetWindowMinimize),
            JS_CFUNC_DEF("unMinimize", 0, JsWidgetWindowUnMinimize),
            JS_CFUNC_DEF("maximize", 0, JsWidgetWindowMaximize),
            JS_CFUNC_DEF("restore", 0, JsWidgetWindowRestore),
            JS_CFUNC_DEF("toggleMaximize", 0, JsWidgetWindowToggleMaximize),
            JS_CFUNC_DEF("isMaximized", 0, JsWidgetWindowIsMaximized),
            JS_CFUNC_DEF("isMinimized", 0, JsWidgetWindowIsMinimized),
            JS_CFUNC_DEF("getHandle", 0, JsWidgetWindowGetHandle),
            JS_CFUNC_DEF("getInternalPointer", 0, JsWidgetWindowGetInternalPointer),
            JS_CFUNC_DEF("getTitle", 0, JsWidgetWindowGetTitle),
            JS_CFUNC_DEF("on", 2, JsWidgetWindowOn),
            JS_CFUNC_DEF("setContextMenu", 1, JsWidgetWindowSetContextMenu),
            JS_CFUNC_DEF("clearContextMenu", 0, JsWidgetWindowClearContextMenu),
            JS_CFUNC_DEF("disableContextMenu", 1, JsWidgetWindowDisableContextMenu),
            JS_CFUNC_DEF("showDefaultContextMenuItems", 1, JsWidgetWindowShowDefaultContextMenuItems),
        };
    } // namespace

    void InitWidgetWindowEventBindings(JSClassID widgetWindowClassId)
    {
        g_widgetWindowClassId = widgetWindowClassId;
    }

    void AttachWidgetWindowEventMethods(JSContext *ctx, JSValue proto)
    {
        JS_SetPropertyFunctionList(
            ctx,
            proto,
            kWidgetWindowEventFuncs,
            sizeof(kWidgetWindowEventFuncs) / sizeof(kWidgetWindowEventFuncs[0]));
    }

    void InvokeWidgetContextMenuCallback(const std::wstring &, int)
    {
        // Routed via JSEngine::OnWidgetContextCommand.
    }
} // namespace novadesk::scripting::quickjs
