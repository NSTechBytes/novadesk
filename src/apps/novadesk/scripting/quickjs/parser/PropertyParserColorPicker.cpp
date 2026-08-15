#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../render/ColorPickerElement.h"
namespace PropertyParser {
void ParseColorPickerOptions(JSContext* ctx, JSValueConst obj, ColorPickerOptions& o, const std::wstring& base) {
 ParseElementOptions(ctx,obj,o,base); std::wstring c=Js::GetStringProp(ctx,obj,"color"); BYTE a=255; if(!c.empty()) ColorUtil::ParseRGBA(c,o.color,a); Js::GetEventCallbackProp(ctx,obj,"onChange",o.onChangeCallbackId); }
void ApplyColorPickerOptions(ColorPickerElement* e,const ColorPickerOptions&o){ ApplyElementOptions(e,o); e->SetColor(o.color); e->m_OnChangeCallbackId=o.onChangeCallbackId; }
void PreFillColorPickerOptions(ColorPickerOptions&o,ColorPickerElement*e){ PreFillElementOptions(o,e); o.color=e->GetColor(); o.onChangeCallbackId=e->m_OnChangeCallbackId; }
}
