#pragma once

#include "quickjs.h"

namespace novadesk::scripting::quickjs {
void SetWidgetUiDebug(bool debug);
JSClassID EnsureWidgetWindowClass(JSContext* ctx);
JSValue JsWidgetWindowCtor(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv);
}

