#include "WidgetUiBindings.h"

#include <algorithm>
#include <string>
#include <vector>

#include "../../domain/Widget.h"
#include "../../render/BarElement.h"
#include "../../render/ImageElement.h"
#include "../../render/RoundLineElement.h"
#include "../../render/ShapeElement.h"
#include "../../render/TextElement.h"
#include "../../shared/FileUtils.h"
#include "../../shared/Logging.h"
#include "../../shared/PathUtils.h"
#include "../../shared/Settings.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"
#include "ModuleSystem.h"
#include "../parser/PropertyParser.h"
#include "WidgetWindowEventBindings.h"

extern std::vector<Widget*> widgets;

namespace novadesk::scripting::quickjs {
namespace {
bool g_widgetUiDebug = false;
JSClassID g_widgetWindowClassId = 0;
JSClassID g_widgetUiClassId = 0;

Widget* GetWidget(JSContext* ctx, JSValueConst thisVal) {
    return static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetWindowClassId));
}

Widget* GetUiWidget(JSContext* ctx, JSValueConst thisVal) {
    return static_cast<Widget*>(JS_GetOpaque2(ctx, thisVal, g_widgetUiClassId));
}

Widget* GetAnyWidget(JSContext* ctx, JSValueConst thisVal) {
    Widget* widget = GetWidget(ctx, thisVal);
    if (!widget) {
        widget = GetUiWidget(ctx, thisVal);
    }
    return widget;
}

JSValue ThrowTypeError(JSContext* ctx, const char* method, const char* usage) {
    return JS_ThrowTypeError(ctx, "%s: %s", method, usage);
}

JSValue JsWidgetAddImage(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 1 || !JS_IsObject(argv[0])) return ThrowTypeError(ctx, "addImage", "expected options object");
    PropertyParser::ImageOptions options;
    PropertyParser::ParseImageOptions(ctx, argv[0], options);
    widget->AddImage(options);
    return JS_UNDEFINED;
}

JSValue JsWidgetAddText(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 1 || !JS_IsObject(argv[0])) return ThrowTypeError(ctx, "addText", "expected options object");
    PropertyParser::TextOptions options;
    PropertyParser::ParseTextOptions(ctx, argv[0], options);
    widget->AddText(options);
    return JS_UNDEFINED;
}

JSValue JsWidgetAddBar(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 1 || !JS_IsObject(argv[0])) return ThrowTypeError(ctx, "addBar", "expected options object");
    PropertyParser::BarOptions options;
    PropertyParser::ParseBarOptions(ctx, argv[0], options);
    widget->AddBar(options);
    return JS_UNDEFINED;
}

JSValue JsWidgetAddRoundLine(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 1 || !JS_IsObject(argv[0])) return ThrowTypeError(ctx, "addRoundLine", "expected options object");
    PropertyParser::RoundLineOptions options;
    PropertyParser::ParseRoundLineOptions(ctx, argv[0], options);
    widget->AddRoundLine(options);
    return JS_UNDEFINED;
}

JSValue JsWidgetAddShape(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 1 || !JS_IsObject(argv[0])) return ThrowTypeError(ctx, "addShape", "expected options object");
    PropertyParser::ShapeOptions options;
    PropertyParser::ParseShapeOptions(ctx, argv[0], options);
    widget->AddShape(options);
    return JS_UNDEFINED;
}

JSValue JsWidgetRemoveElements(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    std::wstring id;
    if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0])) {
        const char* idUtf8 = JS_ToCString(ctx, argv[0]);
        if (!idUtf8) return JS_EXCEPTION;
        id = Utils::ToWString(idUtf8);
        JS_FreeCString(ctx, idUtf8);
    }
    return JS_NewBool(ctx, widget->RemoveElements(id) ? 1 : 0);
}

JSValue JsWidgetRemoveElementsByGroup(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 1) return ThrowTypeError(ctx, "removeElementsByGroup", "expected group id string");
    const char* groupUtf8 = JS_ToCString(ctx, argv[0]);
    if (!groupUtf8) return JS_EXCEPTION;
    std::wstring group = Utils::ToWString(groupUtf8);
    JS_FreeCString(ctx, groupUtf8);
    widget->RemoveElementsByGroup(group);
    return JS_UNDEFINED;
}

JSValue JsWidgetBeginUpdate(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    widget->BeginUpdate();
    return JS_UNDEFINED;
}

