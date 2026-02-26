#include "System.h"

#include <cstring>
#include <cwchar>
#include <unordered_map>

#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <endpointvolume.h>
#include <powrprof.h>

#include "../domain/DesktopManager.h"
#include "Utils.h"

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "PowrProf.lib")

namespace novadesk::shared::system {

namespace {
struct RegisteredHotkey {
    HotkeyBinding binding;
    UINT modifiers = 0;
    UINT vk = 0;
    bool pressed = false;
};

std::unordered_map<int, RegisteredHotkey> g_hotkeys;
int g_nextHotkeyId = 10000;
HHOOK g_keyboardHook = nullptr;
HWND g_hotkeyMessageWindow = nullptr;

typedef struct _PROCESSOR_POWER_INFORMATION_LOCAL {
    ULONG Number;
    ULONG MaxMhz;
    ULONG CurrentMhz;
    ULONG MhzLimit;
    ULONG MaxIdleState;
    ULONG CurrentIdleState;
} PROCESSOR_POWER_INFORMATION_LOCAL;

IAudioEndpointVolume* GetVolumeInterface() {
    IMMDeviceEnumerator* enumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioEndpointVolume* volume = nullptr;

    HRESULT hr = CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void**>(&enumerator)
    );
    if (FAILED(hr) || !enumerator) {
        return nullptr;
    }

    hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
    enumerator->Release();
    if (FAILED(hr) || !device) {
        return nullptr;
    }

    hr = device->Activate(
        __uuidof(IAudioEndpointVolume),
        CLSCTX_ALL,
        nullptr,
        reinterpret_cast<void**>(&volume)
    );
    device->Release();
    if (FAILED(hr)) {
        return nullptr;
    }

