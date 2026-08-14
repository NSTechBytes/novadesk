/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */


#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../shared/Utils.h"
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <string>
#include <vector>

namespace novadesk::scripting::quickjs::parser
{
    void ParseWidgetWindowOptions(JSContext *ctx, JSValueConst options, WidgetWindowOptions &out)
    {
        if (!JS_IsObject(options))
        {
            return;
        }

        std::wstring id = PropertyParser::Js::GetStringProp(ctx, options, "id");
        if (!id.empty())
            out.id = id;

        int32_t v = 0;
        JSValue widthVal = JS_GetPropertyStr(ctx, options, "width");
        if (!JS_IsUndefined(widthVal) && !JS_IsNull(widthVal) && JS_ToInt32(ctx, &v, widthVal) == 0)
        {
            out.width = static_cast<int>(v);
            out.hasWidth = true;
        }
        JS_FreeValue(ctx, widthVal);

        JSValue heightVal = JS_GetPropertyStr(ctx, options, "height");
        if (!JS_IsUndefined(heightVal) && !JS_IsNull(heightVal) && JS_ToInt32(ctx, &v, heightVal) == 0)
        {
            out.height = static_cast<int>(v);
            out.hasHeight = true;
        }
        JS_FreeValue(ctx, heightVal);

        JSValue xVal = JS_GetPropertyStr(ctx, options, "x");
        if (!JS_IsUndefined(xVal) && JS_ToInt32(ctx, &v, xVal) == 0)
        {
            out.x = static_cast<int>(v);
            out.hasX = true;
        }
        JS_FreeValue(ctx, xVal);

        JSValue yVal = JS_GetPropertyStr(ctx, options, "y");
        if (!JS_IsUndefined(yVal) && JS_ToInt32(ctx, &v, yVal) == 0)
        {
            out.y = static_cast<int>(v);
            out.hasY = true;
        }
        JS_FreeValue(ctx, yVal);

        out.hasDraggable = PropertyParser::Js::GetBoolProp(ctx, options, "draggable", out.draggable);
        out.hasClickThrough = PropertyParser::Js::GetBoolProp(ctx, options, "clickThrough", out.clickThrough);
        out.hasKeepOnScreen = PropertyParser::Js::GetBoolProp(ctx, options, "keepOnScreen", out.keepOnScreen);
        out.hasSnapEdges = PropertyParser::Js::GetBoolProp(ctx, options, "snapEdges", out.snapEdges);
        out.hasShowInToolbar = PropertyParser::Js::GetBoolProp(ctx, options, "showInToolbar", out.showInToolbar);

        JSValue toolbarIconVal = JS_GetPropertyStr(ctx, options, "toolbarIcon");
        if (!JS_IsUndefined(toolbarIconVal) && !JS_IsNull(toolbarIconVal))
        {
            const char *s = JS_ToCString(ctx, toolbarIconVal);
            if (s)
            {
                out.toolbarIcon = Utils::ToWString(s);
                out.hasToolbarIcon = true;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, toolbarIconVal);

        JSValue toolbarTitleVal = JS_GetPropertyStr(ctx, options, "toolbarTitle");
        if (!JS_IsUndefined(toolbarTitleVal) && !JS_IsNull(toolbarTitleVal))
        {
            const char *s = JS_ToCString(ctx, toolbarTitleVal);
            if (s)
            {
                out.toolbarTitle = Utils::ToWString(s);
                out.hasToolbarTitle = true;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, toolbarTitleVal);

        out.hasShow = PropertyParser::Js::GetBoolProp(ctx, options, "show", out.show);

        std::wstring bg = PropertyParser::Js::GetStringProp(ctx, options, "backgroundColor");
        if (!bg.empty())
        {
            out.backgroundColor = bg;
            bool hasBg = false;
            PropertyParser::Js::ParseGradientOrColor(bg, out.color, out.bgAlpha, out.bgGradient, hasBg);
            out.hasBackgroundColor = true;
        }

        JSValue backgroundImageV = JS_GetPropertyStr(ctx, options, "backgroundImage");
        if (!JS_IsUndefined(backgroundImageV) && !JS_IsNull(backgroundImageV))
        {
            const char *value = JS_ToCString(ctx, backgroundImageV);
            if (value)
            {
                out.backgroundImage = Utils::ToWString(value);
                out.hasBackgroundImage = true;
                JS_FreeCString(ctx, value);
            }
        }
        JS_FreeValue(ctx, backgroundImageV);

        JSValue backgroundImageFallbackV = JS_GetPropertyStr(ctx, options, "backgroundImageFallback");
        if (!JS_IsUndefined(backgroundImageFallbackV) && !JS_IsNull(backgroundImageFallbackV))
        {
            const char *value = JS_ToCString(ctx, backgroundImageFallbackV);
            if (value)
            {
                out.backgroundImageFallback = Utils::ToWString(value);
                out.hasBackgroundImageFallback = true;
                JS_FreeCString(ctx, value);
            }
        }
        JS_FreeValue(ctx, backgroundImageFallbackV);

        JSValue backgroundImageSizeV = JS_GetPropertyStr(ctx, options, "backgroundImageSize");
        if (JS_IsString(backgroundImageSizeV))
        {
            const char *value = JS_ToCString(ctx, backgroundImageSizeV);
            if (value)
            {
                std::wstring backgroundImageSize = Utils::ToWString(value);
                std::transform(backgroundImageSize.begin(), backgroundImageSize.end(), backgroundImageSize.begin(), ::towlower);
                if (backgroundImageSize == L"cover" || backgroundImageSize == L"contain" || backgroundImageSize == L"stretch")
                {
                    out.backgroundImageSize = backgroundImageSize;
                    out.backgroundImageSizeIsExplicit = false;
                    out.hasBackgroundImageSize = true;
                }
                JS_FreeCString(ctx, value);
            }
        }
        else if (JS_IsObject(backgroundImageSizeV))
        {
            JSValue widthV = JS_GetPropertyStr(ctx, backgroundImageSizeV, "width");
            JSValue heightV = JS_GetPropertyStr(ctx, backgroundImageSizeV, "height");
            double width = 0.0, height = 0.0;
            const bool widthProvided = !JS_IsUndefined(widthV);
            const bool heightProvided = !JS_IsUndefined(heightV);
            const bool hasWidth = widthProvided && JS_IsNumber(widthV) && JS_ToFloat64(ctx, &width, widthV) == 0 && std::isfinite(width) && width > 0.0;
            const bool hasHeight = heightProvided && JS_IsNumber(heightV) && JS_ToFloat64(ctx, &height, heightV) == 0 && std::isfinite(height) && height > 0.0;
            JS_FreeValue(ctx, widthV);
            JS_FreeValue(ctx, heightV);
            if ((!widthProvided || hasWidth) && (!heightProvided || hasHeight) && (hasWidth || hasHeight))
            {
                out.backgroundImageSizeIsExplicit = true;
                out.backgroundImageSizeWidth = static_cast<float>(width);
                out.backgroundImageSizeHeight = static_cast<float>(height);
                out.backgroundImageSizeHasWidth = hasWidth;
                out.backgroundImageSizeHasHeight = hasHeight;
                out.hasBackgroundImageSize = true;
            }
        }
        JS_FreeValue(ctx, backgroundImageSizeV);
        std::wstring backgroundImagePosition = PropertyParser::Js::GetStringProp(ctx, options, "backgroundImagePosition");
        std::transform(backgroundImagePosition.begin(), backgroundImagePosition.end(), backgroundImagePosition.begin(), ::towlower);
        static const std::vector<std::wstring> positions = { L"top-left", L"top", L"top-right", L"left", L"center", L"right", L"bottom-left", L"bottom", L"bottom-right" };
        if (std::find(positions.begin(), positions.end(), backgroundImagePosition) != positions.end())
        {
            out.backgroundImagePosition = backgroundImagePosition;
            out.hasBackgroundImagePosition = true;
        }

        JSValue opacityVal = JS_GetPropertyStr(ctx, options, "opacity");
        if (!JS_IsUndefined(opacityVal) && !JS_IsNull(opacityVal))
        {
            if (JS_IsString(opacityVal))
            {
                const char *s = JS_ToCString(ctx, opacityVal);
                if (s)
                {
                    std::wstring ws = Utils::ToWString(s);
                    JS_FreeCString(ctx, s);
                    ws.erase(std::remove_if(ws.begin(), ws.end(), iswspace), ws.end());
                    if (!ws.empty() && ws.back() == L'%')
                    {
                        ws.pop_back();
                        try
                        {
                            float pct = std::stof(ws);
                            pct = std::max(0.0f, std::min(100.0f, pct));
                            out.windowOpacity = static_cast<BYTE>((pct / 100.0f) * 255.0f);
                            out.hasWindowOpacity = true;
                        }
                        catch (...)
                        {
                        }
                    }
                    else
                    {
                        try
                        {
                            float val = std::stof(ws);
                            if (val <= 1.0f)
                                val *= 255.0f;
                            val = std::max(0.0f, std::min(255.0f, val));
                            out.windowOpacity = static_cast<BYTE>(val);
                            out.hasWindowOpacity = true;
                        }
                        catch (...)
                        {
                        }
                    }
                }
            }
            else
            {
                double d = 1.0;
                if (JS_ToFloat64(ctx, &d, opacityVal) == 0)
                {
                    if (d <= 1.0)
                        d *= 255.0;
                    d = std::max(0.0, std::min(255.0, d));
                    out.windowOpacity = static_cast<BYTE>(d);
                    out.hasWindowOpacity = true;
                }
            }
        }
        JS_FreeValue(ctx, opacityVal);

        std::wstring zPosStr = PropertyParser::Js::GetStringProp(ctx, options, "zPos");
        std::transform(zPosStr.begin(), zPosStr.end(), zPosStr.begin(), ::towlower);
        if (zPosStr == L"ondesktop")
        {
            out.zPos = -2;
            out.hasZPos = true;
        }
        else if (zPosStr == L"onbottom")
        {
            out.zPos = -1;
            out.hasZPos = true;
        }
        else if (zPosStr == L"normal")
        {
            out.zPos = 0;
            out.hasZPos = true;
        }
        else if (zPosStr == L"ontop")
        {
            out.zPos = 1;
            out.hasZPos = true;
        }
        else if (zPosStr == L"ontopmost")
        {
            out.zPos = 2;
            out.hasZPos = true;
        }

        std::wstring scriptPath = PropertyParser::Js::GetStringProp(ctx, options, "script");
        if (!scriptPath.empty())
        {
            out.scriptPath = scriptPath;
            out.hasScriptPath = true;
        }
    }

    void ParseWidgetWindowSize(JSContext *ctx, JSValueConst options, int &width, int &height)
    {
        WidgetWindowOptions parsed;
        ParseWidgetWindowOptions(ctx, options, parsed);
        width = parsed.width;
        height = parsed.height;
    }
} // namespace novadesk::scripting::quickjs::parser
