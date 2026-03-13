#pragma once

#include <windows.h>
#include <vector>
#include <string>
#include <cstdint>

namespace novadesk::shared::system
{
    struct DisplayRect
    {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;
    };

    struct DisplayMonitorInfo
    {
        int id = 0;
        bool active = false;
        std::wstring deviceName;
        std::wstring monitorName;
        DisplayRect work;
        DisplayRect screen;
    };

    struct DisplayMetrics
    {
        int virtualTop = 0;
        int virtualLeft = 0;
        int virtualHeight = 0;
        int virtualWidth = 0;
        int primaryIndex = 0;
        std::vector<DisplayMonitorInfo> monitors;
    };

    struct PowerStatus
    {
        int acline = 0;
        int status = 0;
        int status2 = 0;
        double lifetime = 0.0;
        int percent = 0;
        double mhz = 0.0;
        double hz = 0.0;
    };

    struct CpuStats
    {
        double usage = 0.0;
    };

    struct MemoryStats
    {
        double total = 0.0;
        double available = 0.0;
        double used = 0.0;
        int percent = 0;
    };

    struct NetworkStats
    {
        double netIn = 0.0;
        double netOut = 0.0;
        double totalIn = 0.0;
        double totalOut = 0.0;
    };

    struct MousePosition
    {
        int x = 0;
        int y = 0;
    };

    struct DiskStats
    {
        double total = 0.0;
        double available = 0.0;
        double used = 0.0;
        int percent = 0;
    };

    struct AppVolumeSessionInfo
    {
        uint32_t pid = 0;
        std::wstring processName;
        std::wstring fileName;
        std::wstring filePath;
        std::wstring iconPath;
        std::wstring displayName;
        float volume = 0.0f; // 0..1
        float peak = 0.0f;   // 0..1
        bool muted = false;
    };

    enum class RegistryValueType
    {
        None = 0,
        String,
        Number
    };

    struct RegistryValue
    {
        RegistryValueType type = RegistryValueType::None;
        std::wstring stringValue;
        double numberValue = 0.0;
    };

    bool ClipboardSetText(const std::wstring &text);
    bool ClipboardGetText(std::wstring &outText);

    bool SetWallpaper(const std::wstring &imagePath, const std::wstring &style = L"fill");
    bool GetCurrentWallpaperPath(std::wstring &outPath);

    bool GetPowerStatus(PowerStatus &outStatus);
    bool GetCpuStats(CpuStats &outStats);
    bool GetMemoryStats(MemoryStats &outStats);
    bool GetNetworkStats(NetworkStats &outStats);
    bool GetMousePosition(MousePosition &outPos);
    bool GetDiskStats(const std::wstring &path, DiskStats &outStats);
    DisplayMetrics GetDisplayMetrics();

    bool AudioSetVolume(int volumePercent);
    int AudioGetVolume();
    bool AudioPlaySound(const std::wstring &path, bool loop);
    void AudioStopSound();
    bool AppVolumeListSessions(std::vector<AppVolumeSessionInfo> &sessions);
    bool AppVolumeGetByPid(uint32_t pid, float &outVolume, bool &outMuted, float &outPeak);
    bool AppVolumeGetByProcessName(const std::wstring &processName, float &outVolume, bool &outMuted, float &outPeak);
    bool AppVolumeSetVolumeByPid(uint32_t pid, float volume01);
    bool AppVolumeSetVolumeByProcessName(const std::wstring &processName, float volume01);
    bool AppVolumeSetMuteByPid(uint32_t pid, bool mute);
    bool AppVolumeSetMuteByProcessName(const std::wstring &processName, bool mute);
    bool JsonReadTextFile(const std::wstring &path, std::string &outText);
    bool JsonWriteTextFile(const std::wstring &path, const std::string &text);
    bool JsonMergePatchFile(const std::wstring &path, const std::string &patchText);
    std::wstring GetEnv(const std::wstring &name);
    std::vector<std::pair<std::wstring, std::wstring>> GetAllEnv();
    bool Execute(const std::wstring &target, const std::wstring &parameters = L"", const std::wstring &workingDir = L"", int show = SW_SHOWNORMAL);
    bool WebFetch(const std::wstring &url, std::string &outData);

    bool RegistryReadData(const std::wstring &fullPath, const std::wstring &valueName, RegistryValue &outValue);
    bool RegistryWriteString(const std::wstring &fullPath, const std::wstring &valueName, const std::wstring &value);
    bool RegistryWriteNumber(const std::wstring &fullPath, const std::wstring &valueName, double value);


} // namespace novadesk::shared::system
