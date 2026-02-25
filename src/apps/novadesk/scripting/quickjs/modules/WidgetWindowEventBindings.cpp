#include "WidgetWindowEventBindings.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include "../../domain/Widget.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include "../parser/PropertyParser.h"

extern std::vector<Widget*> widgets;

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

JSValue JsWidgetWindowSetProperties(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_EXCEPTION;
    if (argc < 1 || !JS_IsObject(argv[0])) {
        return ThrowTypeError(ctx, "setProperties", "expected options object");
    }

    parser::WidgetWindowOptions parsed;
    parser::ParseWidgetWindowOptions(ctx, argv[0], parsed);

    if (parsed.hasX || parsed.hasY || parsed.hasWidth || parsed.hasHeight) {
        const int x = parsed.hasX ? parsed.x : CW_USEDEFAULT;
        const int y = parsed.hasY ? parsed.y : CW_USEDEFAULT;
        const int w = parsed.hasWidth ? parsed.width : -1;
        const int h = parsed.hasHeight ? parsed.height : -1;
        widget->SetWindowPosition(x, y, w, h);
    }

    if (parsed.hasBackgroundColor) {
        widget->SetBackgroundColor(parsed.backgroundColor);
    }
    if (parsed.hasWindowOpacity) {
        widget->SetWindowOpacity(parsed.windowOpacity);
    }
    if (parsed.hasDraggable) {
        widget->SetDraggable(parsed.draggable);
    }
    if (parsed.hasClickThrough) {
        widget->SetClickThrough(parsed.clickThrough);
    }
    if (parsed.hasKeepOnScreen) {
        widget->SetKeepOnScreen(parsed.keepOnScreen);
    }
    if (parsed.hasSnapEdges) {
        widget->SetSnapEdges(parsed.snapEdges);
    }
    if (parsed.hasShow) {
        if (parsed.show) widget->Show();
        else widget->Hide();
    }
    if (parsed.hasZPos) {
        widget->ChangeZPos(static_cast<ZPOSITION>(parsed.zPos));
    }

    return JS_DupValue(ctx, thisVal);
}

JSValue JsWidgetWindowGetProperties(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_EXCEPTION;

    const WidgetOptions& o = widget->GetOptions();
    JSValue out = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, out, "id", JS_NewString(ctx, Utils::ToString(o.id).c_str()));
    JS_SetPropertyStr(ctx, out, "x", JS_NewInt32(ctx, o.x));
    JS_SetPropertyStr(ctx, out, "y", JS_NewInt32(ctx, o.y));
    JS_SetPropertyStr(ctx, out, "width", JS_NewInt32(ctx, o.width));
    JS_SetPropertyStr(ctx, out, "height", JS_NewInt32(ctx, o.height));
    JS_SetPropertyStr(ctx, out, "draggable", JS_NewBool(ctx, o.draggable ? 1 : 0));
    JS_SetPropertyStr(ctx, out, "clickThrough", JS_NewBool(ctx, o.clickThrough ? 1 : 0));
    JS_SetPropertyStr(ctx, out, "keepOnScreen", JS_NewBool(ctx, o.keepOnScreen ? 1 : 0));
    JS_SetPropertyStr(ctx, out, "snapEdges", JS_NewBool(ctx, o.snapEdges ? 1 : 0));
    JS_SetPropertyStr(ctx, out, "show", JS_NewBool(ctx, IsWindowVisible(widget->GetWindow()) ? 1 : 0));
    JS_SetPropertyStr(ctx, out, "windowOpacity", JS_NewInt32(ctx, static_cast<int>(o.windowOpacity)));
    JS_SetPropertyStr(ctx, out, "backgroundColor", JS_NewString(ctx, Utils::ToString(o.backgroundColor).c_str()));
    JS_SetPropertyStr(ctx, out, "zPos", JS_NewInt32(ctx, static_cast<int>(o.zPos)));
    JS_SetPropertyStr(ctx, out, "script", JS_NewString(ctx, Utils::ToString(o.scriptPath).c_str()));
    return out;
}

JSValue JsWidgetWindowClose(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_UNDEFINED;

    auto it = std::find(widgets.begin(), widgets.end(), widget);
    if (it != widgets.end()) {
        widgets.erase(it);
        delete widget;
    }
    return JS_UNDEFINED;
}

JSValue JsWidgetWindowRefresh(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_EXCEPTION;
    widget->Refresh();
    return JS_UNDEFINED;
}

JSValue JsWidgetWindowSetFocus(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_EXCEPTION;
    widget->SetFocus();
    return JS_UNDEFINED;
}

JSValue JsWidgetWindowUnFocus(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_EXCEPTION;
    widget->UnFocus();
    return JS_UNDEFINED;
}

JSValue JsWidgetWindowGetHandle(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_NULL;
    return JS_NewInt64(ctx, static_cast<int64_t>(reinterpret_cast<uintptr_t>(widget->GetWindow())));
}

JSValue JsWidgetWindowGetInternalPointer(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_NULL;
    return JS_NewInt64(ctx, static_cast<int64_t>(reinterpret_cast<uintptr_t>(widget)));
}

JSValue JsWidgetWindowGetTitle(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
    if (!widget) return JS_NewString(ctx, "");
    return JS_NewString(ctx, Utils::ToString(widget->GetTitle()).c_str());
}

const JSCFunctionListEntry kWidgetWindowEventFuncs[] = {
    JS_CFUNC_DEF("setProperties", 1, JsWidgetWindowSetProperties),
    JS_CFUNC_DEF("getProperties", 0, JsWidgetWindowGetProperties),
    JS_CFUNC_DEF("close", 0, JsWidgetWindowClose),
    JS_CFUNC_DEF("refresh", 0, JsWidgetWindowRefresh),
    JS_CFUNC_DEF("setFocus", 0, JsWidgetWindowSetFocus),
    JS_CFUNC_DEF("unFocus", 0, JsWidgetWindowUnFocus),
    JS_CFUNC_DEF("getHandle", 0, JsWidgetWindowGetHandle),
    JS_CFUNC_DEF("getInternalPointer", 0, JsWidgetWindowGetInternalPointer),
    JS_CFUNC_DEF("getTitle", 0, JsWidgetWindowGetTitle),
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
