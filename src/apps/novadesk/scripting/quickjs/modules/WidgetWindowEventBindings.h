#pragma once

#include <string>
#include "quickjs.h"

namespace novadesk::scripting::quickjs {
void InitWidgetWindowEventBindings(JSClassID widgetWindowClassId);
void AttachWidgetWindowEventMethods(JSContext* ctx, JSValue proto);
void InvokeWidgetContextMenuCallback(const std::wstring& widgetId, int commandId);
}
