#pragma once

#include <string>
#include "quickjs.h"

class Widget;

namespace novadesk::scripting::quickjs
{
    void SetWidgetUiDebug(bool debug);
    JSClassID EnsureWidgetWindowClass(JSContext *ctx);
    JSValue JsWidgetWindowCtor(JSContext *ctx, JSValueConst thisVal, int argc, JSValueConst *argv);
    bool ExecuteWidgetUiScript(JSContext *ctx, Widget *widget, const std::wstring &scriptPath);
}