    return volume;
}

HKEY GetRegistryRootKey(const std::wstring& root) {
    if (root == L"HKCU" || root == L"HKEY_CURRENT_USER") return HKEY_CURRENT_USER;
    if (root == L"HKLM" || root == L"HKEY_LOCAL_MACHINE") return HKEY_LOCAL_MACHINE;
    if (root == L"HKCR" || root == L"HKEY_CLASSES_ROOT") return HKEY_CLASSES_ROOT;
    if (root == L"HKU" || root == L"HKEY_USERS") return HKEY_USERS;
    return nullptr;
}

bool SplitRegistryPath(const std::wstring& fullPath, HKEY& outRoot, std::wstring& outSubKey) {
    const size_t p = fullPath.find(L'\\');
    if (p == std::wstring::npos) {
        return false;
    }
    const std::wstring rootPart = fullPath.substr(0, p);
    outSubKey = fullPath.substr(p + 1);
    outRoot = GetRegistryRootKey(rootPart);
    return outRoot != nullptr && !outSubKey.empty();
}

bool ParseHotkeyString(const std::wstring& hotkey, UINT& modifiers, UINT& vk) {
    modifiers = 0;
    vk = 0;

    size_t start = 0;
    while (start <= hotkey.size()) {
        size_t plus = hotkey.find(L'+', start);
        std::wstring token = (plus == std::wstring::npos) ? hotkey.substr(start) : hotkey.substr(start, plus - start);
        token = Utils::TrimUpper(token);

        if (token == L"CTRL" || token == L"CONTROL") modifiers |= MOD_CONTROL;
        else if (token == L"ALT") modifiers |= MOD_ALT;
        else if (token == L"SHIFT") modifiers |= MOD_SHIFT;
        else if (token == L"WIN" || token == L"WINDOWS") modifiers |= MOD_WIN;
        else if (token.size() == 1 && token[0] >= L'A' && token[0] <= L'Z') vk = static_cast<UINT>(token[0]);
        else if (token.size() == 1 && token[0] >= L'0' && token[0] <= L'9') vk = static_cast<UINT>(token[0]);
        else if (token.size() >= 2 && token[0] == L'F') {
            int f = _wtoi(token.c_str() + 1);
            if (f >= 1 && f <= 24) vk = VK_F1 + (f - 1);
        } else if (token == L"SPACE") vk = VK_SPACE;
        else if (token == L"ENTER" || token == L"RETURN") vk = VK_RETURN;
        else if (token == L"TAB") vk = VK_TAB;
        else if (token == L"ESC" || token == L"ESCAPE") vk = VK_ESCAPE;
        else if (token == L"BACKSPACE") vk = VK_BACK;
        else if (token == L"DELETE" || token == L"DEL") vk = VK_DELETE;
        else if (token == L"INSERT" || token == L"INS") vk = VK_INSERT;
        else if (token == L"HOME") vk = VK_HOME;
        else if (token == L"END") vk = VK_END;
        else if (token == L"PAGEUP" || token == L"PGUP") vk = VK_PRIOR;
        else if (token == L"PAGEDOWN" || token == L"PGDN") vk = VK_NEXT;
        else if (token == L"LEFT") vk = VK_LEFT;
        else if (token == L"RIGHT") vk = VK_RIGHT;
        else if (token == L"UP") vk = VK_UP;
        else if (token == L"DOWN") vk = VK_DOWN;

        if (plus == std::wstring::npos) break;
        start = plus + 1;
    }
    return vk != 0;
}

bool IsModifierVk(UINT vk) {
    return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
           vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
           vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
           vk == VK_LWIN || vk == VK_RWIN;
}

bool IsDownWithEvent(UINT checkVk, UINT eventVk, bool eventDown) {
    if (checkVk == VK_CONTROL) {
        if (eventVk == VK_LCONTROL || eventVk == VK_RCONTROL || eventVk == VK_CONTROL) return eventDown;
        return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    }
    if (checkVk == VK_MENU) {
        if (eventVk == VK_LMENU || eventVk == VK_RMENU || eventVk == VK_MENU) return eventDown;
        return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    }
    if (checkVk == VK_SHIFT) {
        if (eventVk == VK_LSHIFT || eventVk == VK_RSHIFT || eventVk == VK_SHIFT) return eventDown;
        return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    }
    if (checkVk == VK_LWIN || checkVk == VK_RWIN) {
        if (eventVk == checkVk) return eventDown;
        return (GetAsyncKeyState(checkVk) & 0x8000) != 0;
    }
    if (eventVk == checkVk) return eventDown;
    return (GetAsyncKeyState(checkVk) & 0x8000) != 0;
}

bool IsHotkeyPressedNow(const RegisteredHotkey& hk, UINT eventVk, bool eventDown) {
    if ((hk.modifiers & MOD_CONTROL) && !IsDownWithEvent(VK_CONTROL, eventVk, eventDown)) return false;
    if ((hk.modifiers & MOD_ALT) && !IsDownWithEvent(VK_MENU, eventVk, eventDown)) return false;
    if ((hk.modifiers & MOD_SHIFT) && !IsDownWithEvent(VK_SHIFT, eventVk, eventDown)) return false;
    if ((hk.modifiers & MOD_WIN) &&
        !IsDownWithEvent(VK_LWIN, eventVk, eventDown) &&
        !IsDownWithEvent(VK_RWIN, eventVk, eventDown)) return false;
    return IsDownWithEvent(hk.vk, eventVk, eventDown);
}

LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && lParam) {
        const auto* kb = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
        const UINT vk = kb->vkCode;
        const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
        if ((isDown || isUp) && g_hotkeyMessageWindow) {
            const bool relevant = IsModifierVk(vk);
            for (auto& kv : g_hotkeys) {
                RegisteredHotkey& hk = kv.second;
                if (!relevant && vk != hk.vk) {
                    continue;
                }

                const bool nowPressed = IsHotkeyPressedNow(hk, vk, isDown);
                if (nowPressed && !hk.pressed) {
                    hk.pressed = true;
                    PostMessageW(g_hotkeyMessageWindow, WM_HOTKEY, static_cast<WPARAM>(kv.first), 0);
                } else if (!nowPressed && hk.pressed) {
                    hk.pressed = false;
                    PostMessageW(g_hotkeyMessageWindow, WM_NOVADESK_HOTKEY_UP, static_cast<WPARAM>(kv.first), 0);
                }
            }
        }
    }
    return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
}

