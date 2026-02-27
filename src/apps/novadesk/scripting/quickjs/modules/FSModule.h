#pragma once

#include "quickjs.h"

namespace novadesk::scripting::quickjs
{
    JSModuleDef *EnsureFsModule(JSContext *ctx, const char *moduleName);
}
