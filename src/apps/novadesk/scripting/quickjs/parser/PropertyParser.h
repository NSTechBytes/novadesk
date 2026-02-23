#pragma once

#include <string>

#include "quickjs.h"

namespace novadesk::scripting::quickjs::parser {
std::string ParseMainFromMeta(const std::string& metaJson);
void ParseWidgetWindowSize(JSContext* ctx, JSValueConst options, int& width, int& height);
}  // namespace novadesk::scripting::quickjs::parser
