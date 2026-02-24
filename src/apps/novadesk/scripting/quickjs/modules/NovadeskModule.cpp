#include "NovadeskModule.h"

#include <string>
#include <vector>

#include "../../domain/Widget.h"
#include "../../shared/Logging.h"
#include "../../shared/Utils.h"
#include "../parser/PropertyParser.h"

extern std::vector<Widget*> widgets;

namespace novadesk::scripting::quickjs {
namespace {
bool g_moduleDebug = false;

JSValue JsWidgetWindowCtor(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    int width = 800;
    int height = 600;
    std::wstring id = L"widget";

    if (argc > 0 && JS_IsObject(argv[0])) {
        novadesk::scripting::quickjs::parser::ParseWidgetWindowSize(ctx, argv[0], width, height);

        JSValue idVal = JS_GetPropertyStr(ctx, argv[0], "id");
        if (!JS_IsUndefined(idVal) && !JS_IsNull(idVal)) {
            const char* idUtf8 = JS_ToCString(ctx, idVal);
            if (idUtf8 && *idUtf8) {
                id = Utils::ToWString(idUtf8);
                JS_FreeCString(ctx, idUtf8);
            }
        }
        JS_FreeValue(ctx, idVal);
    }

    WidgetOptions options;
    options.id = id;
    options.width = width;
    options.height = height;
    options.m_WDefined = (width > 0);
    options.m_HDefined = (height > 0);
    options.show = true;

    Widget* widget = new Widget(options);
    if (!widget->Create()) {
        delete widget;
        return JS_ThrowInternalError(ctx, "Failed to create widget window");
    }

    widget->Show();
    widgets.push_back(widget);

    if (g_moduleDebug) {
        Logging::Log(LogLevel::Debug, L"[novadesk] JsCreateWindow id=%s width=%d height=%d", id.c_str(), width, height);
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
