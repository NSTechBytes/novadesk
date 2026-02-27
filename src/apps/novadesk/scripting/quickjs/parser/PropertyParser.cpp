#include "PropertyParser.h"

#include <algorithm>
#include <cwctype>

#include "../../../shared/ColorUtil.h"
#include "../../../shared/PathUtils.h"
#include "../../../shared/Utils.h"
#include "../engine/JSEngine.h"

namespace PropertyParser
{
    namespace
    {
        std::wstring GetStringProp(JSContext *ctx, JSValueConst obj, const char *key)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return L"";
            }
            const char *s = JS_ToCString(ctx, v);
            std::wstring out;
            if (s)
            {
                out = Utils::ToWString(s);
                JS_FreeCString(ctx, s);
            }
            JS_FreeValue(ctx, v);
            return out;
        }

        bool GetIntProp(JSContext *ctx, JSValueConst obj, const char *key, int &out)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            int32_t i = 0;
            bool ok = JS_ToInt32(ctx, &i, v) == 0;
            JS_FreeValue(ctx, v);
            if (ok)
                out = static_cast<int>(i);
            return ok;
        }

        bool GetFloatProp(JSContext *ctx, JSValueConst obj, const char *key, float &out)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            double d = 0.0;
            bool ok = JS_ToFloat64(ctx, &d, v) == 0;
            JS_FreeValue(ctx, v);
            if (ok)
                out = static_cast<float>(d);
            return ok;
        }

        bool GetBoolProp(JSContext *ctx, JSValueConst obj, const char *key, bool &out)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            int b = JS_ToBool(ctx, v);
            JS_FreeValue(ctx, v);
            if (b < 0)
                return false;
            out = b != 0;
            return true;
        }

        bool GetFloatArrayProp(JSContext *ctx, JSValueConst obj, const char *key, std::vector<float> &out, int minSize)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || !JS_IsArray(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }

            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, v, "length");
            if (JS_ToUint32(ctx, &len, lenV) != 0)
            {
                JS_FreeValue(ctx, lenV);
                JS_FreeValue(ctx, v);
                return false;
            }
            JS_FreeValue(ctx, lenV);

            std::vector<float> tmp;
            tmp.reserve(len);
            for (uint32_t i = 0; i < len; ++i)
            {
                JSValue iv = JS_GetPropertyUint32(ctx, v, i);
                double d = 0.0;
                if (JS_ToFloat64(ctx, &d, iv) == 0)
                {
                    tmp.push_back(static_cast<float>(d));
                }
                JS_FreeValue(ctx, iv);
            }
            JS_FreeValue(ctx, v);

            if (static_cast<int>(tmp.size()) < minSize)
                return false;
            out = std::move(tmp);
            return true;
        }

        bool GetEventCallbackProp(JSContext *ctx, JSValueConst obj, const char *key, int &outId)
        {
            JSValue v = JS_GetPropertyStr(ctx, obj, key);
            if (JS_IsException(v) || JS_IsUndefined(v) || JS_IsNull(v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            if (!JS_IsFunction(ctx, v))
            {
                JS_FreeValue(ctx, v);
                return false;
            }
            outId = JSEngine::RegisterEventCallback(ctx, v);
            JS_FreeValue(ctx, v);
            return outId >= 0;
        }

        void ParseGradientOrColor(const std::wstring &v, COLORREF &color, BYTE &alpha, GradientInfo &gradient, bool &hasColor)
        {
            if (v.empty())
                return;
            GradientInfo parsed;
            if (Utils::ParseGradientString(v, parsed))
            {
                gradient = parsed;
                hasColor = true;
                return;
            }
            if (ColorUtil::ParseRGBA(v, color, alpha))
            {
                hasColor = true;
            }
        }

        D2D1_COMBINE_MODE ParseCombineMode(const std::wstring &s)
        {
            if (s == L"intersect")
                return D2D1_COMBINE_MODE_INTERSECT;
            if (s == L"exclude")
                return D2D1_COMBINE_MODE_EXCLUDE;
            if (s == L"xor")
                return D2D1_COMBINE_MODE_XOR;
            return D2D1_COMBINE_MODE_UNION;
        }
    } // namespace

    void ParseElementOptions(JSContext *ctx, JSValueConst obj, ElementOptions &options, const std::wstring &)
    {
        if (!JS_IsObject(obj))
            return;

        options.id = GetStringProp(ctx, obj, "id");
        GetIntProp(ctx, obj, "x", options.x);
        GetIntProp(ctx, obj, "y", options.y);
        GetIntProp(ctx, obj, "width", options.width);
        GetIntProp(ctx, obj, "height", options.height);
        GetFloatProp(ctx, obj, "rotate", options.rotate);

        std::wstring bg = GetStringProp(ctx, obj, "backgroundColor");
        ParseGradientOrColor(bg, options.solidColor, options.solidAlpha, options.solidGradient, options.hasSolidColor);
        GetIntProp(ctx, obj, "backgroundColorRadius", options.solidColorRadius);

        std::wstring bevelType = GetStringProp(ctx, obj, "bevelType");
        if (bevelType == L"raised")
            options.bevelType = 1;
        else if (bevelType == L"sunken")
            options.bevelType = 2;
        else if (bevelType == L"emboss")
            options.bevelType = 3;
        else if (bevelType == L"pillow")
            options.bevelType = 4;
        else if (!bevelType.empty())
            options.bevelType = 0;

        GetIntProp(ctx, obj, "bevelWidth", options.bevelWidth);
        BYTE a = options.bevelAlpha;
        std::wstring b1 = GetStringProp(ctx, obj, "bevelColor");
        if (!b1.empty())
            ColorUtil::ParseRGBA(b1, options.bevelColor, a), options.bevelAlpha = a;
        a = options.bevelAlpha2;
        std::wstring b2 = GetStringProp(ctx, obj, "bevelColor2");
        if (!b2.empty())
            ColorUtil::ParseRGBA(b2, options.bevelColor2, a), options.bevelAlpha2 = a;

        JSValue pad = JS_GetPropertyStr(ctx, obj, "padding");
        if (JS_IsNumber(pad))
        {
            int32_t p = 0;
            if (JS_ToInt32(ctx, &p, pad) == 0)
            {
                options.paddingLeft = options.paddingTop = options.paddingRight = options.paddingBottom = p;
            }
        }
        else if (JS_IsArray(pad))
        {
            std::vector<float> arr;
            if (GetFloatArrayProp(ctx, obj, "padding", arr, 1))
            {
                if (arr.size() >= 4)
                {
                    options.paddingLeft = static_cast<int>(arr[0]);
                    options.paddingTop = static_cast<int>(arr[1]);
                    options.paddingRight = static_cast<int>(arr[2]);
                    options.paddingBottom = static_cast<int>(arr[3]);
                }
                else if (arr.size() >= 2)
                {
                    options.paddingLeft = options.paddingRight = static_cast<int>(arr[0]);
                    options.paddingTop = options.paddingBottom = static_cast<int>(arr[1]);
                }
            }
        }
        JS_FreeValue(ctx, pad);

        GetBoolProp(ctx, obj, "antiAlias", options.antialias);
        GetBoolProp(ctx, obj, "show", options.show);
        options.containerId = GetStringProp(ctx, obj, "container");
        options.groupId = GetStringProp(ctx, obj, "group");
        GetBoolProp(ctx, obj, "mouseEventCursor", options.mouseEventCursor);
        options.mouseEventCursorName = GetStringProp(ctx, obj, "mouseEventCursorName");
        options.cursorsDir = GetStringProp(ctx, obj, "cursorsDir");
        if (!options.cursorsDir.empty())
        {
            options.cursorsDir = PathUtils::ResolvePath(options.cursorsDir);
        }

        if (GetFloatArrayProp(ctx, obj, "transformMatrix", options.transformMatrix, 6))
        {
            options.hasTransformMatrix = true;
        }

        GetEventCallbackProp(ctx, obj, "onLeftMouseUp", options.onLeftMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onLeftMouseDown", options.onLeftMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onLeftDoubleClick", options.onLeftDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightMouseUp", options.onRightMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightMouseDown", options.onRightMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onRightDoubleClick", options.onRightDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleMouseUp", options.onMiddleMouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleMouseDown", options.onMiddleMouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onMiddleDoubleClick", options.onMiddleDoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1MouseUp", options.onX1MouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1MouseDown", options.onX1MouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onX1DoubleClick", options.onX1DoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2MouseUp", options.onX2MouseUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2MouseDown", options.onX2MouseDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onX2DoubleClick", options.onX2DoubleClickCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollUp", options.onScrollUpCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollDown", options.onScrollDownCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollLeft", options.onScrollLeftCallbackId);
        GetEventCallbackProp(ctx, obj, "onScrollRight", options.onScrollRightCallbackId);
        GetEventCallbackProp(ctx, obj, "onMouseOver", options.onMouseOverCallbackId);
        GetEventCallbackProp(ctx, obj, "onMouseLeave", options.onMouseLeaveCallbackId);

        options.tooltipText = GetStringProp(ctx, obj, "tooltipText");
        options.tooltipTitle = GetStringProp(ctx, obj, "tooltipTitle");
        options.tooltipIcon = GetStringProp(ctx, obj, "tooltipIcon");
        GetIntProp(ctx, obj, "tooltipMaxWidth", options.tooltipMaxWidth);
        GetIntProp(ctx, obj, "tooltipMaxHeight", options.tooltipMaxHeight);
        GetBoolProp(ctx, obj, "tooltipBalloon", options.tooltipBalloon);
    }

    void ParseImageOptions(JSContext *ctx, JSValueConst obj, ImageOptions &options, const std::wstring &)
    {
        ParseElementOptions(ctx, obj, options);
        options.path = GetStringProp(ctx, obj, "path");
        if (!options.path.empty())
        {
            options.path = PathUtils::ResolvePath(options.path);
        }

        std::wstring aspect = GetStringProp(ctx, obj, "preserveAspectRatio");
        if (aspect == L"preserve")
            options.preserveAspectRatio = IMAGE_ASPECT_PRESERVE;
        else if (aspect == L"crop")
            options.preserveAspectRatio = IMAGE_ASPECT_CROP;
        else if (aspect == L"stretch")
            options.preserveAspectRatio = IMAGE_ASPECT_STRETCH;

        GetBoolProp(ctx, obj, "grayscale", options.grayscale);
        GetBoolProp(ctx, obj, "tile", options.tile);
        int alpha = 255;
        if (GetIntProp(ctx, obj, "imageAlpha", alpha))
            options.imageAlpha = static_cast<BYTE>(alpha);

        std::wstring tint = GetStringProp(ctx, obj, "imageTint");
        if (!tint.empty() && ColorUtil::ParseRGBA(tint, options.imageTint, options.imageTintAlpha))
        {
            options.hasImageTint = true;
        }

        std::vector<float> colorMatrix;
        if (GetFloatArrayProp(ctx, obj, "colorMatrix", colorMatrix, 20))
        {
            if (colorMatrix.size() >= 20)
            {
                for (int i = 0; i < 20; ++i)
                    options.colorMatrix[i] = colorMatrix[i];
                options.hasColorMatrix = true;
            }
        }
    }

    void ParseTextOptions(JSContext *ctx, JSValueConst obj, TextOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        options.text = GetStringProp(ctx, obj, "text");
        std::wstring fontFace = GetStringProp(ctx, obj, "fontFace");
        if (!fontFace.empty())
            options.fontFace = fontFace;
        GetIntProp(ctx, obj, "fontSize", options.fontSize);

        std::wstring fc = GetStringProp(ctx, obj, "fontColor");
        if (!fc.empty())
        {
            ParseGradientOrColor(fc, options.fontColor, options.alpha, options.fontGradient, options.hasSolidColor);
        }

        GetFloatProp(ctx, obj, "letterSpacing", options.letterSpacing);
        GetBoolProp(ctx, obj, "underLine", options.underLine);
        GetBoolProp(ctx, obj, "strikeThrough", options.strikeThrough);

        std::wstring caseStr = GetStringProp(ctx, obj, "case");
        if (caseStr == L"upper")
            options.textCase = TEXT_CASE_UPPER;
        else if (caseStr == L"lower")
            options.textCase = TEXT_CASE_LOWER;
        else if (caseStr == L"capitalize")
            options.textCase = TEXT_CASE_CAPITALIZE;
        else if (caseStr == L"sentence")
            options.textCase = TEXT_CASE_SENTENCE;

        JSValue fw = JS_GetPropertyStr(ctx, obj, "fontWeight");
        if (JS_IsNumber(fw))
        {
            int32_t w = 400;
            if (JS_ToInt32(ctx, &w, fw) == 0)
                options.fontWeight = w;
        }
        else if (JS_IsString(fw))
        {
            const char *s = JS_ToCString(ctx, fw);
            if (s)
            {
                std::wstring weight = Utils::ToWString(s);
                std::transform(weight.begin(), weight.end(), weight.begin(), ::towlower);
                if (weight == L"thin")
                    options.fontWeight = 100;
                else if (weight == L"extralight" || weight == L"ultralight")
                    options.fontWeight = 200;
                else if (weight == L"light")
                    options.fontWeight = 300;
                else if (weight == L"normal" || weight == L"regular")
                    options.fontWeight = 400;
                else if (weight == L"medium")
                    options.fontWeight = 500;
                else if (weight == L"semibold" || weight == L"demibold")
                    options.fontWeight = 600;
                else if (weight == L"bold")
                    options.fontWeight = 700;
                else if (weight == L"extrabold" || weight == L"ultrabold")
                    options.fontWeight = 800;
                else if (weight == L"black" || weight == L"heavy")
                    options.fontWeight = 900;
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, fw);

        options.fontPath = GetStringProp(ctx, obj, "fontPath");
        if (!options.fontPath.empty())
        {
            options.fontPath = PathUtils::ResolvePath(options.fontPath, baseDir);
        }

        std::wstring style = GetStringProp(ctx, obj, "fontStyle");
        options.italic = (style == L"italic");

        std::wstring align = GetStringProp(ctx, obj, "textAlign");
        if (align.empty())
            align = GetStringProp(ctx, obj, "align");
        std::transform(align.begin(), align.end(), align.begin(), ::towlower);
        if (align == L"left" || align == L"lefttop")
            options.textAlign = TEXT_ALIGN_LEFT_TOP;
        else if (align == L"center" || align == L"centertop")
            options.textAlign = TEXT_ALIGN_CENTER_TOP;
        else if (align == L"right" || align == L"righttop")
            options.textAlign = TEXT_ALIGN_RIGHT_TOP;
        else if (align == L"leftcenter")
            options.textAlign = TEXT_ALIGN_LEFT_CENTER;
        else if (align == L"centercenter" || align == L"middlecenter" || align == L"middle")
            options.textAlign = TEXT_ALIGN_CENTER_CENTER;
        else if (align == L"rightcenter")
            options.textAlign = TEXT_ALIGN_RIGHT_CENTER;
        else if (align == L"leftbottom")
            options.textAlign = TEXT_ALIGN_LEFT_BOTTOM;
        else if (align == L"centerbottom")
            options.textAlign = TEXT_ALIGN_CENTER_BOTTOM;
        else if (align == L"rightbottom")
            options.textAlign = TEXT_ALIGN_RIGHT_BOTTOM;

        std::wstring clip = GetStringProp(ctx, obj, "clipString");
        if (clip == L"none")
            options.clip = TEXT_CLIP_NONE;
        else if (clip == L"on" || clip == L"clip")
            options.clip = TEXT_CLIP_ON;
        else if (clip == L"wrap")
            options.clip = TEXT_CLIP_WRAP;
        else if (clip == L"ellipsis")
            options.clip = TEXT_CLIP_ELLIPSIS;
    }

    void ParseBarOptions(JSContext *ctx, JSValueConst obj, BarOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);
        GetFloatProp(ctx, obj, "value", options.value);

        std::wstring orientation = GetStringProp(ctx, obj, "orientation");
        if (orientation == L"vertical")
            options.orientation = BAR_VERTICAL;
        else if (orientation == L"horizontal")
            options.orientation = BAR_HORIZONTAL;
        GetIntProp(ctx, obj, "barCornerRadius", options.barCornerRadius);

        std::wstring bc = GetStringProp(ctx, obj, "barColor");
        ParseGradientOrColor(bc, options.barColor, options.barAlpha, options.barGradient, options.hasBarColor);
    }

    void ParseRoundLineOptions(JSContext *ctx, JSValueConst obj, RoundLineOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        GetFloatProp(ctx, obj, "value", options.value);
        GetIntProp(ctx, obj, "radius", options.radius);
        GetIntProp(ctx, obj, "thickness", options.thickness);
        GetIntProp(ctx, obj, "endThickness", options.endThickness);
        GetFloatProp(ctx, obj, "startAngle", options.startAngle);
        GetFloatProp(ctx, obj, "totalAngle", options.totalAngle);
        GetBoolProp(ctx, obj, "clockwise", options.clockwise);

        std::wstring capType = GetStringProp(ctx, obj, "capType");
        if (capType == L"round")
            options.startCap = options.endCap = ROUNDLINE_CAP_ROUND;
        else if (capType == L"flat")
            options.startCap = options.endCap = ROUNDLINE_CAP_FLAT;

        std::wstring startCap = GetStringProp(ctx, obj, "startCap");
        if (startCap == L"round")
            options.startCap = ROUNDLINE_CAP_ROUND;
        else if (startCap == L"flat")
            options.startCap = ROUNDLINE_CAP_FLAT;

        std::wstring endCap = GetStringProp(ctx, obj, "endCap");
        if (endCap == L"round")
            options.endCap = ROUNDLINE_CAP_ROUND;
        else if (endCap == L"flat")
            options.endCap = ROUNDLINE_CAP_FLAT;

        std::vector<float> dash;
        if (GetFloatArrayProp(ctx, obj, "dashArray", dash, 1))
            options.dashArray = dash;

        GetIntProp(ctx, obj, "ticks", options.ticks);

        std::wstring lc = GetStringProp(ctx, obj, "lineColor");
        ParseGradientOrColor(lc, options.lineColor, options.lineAlpha, options.lineGradient, options.hasLineColor);

        std::wstring lcb = GetStringProp(ctx, obj, "lineColorBg");
        ParseGradientOrColor(lcb, options.lineColorBg, options.lineAlphaBg, options.lineGradientBg, options.hasLineColorBg);
    }

    void ParseShapeOptions(JSContext *ctx, JSValueConst obj, ShapeOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        options.shapeType = GetStringProp(ctx, obj, "shapeType");
        GetFloatProp(ctx, obj, "strokeWidth", options.strokeWidth);

        std::wstring stroke = GetStringProp(ctx, obj, "strokeColor");
        bool hasStroke = false;
        ParseGradientOrColor(stroke, options.strokeColor, options.strokeAlpha, options.strokeGradient, hasStroke);

        std::wstring fill = GetStringProp(ctx, obj, "fillColor");
        bool hasFill = false;
        ParseGradientOrColor(fill, options.fillColor, options.fillAlpha, options.fillGradient, hasFill);

        GetFloatProp(ctx, obj, "radiusX", options.radiusX);
        GetFloatProp(ctx, obj, "radiusY", options.radiusY);
        GetFloatProp(ctx, obj, "startX", options.startX);
        GetFloatProp(ctx, obj, "startY", options.startY);
        GetFloatProp(ctx, obj, "endX", options.endX);
        GetFloatProp(ctx, obj, "endY", options.endY);
        options.curveType = GetStringProp(ctx, obj, "curveType");
        GetFloatProp(ctx, obj, "controlX", options.controlX);
        GetFloatProp(ctx, obj, "controlY", options.controlY);
        GetFloatProp(ctx, obj, "control2X", options.control2X);
        GetFloatProp(ctx, obj, "control2Y", options.control2Y);
        GetFloatProp(ctx, obj, "startAngle", options.startArcAngle);
        GetFloatProp(ctx, obj, "endAngle", options.endArcAngle);
        GetBoolProp(ctx, obj, "clockwise", options.arcClockwise);
        options.pathData = GetStringProp(ctx, obj, "pathData");

        std::wstring cap = GetStringProp(ctx, obj, "strokeStartCap");
        if (!cap.empty())
            options.strokeStartCap = Utils::GetCapStyle(cap);
        cap = GetStringProp(ctx, obj, "strokeEndCap");
        if (!cap.empty())
            options.strokeEndCap = Utils::GetCapStyle(cap);
        cap = GetStringProp(ctx, obj, "strokeDashCap");
        if (!cap.empty())
            options.strokeDashCap = Utils::GetCapStyle(cap);

        std::wstring join = GetStringProp(ctx, obj, "strokeLineJoin");
        if (!join.empty())
            options.strokeLineJoin = Utils::GetLineJoin(join);
        GetFloatProp(ctx, obj, "strokeDashOffset", options.strokeDashOffset);
        GetFloatArrayProp(ctx, obj, "strokeDashes", options.strokeDashes, 1);

        GetBoolProp(ctx, obj, "isCombine", options.isCombine);
        options.combineBaseId = GetStringProp(ctx, obj, "combineBaseId");
        GetBoolProp(ctx, obj, "combineConsumeAll", options.combineConsumeAll);
        options.hasCombineConsumeAll = true;

        JSValue ops = JS_GetPropertyStr(ctx, obj, "combineOps");
        if (JS_IsArray(ops))
        {
            uint32_t len = 0;
            JSValue lenV = JS_GetPropertyStr(ctx, ops, "length");
            if (JS_ToUint32(ctx, &len, lenV) == 0)
            {
                for (uint32_t i = 0; i < len; ++i)
                {
                    JSValue it = JS_GetPropertyUint32(ctx, ops, i);
                    if (JS_IsObject(it))
                    {
                        ShapeCombineOp op;
                        op.id = GetStringProp(ctx, it, "id");
                        op.mode = ParseCombineMode(GetStringProp(ctx, it, "mode"));
                        if (GetBoolProp(ctx, it, "consume", op.consume))
                            op.hasConsume = true;
                        if (!op.id.empty())
                            options.combineOps.push_back(op);
                    }
                    JS_FreeValue(ctx, it);
                }
            }
            JS_FreeValue(ctx, lenV);
        }
        JS_FreeValue(ctx, ops);
    }

    void ApplyElementOptions(Element *element, const ElementOptions &options)
    {
        if (!element)
            return;
        element->SetPosition(options.x, options.y);
        element->SetSize(options.width, options.height);
        element->SetRotate(options.rotate);
        element->SetAntiAlias(options.antialias);
        element->SetShow(options.show);
        element->SetContainerId(options.containerId);
        element->SetGroupId(options.groupId);
        element->SetMouseEventCursor(options.mouseEventCursor);
        element->SetMouseEventCursorName(options.mouseEventCursorName);
        element->SetCursorsDir(options.cursorsDir);
        element->SetCornerRadius(options.solidColorRadius);
        element->SetPadding(options.paddingLeft, options.paddingTop, options.paddingRight, options.paddingBottom);

        if (options.solidGradient.type != GRADIENT_NONE)
        {
            element->SetSolidGradient(options.solidGradient);
        }
        else if (options.hasSolidColor)
        {
            element->SetSolidColor(options.solidColor, options.solidAlpha);
        }

        if (options.bevelType > 0)
        {
            element->SetBevel(options.bevelType, options.bevelWidth, options.bevelColor, options.bevelAlpha, options.bevelColor2, options.bevelAlpha2);
        }
        else
        {
            element->SetBevel(0, 0, 0, 0, 0, 0);
        }

        if (options.hasTransformMatrix && options.transformMatrix.size() >= 6)
        {
            element->SetTransformMatrix(options.transformMatrix.data());
        }

        if (options.onLeftMouseUpCallbackId != -1)
            element->m_OnLeftMouseUpCallbackId = options.onLeftMouseUpCallbackId;
        if (options.onLeftMouseDownCallbackId != -1)
            element->m_OnLeftMouseDownCallbackId = options.onLeftMouseDownCallbackId;
        if (options.onLeftDoubleClickCallbackId != -1)
            element->m_OnLeftDoubleClickCallbackId = options.onLeftDoubleClickCallbackId;
        if (options.onRightMouseUpCallbackId != -1)
            element->m_OnRightMouseUpCallbackId = options.onRightMouseUpCallbackId;
        if (options.onRightMouseDownCallbackId != -1)
            element->m_OnRightMouseDownCallbackId = options.onRightMouseDownCallbackId;
        if (options.onRightDoubleClickCallbackId != -1)
            element->m_OnRightDoubleClickCallbackId = options.onRightDoubleClickCallbackId;
        if (options.onMiddleMouseUpCallbackId != -1)
            element->m_OnMiddleMouseUpCallbackId = options.onMiddleMouseUpCallbackId;
        if (options.onMiddleMouseDownCallbackId != -1)
            element->m_OnMiddleMouseDownCallbackId = options.onMiddleMouseDownCallbackId;
        if (options.onMiddleDoubleClickCallbackId != -1)
            element->m_OnMiddleDoubleClickCallbackId = options.onMiddleDoubleClickCallbackId;
        if (options.onX1MouseUpCallbackId != -1)
            element->m_OnX1MouseUpCallbackId = options.onX1MouseUpCallbackId;
        if (options.onX1MouseDownCallbackId != -1)
            element->m_OnX1MouseDownCallbackId = options.onX1MouseDownCallbackId;
        if (options.onX1DoubleClickCallbackId != -1)
            element->m_OnX1DoubleClickCallbackId = options.onX1DoubleClickCallbackId;
        if (options.onX2MouseUpCallbackId != -1)
            element->m_OnX2MouseUpCallbackId = options.onX2MouseUpCallbackId;
        if (options.onX2MouseDownCallbackId != -1)
            element->m_OnX2MouseDownCallbackId = options.onX2MouseDownCallbackId;
        if (options.onX2DoubleClickCallbackId != -1)
            element->m_OnX2DoubleClickCallbackId = options.onX2DoubleClickCallbackId;
        if (options.onScrollUpCallbackId != -1)
            element->m_OnScrollUpCallbackId = options.onScrollUpCallbackId;
        if (options.onScrollDownCallbackId != -1)
            element->m_OnScrollDownCallbackId = options.onScrollDownCallbackId;
        if (options.onScrollLeftCallbackId != -1)
            element->m_OnScrollLeftCallbackId = options.onScrollLeftCallbackId;
        if (options.onScrollRightCallbackId != -1)
            element->m_OnScrollRightCallbackId = options.onScrollRightCallbackId;
        if (options.onMouseOverCallbackId != -1)
            element->m_OnMouseOverCallbackId = options.onMouseOverCallbackId;
        if (options.onMouseLeaveCallbackId != -1)
            element->m_OnMouseLeaveCallbackId = options.onMouseLeaveCallbackId;

        if (!options.tooltipText.empty())
        {
            element->SetToolTip(
                options.tooltipText,
                options.tooltipTitle,
                options.tooltipIcon,
                options.tooltipMaxWidth,
                options.tooltipMaxHeight,
                options.tooltipBalloon);
        }
    }

    void ApplyImageOptions(ImageElement *element, const ImageOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        if (!options.path.empty())
            element->UpdateImage(options.path);
        element->SetPreserveAspectRatio(options.preserveAspectRatio);
        element->SetGrayscale(options.grayscale);
        element->SetTile(options.tile);
        element->SetImageAlpha(options.imageAlpha);
        if (options.hasImageTint)
            element->SetImageTint(options.imageTint, options.imageTintAlpha);
        if (options.hasColorMatrix)
            element->SetColorMatrix(options.colorMatrix.data());
    }

    void ApplyTextOptions(TextElement *element, const TextOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetText(options.text);
        element->SetFontFace(options.fontFace);
        element->SetFontSize(options.fontSize);
        element->SetFontColor(options.fontColor, options.alpha);
        element->SetFontWeight(options.fontWeight);
        element->SetItalic(options.italic);
        element->SetTextAlign(options.textAlign);
        element->SetClip(options.clip);
        element->SetFontPath(options.fontPath);
        element->SetShadows(options.shadows);
        element->SetFontGradient(options.fontGradient);
        element->SetLetterSpacing(options.letterSpacing);
        element->SetUnderline(options.underLine);
        element->SetStrikethrough(options.strikeThrough);
        element->SetTextCase(options.textCase);
    }

    void ApplyBarOptions(BarElement *element, const BarOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetValue(options.value);
        element->SetOrientation(options.orientation);
        element->SetBarCornerRadius(options.barCornerRadius);
        if (options.barGradient.type != GRADIENT_NONE)
            element->SetBarGradient(options.barGradient);
        else if (options.hasBarColor)
            element->SetBarColor(options.barColor, options.barAlpha);
    }

    void ApplyRoundLineOptions(RoundLineElement *element, const RoundLineOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);
        element->SetValue(options.value);
        element->SetRadius(options.radius);
        element->SetThickness(options.thickness);
        element->SetEndThickness(options.endThickness);
        element->SetStartAngle(options.startAngle);
        element->SetTotalAngle(options.totalAngle);
        element->SetClockwise(options.clockwise);
        element->SetStartCap(options.startCap);
        element->SetEndCap(options.endCap);
        element->SetDashArray(options.dashArray);
        element->SetTicks(options.ticks);
        if (options.lineGradient.type != GRADIENT_NONE)
            element->SetLineGradient(options.lineGradient);
        else if (options.hasLineColor)
            element->SetLineColor(options.lineColor, options.lineAlpha);
        if (options.lineGradientBg.type != GRADIENT_NONE)
            element->SetLineGradientBg(options.lineGradientBg);
        else if (options.hasLineColorBg)
            element->SetLineColorBg(options.lineColorBg, options.lineAlphaBg);
    }

    void ApplyShapeOptions(ShapeElement *element, const ShapeOptions &options)
    {
        if (!element)
            return;
        ApplyElementOptions(element, options);

        if (options.strokeGradient.type != GRADIENT_NONE)
        {
            element->SetStroke(options.strokeWidth, options.strokeColor, options.strokeAlpha);
            element->SetStrokeGradient(options.strokeGradient);
        }
        else
        {
            element->SetStroke(options.strokeWidth, options.strokeColor, options.strokeAlpha);
        }

        if (options.fillGradient.type != GRADIENT_NONE)
            element->SetFillGradient(options.fillGradient);
        else
            element->SetFill(options.fillColor, options.fillAlpha);

        element->SetRadii(options.radiusX, options.radiusY);
        element->SetLinePoints(options.startX, options.startY, options.endX, options.endY);
        element->SetArcParams(options.startArcAngle, options.endArcAngle, options.arcClockwise);
        element->SetPathData(options.pathData);
        element->SetCurveParams(
            options.startX,
            options.startY,
            options.controlX,
            options.controlY,
            options.control2X,
            options.control2Y,
            options.endX,
            options.endY,
            options.curveType);

        element->SetStrokeStyle(
            options.strokeStartCap,
            options.strokeEndCap,
            options.strokeDashCap,
            options.strokeLineJoin,
            options.strokeDashOffset,
            options.strokeDashes);
    }

    void PreFillImageOptions(ImageOptions &options, ImageElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->GetWidth();
        options.height = element->GetHeight();
        options.containerId = element->GetContainerId();
        options.path = element->GetImagePath();
        options.preserveAspectRatio = element->GetPreserveAspectRatio();
        options.imageAlpha = element->GetImageAlpha();
        options.grayscale = element->IsGrayscale();
        options.tile = element->IsTile();
        if (element->HasImageTint())
        {
            options.hasImageTint = true;
            options.imageTint = element->GetImageTint();
            options.imageTintAlpha = element->GetImageTintAlpha();
        }
    }

    void PreFillTextOptions(TextOptions &options, TextElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->GetWidth();
        options.height = element->GetHeight();
        options.containerId = element->GetContainerId();
        options.text = element->GetText();
        options.fontFace = element->GetFontFace();
        options.fontSize = element->GetFontSize();
        options.fontColor = element->GetFontColor();
        options.alpha = element->GetFontAlpha();
        options.fontWeight = element->GetFontWeight();
        options.italic = element->IsItalic();
        options.textAlign = element->GetTextAlign();
        options.clip = element->GetClipString();
        options.fontPath = element->GetFontPath();
        options.shadows = element->GetShadows();
        options.fontGradient = element->GetFontGradient();
        options.letterSpacing = element->GetLetterSpacing();
        options.underLine = element->GetUnderline();
        options.strikeThrough = element->GetStrikethrough();
        options.textCase = element->GetTextCase();
    }

    void PreFillBarOptions(BarOptions &options, BarElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->GetWidth();
        options.height = element->GetHeight();
        options.containerId = element->GetContainerId();
        options.value = element->GetValue();
        options.orientation = element->GetOrientation();
        options.barCornerRadius = element->GetBarCornerRadius();
        options.hasBarColor = element->HasBarColor();
        options.barColor = element->GetBarColor();
        options.barAlpha = element->GetBarAlpha();
        options.barGradient = element->GetBarGradient();
    }

    void PreFillRoundLineOptions(RoundLineOptions &options, RoundLineElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->GetWidth();
        options.height = element->GetHeight();
        options.containerId = element->GetContainerId();
        options.value = element->GetValue();
        options.radius = element->GetRadius();
        options.thickness = element->GetThickness();
        options.endThickness = element->GetEndThickness();
        options.startAngle = element->GetStartAngle();
        options.totalAngle = element->GetTotalAngle();
        options.clockwise = element->IsClockwise();
        options.startCap = element->GetStartCap();
        options.endCap = element->GetEndCap();
        options.dashArray = element->GetDashArray();
        options.ticks = element->GetTicks();
        options.hasLineColor = element->HasLineColor();
        options.lineColor = element->GetLineColor();
        options.lineAlpha = element->GetLineAlpha();
        options.hasLineColorBg = element->HasLineColorBg();
        options.lineColorBg = element->GetLineColorBg();
        options.lineAlphaBg = element->GetLineAlphaBg();
        options.lineGradient = element->GetLineGradient();
        options.lineGradientBg = element->GetLineGradientBg();
    }

    void PreFillShapeOptions(ShapeOptions &options, ShapeElement *element)
    {
        if (!element)
            return;
        options.id = element->GetId();
        options.x = element->GetX();
        options.y = element->GetY();
        options.width = element->GetWidth();
        options.height = element->GetHeight();
        options.containerId = element->GetContainerId();
        options.strokeWidth = element->GetStrokeWidth();
        options.strokeColor = element->GetStrokeColor();
        options.strokeAlpha = element->GetStrokeAlpha();
        options.strokeGradient = element->GetStrokeGradient();
        options.fillColor = element->GetFillColor();
        options.fillAlpha = element->GetFillAlpha();
        options.fillGradient = element->GetFillGradient();
        options.radiusX = element->GetRadiusX();
        options.radiusY = element->GetRadiusY();
        options.startX = element->GetStartX();
        options.startY = element->GetStartY();
        options.endX = element->GetEndX();
        options.endY = element->GetEndY();
        options.curveType = element->GetCurveType();
        options.controlX = element->GetControlX();
        options.controlY = element->GetControlY();
        options.control2X = element->GetControl2X();
        options.control2Y = element->GetControl2Y();
        options.startArcAngle = element->GetStartAngle();
        options.endArcAngle = element->GetEndAngle();
        options.arcClockwise = element->IsClockwise();
        options.pathData = element->GetPathData();
        options.strokeStartCap = element->GetStrokeStartCap();
        options.strokeEndCap = element->GetStrokeEndCap();
        options.strokeDashCap = element->GetStrokeDashCap();
        options.strokeLineJoin = element->GetStrokeLineJoin();
        options.strokeDashOffset = element->GetStrokeDashOffset();
        options.strokeDashes = element->GetStrokeDashes();
    }

    void ParseElementOptions(duk_context *, ElementOptions &) {}
    void ParseImageOptions(duk_context *, ImageOptions &) {}
    void ParseTextOptions(duk_context *, TextOptions &) {}
    void ParseBarOptions(duk_context *, BarOptions &) {}
    void ParseRoundLineOptions(duk_context *, RoundLineOptions &) {}
    void ParseShapeOptions(duk_context *, ShapeOptions &) {}
}

