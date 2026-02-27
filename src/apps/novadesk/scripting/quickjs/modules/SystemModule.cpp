#include "SystemModule.h"

#include <chrono>
#include <string>

#include "../../shared/System.h"
#include "../../shared/Utils.h"
#include "../engine/JSEngine.h"

namespace novadesk::scripting::quickjs {
namespace {
shared::system::NetworkStats g_cachedNetworkStats{};
std::chrono::steady_clock::time_point g_cachedNetworkAt = std::chrono::steady_clock::time_point::min();

bool ReadNetworkCached(shared::system::NetworkStats& out) {
    const auto now = std::chrono::steady_clock::now();
    constexpr auto kMinResample = std::chrono::milliseconds(400);

    if (g_cachedNetworkAt == std::chrono::steady_clock::time_point::min() ||
        (now - g_cachedNetworkAt) >= kMinResample) {
        if (!shared::system::GetNetworkStats(g_cachedNetworkStats)) {
            return false;
        }
        g_cachedNetworkAt = now;
    }

    out = g_cachedNetworkStats;
    return true;
}

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

JSValue JsCpuUsage(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::CpuStats stats;
    if (!shared::system::GetCpuStats(stats)) {
        return JS_NewFloat64(ctx, 0.0);
    }
    return JS_NewFloat64(ctx, stats.usage);
}

JSValue JsMemoryTotalBytes(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::MemoryStats stats;
    if (!shared::system::GetMemoryStats(stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.total);
}

JSValue JsMemoryAvailableBytes(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::MemoryStats stats;
    if (!shared::system::GetMemoryStats(stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.available);
}

JSValue JsMemoryUsedBytes(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::MemoryStats stats;
    if (!shared::system::GetMemoryStats(stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.used);
}

JSValue JsMemoryUsagePercent(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::MemoryStats stats;
    if (!shared::system::GetMemoryStats(stats)) return JS_NewInt32(ctx, 0);
    return JS_NewInt32(ctx, stats.percent);
}

JSValue JsNetworkRxSpeed(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::NetworkStats stats;
    if (!ReadNetworkCached(stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.netIn);
}

JSValue JsNetworkTxSpeed(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::NetworkStats stats;
    if (!ReadNetworkCached(stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.netOut);
}

JSValue JsNetworkBytesReceived(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::NetworkStats stats;
    if (!ReadNetworkCached(stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.totalIn);
}

JSValue JsNetworkBytesSent(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::NetworkStats stats;
    if (!ReadNetworkCached(stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.totalOut);
}

JSValue JsMouseClientX(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::MousePosition pos;
    if (!shared::system::GetMousePosition(pos)) {
        return JS_NewInt32(ctx, 0);
    }
    return JS_NewInt32(ctx, pos.x);
}

JSValue JsMouseClientY(JSContext* ctx, JSValueConst, int, JSValueConst*) {
    shared::system::MousePosition pos;
    if (!shared::system::GetMousePosition(pos)) {
        return JS_NewInt32(ctx, 0);
    }
    return JS_NewInt32(ctx, pos.y);
}

std::wstring ReadOptionalPathArg(JSContext* ctx, int argc, JSValueConst* argv) {
    if (argc < 1 || JS_IsUndefined(argv[0]) || JS_IsNull(argv[0])) {
        return L"";
    }
    const char* s = JS_ToCString(ctx, argv[0]);
    if (!s) return L"";
    std::wstring out = Utils::ToWString(s);
    JS_FreeCString(ctx, s);
    return out;
}

JSValue JsDiskTotalBytes(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    shared::system::DiskStats stats;
    if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.total);
}

JSValue JsDiskAvailableBytes(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    shared::system::DiskStats stats;
    if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.available);
}

JSValue JsDiskUsedBytes(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    shared::system::DiskStats stats;
    if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats)) return JS_NewFloat64(ctx, 0.0);
    return JS_NewFloat64(ctx, stats.used);
}

JSValue JsDiskUsagePercent(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    shared::system::DiskStats stats;
    if (!shared::system::GetDiskStats(ReadOptionalPathArg(ctx, argc, argv), stats)) return JS_NewInt32(ctx, 0);
    return JS_NewInt32(ctx, stats.percent);
}

JSValue JsAudioLevelStats(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    shared::system::AudioLevelConfig cfg;

    if (argc > 0 && !JS_IsUndefined(argv[0]) && !JS_IsNull(argv[0]) && JS_IsObject(argv[0])) {
        JSValue v = JS_GetPropertyStr(ctx, argv[0], "port");
        if (!JS_IsUndefined(v) && !JS_IsNull(v)) {
            const char* s = JS_ToCString(ctx, v);
            if (s) { cfg.port = s; JS_FreeCString(ctx, s); }
        }
        JS_FreeValue(ctx, v);

        v = JS_GetPropertyStr(ctx, argv[0], "id");
        if (!JS_IsUndefined(v) && !JS_IsNull(v)) {
            const char* s = JS_ToCString(ctx, v);
            if (s) { cfg.deviceId = Utils::ToWString(s); JS_FreeCString(ctx, s); }
        }
        JS_FreeValue(ctx, v);

        auto readInt = [&](const char* key, int& dst) {
            JSValue iv = JS_GetPropertyStr(ctx, argv[0], key);
            if (!JS_IsUndefined(iv) && !JS_IsNull(iv)) {
                int32_t x = dst;
                if (JS_ToInt32(ctx, &x, iv) == 0) dst = static_cast<int>(x);
            }
            JS_FreeValue(ctx, iv);
        };
        auto readDouble = [&](const char* key, double& dst) {
            JSValue dv = JS_GetPropertyStr(ctx, argv[0], key);
            if (!JS_IsUndefined(dv) && !JS_IsNull(dv)) {
                double x = dst;
                if (JS_ToFloat64(ctx, &x, dv) == 0) dst = x;
            }
            JS_FreeValue(ctx, dv);
        };

        readInt("fftSize", cfg.fftSize);
        readInt("fftOverlap", cfg.fftOverlap);
        readInt("bands", cfg.bands);
        readInt("rmsAttack", cfg.rmsAttack);
        readInt("rmsDecay", cfg.rmsDecay);
        readInt("peakAttack", cfg.peakAttack);
        readInt("peakDecay", cfg.peakDecay);
        readInt("fftAttack", cfg.fftAttack);
        readInt("fftDecay", cfg.fftDecay);

        readDouble("freqMin", cfg.freqMin);
        readDouble("freqMax", cfg.freqMax);
        readDouble("sensitivity", cfg.sensitivity);
        readDouble("rmsGain", cfg.rmsGain);
        readDouble("peakGain", cfg.peakGain);
    }

    shared::system::AudioLevelStats stats;
    if (!shared::system::GetAudioLevelStats(stats, cfg)) {
        return JS_NULL;
    }

    JSValue out = JS_NewObject(ctx);

    JSValue rms = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, rms, 0, JS_NewFloat64(ctx, stats.rms[0]));
    JS_SetPropertyUint32(ctx, rms, 1, JS_NewFloat64(ctx, stats.rms[1]));
    JS_SetPropertyStr(ctx, out, "rms", rms);

    JSValue peak = JS_NewArray(ctx);
    JS_SetPropertyUint32(ctx, peak, 0, JS_NewFloat64(ctx, stats.peak[0]));
    JS_SetPropertyUint32(ctx, peak, 1, JS_NewFloat64(ctx, stats.peak[1]));
    JS_SetPropertyStr(ctx, out, "peak", peak);

    JSValue bandsArr = JS_NewArray(ctx);
    for (uint32_t i = 0; i < static_cast<uint32_t>(stats.bands.size()); ++i) {
        JS_SetPropertyUint32(ctx, bandsArr, i, JS_NewFloat64(ctx, stats.bands[i]));
    }
    JS_SetPropertyStr(ctx, out, "bands", bandsArr);

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

JSValue JsRegistryReadData(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_ThrowTypeError(ctx, "registry.readData(path, valueName)");
    const char* p = JS_ToCString(ctx, argv[0]);
    const char* v = JS_ToCString(ctx, argv[1]);
    if (!p || !v) {
        if (p) JS_FreeCString(ctx, p);
        if (v) JS_FreeCString(ctx, v);
        return JS_EXCEPTION;
    }

    std::wstring fullPath = Utils::ToWString(p);
    std::wstring valueName = Utils::ToWString(v);
    JS_FreeCString(ctx, p);
    JS_FreeCString(ctx, v);

    shared::system::RegistryValue out;
    if (!shared::system::RegistryReadData(fullPath, valueName, out)) {
        return JS_NULL;
    }

    if (out.type == shared::system::RegistryValueType::String) {
        return JS_NewString(ctx, Utils::ToString(out.stringValue).c_str());
    }
    if (out.type == shared::system::RegistryValueType::Number) {
        return JS_NewFloat64(ctx, out.numberValue);
    }
    return JS_NULL;
}

JSValue JsRegistryWriteData(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 3) return JS_ThrowTypeError(ctx, "registry.writeData(path, valueName, value)");
    const char* p = JS_ToCString(ctx, argv[0]);
    const char* v = JS_ToCString(ctx, argv[1]);
    if (!p || !v) {
        if (p) JS_FreeCString(ctx, p);
        if (v) JS_FreeCString(ctx, v);
        return JS_EXCEPTION;
    }

    std::wstring fullPath = Utils::ToWString(p);
    std::wstring valueName = Utils::ToWString(v);
    JS_FreeCString(ctx, p);
    JS_FreeCString(ctx, v);

    bool ok = false;
    if (JS_IsString(argv[2])) {
        const char* s = JS_ToCString(ctx, argv[2]);
        if (!s) return JS_EXCEPTION;
        ok = shared::system::RegistryWriteString(fullPath, valueName, Utils::ToWString(s));
        JS_FreeCString(ctx, s);
    } else {
        double n = 0;
        if (JS_ToFloat64(ctx, &n, argv[2]) == 0) {
            ok = shared::system::RegistryWriteNumber(fullPath, valueName, n);
        }
    }

    return JS_NewBool(ctx, ok ? 1 : 0);
}

JSValue JsHotkeyRegisterHotkey(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 2) return JS_ThrowTypeError(ctx, "hotkey.register(hotkey, handler)");
    const char* hotkeyC = JS_ToCString(ctx, argv[0]);
    if (!hotkeyC) return JS_EXCEPTION;
    std::wstring hotkey = Utils::ToWString(hotkeyC);
    JS_FreeCString(ctx, hotkeyC);

    int keyDownId = -1;
    int keyUpId = -1;
    if (JS_IsFunction(ctx, argv[1])) {
        keyDownId = JSEngine::RegisterEventCallback(ctx, argv[1]);
    } else if (JS_IsObject(argv[1])) {
        JSValue kd = JS_GetPropertyStr(ctx, argv[1], "onKeyDown");
        if (JS_IsFunction(ctx, kd)) {
            keyDownId = JSEngine::RegisterEventCallback(ctx, kd);
        }
        JS_FreeValue(ctx, kd);
        JSValue ku = JS_GetPropertyStr(ctx, argv[1], "onKeyUp");
        if (JS_IsFunction(ctx, ku)) {
            keyUpId = JSEngine::RegisterEventCallback(ctx, ku);
        }
        JS_FreeValue(ctx, ku);
    } else {
        return JS_ThrowTypeError(ctx, "hotkey.register handler must be function or object");
    }

    int id = shared::system::RegisterHotkey(JSEngine::GetMessageWindow(), hotkey, keyDownId, keyUpId);
    return JS_NewInt32(ctx, id);
}

JSValue JsHotkeyUnregisterHotkey(JSContext* ctx, JSValueConst, int argc, JSValueConst* argv) {
    if (argc < 1) return JS_ThrowTypeError(ctx, "hotkey.unregister(id)");
    int32_t id = 0;
    if (JS_ToInt32(ctx, &id, argv[0]) != 0) {
        return JS_ThrowTypeError(ctx, "hotkey.unregister(id) expects number");
    }
    return JS_NewBool(ctx, shared::system::UnregisterHotkey(JSEngine::GetMessageWindow(), id) ? 1 : 0);
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

    JSValue cpu = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, cpu, "usage", JS_NewCFunction(ctx, JsCpuUsage, "usage", 0));
    JS_SetModuleExport(ctx, m, "cpu", cpu);

    JSValue memory = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, memory, "totalBytes", JS_NewCFunction(ctx, JsMemoryTotalBytes, "totalBytes", 0));
    JS_SetPropertyStr(ctx, memory, "availableBytes", JS_NewCFunction(ctx, JsMemoryAvailableBytes, "availableBytes", 0));
    JS_SetPropertyStr(ctx, memory, "usedBytes", JS_NewCFunction(ctx, JsMemoryUsedBytes, "usedBytes", 0));
    JS_SetPropertyStr(ctx, memory, "usagePercent", JS_NewCFunction(ctx, JsMemoryUsagePercent, "usagePercent", 0));
    JS_SetModuleExport(ctx, m, "memory", memory);

