/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>

#include <d2d1.h>
#include <windows.h>

#include "quickjs.h"

#include "../../../render/Element.h"
#include "../../../render/TextElement.h"
#include "PropertyParserTypes.h"

/**
 * @brief JavaScript property extraction utilities for QuickJS.
 *
 * @note Provides helper functions for reading typed properties from JS objects,
 *       parsing gradient/color strings, and extracting callback IDs.
 */
namespace PropertyParser {
namespace Js {

/// Splits a comma-separated string, respecting nested parentheses.
bool TrySplitByComma(const std::wstring &s, std::vector<std::wstring> &parts);

/// Splits a comma-separated string into parts.
std::vector<std::wstring> SplitByComma(const std::wstring &s);

/// Gets a global property from the JavaScript context.
JSValue GetGlobalProperty(JSContext *ctx, const char *key);

// ============================================================================
// Typed Property Getters
// ============================================================================

/// Gets a string property from a JS object.
std::wstring GetStringProp(JSContext *ctx, JSValueConst obj, const char *key);

/// Gets an integer property from a JS object.
bool GetIntProp(JSContext *ctx, JSValueConst obj, const char *key, int &out);

/// Gets a float property from a JS object.
bool GetFloatProp(JSContext *ctx, JSValueConst obj, const char *key,
                  float &out);

/// Gets a boolean property from a JS object.
bool GetBoolProp(JSContext *ctx, JSValueConst obj, const char *key, bool &out);

/// Gets a float array property from a JS object.
bool GetFloatArrayProp(JSContext *ctx, JSValueConst obj, const char *key,
                       std::vector<float> &out, int minSize);

/// Gets an event callback ID from a JS function property.
bool GetEventCallbackProp(JSContext *ctx, JSValueConst obj, const char *key,
                          int &outId);

// ============================================================================
// Parsing Helpers
// ============================================================================

/// Parses a gradient or color string into color/alpha/gradient components.
void ParseGradientOrColor(const std::wstring &v, COLORREF &color, BYTE &alpha,
                          GradientInfo &gradient, bool &hasColor);

/// Parses a combine mode string to D2D1_COMBINE_MODE.
D2D1_COMBINE_MODE ParseCombineMode(const std::wstring &s);

/// Gets a float array property, allowing empty arrays.
bool GetFloatArrayPropAllowEmpty(JSContext *ctx, JSValueConst obj,
                                 const char *key, std::vector<float> &out);

/// Parses prefixed general image options from a JS object.
void ParsePrefixedGeneralImageOptions(JSContext *ctx, JSValueConst obj,
                                      const std::string &prefix,
                                      GeneralImageOptions &out);

/// Parses text shadow options from a JS object.
void ParseTextShadows(JSContext *ctx, JSValueConst obj,
                      std::vector<TextShadow> &shadows);

} // namespace Js
} // namespace PropertyParser
