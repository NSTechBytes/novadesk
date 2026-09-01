#pragma once

#include <string>
#include <vector>
#include "quickjs.h"

namespace novadesk::scripting::quickjs {
// Call once at startup (before the JS engine runs) with the raw command-line
// argv.
void SetAppArgv(const std::vector<std::wstring> &argv);
void SetModuleDebug(bool debug);
JSModuleDef *EnsureNovadeskModule(JSContext *ctx, const char *moduleName);
void UnloadAllAddons();
} // namespace novadesk::scripting::quickjs
