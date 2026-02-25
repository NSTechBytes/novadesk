#include "NovadeskModule.h"

#include "../../shared/Logging.h"
#include "WidgetUiBindings.h"

namespace novadesk::scripting::quickjs {
namespace {
bool g_moduleDebug = false;

int NovadeskModuleInit(JSContext* ctx, JSModuleDef* m) {
    EnsureWidgetWindowClass(ctx);
    JSValue ctor = JS_NewCFunction2(ctx, JsWidgetWindowCtor, "WidgetWindow", 1, JS_CFUNC_constructor, 0);
    JSValue proto = JS_GetClassProto(ctx, EnsureWidgetWindowClass(ctx));
    JS_SetConstructor(ctx, ctor, proto);
    JS_SetModuleExport(ctx, m, "WidgetWindow", ctor);
    JS_FreeValue(ctx, proto);
    return 0;
}
}  // namespace

void SetModuleDebug(bool debug) {
    g_moduleDebug = debug;
    SetWidgetUiDebug(debug);
}

JSModuleDef* EnsureNovadeskModule(JSContext* ctx, const char* moduleName) {
    JSModuleDef* m = JS_NewCModule(ctx, moduleName, NovadeskModuleInit);
    if (!m) {
        return nullptr;
    }
    if (JS_AddModuleExport(ctx, m, "WidgetWindow") < 0) {
        return nullptr;
    }
    return m;
}
}  // namespace novadesk::scripting::quickjs
