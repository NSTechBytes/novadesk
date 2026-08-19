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
#include <string>
#include <cstring>
namespace PropertyParser
{
    using namespace Js;

    static bool HasDisallowedTweenTextProps(JSContext *ctx, JSValueConst obj)
    {
        static const char *kForbidden[] = {"fontSize", "fontWeight", "letterSpacing", "fontColor"};
        for (const char *key : kForbidden)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            const bool present = !JS_IsException(v) && !JS_IsUndefined(v) && !JS_IsNull(v);
            JS_FreeValue(ctx, v);
            if (present)
                return true;
        }
        return false;
    }

    static void ParseAnimationTransformObject(
        JSContext *ctx,
        JSValueConst obj,
        bool &hasX,
        float &x,
        bool &hasY,
        float &y,
        bool &hasWidth,
        float &width,
        bool &hasHeight,
        float &height,
        bool &hasRotate,
        float &rotate,
        bool &hasXExpr,
        std::wstring &xExpr,
        bool &hasYExpr,
        std::wstring &yExpr)
    {
        // x — numeric or string keyword
        JSValue xVal = JS_GetPropertyStr(ctx, obj, "x");
        if (!JS_IsException(xVal) && !JS_IsUndefined(xVal) && !JS_IsNull(xVal))
        {
            if (JS_IsString(xVal))
            {
                const char *str = JS_ToCString(ctx, xVal);
                if (str) { xExpr = Utils::ToWString(str); hasXExpr = !xExpr.empty(); JS_FreeCString(ctx, str); }
            }
            else
            {
                double d = 0;
                if (JS_ToFloat64(ctx, &d, xVal) >= 0) { x = static_cast<float>(d); hasX = true; }
            }
        }
        JS_FreeValue(ctx, xVal);

        // y — numeric or string keyword
        JSValue yVal = JS_GetPropertyStr(ctx, obj, "y");
        if (!JS_IsException(yVal) && !JS_IsUndefined(yVal) && !JS_IsNull(yVal))
        {
            if (JS_IsString(yVal))
            {
                const char *str = JS_ToCString(ctx, yVal);
                if (str) { yExpr = Utils::ToWString(str); hasYExpr = !yExpr.empty(); JS_FreeCString(ctx, str); }
            }
            else
            {
                double d = 0;
                if (JS_ToFloat64(ctx, &d, yVal) >= 0) { y = static_cast<float>(d); hasY = true; }
            }
        }
        JS_FreeValue(ctx, yVal);

        hasWidth  = GetFloatProp(ctx, obj, "width",  width);
        hasHeight = GetFloatProp(ctx, obj, "height", height);
        hasRotate = GetFloatProp(ctx, obj, "rotate", rotate);
    }

    static void ParseAnimationTargetObject(
        JSContext *ctx,
        JSValueConst obj,
        bool &hasX,
        float &x,
        bool &hasY,
        float &y,
        bool &hasWidth,
        float &width,
        bool &hasHeight,
        float &height,
        bool &hasRotate,
        float &rotate,
        bool &hasFontSize,
        float &fontSize,
        bool &hasFontWeight,
        float &fontWeight,
        bool &hasLetterSpacing,
        float &letterSpacing,
        bool &hasFontColor,
        float &fontColorR,
        float &fontColorG,
        float &fontColorB,
        float &fontAlpha,
        bool &hasXExpr,
        std::wstring &xExpr,
        bool &hasYExpr,
        std::wstring &yExpr)
    {
        // Parse x — supports numeric or string keyword
        JSValue xVal = JS_GetPropertyStr(ctx, obj, "x");
        if (!JS_IsException(xVal) && !JS_IsUndefined(xVal) && !JS_IsNull(xVal))
        {
            if (JS_IsString(xVal))
            {
                const char *str = JS_ToCString(ctx, xVal);
                if (str)
                {
                    xExpr = Utils::ToWString(str);
                    hasXExpr = !xExpr.empty();
                    JS_FreeCString(ctx, str);
                }
            }
            else
            {
                double d = 0;
                if (JS_ToFloat64(ctx, &d, xVal) >= 0)
                { x = static_cast<float>(d); hasX = true; }
            }
        }
        JS_FreeValue(ctx, xVal);

        // Parse y — supports numeric or string keyword
        JSValue yVal = JS_GetPropertyStr(ctx, obj, "y");
        if (!JS_IsException(yVal) && !JS_IsUndefined(yVal) && !JS_IsNull(yVal))
        {
            if (JS_IsString(yVal))
            {
                const char *str = JS_ToCString(ctx, yVal);
                if (str)
                {
                    yExpr = Utils::ToWString(str);
                    hasYExpr = !yExpr.empty();
                    JS_FreeCString(ctx, str);
                }
            }
            else
            {
                double d = 0;
                if (JS_ToFloat64(ctx, &d, yVal) >= 0)
                { y = static_cast<float>(d); hasY = true; }
            }
        }
        JS_FreeValue(ctx, yVal);

        hasWidth = GetFloatProp(ctx, obj, "width", width);
        hasHeight = GetFloatProp(ctx, obj, "height", height);
        hasRotate = GetFloatProp(ctx, obj, "rotate", rotate);
        hasFontSize = GetFloatProp(ctx, obj, "fontSize", fontSize);
        hasFontWeight = GetFloatProp(ctx, obj, "fontWeight", fontWeight);
        hasLetterSpacing = GetFloatProp(ctx, obj, "letterSpacing", letterSpacing);

        std::wstring fontColor = GetStringProp(ctx, obj, "fontColor");
        if (!fontColor.empty())
        {
            COLORREF color = 0;
            BYTE alpha = 255;
            if (ColorUtil::ParseRGBA(fontColor, color, alpha))
            {
                hasFontColor = true;
                fontColorR = static_cast<float>(GetRValue(color));
                fontColorG = static_cast<float>(GetGValue(color));
                fontColorB = static_cast<float>(GetBValue(color));
                fontAlpha = static_cast<float>(alpha);
            }
        }
    }

    static bool ParseKeyframeOffsetValue(JSContext *ctx, JSValueConst val, float &outOffset)
    {
        if (JS_IsNumber(val) || JS_IsBool(val))
        {
            double d = 0.0;
            if (JS_ToFloat64(ctx, &d, val) != 0)
                return false;
            if (d < 0.0)
                d = 0.0;
            if (d > 1.0)
                d = 1.0;
            outOffset = static_cast<float>(d);
            return true;
        }

        if (!JS_IsString(val))
            return false;

        const char *utf8 = JS_ToCString(ctx, val);
        if (!utf8)
            return false;
        std::string s = utf8;
        JS_FreeCString(ctx, utf8);

        size_t end = s.find_last_not_of(" \t\r\n");
        if (end == std::string::npos)
            return false;
        s = s.substr(0, end + 1);

        bool isPercent = false;
        if (!s.empty() && s.back() == '%')
        {
            isPercent = true;
            s.pop_back();
        }

        try
        {
            size_t pos = 0;
            double d = std::stod(s, &pos);
            if (pos != s.size())
                return false;
            if (isPercent)
                d /= 100.0;
            if (d < 0.0)
                d = 0.0;
            if (d > 1.0)
                d = 1.0;
            outOffset = static_cast<float>(d);
            return true;
        }
        catch (...)
        {
            return false;
        }
    }

    static void ParseAnimationKeyframeValues(JSContext *ctx, JSValueConst obj, AnimationKeyframeOptions &kf)
    {
        ParseAnimationTargetObject(
            ctx,
            obj,
            kf.hasX,
            kf.x,
            kf.hasY,
            kf.y,
            kf.hasWidth,
            kf.width,
            kf.hasHeight,
            kf.height,
            kf.hasRotate,
            kf.rotate,
            kf.hasFontSize,
            kf.fontSize,
            kf.hasFontWeight,
            kf.fontWeight,
            kf.hasLetterSpacing,
            kf.letterSpacing,
            kf.hasFontColor,
            kf.fontColorR,
            kf.fontColorG,
            kf.fontColorB,
            kf.fontAlpha,
            kf.hasXExpr,
            kf.xExpr,
            kf.hasYExpr,
            kf.yExpr);

        std::wstring easing = GetStringProp(ctx, obj, "easing");
        if (!easing.empty())
            kf.easing = easing;
    }

    static bool ParseAnimationKeyframeEntry(JSContext *ctx, JSValueConst obj, float forcedOffset, bool hasForcedOffset, AnimationKeyframeOptions &out)
    {
        if (!JS_IsObject(obj))
            return false;

        out = AnimationKeyframeOptions{};
        if (hasForcedOffset)
        {
            out.offset = forcedOffset;
            out.hasOffset = true;
        }
        else
        {
            JSValue offsetVal = JS_GetPropertyStr(ctx, obj, "offset");
            if (JS_IsException(offsetVal) || JS_IsUndefined(offsetVal) || JS_IsNull(offsetVal))
            {
                JS_FreeValue(ctx, offsetVal);
                offsetVal = JS_GetPropertyStr(ctx, obj, "at");
            }
            if (!JS_IsException(offsetVal) && !JS_IsUndefined(offsetVal) && !JS_IsNull(offsetVal))
            {
                float parsed = 0.0f;
                if (ParseKeyframeOffsetValue(ctx, offsetVal, parsed))
                {
                    out.offset = parsed;
                    out.hasOffset = true;
                }
            }
            JS_FreeValue(ctx, offsetVal);
        }

        ParseAnimationKeyframeValues(ctx, obj, out);
        return out.hasOffset && out.HasAnyProps();
    }

    static void ParseAnimationKeyframesArray(JSContext *ctx, JSValueConst arr, AnimationOptions &options)
    {
        uint32_t len = 0;
        JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
        JS_ToUint32(ctx, &len, lenV);
        JS_FreeValue(ctx, lenV);

        for (uint32_t i = 0; i < len; ++i)
        {
            JSValue item = JS_GetPropertyUint32(ctx, arr, i);
            AnimationKeyframeOptions kf{};
            if (!ParseAnimationKeyframeEntry(ctx, item, 0.0f, false, kf))
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"each keyframes entry needs offset/at and at least one property";
            }
            else
            {
                options.keyframes.push_back(kf);
            }
            JS_FreeValue(ctx, item);
        }
    }

    static void ParseAnimationKeyframesObject(JSContext *ctx, JSValueConst obj, AnimationOptions &options)
    {
        JSPropertyEnum *tab = nullptr;
        uint32_t tabLen = 0;
        if (JS_GetOwnPropertyNames(ctx, &tab, &tabLen, obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
        {
            options.keyframesInvalid = true;
            options.keyframesError = L"invalid keyframes object";
            return;
        }

        for (uint32_t i = 0; i < tabLen; ++i)
        {
            JSValue key = JS_AtomToValue(ctx, tab[i].atom);
            const char *keyUtf8 = JS_ToCString(ctx, key);
            if (!keyUtf8)
            {
                JS_FreeValue(ctx, key);
                continue;
            }

            float offset = 0.0f;
            JSValue keyStr = JS_NewString(ctx, keyUtf8);
            const bool hasOffset = ParseKeyframeOffsetValue(ctx, keyStr, offset);
            JS_FreeValue(ctx, keyStr);
            JS_FreeCString(ctx, keyUtf8);
            JS_FreeValue(ctx, key);

            if (!hasOffset)
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"keyframes object keys must be percentages like \"0%\" or \"33%\"";
                continue;
            }

            JSValue item = JS_GetProperty(ctx, obj, tab[i].atom);
            AnimationKeyframeOptions kf{};
            if (!ParseAnimationKeyframeEntry(ctx, item, offset, true, kf))
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"each keyframes entry must include at least one property";
            }
            else
            {
                options.keyframes.push_back(kf);
            }
            JS_FreeValue(ctx, item);
        }
        for (uint32_t i = 0; i < tabLen; ++i)
        {
            JS_FreeAtom(ctx, tab[i].atom);
        }
        js_free(ctx, tab);
    }

    void ParseAnimationOptions(JSContext *ctx, JSValueConst obj, AnimationOptions &options)
    {
        options.id = GetStringProp(ctx, obj, "id");
        GetIntProp(ctx, obj, "duration", options.duration);
        if (options.duration <= 0)
            options.duration = 250;

        std::wstring easing = GetStringProp(ctx, obj, "easing");
        if (!easing.empty())
            options.easing = easing;

        options.iterationCount = 1;
        options.iterationInfinite = false;
        options.hasIterationCount = false;
        options.iterationCountInvalid = false;
        JSValue iterationVal = JS_GetPropertyStr(ctx, obj, "iterationCount");
        if (!JS_IsException(iterationVal) && !JS_IsUndefined(iterationVal) && !JS_IsNull(iterationVal))
        {
            options.hasIterationCount = true;
            if (JS_IsString(iterationVal))
            {
                const char *iterUtf8 = JS_ToCString(ctx, iterationVal);
                if (iterUtf8)
                {
                    std::string iterStr = iterUtf8;
                    JS_FreeCString(ctx, iterUtf8);
                    std::transform(iterStr.begin(), iterStr.end(), iterStr.begin(), ::tolower);
                    if (iterStr == "infinite")
                        options.iterationInfinite = true;
                    else
                        options.iterationCountInvalid = true;
                }
            }
            else
            {
                int count = 0;
                if (GetIntProp(ctx, obj, "iterationCount", count))
                {
                    if (count >= 1)
                        options.iterationCount = count;
                    else
                        options.iterationCountInvalid = true;
                }
                else
                    options.iterationCountInvalid = true;
            }
        }
        JS_FreeValue(ctx, iterationVal);

        JSValue keyframesVal = JS_GetPropertyStr(ctx, obj, "keyframes");
        if (!JS_IsException(keyframesVal) && !JS_IsUndefined(keyframesVal) && !JS_IsNull(keyframesVal))
        {
            options.hasKeyframes = true;
            if (JS_IsArray(keyframesVal))
                ParseAnimationKeyframesArray(ctx, keyframesVal, options);
            else if (JS_IsObject(keyframesVal))
                ParseAnimationKeyframesObject(ctx, keyframesVal, options);
            else
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"keyframes must be an array or object with percent keys";
            }
        }
        JS_FreeValue(ctx, keyframesVal);

        if (options.hasKeyframes)
        {
            JSValue toCheck = JS_GetPropertyStr(ctx, obj, "to");
            JSValue fromCheck = JS_GetPropertyStr(ctx, obj, "from");
            const bool hasTo = JS_IsObject(toCheck);
            const bool hasFrom = JS_IsObject(fromCheck);
            JS_FreeValue(ctx, toCheck);
            JS_FreeValue(ctx, fromCheck);
            if (hasTo || hasFrom)
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"use either keyframes or from/to, not both";
            }

            if (!options.keyframesInvalid)
            {
                if (options.keyframes.size() < 2)
                {
                    options.keyframesInvalid = true;
                    options.keyframesError = L"keyframes requires at least 2 stops";
                }
                else
                {
                    std::sort(options.keyframes.begin(), options.keyframes.end(), [](const AnimationKeyframeOptions &a, const AnimationKeyframeOptions &b)
                              { return a.offset < b.offset; });
                    for (size_t i = 1; i < options.keyframes.size(); ++i)
                    {
                        if (options.keyframes[i].offset <= options.keyframes[i - 1].offset)
                        {
                            options.keyframesInvalid = true;
                            options.keyframesError = L"keyframes offsets must be strictly increasing";
                            break;
                        }
                    }
                }
            }
            return;
        }

        JSValue toVal = JS_GetPropertyStr(ctx, obj, "to");
        if (JS_IsObject(toVal))
        {
            if (HasDisallowedTweenTextProps(ctx, toVal))
            {
                options.tweenInvalid = true;
                options.tweenError =
                    L"from/to support only x, y, width, height, and rotate; use keyframes for text properties";
            }
            else
            {
                ParseAnimationTransformObject(
                    ctx,
                    toVal,
                    options.hasX,
                    options.x,
                    options.hasY,
                    options.y,
                    options.hasWidth,
                    options.width,
                    options.hasHeight,
                    options.height,
                    options.hasRotate,
                    options.rotate,
                    options.hasXExpr,
                    options.xExpr,
                    options.hasYExpr,
                    options.yExpr);
            }
        }
        JS_FreeValue(ctx, toVal);

        JSValue fromVal = JS_GetPropertyStr(ctx, obj, "from");
        if (JS_IsObject(fromVal))
        {
            if (HasDisallowedTweenTextProps(ctx, fromVal))
            {
                options.tweenInvalid = true;
                options.tweenError =
                    L"from/to support only x, y, width, height, and rotate; use keyframes for text properties";
            }
            else
            {
                ParseAnimationTransformObject(
                    ctx,
                    fromVal,
                    options.fromHasX,
                    options.fromX,
                    options.fromHasY,
                    options.fromY,
                    options.fromHasWidth,
                    options.fromWidth,
                    options.fromHasHeight,
                    options.fromHeight,
                    options.fromHasRotate,
                    options.fromRotate,
                    options.fromHasXExpr,
                    options.fromXExpr,
                    options.fromHasYExpr,
                    options.fromYExpr);
            }
        }
        JS_FreeValue(ctx, fromVal);
    }

    static void ParseWindowAnimationTargetObject(
        JSContext *ctx,
        JSValueConst obj,
        bool &hasX, float &x,
        bool &hasY, float &y,
        bool &hasWidth, float &width,
        bool &hasHeight, float &height,
        bool &hasOpacity, float &opacity,
        bool &hasBgColor, float &bgR, float &bgG, float &bgB, float &bgA,
        bool &hasXExpr, std::wstring &xExpr,
        bool &hasYExpr, std::wstring &yExpr,
        bool &hasPosition, std::wstring &position,
        float &offsetX, float &offsetY)
    {
        // Parse x
        JSValue xVal = JS_GetPropertyStr(ctx, obj, "x");
        if (!JS_IsException(xVal) && !JS_IsUndefined(xVal) && !JS_IsNull(xVal))
        {
            if (JS_IsString(xVal))
            {
                const char *str = JS_ToCString(ctx, xVal);
                if (str)
                {
                    xExpr = Utils::ToWString(str);
                    hasXExpr = !xExpr.empty();
                    JS_FreeCString(ctx, str);
                }
            }
            else
            {
                double d = 0;
                if (JS_ToFloat64(ctx, &d, xVal) >= 0)
                {
                    x = static_cast<float>(d);
                    hasX = true;
                }
            }
        }
        JS_FreeValue(ctx, xVal);

        // Parse y
        JSValue yVal = JS_GetPropertyStr(ctx, obj, "y");
        if (!JS_IsException(yVal) && !JS_IsUndefined(yVal) && !JS_IsNull(yVal))
        {
            if (JS_IsString(yVal))
            {
                const char *str = JS_ToCString(ctx, yVal);
                if (str)
                {
                    yExpr = Utils::ToWString(str);
                    hasYExpr = !yExpr.empty();
                    JS_FreeCString(ctx, str);
                }
            }
            else
            {
                double d = 0;
                if (JS_ToFloat64(ctx, &d, yVal) >= 0)
                {
                    y = static_cast<float>(d);
                    hasY = true;
                }
            }
        }
        JS_FreeValue(ctx, yVal);

        hasWidth = GetFloatProp(ctx, obj, "width", width);
        if (!hasWidth) hasWidth = GetFloatProp(ctx, obj, "w", width);
        hasHeight = GetFloatProp(ctx, obj, "height", height);
        if (!hasHeight) hasHeight = GetFloatProp(ctx, obj, "h", height);

        float opVal = 1.0f;
        if (GetFloatProp(ctx, obj, "opacity", opVal) || GetFloatProp(ctx, obj, "alpha", opVal))
        {
            hasOpacity = true;
            if (opVal > 1.0f)
                opVal = opVal / 255.0f;
            if (opVal < 0.0f) opVal = 0.0f;
            if (opVal > 1.0f) opVal = 1.0f;
            opacity = opVal;
        }

        std::wstring bgStr = GetStringProp(ctx, obj, "backgroundColor");
        if (bgStr.empty()) bgStr = GetStringProp(ctx, obj, "bgColor");
        if (!bgStr.empty())
        {
            COLORREF c = 0;
            BYTE a = 255;
            if (ColorUtil::ParseRGBA(bgStr, c, a))
            {
                hasBgColor = true;
                bgR = static_cast<float>(GetRValue(c));
                bgG = static_cast<float>(GetGValue(c));
                bgB = static_cast<float>(GetBValue(c));
                bgA = static_cast<float>(a);
            }
        }

        position = GetStringProp(ctx, obj, "position");
        if (position.empty())
            position = GetStringProp(ctx, obj, "align");
        hasPosition = !position.empty();

        GetFloatProp(ctx, obj, "offsetX", offsetX);
        GetFloatProp(ctx, obj, "offsetY", offsetY);
    }

    static void ParseWindowAnimationKeyframeValues(JSContext *ctx, JSValueConst obj, WindowAnimationKeyframeOptions &kf)
    {
        ParseWindowAnimationTargetObject(
            ctx,
            obj,
            kf.hasX,
            kf.x,
            kf.hasY,
            kf.y,
            kf.hasWidth,
            kf.width,
            kf.hasHeight,
            kf.height,
            kf.hasOpacity,
            kf.opacity,
            kf.hasBackgroundColor,
            kf.bgColorR,
            kf.bgColorG,
            kf.bgColorB,
            kf.bgAlpha,
            kf.hasXExpr,
            kf.xExpr,
            kf.hasYExpr,
            kf.yExpr,
            kf.hasPosition,
            kf.position,
            kf.offsetX,
            kf.offsetY);

        std::wstring easing = GetStringProp(ctx, obj, "easing");
        if (!easing.empty())
            kf.easing = easing;
    }

    static bool ParseWindowAnimationKeyframeEntry(JSContext *ctx, JSValueConst obj, float forcedOffset, bool hasForcedOffset, WindowAnimationKeyframeOptions &out)
    {
        if (!JS_IsObject(obj))
            return false;

        out = WindowAnimationKeyframeOptions{};
        if (hasForcedOffset)
        {
            out.offset = forcedOffset;
            out.hasOffset = true;
        }
        else
        {
            JSValue offsetVal = JS_GetPropertyStr(ctx, obj, "offset");
            if (JS_IsException(offsetVal) || JS_IsUndefined(offsetVal) || JS_IsNull(offsetVal))
            {
                JS_FreeValue(ctx, offsetVal);
                offsetVal = JS_GetPropertyStr(ctx, obj, "at");
            }
            if (!JS_IsException(offsetVal) && !JS_IsUndefined(offsetVal) && !JS_IsNull(offsetVal))
            {
                float parsed = 0.0f;
                if (ParseKeyframeOffsetValue(ctx, offsetVal, parsed))
                {
                    out.offset = parsed;
                    out.hasOffset = true;
                }
            }
            JS_FreeValue(ctx, offsetVal);
        }

        ParseWindowAnimationKeyframeValues(ctx, obj, out);
        return out.hasOffset && out.HasAnyProps();
    }

    static void ParseWindowAnimationKeyframesArray(JSContext *ctx, JSValueConst arr, WindowAnimationOptions &options)
    {
        uint32_t len = 0;
        JSValue lenV = JS_GetPropertyStr(ctx, arr, "length");
        JS_ToUint32(ctx, &len, lenV);
        JS_FreeValue(ctx, lenV);

        for (uint32_t i = 0; i < len; ++i)
        {
            JSValue item = JS_GetPropertyUint32(ctx, arr, i);
            WindowAnimationKeyframeOptions kf{};
            if (!ParseWindowAnimationKeyframeEntry(ctx, item, 0.0f, false, kf))
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"each keyframes entry needs offset/at and at least one window property";
            }
            else
            {
                options.keyframes.push_back(kf);
            }
            JS_FreeValue(ctx, item);
        }
    }

    static void ParseWindowAnimationKeyframesObject(JSContext *ctx, JSValueConst obj, WindowAnimationOptions &options)
    {
        JSPropertyEnum *tab = nullptr;
        uint32_t tabLen = 0;
        if (JS_GetOwnPropertyNames(ctx, &tab, &tabLen, obj, JS_GPN_STRING_MASK | JS_GPN_ENUM_ONLY) < 0)
        {
            options.keyframesInvalid = true;
            options.keyframesError = L"invalid keyframes object";
            return;
        }

        for (uint32_t i = 0; i < tabLen; ++i)
        {
            JSValue key = JS_AtomToValue(ctx, tab[i].atom);
            const char *keyUtf8 = JS_ToCString(ctx, key);
            if (!keyUtf8)
            {
                JS_FreeValue(ctx, key);
                continue;
            }

            float offset = 0.0f;
            JSValue keyStr = JS_NewString(ctx, keyUtf8);
            const bool hasOffset = ParseKeyframeOffsetValue(ctx, keyStr, offset);
            JS_FreeValue(ctx, keyStr);
            JS_FreeCString(ctx, keyUtf8);
            JS_FreeValue(ctx, key);

            if (!hasOffset)
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"keyframes object keys must be percentages like \"0%\" or \"50%\"";
                continue;
            }

            JSValue item = JS_GetProperty(ctx, obj, tab[i].atom);
            WindowAnimationKeyframeOptions kf{};
            if (!ParseWindowAnimationKeyframeEntry(ctx, item, offset, true, kf))
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"each keyframes entry must include at least one property";
            }
            else
            {
                options.keyframes.push_back(kf);
            }
            JS_FreeValue(ctx, item);
        }
        for (uint32_t i = 0; i < tabLen; ++i)
        {
            JS_FreeAtom(ctx, tab[i].atom);
        }
        js_free(ctx, tab);
    }

    void ParseWindowAnimationOptions(JSContext *ctx, JSValueConst obj, WindowAnimationOptions &options)
    {
        GetIntProp(ctx, obj, "duration", options.duration);
        if (options.duration <= 0)
            options.duration = 250;

        std::wstring easing = GetStringProp(ctx, obj, "easing");
        if (!easing.empty())
            options.easing = easing;

        options.iterationCount = 1;
        options.iterationInfinite = false;
        options.hasIterationCount = false;
        options.iterationCountInvalid = false;
        JSValue iterationVal = JS_GetPropertyStr(ctx, obj, "iterationCount");
        if (!JS_IsException(iterationVal) && !JS_IsUndefined(iterationVal) && !JS_IsNull(iterationVal))
        {
            options.hasIterationCount = true;
            if (JS_IsString(iterationVal))
            {
                const char *iterUtf8 = JS_ToCString(ctx, iterationVal);
                if (iterUtf8)
                {
                    std::string iterStr = iterUtf8;
                    JS_FreeCString(ctx, iterUtf8);
                    std::transform(iterStr.begin(), iterStr.end(), iterStr.begin(), ::tolower);
                    if (iterStr == "infinite")
                        options.iterationInfinite = true;
                    else
                        options.iterationCountInvalid = true;
                }
            }
            else
            {
                int count = 0;
                if (GetIntProp(ctx, obj, "iterationCount", count))
                {
                    if (count >= 1)
                        options.iterationCount = count;
                    else
                        options.iterationCountInvalid = true;
                }
                else
                    options.iterationCountInvalid = true;
            }
        }
        JS_FreeValue(ctx, iterationVal);

        JSValue keyframesVal = JS_GetPropertyStr(ctx, obj, "keyframes");
        if (!JS_IsException(keyframesVal) && !JS_IsUndefined(keyframesVal) && !JS_IsNull(keyframesVal))
        {
            options.hasKeyframes = true;
            if (JS_IsArray(keyframesVal))
                ParseWindowAnimationKeyframesArray(ctx, keyframesVal, options);
            else if (JS_IsObject(keyframesVal))
                ParseWindowAnimationKeyframesObject(ctx, keyframesVal, options);
            else
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"keyframes must be an array or object with percent keys";
            }
        }
        JS_FreeValue(ctx, keyframesVal);

        if (options.hasKeyframes)
        {
            JSValue toCheck = JS_GetPropertyStr(ctx, obj, "to");
            JSValue fromCheck = JS_GetPropertyStr(ctx, obj, "from");
            const bool hasTo = JS_IsObject(toCheck);
            const bool hasFrom = JS_IsObject(fromCheck);
            JS_FreeValue(ctx, toCheck);
            JS_FreeValue(ctx, fromCheck);
            if (hasTo || hasFrom)
            {
                options.keyframesInvalid = true;
                options.keyframesError = L"use either keyframes or from/to, not both";
            }

            if (!options.keyframesInvalid)
            {
                if (options.keyframes.size() < 2)
                {
                    options.keyframesInvalid = true;
                    options.keyframesError = L"keyframes requires at least 2 stops";
                }
                else
                {
                    std::sort(options.keyframes.begin(), options.keyframes.end(), [](const WindowAnimationKeyframeOptions &a, const WindowAnimationKeyframeOptions &b)
                              { return a.offset < b.offset; });
                    for (size_t i = 1; i < options.keyframes.size(); ++i)
                    {
                        if (options.keyframes[i].offset <= options.keyframes[i - 1].offset)
                        {
                            options.keyframesInvalid = true;
                            options.keyframesError = L"keyframes offsets must be strictly increasing";
                            break;
                        }
                    }
                }
            }
            return;
        }

        JSValue toVal = JS_GetPropertyStr(ctx, obj, "to");
        if (JS_IsObject(toVal))
        {
            ParseWindowAnimationTargetObject(
                ctx,
                toVal,
                options.hasX,
                options.x,
                options.hasY,
                options.y,
                options.hasWidth,
                options.width,
                options.hasHeight,
                options.height,
                options.hasOpacity,
                options.opacity,
                options.hasBackgroundColor,
                options.bgColorR,
                options.bgColorG,
                options.bgColorB,
                options.bgAlpha,
                options.hasXExpr,
                options.xExpr,
                options.hasYExpr,
                options.yExpr,
                options.hasPosition,
                options.position,
                options.offsetX,
                options.offsetY);
        }
        JS_FreeValue(ctx, toVal);

        JSValue fromVal = JS_GetPropertyStr(ctx, obj, "from");
        if (JS_IsObject(fromVal))
        {
            ParseWindowAnimationTargetObject(
                ctx,
                fromVal,
                options.fromHasX,
                options.fromX,
                options.fromHasY,
                options.fromY,
                options.fromHasWidth,
                options.fromWidth,
                options.fromHasHeight,
                options.fromHeight,
                options.fromHasOpacity,
                options.fromOpacity,
                options.fromHasBackgroundColor,
                options.fromBgColorR,
                options.fromBgColorG,
                options.fromBgColorB,
                options.fromBgAlpha,
                options.fromHasXExpr,
                options.fromXExpr,
                options.fromHasYExpr,
                options.fromYExpr,
                options.fromHasPosition,
                options.fromPosition,
                options.fromOffsetX,
                options.fromOffsetY);
        }
        JS_FreeValue(ctx, fromVal);
    }
}