void EnsureKeyboardHook(HWND messageWindow) {
    g_hotkeyMessageWindow = messageWindow;
    if (!g_keyboardHook) {
        g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandleW(nullptr), 0);
    }
}

void MaybeRemoveKeyboardHook() {
    if (g_hotkeys.empty() && g_keyboardHook) {
        UnhookWindowsHookEx(g_keyboardHook);
        g_keyboardHook = nullptr;
        g_hotkeyMessageWindow = nullptr;
    }
}

}  // namespace

bool ClipboardSetText(const std::wstring& text) {
    if (!OpenClipboard(nullptr)) {
        return false;
    }
    EmptyClipboard();

    const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!hMem) {
        CloseClipboard();
        return false;
    }

    void* ptr = GlobalLock(hMem);
    if (!ptr) {
        GlobalFree(hMem);
        CloseClipboard();
        return false;
    }
    memcpy(ptr, text.c_str(), bytes);
    GlobalUnlock(hMem);

    if (!SetClipboardData(CF_UNICODETEXT, hMem)) {
        GlobalFree(hMem);
        CloseClipboard();
        return false;
    }

    CloseClipboard();
    return true;
}

bool ClipboardGetText(std::wstring& outText) {
    outText.clear();
    if (!OpenClipboard(nullptr)) {
        return false;
    }

    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        CloseClipboard();
        return false;
    }

    const wchar_t* text = static_cast<const wchar_t*>(GlobalLock(hData));
    if (!text) {
        CloseClipboard();
        return false;
    }

    outText = text;
    GlobalUnlock(hData);
    CloseClipboard();
    return true;
}

bool SetWallpaper(const std::wstring& imagePath, const std::wstring& style) {
    const wchar_t* wallpaperStyle = L"10";
    const wchar_t* tileWallpaper = L"0";
    if (style == L"fill") {
        wallpaperStyle = L"10"; tileWallpaper = L"0";
    } else if (style == L"fit") {
        wallpaperStyle = L"6"; tileWallpaper = L"0";
    } else if (style == L"stretch") {
        wallpaperStyle = L"2"; tileWallpaper = L"0";
    } else if (style == L"tile") {
        wallpaperStyle = L"0"; tileWallpaper = L"1";
    } else if (style == L"center") {
        wallpaperStyle = L"0"; tileWallpaper = L"0";
    } else if (style == L"span") {
        wallpaperStyle = L"22"; tileWallpaper = L"0";
    }

    HKEY key = nullptr;
    const LONG openRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &key);
    if (openRes == ERROR_SUCCESS) {
        RegSetValueExW(
            key,
            L"WallpaperStyle",
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(wallpaperStyle),
            static_cast<DWORD>((wcslen(wallpaperStyle) + 1) * sizeof(wchar_t)));
        RegSetValueExW(
            key,
            L"TileWallpaper",
            0,
            REG_SZ,
            reinterpret_cast<const BYTE*>(tileWallpaper),
            static_cast<DWORD>((wcslen(tileWallpaper) + 1) * sizeof(wchar_t)));
        RegCloseKey(key);
    }

    return SystemParametersInfoW(
               SPI_SETDESKWALLPAPER,
               0,
               const_cast<wchar_t*>(imagePath.c_str()),
               SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE) == TRUE;
}

bool GetCurrentWallpaperPath(std::wstring& outPath) {
    outPath.assign(MAX_PATH, L'\0');
    const BOOL ok = SystemParametersInfoW(
        SPI_GETDESKWALLPAPER,
        static_cast<UINT>(outPath.size()),
        outPath.data(),
        0);
    if (!ok) {
        outPath.clear();
        return false;
    }
    outPath.resize(wcslen(outPath.c_str()));
    return !outPath.empty();
}

