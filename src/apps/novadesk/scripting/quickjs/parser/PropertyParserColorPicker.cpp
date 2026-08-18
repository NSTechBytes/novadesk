#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../render/ColorPickerElement.h"

namespace PropertyParser {

// ── helpers ──────────────────────────────────────────────────────────────────
namespace {
    void ParseColorAlpha(JSContext* ctx, JSValueConst obj, const char* key,
                         COLORREF& color, BYTE& alpha)
    {
        std::wstring s = Js::GetStringProp(ctx, obj, key);
        if (!s.empty())
            ColorUtil::ParseRGBA(s, color, alpha);
    }

    bool ParseBoolProp(JSContext* ctx, JSValueConst obj, const char* key, bool def)
    {
        JSValue v = JS_GetPropertyStr(ctx, obj, key);
        if (JS_IsUndefined(v) || JS_IsNull(v))
        {
            JS_FreeValue(ctx, v);
            return def;
        }
        bool result = (JS_ToBool(ctx, v) == 1);
        JS_FreeValue(ctx, v);
        return result;
    }
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
void ParseColorPickerOptions(JSContext* ctx, JSValueConst obj,
                              ColorPickerOptions& o, const std::wstring& base)
{
    ParseElementOptions(ctx, obj, o, base);

    // color
    std::wstring c = Js::GetStringProp(ctx, obj, "color");
    BYTE a = 255;
    if (!c.empty())
        ColorUtil::ParseRGBA(c, o.color, a);

    // swatch styling
    {
        JSValue v = JS_GetPropertyStr(ctx, obj, "borderRadius");
        if (!JS_IsUndefined(v))
        {
            double d = 0.0;
            JS_ToFloat64(ctx, &d, v);
            o.borderRadius = static_cast<float>(d);
        }
        JS_FreeValue(ctx, v);
    }
    {
        JSValue v = JS_GetPropertyStr(ctx, obj, "borderWidth");
        if (!JS_IsUndefined(v))
        {
            double d = 0.0;
            JS_ToFloat64(ctx, &d, v);
            o.borderWidth = static_cast<float>(d);
        }
        JS_FreeValue(ctx, v);
    }
    ParseColorAlpha(ctx, obj, "borderColor", o.borderColor, o.borderAlpha);
    {
        JSValue v = JS_GetPropertyStr(ctx, obj, "opacity");
        if (!JS_IsUndefined(v))
        {
            double d = 1.0;
            JS_ToFloat64(ctx, &d, v);
            o.opacity = static_cast<float>(d);
        }
        JS_FreeValue(ctx, v);
    }
    {
        std::wstring shape = Js::GetStringProp(ctx, obj, "shape");
        if (!shape.empty())
        {
            std::wstring lower = shape;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
            o.circleShape = (lower == L"circle" || lower == L"ellipse");
        }
    }

    // popup appearance
    ParseColorAlpha(ctx, obj, "popupBackground",  o.popupBackground,  o.popupBackgroundAlpha);
    ParseColorAlpha(ctx, obj, "popupAccentColor",  o.popupAccentColor,  o.popupAccentAlpha);
    ParseColorAlpha(ctx, obj, "popupBorderColor",  o.popupBorderColor,  o.popupBorderAlpha);

    std::wstring inBg = Js::GetStringProp(ctx, obj, "popupInputBackground");
    if (inBg.empty()) inBg = Js::GetStringProp(ctx, obj, "popupInputBgColor");
    if (!inBg.empty())
    {
        ColorUtil::ParseRGBA(inBg, o.popupInputBackground, o.popupInputBackgroundAlpha);
        o.hasPopupInputBackground = true;
    }

    std::wstring inColor = Js::GetStringProp(ctx, obj, "popupInputColor");
    if (inColor.empty()) inColor = Js::GetStringProp(ctx, obj, "popupInputTextColor");
    if (!inColor.empty())
    {
        ColorUtil::ParseRGBA(inColor, o.popupInputColor, o.popupInputColorAlpha);
        o.hasPopupInputColor = true;
    }

    // popup behavior
    {
        JSValue v = JS_GetPropertyStr(ctx, obj, "showEyedropper");
        if (!JS_IsUndefined(v) && !JS_IsNull(v))
            o.showEyedropper = (JS_ToBool(ctx, v) == 1);
        JS_FreeValue(ctx, v);
    }
    {
        JSValue v = JS_GetPropertyStr(ctx, obj, "showFormatToggle");
        if (!JS_IsUndefined(v) && !JS_IsNull(v))
            o.showFormatToggle = (JS_ToBool(ctx, v) == 1);
        JS_FreeValue(ctx, v);
    }
    {
        std::wstring mode = Js::GetStringProp(ctx, obj, "defaultMode");
        if (!mode.empty())
        {
            std::wstring lower = mode;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
            o.defaultHexMode = (lower == L"hex");
        }
    }

    // callbacks
    Js::GetEventCallbackProp(ctx, obj, "onChange",        o.onChangeCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onOpen",          o.onOpenCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onClose",         o.onCloseCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onCancel",        o.onCancelCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onEyedropperOpen",o.onEyedropperOpenCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onEyedropperPick",o.onEyedropperPickCallbackId);
}

// ─────────────────────────────────────────────────────────────────────────────
void ApplyColorPickerOptions(ColorPickerElement* e, const ColorPickerOptions& o)
{
    ApplyElementOptions(e, o);
    e->SetColor(o.color);

    // swatch
    e->m_BorderRadius = o.borderRadius;
    e->m_BorderWidth  = o.borderWidth;
    e->m_BorderColor  = o.borderColor;
    e->m_BorderAlpha  = o.borderAlpha;
    e->m_Opacity      = o.opacity;
    e->m_CircleShape  = o.circleShape;

    // popup appearance
    e->m_PopupBackground          = o.popupBackground;
    e->m_PopupBackgroundAlpha     = o.popupBackgroundAlpha;
    e->m_PopupAccentColor         = o.popupAccentColor;
    e->m_PopupAccentAlpha         = o.popupAccentAlpha;
    e->m_PopupBorderColor         = o.popupBorderColor;
    e->m_PopupBorderAlpha         = o.popupBorderAlpha;
    e->m_PopupInputBackground      = o.popupInputBackground;
    e->m_PopupInputBackgroundAlpha = o.popupInputBackgroundAlpha;
    e->m_HasPopupInputBackground  = o.hasPopupInputBackground;
    e->m_PopupInputColor          = o.popupInputColor;
    e->m_PopupInputColorAlpha     = o.popupInputColorAlpha;
    e->m_HasPopupInputColor       = o.hasPopupInputColor;

    // popup behavior
    e->m_ShowEyedropper   = o.showEyedropper;
    e->m_ShowFormatToggle = o.showFormatToggle;
    e->m_DefaultHexMode   = o.defaultHexMode;

    // callbacks
    e->m_OnChangeCallbackId          = o.onChangeCallbackId;
    e->m_OnOpenCallbackId            = o.onOpenCallbackId;
    e->m_OnCloseCallbackId           = o.onCloseCallbackId;
    e->m_OnCancelCallbackId          = o.onCancelCallbackId;
    e->m_OnEyedropperOpenCallbackId  = o.onEyedropperOpenCallbackId;
    e->m_OnEyedropperPickCallbackId  = o.onEyedropperPickCallbackId;
}

// ─────────────────────────────────────────────────────────────────────────────
void PreFillColorPickerOptions(ColorPickerOptions& o, ColorPickerElement* e)
{
    PreFillElementOptions(o, e);
    o.color = e->GetColor();

    // swatch
    o.borderRadius = e->m_BorderRadius;
    o.borderWidth  = e->m_BorderWidth;
    o.borderColor  = e->m_BorderColor;
    o.borderAlpha  = e->m_BorderAlpha;
    o.opacity      = e->m_Opacity;
    o.circleShape  = e->m_CircleShape;

    // popup appearance
    o.popupBackground          = e->m_PopupBackground;
    o.popupBackgroundAlpha     = e->m_PopupBackgroundAlpha;
    o.popupAccentColor         = e->m_PopupAccentColor;
    o.popupAccentAlpha         = e->m_PopupAccentAlpha;
    o.popupBorderColor         = e->m_PopupBorderColor;
    o.popupBorderAlpha         = e->m_PopupBorderAlpha;
    o.popupInputBackground      = e->m_PopupInputBackground;
    o.popupInputBackgroundAlpha = e->m_PopupInputBackgroundAlpha;
    o.hasPopupInputBackground  = e->m_HasPopupInputBackground;
    o.popupInputColor          = e->m_PopupInputColor;
    o.popupInputColorAlpha     = e->m_PopupInputColorAlpha;
    o.hasPopupInputColor       = e->m_HasPopupInputColor;

    // popup behavior
    o.showEyedropper   = e->m_ShowEyedropper;
    o.showFormatToggle = e->m_ShowFormatToggle;
    o.defaultHexMode   = e->m_DefaultHexMode;

    // callbacks
    o.onChangeCallbackId         = e->m_OnChangeCallbackId;
    o.onOpenCallbackId           = e->m_OnOpenCallbackId;
    o.onCloseCallbackId          = e->m_OnCloseCallbackId;
    o.onCancelCallbackId         = e->m_OnCancelCallbackId;
    o.onEyedropperOpenCallbackId = e->m_OnEyedropperOpenCallbackId;
    o.onEyedropperPickCallbackId = e->m_OnEyedropperPickCallbackId;
}

} // namespace PropertyParser