JSValue JsWidgetEndUpdate(JSContext* ctx, JSValueConst thisVal, int, JSValueConst*) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    widget->EndUpdate();
    return JS_UNDEFINED;
}

JSValue JsWidgetSetElementProperties(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 2 || !JS_IsObject(argv[1])) return ThrowTypeError(ctx, "setElementProperties", "expected (id, options)");

    const char* idUtf8 = JS_ToCString(ctx, argv[0]);
    if (!idUtf8) return JS_EXCEPTION;
    std::wstring id = Utils::ToWString(idUtf8);
    JS_FreeCString(ctx, idUtf8);

    Element* element = widget->FindElementById(id);
    if (!element) return JS_UNDEFINED;

    if (auto* image = dynamic_cast<ImageElement*>(element)) {
        PropertyParser::ImageOptions options;
        PropertyParser::PreFillImageOptions(options, image);
        PropertyParser::ParseImageOptions(ctx, argv[1], options);
        PropertyParser::ApplyImageOptions(image, options);
    } else if (auto* text = dynamic_cast<TextElement*>(element)) {
        PropertyParser::TextOptions options;
        PropertyParser::PreFillTextOptions(options, text);
        PropertyParser::ParseTextOptions(ctx, argv[1], options);
        PropertyParser::ApplyTextOptions(text, options);
    } else if (auto* bar = dynamic_cast<BarElement*>(element)) {
        PropertyParser::BarOptions options;
        PropertyParser::PreFillBarOptions(options, bar);
        PropertyParser::ParseBarOptions(ctx, argv[1], options);
        PropertyParser::ApplyBarOptions(bar, options);
    } else if (auto* round = dynamic_cast<RoundLineElement*>(element)) {
        PropertyParser::RoundLineOptions options;
        PropertyParser::PreFillRoundLineOptions(options, round);
        PropertyParser::ParseRoundLineOptions(ctx, argv[1], options);
        PropertyParser::ApplyRoundLineOptions(round, options);
    } else if (auto* shape = dynamic_cast<ShapeElement*>(element)) {
        PropertyParser::ShapeOptions options;
        PropertyParser::PreFillShapeOptions(options, shape);
        PropertyParser::ParseShapeOptions(ctx, argv[1], options);
        PropertyParser::ApplyShapeOptions(shape, options);
    }

    widget->Redraw();
    return JS_UNDEFINED;
}

JSValue JsWidgetSetElementPropertiesByGroup(JSContext* ctx, JSValueConst thisVal, int argc, JSValueConst* argv) {
    Widget* widget = GetAnyWidget(ctx, thisVal);
    if (!widget) return JS_EXCEPTION;
    if (argc < 2) return ThrowTypeError(ctx, "setElementPropertiesByGroup", "expected (groupId, options)");
    const char* groupUtf8 = JS_ToCString(ctx, argv[0]);
    if (!groupUtf8) return JS_EXCEPTION;
    std::wstring group = Utils::ToWString(groupUtf8);
    JS_FreeCString(ctx, groupUtf8);
    widget->SetGroupProperties(group, reinterpret_cast<duk_context*>(ctx));
    widget->Redraw();
    return JS_UNDEFINED;
}

void JsWidgetFinalizer(JSRuntime*, JSValue) {
    // Native widget lifetime is owned by Novadesk's global widget list.
}

const JSCFunctionListEntry kWidgetProtoFuncs[] = {
    JS_CFUNC_DEF("addImage", 1, JsWidgetAddImage),
    JS_CFUNC_DEF("addText", 1, JsWidgetAddText),
    JS_CFUNC_DEF("addBar", 1, JsWidgetAddBar),
    JS_CFUNC_DEF("addRoundLine", 1, JsWidgetAddRoundLine),
    JS_CFUNC_DEF("addShape", 1, JsWidgetAddShape),
    JS_CFUNC_DEF("setElementProperties", 2, JsWidgetSetElementProperties),
    JS_CFUNC_DEF("setElementPropertiesByGroup", 2, JsWidgetSetElementPropertiesByGroup),
    JS_CFUNC_DEF("removeElements", 1, JsWidgetRemoveElements),
    JS_CFUNC_DEF("removeElementsByGroup", 1, JsWidgetRemoveElementsByGroup),
    JS_CFUNC_DEF("beginUpdate", 0, JsWidgetBeginUpdate),
    JS_CFUNC_DEF("endUpdate", 0, JsWidgetEndUpdate),
};

