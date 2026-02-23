#pragma once

#include "quickjs.h"

namespace novadesk::scripting::quickjs {
void SetModuleSystemDebug(bool debug);
char* ModuleNormalizeName(JSContext* ctx, const char* baseName, const char* name, void* opaque);
JSModuleDef* ModuleLoader(JSContext* ctx, const char* moduleName, void* opaque);
}  // namespace novadesk::scripting::quickjs