bool GetPowerStatus(PowerStatus& outStatus) {
    SYSTEM_POWER_STATUS sps{};
    if (!GetSystemPowerStatus(&sps)) {
        return false;
    }

    outStatus.acline = (sps.ACLineStatus == 1) ? 1 : 0;

    int status = 4;
    if (sps.BatteryFlag & 128) status = 0;
    else if (sps.BatteryFlag & 8) status = 1;
    else if (sps.BatteryFlag & 4) status = 2;
    else if (sps.BatteryFlag & 2) status = 3;

    outStatus.status = status;
    outStatus.status2 = sps.BatteryFlag;
    outStatus.lifetime = static_cast<double>(sps.BatteryLifeTime);
    outStatus.percent = (sps.BatteryLifePercent == 255) ? 0 : sps.BatteryLifePercent;

    SYSTEM_INFO si{};
    GetSystemInfo(&si);
    const UINT cpuCount = static_cast<UINT>(si.dwNumberOfProcessors);
    outStatus.mhz = 0.0;

    if (cpuCount > 0) {
        auto* ppi = new PROCESSOR_POWER_INFORMATION_LOCAL[cpuCount];
        if (CallNtPowerInformation(
                ProcessorInformation,
                nullptr,
                0,
                ppi,
                sizeof(PROCESSOR_POWER_INFORMATION_LOCAL) * cpuCount) == 0) {
            outStatus.mhz = static_cast<double>(ppi[0].CurrentMhz);
        }
        delete[] ppi;
    }

    outStatus.hz = outStatus.mhz * 1000000.0;
    return true;
}

DisplayMetrics GetDisplayMetrics() {
    const auto& src = ::System::GetMultiMonitorInfo();
    DisplayMetrics out;
    out.virtualTop = src.vsT;
    out.virtualLeft = src.vsL;
    out.virtualHeight = src.vsH;
    out.virtualWidth = src.vsW;
    out.primaryIndex = src.primaryIndex;

    out.monitors.reserve(src.monitors.size());
    for (const auto& m : src.monitors) {
        DisplayMonitorInfo dm;
        dm.active = m.active;
        dm.deviceName = m.deviceName;
        dm.monitorName = m.monitorName;
        dm.screen.left = m.screen.left;
        dm.screen.top = m.screen.top;
        dm.screen.right = m.screen.right;
        dm.screen.bottom = m.screen.bottom;
        out.monitors.push_back(dm);
    }
    return out;
}

bool AudioSetVolume(int volumePercent) {
    if (volumePercent < 0) volumePercent = 0;
    if (volumePercent > 100) volumePercent = 100;

    IAudioEndpointVolume* volume = GetVolumeInterface();
    if (!volume) {
        return false;
    }

    const float scalar = static_cast<float>(volumePercent) / 100.0f;
    const HRESULT hr = volume->SetMasterVolumeLevelScalar(scalar, nullptr);
    volume->Release();
    return SUCCEEDED(hr);
}

int AudioGetVolume() {
    IAudioEndpointVolume* volume = GetVolumeInterface();
    if (!volume) {
        return 0;
    }

    float scalar = 0.0f;
    volume->GetMasterVolumeLevelScalar(&scalar);
    volume->Release();

    const int result = static_cast<int>(scalar * 100.0f + 0.5f);
    return (result < 0) ? 0 : ((result > 100) ? 100 : result);
}

bool AudioPlaySound(const std::wstring& path, bool loop) {
    DWORD flags = SND_FILENAME | SND_ASYNC | SND_NODEFAULT;
    if (loop) {
        flags |= SND_LOOP;
    }
    return PlaySoundW(path.c_str(), nullptr, flags) == TRUE;
}

void AudioStopSound() {
    PlaySoundW(nullptr, nullptr, 0);
}