bool RunWidgetUiScriptImpl(JSContext* ctx, Widget* widget, const std::wstring& scriptPath) {
    if (!widget || scriptPath.empty()) return true;

    const std::wstring baseDir = JSEngine::GetEntryScriptDir();
    const std::wstring absPath = PathUtils::ResolvePath(
        scriptPath,
        baseDir.empty() ? PathUtils::GetWidgetsDir() : baseDir
    );
    const std::string scriptSource = FileUtils::ReadFileContent(absPath);
    if (scriptSource.empty()) {
        Logging::Log(LogLevel::Error, L"[novadesk] widget ui script not found: %s", absPath.c_str());
        return false;
    }

    if (g_widgetUiClassId == 0) {
        JS_NewClassID(JS_GetRuntime(ctx), &g_widgetUiClassId);
        JSClassDef uiCls{};
        uiCls.class_name = "WidgetUiBridge";
        JS_NewClass(JS_GetRuntime(ctx), g_widgetUiClassId, &uiCls);
        JSValue uiProto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, uiProto, kWidgetProtoFuncs, sizeof(kWidgetProtoFuncs) / sizeof(kWidgetProtoFuncs[0]));
        JS_SetClassProto(ctx, g_widgetUiClassId, uiProto);
    }

    JSValue uiObj = JS_NewObjectClass(ctx, g_widgetUiClassId);
    if (JS_IsException(uiObj)) return false;
    JS_SetOpaque(uiObj, widget);

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue ipcObj = JSEngine::CreateUiIpcObject(ctx);
    JS_SetPropertyStr(ctx, global, "ui", JS_DupValue(ctx, uiObj));
    JS_SetPropertyStr(ctx, global, "ipcRenderer", JS_DupValue(ctx, ipcObj));
    const std::string fileName = Utils::ToString(absPath);
    const std::string dirName = Utils::ToString(PathUtils::GetParentDir(absPath));
    JS_SetPropertyStr(ctx, global, "__filename", JS_NewString(ctx, fileName.c_str()));
    JS_SetPropertyStr(ctx, global, "__dirname", JS_NewString(ctx, dirName.c_str()));
    JS_FreeValue(ctx, global);

    const std::string scriptPrelude =
        "const ui = globalThis.ui;\n"
        "const ipcRenderer = globalThis.ipcRenderer;\n"
        "const __filename = globalThis.__filename;\n"
        "const __dirname = globalThis.__dirname;\n";
    const std::string scriptSourceWithPrelude = scriptPrelude + scriptSource;

    JSValue evalResult = JS_Eval(ctx, scriptSourceWithPrelude.c_str(), scriptSourceWithPrelude.size(), fileName.c_str(), JS_EVAL_TYPE_GLOBAL);

    JSValue global2 = JS_GetGlobalObject(ctx);
    JSAtom uiAtom = JS_NewAtom(ctx, "ui");
    JS_DeleteProperty(ctx, global2, uiAtom, 0);
    JS_FreeAtom(ctx, uiAtom);
    JSAtom ipcAtom = JS_NewAtom(ctx, "ipcRenderer");
    JS_DeleteProperty(ctx, global2, ipcAtom, 0);
    JS_FreeAtom(ctx, ipcAtom);
    JSAtom filename2Atom = JS_NewAtom(ctx, "__filename");
    JS_DeleteProperty(ctx, global2, filename2Atom, 0);
    JS_FreeAtom(ctx, filename2Atom);
    JSAtom dirname2Atom = JS_NewAtom(ctx, "__dirname");
    JS_DeleteProperty(ctx, global2, dirname2Atom, 0);
    JS_FreeAtom(ctx, dirname2Atom);
    JS_FreeValue(ctx, global2);
    JS_FreeValue(ctx, uiObj);
    JS_FreeValue(ctx, ipcObj);

    if (JS_IsException(evalResult)) {
        JSValue ex = JS_GetException(ctx);
        const char* msg = JS_ToCString(ctx, ex);
        if (msg) {
            Logging::Log(LogLevel::Error, L"[novadesk] ui script error (%s): %S", absPath.c_str(), msg);
            JS_FreeCString(ctx, msg);
        } else {
            Logging::Log(LogLevel::Error, L"[novadesk] ui script error (%s)", absPath.c_str());
        }
        JS_FreeValue(ctx, ex);
        JS_FreeValue(ctx, evalResult);
        return false;
    }

    JS_FreeValue(ctx, evalResult);
    return true;
}
}  // namespace

