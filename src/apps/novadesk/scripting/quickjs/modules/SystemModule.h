#pragma once

#include "quickjs.h"

namespace novadesk::scripting::quickjs
{
    JSModuleDef *EnsureSystemModule(JSContext *ctx, const char *moduleName);
}
