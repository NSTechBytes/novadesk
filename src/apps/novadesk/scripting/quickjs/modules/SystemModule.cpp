#include "SystemModule.h"

#include <string>

#include "../../shared/System.h"
#include "../../shared/Utils.h"

namespace novadesk::scripting::quickjs {
namespace {
JSValue JsClipboardSetText(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "clipboard.setText(text) requires text");
    const char* s = JS_ToCString(ctx, argv[0]);
    if (!s) return JS_EXCEPTION;
    std::wstring text = Utils::ToWString(s);
    JS_FreeCString(ctx, s);
    return JS_NewBool(ctx, shared::system::ClipboardSetText(text) ? 1 : 0);
}

JSValue JsClipboardGetText(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    std::wstring text;
    if (!shared::system::ClipboardGetText(text)) {
        return JS_NewString(ctx, "");
    }
    return JS_NewString(ctx, Utils::ToString(text).c_str());
}

JSValue JsWallpaperSet(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "wallpaper.set(path[, style]) requires path");
    const char* p = JS_ToCString(ctx, argv[0]);
    if (!p) return JS_EXCEPTION;
    std::wstring path = Utils::ToWString(p);
    JS_FreeCString(ctx, p);
    std::wstring style = L"fill";
    if (argc > 1) {
        const char* st = JS_ToCString(ctx, argv[1]);
        if (st) {
            style = Utils::ToWString(st);
            JS_FreeCString(ctx, st);
        }
    }
    return JS_NewBool(ctx, shared::system::SetWallpaper(path, style) ? 1 : 0);
}

JSValue JsWallpaperGet(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    std::wstring p;
    if (!shared::system::GetCurrentWallpaperPath(p)) return JS_NewString(ctx, "");
    return JS_NewString(ctx, Utils::ToString(p).c_str());
}

JSValue JsPowerGetStatus(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::PowerStatus status;
    if (!shared::system::GetPowerStatus(status)) return JS_NULL;
    JSValue out = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, out, "acline", JS_NewInt32(ctx, status.acline));
    JS_SetPropertyStr(ctx, out, "status", JS_NewInt32(ctx, status.status));
    JS_SetPropertyStr(ctx, out, "status2", JS_NewInt32(ctx, status.status2));
    JS_SetPropertyStr(ctx, out, "lifetime", JS_NewFloat64(ctx, status.lifetime));
    JS_SetPropertyStr(ctx, out, "percent", JS_NewInt32(ctx, status.percent));
    JS_SetPropertyStr(ctx, out, "mhz", JS_NewFloat64(ctx, status.mhz));
    JS_SetPropertyStr(ctx, out, "hz", JS_NewFloat64(ctx, status.hz));
    return out;
}

JSValue JsFileIconExtractIcon(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_ThrowTypeError(ctx, "fileIcon.extractIcon(filePath, outIcoPath[, size])");
    const char* p1 = JS_ToCString(ctx, argv[0]);
    const char* p2 = JS_ToCString(ctx, argv[1]);
    if (!p1 || !p2) {
        if (p1) JS_FreeCString(ctx, p1);
        if (p2) JS_FreeCString(ctx, p2);
        return JS_EXCEPTION;
    }
    std::wstring filePath = Utils::ToWString(p1);
    std::wstring outPath = Utils::ToWString(p2);
    JS_FreeCString(ctx, p1);
    JS_FreeCString(ctx, p2);
    int32_t size = 48;
    if (argc > 2) JS_ToInt32(ctx, &size, argv[2]);
    return JS_NewBool(ctx, Utils::ExtractFileIconToIco(filePath, outPath, static_cast<int>(size)) ? 1 : 0);
}

JSValue JsDisplayMetricsGetMetrics(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    const auto& mm = shared::system::GetDisplayMetrics();
    JSValue out = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, out, "virtualLeft", JS_NewInt32(ctx, mm.virtualLeft));
    JS_SetPropertyStr(ctx, out, "virtualTop", JS_NewInt32(ctx, mm.virtualTop));
    JS_SetPropertyStr(ctx, out, "virtualWidth", JS_NewInt32(ctx, mm.virtualWidth));
    JS_SetPropertyStr(ctx, out, "virtualHeight", JS_NewInt32(ctx, mm.virtualHeight));
    JS_SetPropertyStr(ctx, out, "primaryIndex", JS_NewInt32(ctx, mm.primaryIndex));
    JS_SetPropertyStr(ctx, out, "count", JS_NewInt32(ctx, static_cast<int32_t>(mm.monitors.size())));

    JSValue arr = JS_NewArray(ctx);
    uint32_t i = 0;
    for (const auto& m : mm.monitors) {
        JSValue mo = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, mo, "active", JS_NewBool(ctx, m.active ? 1 : 0));
        JS_SetPropertyStr(ctx, mo, "deviceName", JS_NewString(ctx, Utils::ToString(m.deviceName).c_str()));
        JS_SetPropertyStr(ctx, mo, "monitorName", JS_NewString(ctx, Utils::ToString(m.monitorName).c_str()));
        JSValue screen = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, screen, "left", JS_NewInt32(ctx, m.screen.left));
        JS_SetPropertyStr(ctx, screen, "top", JS_NewInt32(ctx, m.screen.top));
        JS_SetPropertyStr(ctx, screen, "right", JS_NewInt32(ctx, m.screen.right));
        JS_SetPropertyStr(ctx, screen, "bottom", JS_NewInt32(ctx, m.screen.bottom));
        JS_SetPropertyStr(ctx, mo, "screen", screen);
        JS_SetPropertyUint32(ctx, arr, i++, mo);
    }
    JS_SetPropertyStr(ctx, out, "monitors", arr);
    return out;
}

