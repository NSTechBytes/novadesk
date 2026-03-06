#pragma once

#include "quickjs.h"

namespace novadesk::scripting::quickjs
{
    void SetModuleDebug(bool debug);
    JSModuleDef *EnsureNovadeskModule(JSContext *ctx, const char *moduleName);
} // namespace novadesk::scripting::quickjs