void SetWidgetUiDebug(bool debug) {
    g_widgetUiDebug = debug;
}

JSClassID EnsureWidgetWindowClass(JSContext* ctx) {
    if (g_widgetWindowClassId == 0) {
        JS_NewClassID(JS_GetRuntime(ctx), &g_widgetWindowClassId);
        JSClassDef cls{};
        cls.class_name = "widgetWindow";
        cls.finalizer = JsWidgetFinalizer;
        JS_NewClass(JS_GetRuntime(ctx), g_widgetWindowClassId, &cls);
        InitWidgetWindowEventBindings(g_widgetWindowClassId);

        JSValue proto = JS_NewObject(ctx);
        JS_SetPropertyFunctionList(ctx, proto, kWidgetProtoFuncs, sizeof(kWidgetProtoFuncs) / sizeof(kWidgetProtoFuncs[0]));
        AttachWidgetWindowEventMethods(ctx, proto);
        JS_SetClassProto(ctx, g_widgetWindowClassId, proto);
    }
    return g_widgetWindowClassId;
}

JSValue JsWidgetWindowCtor(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    parser::WidgetWindowOptions parsed;
    if (argc > 0 && JS_IsObject(argv[0])) {
        parser::ParseWidgetWindowOptions(ctx, argv[0], parsed);
    }

    WidgetOptions options;
    options.id = parsed.id.empty() ? L"widget" : parsed.id;

    if (!options.id.empty()) {
        Settings::LoadWidget(options.id, options);
    }

    if (parsed.hasX) options.x = parsed.x;
    if (parsed.hasY) options.y = parsed.y;

    if (parsed.hasWidth) {
        options.width = parsed.width;
        options.m_WDefined = (parsed.width > 0);
    } else {
        options.width = 0;
        options.m_WDefined = false;
    }
    if (parsed.hasHeight) {
        options.height = parsed.height;
        options.m_HDefined = (parsed.height > 0);
    } else {
        options.height = 0;
        options.m_HDefined = false;
    }

    if (parsed.hasShow) options.show = parsed.show;
    if (parsed.hasScriptPath) options.scriptPath = parsed.scriptPath;
    if (parsed.hasDraggable) options.draggable = parsed.draggable;
    if (parsed.hasClickThrough) options.clickThrough = parsed.clickThrough;
    if (parsed.hasKeepOnScreen) options.keepOnScreen = parsed.keepOnScreen;
    if (parsed.hasSnapEdges) options.snapEdges = parsed.snapEdges;
    if (parsed.hasBackgroundColor) {
        options.backgroundColor = parsed.backgroundColor;
        options.color = parsed.color;
        options.bgAlpha = parsed.bgAlpha;
        options.bgGradient = parsed.bgGradient;
    }
    if (parsed.hasWindowOpacity) options.windowOpacity = parsed.windowOpacity;
    if (parsed.hasZPos) {
        options.zPos = static_cast<ZPOSITION>(parsed.zPos);
    }

    Widget* widget = new Widget(options);
    if (!widget->Create()) {
        delete widget;
        return JS_ThrowInternalError(ctx, "Failed to create widget window");
    }

    if (options.show) {
        widget->Show();
    }
    widgets.push_back(widget);

    if (g_widgetUiDebug) {
        Logging::Log(LogLevel::Debug, L"[novadesk] JsCreateWindow id=%s width=%d height=%d", options.id.c_str(), options.width, options.height);
    }

    JSValue obj = JS_NewObjectClass(ctx, EnsureWidgetWindowClass(ctx));
    if (JS_IsException(obj)) return obj;
    JS_SetOpaque(obj, widget);

    if (!options.scriptPath.empty()) {
        RunWidgetUiScriptImpl(ctx, widget, options.scriptPath);
    }
    return obj;
}

bool ExecuteWidgetUiScript(JSContext* ctx, Widget* widget, const std::wstring& scriptPath) {
    return RunWidgetUiScriptImpl(ctx, widget, scriptPath);
}
}  // namespace novadesk::scripting::quickjs
