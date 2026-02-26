#pragma once

#include <windows.h>
#include <vector>
#include <string>

namespace novadesk::shared::system {
constexpr UINT WM_NOVADESK_HOTKEY_UP = WM_APP + 120;

struct DisplayRect {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;
};

struct DisplayMonitorInfo {
    bool active = false;
    std::wstring deviceName;
    std::wstring monitorName;
    DisplayRect screen;
};

struct DisplayMetrics {
    int virtualTop = 0;
    int virtualLeft = 0;
    int virtualHeight = 0;
    int virtualWidth = 0;
    int primaryIndex = 0;
    std::vector<DisplayMonitorInfo> monitors;
};

struct PowerStatus {
    int acline = 0;
    int status = 0;
    int status2 = 0;
    double lifetime = 0.0;
    int percent = 0;
    double mhz = 0.0;
    double hz = 0.0;
};

struct CpuStats {
    double usage = 0.0;
};

struct MemoryStats {
    double total = 0.0;
    double available = 0.0;
    double used = 0.0;
    int percent = 0;
};

struct NetworkStats {
    double netIn = 0.0;
    double netOut = 0.0;
    double totalIn = 0.0;
    double totalOut = 0.0;
};

struct MousePosition {
    int x = 0;
    int y = 0;
};

enum class RegistryValueType {
    None = 0,
    String,
    Number
};

struct RegistryValue {
    RegistryValueType type = RegistryValueType::None;
    std::wstring stringValue;
    double numberValue = 0.0;
};

struct HotkeyBinding {
    int onKeyDownCallbackId = -1;
    int onKeyUpCallbackId = -1;
};

struct PathParts {
    std::wstring root;
    std::wstring dir;
    std::wstring base;
    std::wstring ext;
    std::wstring name;
};

bool ClipboardSetText(const std::wstring& text);
bool ClipboardGetText(std::wstring& outText);

bool SetWallpaper(const std::wstring& imagePath, const std::wstring& style = L"fill");
bool GetCurrentWallpaperPath(std::wstring& outPath);

bool GetPowerStatus(PowerStatus& outStatus);
bool GetCpuStats(CpuStats& outStats);
bool GetMemoryStats(MemoryStats& outStats);
bool GetNetworkStats(NetworkStats& outStats);
bool GetMousePosition(MousePosition& outPos);
DisplayMetrics GetDisplayMetrics();

bool AudioSetVolume(int volumePercent);
int AudioGetVolume();
bool AudioPlaySound(const std::wstring& path, bool loop);
void AudioStopSound();

bool RegistryReadData(const std::wstring& fullPath, const std::wstring& valueName, RegistryValue& outValue);
bool RegistryWriteString(const std::wstring& fullPath, const std::wstring& valueName, const std::wstring& value);
bool RegistryWriteNumber(const std::wstring& fullPath, const std::wstring& valueName, double value);

int RegisterHotkey(HWND messageWindow, const std::wstring& hotkey, int onKeyDownCallbackId, int onKeyUpCallbackId);
bool UnregisterHotkey(HWND messageWindow, int id);
void ClearHotkeys(HWND messageWindow);
bool ResolveHotkeyMessage(int id, HotkeyBinding& outBinding);

std::wstring PathJoin(const std::vector<std::wstring>& parts);
std::wstring PathBasename(const std::wstring& path, const std::wstring& ext = L"");
std::wstring PathDirname(const std::wstring& path);
std::wstring PathExtname(const std::wstring& path);
bool PathIsAbsolute(const std::wstring& path);
std::wstring PathNormalize(const std::wstring& path);
std::wstring PathRelative(const std::wstring& from, const std::wstring& to);
PathParts PathParse(const std::wstring& path);
std::wstring PathFormat(const PathParts& parts);

}  // namespace novadesk::shared::system