    JSValue network = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, network, "rxSpeed", JS_NewCFunction(ctx, JsNetworkRxSpeed, "rxSpeed", 0));
    JS_SetPropertyStr(ctx, network, "txSpeed", JS_NewCFunction(ctx, JsNetworkTxSpeed, "txSpeed", 0));
    JS_SetPropertyStr(ctx, network, "bytesReceived", JS_NewCFunction(ctx, JsNetworkBytesReceived, "bytesReceived", 0));
    JS_SetPropertyStr(ctx, network, "bytesSent", JS_NewCFunction(ctx, JsNetworkBytesSent, "bytesSent", 0));
    JS_SetModuleExport(ctx, m, "network", network);

    JSValue mouse = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, mouse, "clientX", JS_NewCFunction(ctx, JsMouseClientX, "clientX", 0));
    JS_SetPropertyStr(ctx, mouse, "clientY", JS_NewCFunction(ctx, JsMouseClientY, "clientY", 0));
    JS_SetModuleExport(ctx, m, "mouse", mouse);

    JSValue disk = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, disk, "totalBytes", JS_NewCFunction(ctx, JsDiskTotalBytes, "totalBytes", 1));
    JS_SetPropertyStr(ctx, disk, "availableBytes", JS_NewCFunction(ctx, JsDiskAvailableBytes, "availableBytes", 1));
    JS_SetPropertyStr(ctx, disk, "usedBytes", JS_NewCFunction(ctx, JsDiskUsedBytes, "usedBytes", 1));
    JS_SetPropertyStr(ctx, disk, "usagePercent", JS_NewCFunction(ctx, JsDiskUsagePercent, "usagePercent", 1));
    JS_SetModuleExport(ctx, m, "disk", disk);

    JSValue audioLevel = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, audioLevel, "stats", JS_NewCFunction(ctx, JsAudioLevelStats, "stats", 1));
    JS_SetModuleExport(ctx, m, "audioLevel", audioLevel);

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

    JSValue registry = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, registry, "readData", JS_NewCFunction(ctx, JsRegistryReadData, "readData", 2));
    JS_SetPropertyStr(ctx, registry, "writeData", JS_NewCFunction(ctx, JsRegistryWriteData, "writeData", 3));
    JS_SetModuleExport(ctx, m, "registry", registry);

    JSValue hotkey = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, hotkey, "register", JS_NewCFunction(ctx, JsHotkeyRegisterHotkey, "register", 2));
    JS_SetPropertyStr(ctx, hotkey, "unregister", JS_NewCFunction(ctx, JsHotkeyUnregisterHotkey, "unregister", 1));
    JS_SetModuleExport(ctx, m, "hotkey", hotkey);

    return 0;
}
}  // namespace

JSModuleDef* EnsureSystemModule(JSContext* ctx, const char* moduleName) {
    JSModuleDef* m = JS_NewCModule(ctx, moduleName, SystemModuleInit);
    if (!m) return nullptr;
    if (JS_AddModuleExport(ctx, m, "clipboard") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "wallpaper") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "power") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "cpu") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "memory") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "network") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "mouse") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "disk") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "audioLevel") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "audio") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "brightness") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "fileIcon") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "displayMetrics") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "registry") < 0) return nullptr;
    if (JS_AddModuleExport(ctx, m, "hotkey") < 0) return nullptr;
    return m;
}
}  // namespace novadesk::scripting::quickjs