namespace novadesk::scripting::quickjs::parser
{
    void ParseWidgetWindowOptions(JSContext *ctx, JSValueConst options, WidgetWindowOptions &out)
    {
        if (!JS_IsObject(options))
        {
            return;
        }

        std::wstring id = PropertyParser::GetStringProp(ctx, options, "id");
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

        out.hasDraggable = PropertyParser::GetBoolProp(ctx, options, "draggable", out.draggable);
        out.hasClickThrough = PropertyParser::GetBoolProp(ctx, options, "clickThrough", out.clickThrough);
        out.hasKeepOnScreen = PropertyParser::GetBoolProp(ctx, options, "keepOnScreen", out.keepOnScreen);
        out.hasSnapEdges = PropertyParser::GetBoolProp(ctx, options, "snapEdges", out.snapEdges);
        out.hasShow = PropertyParser::GetBoolProp(ctx, options, "show", out.show);

        std::wstring bg = PropertyParser::GetStringProp(ctx, options, "backgroundColor");
        if (!bg.empty())
        {
            out.backgroundColor = bg;
            bool hasBg = false;
            PropertyParser::ParseGradientOrColor(bg, out.color, out.bgAlpha, out.bgGradient, hasBg);
            out.hasBackgroundColor = true;
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

        std::wstring zPosStr = PropertyParser::GetStringProp(ctx, options, "zPos");
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

        out.scriptPath = PropertyParser::GetStringProp(ctx, options, "script");
        out.hasScriptPath = !out.scriptPath.empty();
    }

    void ParseWidgetWindowSize(JSContext *ctx, JSValueConst options, int &width, int &height)
    {
        WidgetWindowOptions parsed;
        ParseWidgetWindowOptions(ctx, options, parsed);
        width = parsed.width;
        height = parsed.height;
    }
} // namespace novadesk::scripting::quickjs::parser