JSValue JsAudioSetVolume(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "audio.setVolume(value) requires value");
    int32_t value = 0;
    JS_ToInt32(ctx, &value, argv[0]);
    return JS_NewBool(ctx, shared::system::AudioSetVolume(static_cast<int>(value)) ? 1 : 0);
}

JSValue JsAudioGetVolume(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    return JS_NewInt32(ctx, shared::system::AudioGetVolume());
}

JSValue JsAudioPlaySound(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "audio.playSound(path[, loop]) requires path");
    const char* s = JS_ToCString(ctx, argv[0]);
    if (!s) return JS_EXCEPTION;
    std::wstring path = Utils::ToWString(s);
    JS_FreeCString(ctx, s);
    int loop = 0;
    if (argc > 1) {
        loop = JS_ToBool(ctx, argv[1]);
    }
    return JS_NewBool(ctx, shared::system::AudioPlaySound(path, loop != 0) ? 1 : 0);
}

JSValue JsAudioStopSound(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::AudioStopSound();
    return JS_NewBool(ctx, 1);
}

JSValue JsBrightnessGetValue(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    JSValue out = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, out, "supported", JS_NewBool(ctx, 0));
    JS_SetPropertyStr(ctx, out, "current", JS_NewInt32(ctx, 0));
    JS_SetPropertyStr(ctx, out, "min", JS_NewInt32(ctx, 0));
    JS_SetPropertyStr(ctx, out, "max", JS_NewInt32(ctx, 100));
    return out;
}

JSValue JsBrightnessSetValue(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    return JS_NewBool(ctx, 0);
}

int SystemModuleInit(JSContext* ctx, JSModuleDef* m) {
    JSValue clipboard = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, clipboard, "setText", JS_NewCFunction(ctx, JsClipboardSetText, "setText", 1));
    JS_SetPropertyStr(ctx, clipboard, "getText", JS_NewCFunction(ctx, JsClipboardGetText, "getText", 0));
    JS_SetModuleExport(ctx, m, "clipboard", clipboard);

    JSValue wallpaper = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, wallpaper, "set", JS_NewCFunction(ctx, JsWallpaperSet, "set", 2));
    JS_SetPropertyStr(ctx, wallpaper, "getCurrentPath", JS_NewCFunction(ctx, JsWallpaperGet, "getCurrentPath", 0));
    JS_SetModuleExport(ctx, m, "wallpaper", wallpaper);

    JSValue power = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, power, "getStatus", JS_NewCFunction(ctx, JsPowerGetStatus, "getStatus", 0));
    JS_SetModuleExport(ctx, m, "power", power);

    JSValue audio = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, audio, "setVolume", JS_NewCFunction(ctx, JsAudioSetVolume, "setVolume", 1));
    JS_SetPropertyStr(ctx, audio, "getVolume", JS_NewCFunction(ctx, JsAudioGetVolume, "getVolume", 0));
    JS_SetPropertyStr(ctx, audio, "playSound", JS_NewCFunction(ctx, JsAudioPlaySound, "playSound", 2));
    JS_SetPropertyStr(ctx, audio, "stopSound", JS_NewCFunction(ctx, JsAudioStopSound, "stopSound", 0));
    JS_SetModuleExport(ctx, m, "audio", audio);

    JSValue brightness = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, brightness, "getValue", JS_NewCFunction(ctx, JsBrightnessGetValue, "getValue", 1));
    JS_SetPropertyStr(ctx, brightness, "setValue", JS_NewCFunction(ctx, JsBrightnessSetValue, "setValue", 1));
    JS_SetModuleExport(ctx, m, "brightness", brightness);

    JSValue fileIcon = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, fileIcon, "extractIcon", JS_NewCFunction(ctx, JsFileIconExtractIcon, "extractIcon", 3));
    JS_SetPropertyStr(ctx, fileIcon, "extractFileIcon", JS_NewCFunction(ctx, JsFileIconExtractIcon, "extractFileIcon", 3));
    JS_SetModuleExport(ctx, m, "fileIcon", fileIcon);

    JSValue displayMetrics = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, displayMetrics, "getMetrics", JS_NewCFunction(ctx, JsDisplayMetricsGetMetrics, "getMetrics", 0));
    JS_SetPropertyStr(ctx, displayMetrics, "get", JS_NewCFunction(ctx, JsDisplayMetricsGetMetrics, "get", 0));
    JS_SetModuleExport(ctx, m, "displayMetrics", displayMetrics);

    return 0;
}
}  // namespace

JSModuleDef* EnsureSystemModule(JSContext* ctx, const char* moduleName) {
    JSModuleDef* m = JS_NewCModule(ctx, moduleName, SystemModuleInit);
    if (!m) return nullptr;
    if (JS_AddModuleExport(ctx, m, "clipboard") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "wallpaper") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "power") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "audio") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "brightness") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "fileIcon") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "displayMetrics") < 0) return nullptr;
    return m;
}
}  // namespace novadesk::scripting::quickjs
