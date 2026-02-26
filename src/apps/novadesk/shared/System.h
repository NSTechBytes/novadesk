#pragma once

#include <vector>
#include <string>

namespace novadesk::shared::system {

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

bool ClipboardSetText(const std::wstring& text);
bool ClipboardGetText(std::wstring& outText);

bool SetWallpaper(const std::wstring& imagePath, const std::wstring& style = L"fill");
bool GetCurrentWallpaperPath(std::wstring& outPath);

bool GetPowerStatus(PowerStatus& outStatus);
DisplayMetrics GetDisplayMetrics();

bool AudioSetVolume(int volumePercent);
int AudioGetVolume();
bool AudioPlaySound(const std::wstring& path, bool loop);
void AudioStopSound();

bool RegistryReadData(const std::wstring& fullPath, const std::wstring& valueName, RegistryValue& outValue);
bool RegistryWriteString(const std::wstring& fullPath, const std::wstring& valueName, const std::wstring& value);
bool RegistryWriteNumber(const std::wstring& fullPath, const std::wstring& valueName, double value);

}  // namespace novadesk::shared::system
