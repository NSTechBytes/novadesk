#include "NovadeskModule.h"

#include <string>

#include "../../domain/Widget.h"
#include "./parser/PropertyParser.h"

namespace novadesk::scripting::quickjs {
namespace {
bool g_moduleDebug = false;

JSValue JsWidgetWindowCtor(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    int width = 800;
    int height = 600;
    if (argc > 0) {
        novadesk::scripting::quickjs::parser::ParseWidgetWindowSize(ctx, argv[0], width, height);
    }

    RGFW_window* win = novadesk::domain::widget::CreateWidgetWindow(width, height, g_moduleDebug);
    if (!win) {
        return JS_ThrowInternalError(ctx, "Failed to create RGFW window");
    }
    return JS_UNDEFINED;
}

int NovadeskModuleInit(JSContext* ctx, JSModuleDef* m) {
    JSValue ctor = JS_NewCFunction2(ctx, JsWidgetWindowCtor, "WidgetWindow", 1, JS_CFUNC_constructor, 0);
    JS_SetModuleExport(ctx, m, "WidgetWindow", ctor);
    return 0;
}
}  // namespace

void SetModuleDebug(bool debug) {
    g_moduleDebug = debug;
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
