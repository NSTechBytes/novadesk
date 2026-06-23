/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "PropertyParser.h"
#include "PropertyParserJs.h"
#include "../../../shared/ColorUtil.h"
#include "../../../shared/PathUtils.h"
#include "../../../shared/Utils.h"
#include "../../../render/InputBoxElement.h"

namespace PropertyParser
{
    using namespace Js;

    namespace
    {
        bool ParseColorAlphaProp(JSContext *ctx, JSValueConst obj, const char *key, COLORREF &color, BYTE &alpha)
        {
            std::wstring s = GetStringProp(ctx, obj, key);
            if (s.empty())
                return false;
            if (ColorUtil::ParseRGBA(s, color, alpha))
                return true;
            return false;
        }

        TextAlignment ParseAlign(const std::wstring &s)
        {
            std::wstring a = s;
            std::transform(a.begin(), a.end(), a.begin(), ::towlower);
            if (a == L"center")
                return TEXT_ALIGN_CENTER_CENTER;
            if (a == L"right")
                return TEXT_ALIGN_RIGHT_CENTER;
            // default left
            return TEXT_ALIGN_LEFT_CENTER;
        }
    }

    void ParseInputBoxOptions(JSContext *ctx, JSValueConst obj, InputBoxOptions &options, const std::wstring &baseDir)
    {
        ParseElementOptions(ctx, obj, options, baseDir);

        std::wstring text = GetStringProp(ctx, obj, "text");
        if (!text.empty())
            options.text = text;

        std::wstring placeholder = GetStringProp(ctx, obj, "placeholder");
        if (!placeholder.empty())
            options.placeholder = placeholder;

        std::wstring fontFace = GetStringProp(ctx, obj, "fontFace");
        if (!fontFace.empty())
            options.fontFace = fontFace;

        GetIntProp(ctx, obj, "fontSize", options.fontSize);
        GetIntProp(ctx, obj, "fontWeight", options.fontWeight);
        GetBoolProp(ctx, obj, "italic", options.italic);
        GetBoolProp(ctx, obj, "password", options.password);
        GetIntProp(ctx, obj, "maxLength", options.maxLength);
        GetBoolProp(ctx, obj, "multiline", options.multiline);

        std::wstring fontPath = GetStringProp(ctx, obj, "fontPath");
        if (!fontPath.empty())
            options.fontPath = PathUtils::ResolvePath(fontPath, baseDir);

        ParseColorAlphaProp(ctx, obj, "fontColor", options.fontColor, options.fontAlpha);
        ParseColorAlphaProp(ctx, obj, "textColor", options.fontColor, options.fontAlpha);
        ParseColorAlphaProp(ctx, obj, "placeholderColor", options.placeholderColor, options.placeholderAlpha);
        ParseColorAlphaProp(ctx, obj, "caretColor", options.caretColor, options.caretAlpha);
        ParseColorAlphaProp(ctx, obj, "selectionColor", options.selectionColor, options.selectionAlpha);
        {
            std::wstring fillStr = GetStringProp(ctx, obj, "fillColor");
            if (!fillStr.empty())
            {
                std::wstring lower = fillStr;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
                if (lower == L"none" || lower == L"transparent")
                {
                    options.hasFillColor = false;
                }
                else if (ColorUtil::ParseRGBA(fillStr, options.fillColor, options.fillAlpha))
                {
                    options.hasFillColor = true;
                }
            }
        }

        std::wstring align = GetStringProp(ctx, obj, "align");
        if (!align.empty())
            options.textAlign = ParseAlign(align);

        // JS callbacks
        GetEventCallbackProp(ctx, obj, "onTextChange", options.onTextChangeCallbackId);

        // Border (solid only)
        int borderWidth = 0;
        if (GetIntProp(ctx, obj, "borderWidth", borderWidth))
            options.borderWidth = static_cast<float>(borderWidth);
        ParseColorAlphaProp(ctx, obj, "borderColor", options.borderColor, options.borderColorAlpha);
        {
            std::wstring focusStr = GetStringProp(ctx, obj, "borderFocusColor");
            if (!focusStr.empty())
            {
                std::wstring lower = focusStr;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);
                if (lower == L"none" || lower == L"transparent")
                {
                    options.hasBorderFocusColor = false;
                }
                else if (ColorUtil::ParseRGBA(focusStr, options.borderFocusColor, options.borderFocusColorAlpha))
                {
                    options.hasBorderFocusColor = true;
                }
            }
        }
        int borderRadius = 0;
        if (GetIntProp(ctx, obj, "borderRadius", borderRadius))
            options.borderRadius = static_cast<float>(borderRadius);
        GetEventCallbackProp(ctx, obj, "onChange", options.onTextChangeCallbackId);
        GetEventCallbackProp(ctx, obj, "onEnter", options.onEnterCallbackId);
        GetEventCallbackProp(ctx, obj, "onFocus", options.onFocusCallbackId);
        GetEventCallbackProp(ctx, obj, "onBlur", options.onBlurCallbackId);
    }

    void ApplyInputBoxOptions(InputBoxElement *element, const InputBoxOptions &options)
    {
        if (!element)
            return;

        ApplyElementOptions(element, options);

        // Force I-beam cursor for input boxes (ApplyElementOptions may
        // overwrite it with the empty default if no cursor prop was passed).
        element->SetMouseEventCursorName(L"text");

        if (!options.text.empty())
            element->SetText(options.text);
        element->SetFontFace(options.fontFace);
        element->SetFontSize(options.fontSize);
        element->SetFontWeight(options.fontWeight);
        element->SetItalic(options.italic);
        element->SetPasswordMode(options.password);
        element->SetMaxLength(options.maxLength);
        element->SetMultiline(options.multiline);
        if (!options.fontPath.empty())
            element->SetFontPath(options.fontPath);

        element->SetFontColor(options.fontColor, options.fontAlpha);
        element->SetPlaceholder(options.placeholder);
        element->SetPlaceholderColor(options.placeholderColor, options.placeholderAlpha);
        element->SetCaretColor(options.caretColor, options.caretAlpha);
        element->SetSelectionColor(options.selectionColor, options.selectionAlpha);
        if (options.hasFillColor)
            element->SetFillColor(options.fillColor, options.fillAlpha);
        else
            element->ClearFillColor();
        element->SetTextAlign(options.textAlign);
        element->SetBorderWidth(options.borderWidth);
        element->SetBorderRadius(options.borderRadius);
        element->SetBorderColor(options.borderColor, options.borderColorAlpha);
        if (options.hasBorderFocusColor)
            element->SetBorderFocusColor(options.borderFocusColor, options.borderFocusColorAlpha);
        else
            element->ClearBorderFocusColor();

        element->m_OnTextChangeCallbackId = options.onTextChangeCallbackId;
        element->m_OnEnterCallbackId = options.onEnterCallbackId;
        element->m_OnFocusCallbackId = options.onFocusCallbackId;
        element->m_OnBlurCallbackId = options.onBlurCallbackId;
    }

    void PreFillInputBoxOptions(InputBoxOptions &options, InputBoxElement *element)
    {
        if (!element)
            return;
        options.text = element->GetText();
        options.placeholder = element->GetPlaceholder();
        options.fontFace = element->GetFontFace();
        options.fontSize = element->GetFontSize();
        options.fontColor = element->GetFontColor();
        options.fontAlpha = element->GetFontAlpha();
        options.fontWeight = element->GetFontWeight();
        options.italic = element->IsItalic();
        options.textAlign = element->GetTextAlign();
        options.password = element->IsPasswordMode();
        options.maxLength = element->GetMaxLength();
        options.multiline = element->IsMultiline();
        options.hasFillColor = element->HasFillColor();
        options.fillColor = element->GetFillColor();
        options.fillAlpha = element->GetFillAlpha();
        options.borderWidth = element->GetBorderWidth();
        options.borderRadius = element->GetBorderRadius();
        options.borderColor = element->GetBorderColor();
        options.borderColorAlpha = element->GetBorderAlpha();
        options.hasBorderFocusColor = element->HasBorderFocusColor();
        options.borderFocusColor = element->GetBorderFocusColor();
        options.borderFocusColorAlpha = element->GetBorderFocusAlpha();
        options.onTextChangeCallbackId = element->m_OnTextChangeCallbackId;
        options.onEnterCallbackId = element->m_OnEnterCallbackId;
        options.onFocusCallbackId = element->m_OnFocusCallbackId;
        options.onBlurCallbackId = element->m_OnBlurCallbackId;
    }
}
