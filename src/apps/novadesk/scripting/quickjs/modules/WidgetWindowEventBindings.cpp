#include "WidgetWindowEventBindings.h"

#include <string>

#include "../../domain/Widget.h"
#include "../engine/JSEngine.h"

namespace novadesk::scripting::quickjs {
namespace {
JSClassID g_widgetWindowClassId = 0;

JSValue ThrowTypeError(JSContext* ctx, const char* method, const char* usage) {
    return JS_ThrowTypeError(ctx, "%s: %s", method, usage);
}

JSValue JsWidgetWindowOn(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_EXCEPTION;

    if (argc < 2 || !JS_IsFunction(ctx, argv[1])) {
        return ThrowTypeError(ctx, "on", "expected (eventName, callback)");
    }

    const char* eventName = JS_ToCString(ctx, argv[0]);
    if (!eventName || !*eventName) {
        if (eventName) JS_FreeCString(ctx, eventName);
        return ThrowTypeError(ctx, "on", "eventName must be non-empty string");
    }

    const std::string event(eventName);
    JS_FreeCString(ctx, eventName);

    if (!JSApi::RegisterWidgetEventListener(ctx, widget, event, argv[1])) {
        return JS_ThrowInternalError(ctx, "failed to register widget event listener");
    }

    return JS_DupValue(ctx, thisVal);
}

const JSCFunctionListEntry kWidgetWindowEventFuncs[] = {
    JS_CFUNC_DEF("on", 2, JsWidgetWindowOn),
};
}  // namespace

void InitWidgetWindowEventBindings(JSClassID widgetWindowClassId) {
    g_widgetWindowClassId = widgetWindowClassId;
}

void AttachWidgetWindowEventMethods(JSContext* ctx, JSValue proto) {
    JS_SetPropertyFunctionList(
        ctx,
        proto,
        kWidgetWindowEventFuncs,
        sizeof(kWidgetWindowEventFuncs) / sizeof(kWidgetWindowEventFuncs[0]));
}
}  // namespace novadesk::scripting::quickjs

