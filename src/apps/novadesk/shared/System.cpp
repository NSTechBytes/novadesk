#include "System.h"

#include <cstring>
#include <cwchar>

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

}  // namespace novadesk::shared::system
