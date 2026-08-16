#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../render/ColorPickerElement.h"

namespace PropertyParser {
void ParseColorPickerOptions(JSContext* ctx, JSValueConst obj, ColorPickerOptions& o, const std::wstring& base) {
    ParseElementOptions(ctx, obj, o, base);
    std::wstring c = Js::GetStringProp(ctx, obj, "color");
    BYTE a = 255;
    if (!c.empty())
        ColorUtil::ParseRGBA(c, o.color, a);
    Js::GetEventCallbackProp(ctx, obj, "onChange", o.onChangeCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onOpen", o.onOpenCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onClose", o.onCloseCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onCancel", o.onCancelCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onEyedropperOpen", o.onEyedropperOpenCallbackId);
    Js::GetEventCallbackProp(ctx, obj, "onEyedropperPick", o.onEyedropperPickCallbackId);
}

void ApplyColorPickerOptions(ColorPickerElement* e, const ColorPickerOptions& o) {
    ApplyElementOptions(e, o);
    e->SetColor(o.color);
    e->m_OnChangeCallbackId = o.onChangeCallbackId;
    e->m_OnOpenCallbackId = o.onOpenCallbackId;
    e->m_OnCloseCallbackId = o.onCloseCallbackId;
    e->m_OnCancelCallbackId = o.onCancelCallbackId;
    e->m_OnEyedropperOpenCallbackId = o.onEyedropperOpenCallbackId;
    e->m_OnEyedropperPickCallbackId = o.onEyedropperPickCallbackId;
}

void PreFillColorPickerOptions(ColorPickerOptions& o, ColorPickerElement* e) {
    PreFillElementOptions(o, e);
    o.color = e->GetColor();
    o.onChangeCallbackId = e->m_OnChangeCallbackId;
    o.onOpenCallbackId = e->m_OnOpenCallbackId;
    o.onCloseCallbackId = e->m_OnCloseCallbackId;
    o.onCancelCallbackId = e->m_OnCancelCallbackId;
    o.onEyedropperOpenCallbackId = e->m_OnEyedropperOpenCallbackId;
    o.onEyedropperPickCallbackId = e->m_OnEyedropperPickCallbackId;
}
}