bool RegistryReadData(const std::wstring& fullPath, const std::wstring& valueName, RegistryValue& outValue) {
    outValue = RegistryValue{};

    HKEY root = nullptr;
    std::wstring subKey;
    if (!SplitRegistryPath(fullPath, root, subKey)) {
        return false;
    }

    HKEY key = nullptr;
    if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS) {
        return false;
    }

    DWORD type = 0;
    DWORD size = 0;
    if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, nullptr, &size) != ERROR_SUCCESS) {
        RegCloseKey(key);
        return false;
    }

    if (type == REG_SZ || type == REG_EXPAND_SZ) {
        std::vector<wchar_t> buf(size / sizeof(wchar_t) + 1, L'\0');
        if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buf.data()), &size) == ERROR_SUCCESS) {
            outValue.type = RegistryValueType::String;
            outValue.stringValue = buf.data();
            RegCloseKey(key);
            return true;
        }
    } else if (type == REG_DWORD) {
        DWORD value = 0;
        DWORD dwordSize = sizeof(value);
        if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(&value), &dwordSize) == ERROR_SUCCESS) {
            outValue.type = RegistryValueType::Number;
            outValue.numberValue = static_cast<double>(value);
            RegCloseKey(key);
            return true;
        }
    }

    RegCloseKey(key);
    return false;
}

bool RegistryWriteString(const std::wstring& fullPath, const std::wstring& valueName, const std::wstring& value) {
    HKEY root = nullptr;
    std::wstring subKey;
    if (!SplitRegistryPath(fullPath, root, subKey)) {
        return false;
    }

    HKEY key = nullptr;
    if (RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    const auto* raw = reinterpret_cast<const BYTE*>(value.c_str());
    const DWORD rawSize = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
    const bool ok = (RegSetValueExW(key, valueName.c_str(), 0, REG_SZ, raw, rawSize) == ERROR_SUCCESS);
    RegCloseKey(key);
    return ok;
}

bool RegistryWriteNumber(const std::wstring& fullPath, const std::wstring& valueName, double value) {
    HKEY root = nullptr;
    std::wstring subKey;
    if (!SplitRegistryPath(fullPath, root, subKey)) {
        return false;
    }

    HKEY key = nullptr;
    if (RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS) {
        return false;
    }

    const DWORD dword = static_cast<DWORD>(value);
    const bool ok = (RegSetValueExW(
        key,
        valueName.c_str(),
        0,
        REG_DWORD,
        reinterpret_cast<const BYTE*>(&dword),
        sizeof(dword)) == ERROR_SUCCESS);
    RegCloseKey(key);
    return ok;
}

int RegisterHotkey(HWND messageWindow, const std::wstring& hotkey, int onKeyDownCallbackId, int onKeyUpCallbackId) {
    if (!messageWindow) return -1;
    UINT modifiers = 0;
    UINT vk = 0;
    if (!ParseHotkeyString(hotkey, modifiers, vk)) return -1;

    int id = g_nextHotkeyId++;
    EnsureKeyboardHook(messageWindow);
    if (!g_keyboardHook) return -1;

    RegisteredHotkey entry{};
    entry.binding.onKeyDownCallbackId = onKeyDownCallbackId;
    entry.binding.onKeyUpCallbackId = onKeyUpCallbackId;
    entry.modifiers = modifiers;
    entry.vk = vk;
    g_hotkeys[id] = entry;
    return id;
}

bool UnregisterHotkey(HWND messageWindow, int id) {
    auto it = g_hotkeys.find(id);
    if (it == g_hotkeys.end()) return false;
    (void)messageWindow;
    g_hotkeys.erase(it);
    MaybeRemoveKeyboardHook();
    return true;
}

void ClearHotkeys(HWND messageWindow) {
    (void)messageWindow;
    g_hotkeys.clear();
    MaybeRemoveKeyboardHook();
}

bool ResolveHotkeyMessage(int id, HotkeyBinding& outBinding) {
    auto it = g_hotkeys.find(id);
    if (it == g_hotkeys.end()) return false;
    outBinding = it->second.binding;
    return true;
}

}  // namespace novadesk::shared::system
