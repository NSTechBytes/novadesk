#pragma once

#include "quickjs.h"

namespace novadesk::scripting::quickjs {
void InitWidgetWindowEventBindings(JSClassID widgetWindowClassId);
void AttachWidgetWindowEventMethods(JSContext* ctx, JSValue proto);
}

