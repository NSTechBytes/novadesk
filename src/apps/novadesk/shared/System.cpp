#include "System.h"

#include <cstring>
#include <cwchar>
#include <filesystem>
#include <chrono>
#include <unordered_map>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>

#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <audioclient.h>
#include <powrprof.h>
#include <iphlpapi.h>
#include <functiondiscoverykeys.h>
#include <roapi.h>
#include <wininet.h>
#include <winioctl.h>

#ifndef __IAudioMeterInformation_INTERFACE_DEFINED__
#define __IAudioMeterInformation_INTERFACE_DEFINED__
MIDL_INTERFACE("C02216F6-8C67-4B5B-9D00-D008E73E0064")
IAudioMeterInformation : public IUnknown
{
public:
    virtual HRESULT STDMETHODCALLTYPE GetPeakValue(float *pfPeak) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetMeteringChannelCount(UINT * pnChannelCount) = 0;
    virtual HRESULT STDMETHODCALLTYPE GetChannelsPeakValues(UINT32 u32ChannelCount, float *afPeakValues) = 0;
    virtual HRESULT STDMETHODCALLTYPE QueryHardwareSupport(DWORD * pdwHardwareSupportMask) = 0;
};
#endif
// MinGW may not provide __mingw_uuidof support for this interface.
static const IID IID_IAudioMeterInformation_Local =
    {0xC02216F6, 0x8C67, 0x4B5B, {0x9D, 0x00, 0xD0, 0x08, 0xE7, 0x3E, 0x00, 0x64}};

#include "PathUtils.h"
#include "Utils.h"
#include "Logging.h"
#include "../../../third_party/kiss_fft130/kiss_fftr.h"
#include "../../../third_party/json/json.hpp"

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "Wininet.lib")

namespace novadesk::shared::system
{
    using json = nlohmann::json;

    // *********************************************
    //  Hotkeys

    struct RegisteredHotkey
    {
        HotkeyBinding binding;
        UINT modifiers = 0;
        UINT vk = 0;
        bool pressed = false;
    };

    std::unordered_map<int, RegisteredHotkey> g_hotkeys;
    int g_nextHotkeyId = 10000;
    HHOOK g_keyboardHook = nullptr;
    HWND g_hotkeyMessageWindow = nullptr;

    // *********************************************
    //  JSON

    struct Span
    {
        size_t start = 0;
        size_t end = 0;
    };

    // *********************************************
    //  CPU Metrics

    ULONGLONG g_lastIdleTime = 0;
    ULONGLONG g_lastKernelTime = 0;
    ULONGLONG g_lastUserTime = 0;
    bool g_cpuInitialized = false;

    // *********************************************
    //  Network Metrics

    ULONGLONG g_lastTotalIn = 0;
    ULONGLONG g_lastTotalOut = 0;
    std::chrono::steady_clock::time_point g_lastNetworkSample = std::chrono::steady_clock::time_point::min();

    // *********************************************
    //  Power

    typedef struct _PROCESSOR_POWER_INFORMATION_LOCAL
    {
        ULONG Number;
        ULONG MaxMhz;
        ULONG CurrentMhz;
        ULONG MhzLimit;
        ULONG MaxIdleState;
        ULONG CurrentIdleState;
    } PROCESSOR_POWER_INFORMATION_LOCAL;

    // *********************************************
    //  COM Utilities

    struct ComInit
    {
        HRESULT hr = E_FAIL;
        ComInit() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
        ~ComInit()
        {
            if (SUCCEEDED(hr))
                CoUninitialize();
        }
        bool Ok() const { return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE; }
    };

    // *****************************************************************************
    // App Volume
    // *****************************************************************************
    IAudioEndpointVolume *GetVolumeInterface()
    {
        IMMDeviceEnumerator *enumerator = nullptr;
        IMMDevice *device = nullptr;
        IAudioEndpointVolume *volume = nullptr;

        HRESULT hr = CoCreateInstance(
            __uuidof(MMDeviceEnumerator),
            nullptr,
            CLSCTX_ALL,
            __uuidof(IMMDeviceEnumerator),
            reinterpret_cast<void **>(&enumerator));
        if (FAILED(hr) || !enumerator)
        {
            return nullptr;
        }

        hr = enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
        enumerator->Release();
        if (FAILED(hr) || !device)
        {
            return nullptr;
        }

        hr = device->Activate(
            __uuidof(IAudioEndpointVolume),
            CLSCTX_ALL,
            nullptr,
            reinterpret_cast<void **>(&volume));
        device->Release();
        if (FAILED(hr))
        {
            return nullptr;
        }

        return volume;
    }

    std::wstring ToLowerCopy(const std::wstring &s)
    {
        std::wstring out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::towlower);
        return out;
    }

    std::wstring FileNameFromPath(const std::wstring &path)
    {
        size_t pos = path.find_last_of(L"\\/");
        return (pos == std::wstring::npos) ? path : path.substr(pos + 1);
    }

    std::wstring GetProcessPathByPid(DWORD pid)
    {
        std::wstring result;
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!process)
            return result;
        wchar_t buf[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(process, 0, buf, &size))
        {
            result = buf;
        }
        CloseHandle(process);
        return result;
    }

    bool EnsureAppIconDir(std::wstring &outDir)
    {
        std::wstring baseDir = PathUtils::GetAppDataPath();
        if (baseDir.empty())
            return false;
        outDir = baseDir + L"AppIcons\\";
        CreateDirectoryW(baseDir.c_str(), nullptr);
        CreateDirectoryW(outDir.c_str(), nullptr);
        return true;
    }

    bool SetForMatchingSessions(DWORD pid, const std::wstring &processName, bool byPid, float *setVolume, bool *setMute)
    {
        ComInit com;
        if (!com.Ok())
            return false;

        IMMDeviceEnumerator *deviceEnum = nullptr;
        IMMDevice *device = nullptr;
        IAudioSessionManager2 *manager2 = nullptr;
        IAudioSessionEnumerator *sessionEnum = nullptr;
        bool anySet = false;
        std::wstring target = byPid ? L"" : ToLowerCopy(processName);

        do
        {
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&deviceEnum);
            if (FAILED(hr) || !deviceEnum)
                break;
            hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
            if (FAILED(hr) || !device)
                break;
            hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void **)&manager2);
            if (FAILED(hr) || !manager2)
                break;
            hr = manager2->GetSessionEnumerator(&sessionEnum);
            if (FAILED(hr) || !sessionEnum)
                break;

            int count = 0;
            hr = sessionEnum->GetCount(&count);
            if (FAILED(hr))
                break;

            for (int i = 0; i < count; ++i)
            {
                IAudioSessionControl *control = nullptr;
                IAudioSessionControl2 *control2 = nullptr;
                ISimpleAudioVolume *simpleVol = nullptr;

                if (FAILED(sessionEnum->GetSession(i, &control)) || !control)
                    continue;
                if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void **)&control2)) || !control2)
                {
                    control->Release();
                    continue;
                }
                if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void **)&simpleVol)) || !simpleVol)
                {
                    control2->Release();
                    control->Release();
                    continue;
                }

                DWORD curPid = 0;
                control2->GetProcessId(&curPid);
                bool match = false;
                if (byPid)
                {
                    match = (curPid == pid);
                }
                else
                {
                    std::wstring curName = ToLowerCopy(FileNameFromPath(GetProcessPathByPid(curPid)));
                    match = (curName == target);
                }

                if (match)
                {
                    if (setVolume)
                        simpleVol->SetMasterVolume(*setVolume, nullptr);
                    if (setMute)
                        simpleVol->SetMute(*setMute ? TRUE : FALSE, nullptr);
                    anySet = true;
                }

                simpleVol->Release();
                control2->Release();
                control->Release();
            }
        } while (false);

        if (sessionEnum)
            sessionEnum->Release();
        if (manager2)
            manager2->Release();
        if (device)
            device->Release();
        if (deviceEnum)
            deviceEnum->Release();
        return anySet;
    }

    float Clamp01(float v)
    {
        if (v < 0.0f)
            return 0.0f;
        if (v > 1.0f)
            return 1.0f;
        return v;
    }

    bool AppVolumeListSessions(std::vector<AppVolumeSessionInfo> &sessions)
    {
        sessions.clear();
        ComInit com;
        if (!com.Ok())
            return false;

        IMMDeviceEnumerator *deviceEnum = nullptr;
        IMMDevice *device = nullptr;
        IAudioSessionManager2 *manager2 = nullptr;
        IAudioSessionEnumerator *sessionEnum = nullptr;
        bool success = false;

        do
        {
            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **)&deviceEnum);
            if (FAILED(hr) || !deviceEnum)
                break;

            hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
            if (FAILED(hr) || !device)
                break;

            hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void **)&manager2);
            if (FAILED(hr) || !manager2)
                break;

            hr = manager2->GetSessionEnumerator(&sessionEnum);
            if (FAILED(hr) || !sessionEnum)
                break;

            int count = 0;
            hr = sessionEnum->GetCount(&count);
            if (FAILED(hr))
                break;

            for (int i = 0; i < count; ++i)
            {
                IAudioSessionControl *control = nullptr;
                IAudioSessionControl2 *control2 = nullptr;
                ISimpleAudioVolume *simpleVol = nullptr;
                IAudioMeterInformation *meter = nullptr;

                if (FAILED(sessionEnum->GetSession(i, &control)) || !control)
                    continue;
                if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void **)&control2)) || !control2)
                {
                    control->Release();
                    continue;
                }
                if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void **)&simpleVol)) || !simpleVol)
                {
                    control2->Release();
                    control->Release();
                    continue;
                }

                AudioSessionState state{};
                if (FAILED(control->GetState(&state)) || state != AudioSessionStateActive)
                {
                    simpleVol->Release();
                    control2->Release();
                    control->Release();
                    continue;
                }

                DWORD pid = 0;
                control2->GetProcessId(&pid);

                float volume = 0.0f;
                float peak = 0.0f;
                BOOL muted = FALSE;
                simpleVol->GetMasterVolume(&volume);
                simpleVol->GetMute(&muted);
                if (SUCCEEDED(control2->QueryInterface(IID_IAudioMeterInformation_Local, (void **)&meter)) && meter)
                {
                    float meterPeak = 0.0f;
                    if (SUCCEEDED(meter->GetPeakValue(&meterPeak)))
                    {
                        peak = meterPeak;
                    }
                }

                LPWSTR displayName = nullptr;
                std::wstring display;
                if (SUCCEEDED(control->GetDisplayName(&displayName)) && displayName)
                {
                    display = displayName;
                    CoTaskMemFree(displayName);
                }

                AppVolumeSessionInfo info;
                info.pid = static_cast<uint32_t>(pid);
                info.filePath = GetProcessPathByPid(pid);
                info.fileName = FileNameFromPath(info.filePath);
                info.processName = info.fileName;
                info.displayName = display;
                info.volume = volume;
                info.peak = peak;
                info.muted = muted != FALSE;

                if (!info.filePath.empty())
                {
                    std::wstring iconDir;
                    if (EnsureAppIconDir(iconDir))
                    {
                        info.iconPath = iconDir + L"pid_" + std::to_wstring(pid) + L"_48.ico";
                        if (!Utils::ExtractFileIconToIco(info.filePath, info.iconPath, 48))
                        {
                            info.iconPath.clear();
                        }
                    }
                }

                sessions.push_back(info);

                if (meter)
                    meter->Release();
                simpleVol->Release();
                control2->Release();
                control->Release();
            }

            success = true;
        } while (false);

        if (sessionEnum)
            sessionEnum->Release();
        if (manager2)
            manager2->Release();
        if (device)
            device->Release();
        if (deviceEnum)
            deviceEnum->Release();
        return success;
    }

    bool AppVolumeGetByPid(uint32_t pid, float &outVolume, bool &outMuted, float &outPeak)
    {
        std::vector<AppVolumeSessionInfo> sessions;
        if (!AppVolumeListSessions(sessions))
            return false;

        double sum = 0.0;
        int count = 0;
        bool mutedAny = false;
        double peakMax = 0.0;
        for (const auto &s : sessions)
        {
            if (s.pid == pid)
            {
                sum += s.volume;
                mutedAny = mutedAny || s.muted;
                if (s.peak > peakMax)
                    peakMax = s.peak;
                ++count;
            }
        }
        if (count == 0)
            return false;

        outVolume = static_cast<float>(sum / static_cast<double>(count));
        outMuted = mutedAny;
        outPeak = static_cast<float>(peakMax);
        return true;
    }

    bool AppVolumeGetByProcessName(const std::wstring &processName, float &outVolume, bool &outMuted, float &outPeak)
    {
        std::vector<AppVolumeSessionInfo> sessions;
        if (!AppVolumeListSessions(sessions))
            return false;

        std::wstring target = ToLowerCopy(processName);
        double sum = 0.0;
        int count = 0;
        bool mutedAny = false;
        double peakMax = 0.0;
        for (const auto &s : sessions)
        {
            if (ToLowerCopy(s.processName) == target)
            {
                sum += s.volume;
                mutedAny = mutedAny || s.muted;
                if (s.peak > peakMax)
                    peakMax = s.peak;
                ++count;
            }
        }
        if (count == 0)
            return false;

        outVolume = static_cast<float>(sum / static_cast<double>(count));
        outMuted = mutedAny;
        outPeak = static_cast<float>(peakMax);
        return true;
    }

    bool AppVolumeSetVolumeByPid(uint32_t pid, float volume01)
    {
        if (volume01 < 0.0f)
            volume01 = 0.0f;
        if (volume01 > 1.0f)
            volume01 = 1.0f;
        return SetForMatchingSessions(static_cast<DWORD>(pid), L"", true, &volume01, nullptr);
    }

    bool AppVolumeSetVolumeByProcessName(const std::wstring &processName, float volume01)
    {
        if (volume01 < 0.0f)
            volume01 = 0.0f;
        if (volume01 > 1.0f)
            volume01 = 1.0f;
        return SetForMatchingSessions(0, processName, false, &volume01, nullptr);
    }

    bool AppVolumeSetMuteByPid(uint32_t pid, bool mute)
    {
        return SetForMatchingSessions(static_cast<DWORD>(pid), L"", true, nullptr, &mute);
    }

    bool AppVolumeSetMuteByProcessName(const std::wstring &processName, bool mute)
    {
        return SetForMatchingSessions(0, processName, false, nullptr, &mute);
    }

    // *****************************************************************************
    // Audio
    // *****************************************************************************

    bool AudioSetVolume(int volumePercent)
    {
        if (volumePercent < 0)
            volumePercent = 0;
        if (volumePercent > 100)
            volumePercent = 100;

        IAudioEndpointVolume *volume = GetVolumeInterface();
        if (!volume)
        {
            return false;
        }

        const float scalar = static_cast<float>(volumePercent) / 100.0f;
        const HRESULT hr = volume->SetMasterVolumeLevelScalar(scalar, nullptr);
        volume->Release();
        return SUCCEEDED(hr);
    }

    int AudioGetVolume()
    {
        IAudioEndpointVolume *volume = GetVolumeInterface();
        if (!volume)
        {
            return 0;
        }

        float scalar = 0.0f;
        volume->GetMasterVolumeLevelScalar(&scalar);
        volume->Release();

        const int result = static_cast<int>(scalar * 100.0f + 0.5f);
        return (result < 0) ? 0 : ((result > 100) ? 100 : result);
    }

    bool AudioPlaySound(const std::wstring &path, bool loop)
    {
        DWORD flags = SND_FILENAME | SND_ASYNC | SND_NODEFAULT;
        if (loop)
        {
            flags |= SND_LOOP;
        }
        return PlaySoundW(path.c_str(), nullptr, flags) == TRUE;
    }

    void AudioStopSound()
    {
        PlaySoundW(nullptr, nullptr, 0);
    }

    // *****************************************************************************
    // Audio Level
    // *****************************************************************************

    class AudioLevelAnalyzer
    {
    public:
        ~AudioLevelAnalyzer() { Release(); }

        bool GetStats(AudioLevelStats &outStats, const AudioLevelConfig &config)
        {
            if (!EnsureInitialized(config))
            {
                return false;
            }

            PollCapture();

            outStats.rms[0] = Clamp01(m_rms[0] * static_cast<float>(m_config.rmsGain));
            outStats.rms[1] = Clamp01(m_rms[1] * static_cast<float>(m_config.rmsGain));
            outStats.peak[0] = Clamp01(m_peak[0] * static_cast<float>(m_config.peakGain));
            outStats.peak[1] = Clamp01(m_peak[1] * static_cast<float>(m_config.peakGain));
            outStats.bands = BuildBands(m_config.bands);
            return true;
        }

    private:
        AudioLevelConfig m_config;

        IMMDeviceEnumerator *m_enumerator = nullptr;
        IMMDevice *m_device = nullptr;
        IAudioClient *m_audioClient = nullptr;
        IAudioCaptureClient *m_captureClient = nullptr;
        WAVEFORMATEX *m_pwfx = nullptr;

        int m_channels = 2;
        int m_sampleRate = 48000;
        bool m_initialized = false;
        int m_fftSize = 1024;
        int m_fftOverlap = 512;
        int m_fftStride = 512;

        kiss_fftr_cfg m_fftCfg = nullptr;
        std::vector<float> m_ring;
        int m_ringWrite = 0;
        int m_ringFilled = 0;
        int m_fftCountdown = 512;

        std::vector<float> m_fftWindow;
        std::vector<float> m_fftIn;
        std::vector<kiss_fft_cpx> m_fftOut;
        std::vector<float> m_spectrum;

        float m_rms[2] = {0.0f, 0.0f};
        float m_peak[2] = {0.0f, 0.0f};
        float m_kRMS[2] = {0.0f, 0.0f};
        float m_kPeak[2] = {0.0f, 0.0f};
        float m_kFFT[2] = {0.0f, 0.0f};

        bool SameConfig(const AudioLevelConfig &c) const
        {
            return m_config.port == c.port &&
                   m_config.deviceId == c.deviceId &&
                   m_config.fftSize == c.fftSize &&
                   m_config.fftOverlap == c.fftOverlap &&
                   m_config.bands == c.bands &&
                   m_config.freqMin == c.freqMin &&
                   m_config.freqMax == c.freqMax &&
                   m_config.sensitivity == c.sensitivity &&
                   m_config.rmsAttack == c.rmsAttack &&
                   m_config.rmsDecay == c.rmsDecay &&
                   m_config.peakAttack == c.peakAttack &&
                   m_config.peakDecay == c.peakDecay &&
                   m_config.fftAttack == c.fftAttack &&
                   m_config.fftDecay == c.fftDecay &&
                   m_config.rmsGain == c.rmsGain &&
                   m_config.peakGain == c.peakGain;
        }

        int NormalizeFFTSize(int n) const
        {
            if (n < 2)
                n = 1024;
            if (n % 2 != 0)
                ++n;
            return n;
        }

        int ClampMs(int ms) const { return (ms <= 0) ? 1 : ms; }

        float CalcSmoothCoeff(int ms) const
        {
            const double sr = static_cast<double>(m_sampleRate <= 0 ? 48000 : m_sampleRate);
            const double t = static_cast<double>(ClampMs(ms)) * 0.001;
            return static_cast<float>(std::exp(std::log10(0.01) / (sr * t)));
        }

        void RecalculateCoeffs()
        {
            m_kRMS[0] = CalcSmoothCoeff(m_config.rmsAttack);
            m_kRMS[1] = CalcSmoothCoeff(m_config.rmsDecay);
            m_kPeak[0] = CalcSmoothCoeff(m_config.peakAttack);
            m_kPeak[1] = CalcSmoothCoeff(m_config.peakDecay);

            const double fftRate = static_cast<double>(m_sampleRate <= 0 ? 48000 : m_sampleRate) /
                                   static_cast<double>(m_fftStride <= 0 ? 1 : m_fftStride);
            const auto fftCoeff = [&](int ms) -> float
            {
                const double t = static_cast<double>(ClampMs(ms)) * 0.001;
                return static_cast<float>(std::exp(std::log10(0.01) / (fftRate * t)));
            };
            m_kFFT[0] = fftCoeff(m_config.fftAttack);
            m_kFFT[1] = fftCoeff(m_config.fftDecay);
        }

        bool EnsureInitialized(const AudioLevelConfig &config)
        {
            if (m_initialized && SameConfig(config))
                return true;
            if (m_initialized)
                Release();
            m_config = config;

            m_fftSize = NormalizeFFTSize(m_config.fftSize);
            m_fftOverlap = m_config.fftOverlap;
            if (m_fftOverlap < 0)
                m_fftOverlap = m_fftSize / 2;
            if (m_fftOverlap >= m_fftSize)
                m_fftOverlap = m_fftSize / 2;
            m_fftStride = m_fftSize - m_fftOverlap;
            if (m_fftStride <= 0)
                m_fftStride = m_fftSize / 2;
            m_fftCountdown = m_fftStride;

            HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void **>(&m_enumerator));
            if (FAILED(hr) || !m_enumerator)
                return false;

            EDataFlow dataFlow = (m_config.port == "input") ? eCapture : eRender;
            if (!m_config.deviceId.empty())
                hr = m_enumerator->GetDevice(m_config.deviceId.c_str(), &m_device);
            else
                hr = m_enumerator->GetDefaultAudioEndpoint(dataFlow, eMultimedia, &m_device);
            if (FAILED(hr) || !m_device)
                return false;

            hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&m_audioClient));
            if (FAILED(hr) || !m_audioClient)
                return false;

            hr = m_audioClient->GetMixFormat(&m_pwfx);
            if (FAILED(hr) || !m_pwfx)
                return false;

            m_sampleRate = static_cast<int>(m_pwfx->nSamplesPerSec);
            m_channels = static_cast<int>(m_pwfx->nChannels);
            if (m_channels < 1)
                m_channels = 1;
            if (m_channels > 2)
                m_channels = 2;

            DWORD flags = (dataFlow == eRender) ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
            hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, m_pwfx, nullptr);
            if (FAILED(hr))
                return false;

            hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void **>(&m_captureClient));
            if (FAILED(hr) || !m_captureClient)
                return false;

            hr = m_audioClient->Start();
            if (FAILED(hr))
                return false;

            m_fftCfg = kiss_fftr_alloc(m_fftSize, 0, nullptr, nullptr);
            if (!m_fftCfg)
                return false;

            m_ring.assign(static_cast<size_t>(m_fftSize), 0.0f);
            m_fftWindow.resize(static_cast<size_t>(m_fftSize));
            m_fftIn.resize(static_cast<size_t>(m_fftSize));
            m_fftOut.resize(static_cast<size_t>(m_fftSize));
            m_spectrum.assign(static_cast<size_t>(m_fftSize / 2 + 1), 0.0f);

            for (int i = 0; i < m_fftSize; ++i)
            {
                m_fftWindow[static_cast<size_t>(i)] = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * i / static_cast<float>(m_fftSize - 1)));
            }

            RecalculateCoeffs();
            m_initialized = true;
            return true;
        }

        void Release()
        {
            if (m_audioClient)
                m_audioClient->Stop();
            if (m_captureClient)
            {
                m_captureClient->Release();
                m_captureClient = nullptr;
            }
            if (m_audioClient)
            {
                m_audioClient->Release();
                m_audioClient = nullptr;
            }
            if (m_device)
            {
                m_device->Release();
                m_device = nullptr;
            }
            if (m_enumerator)
            {
                m_enumerator->Release();
                m_enumerator = nullptr;
            }
            if (m_pwfx)
            {
                CoTaskMemFree(m_pwfx);
                m_pwfx = nullptr;
            }
            if (m_fftCfg)
            {
                free(m_fftCfg);
                m_fftCfg = nullptr;
            }
            m_initialized = false;
        }

        void PushMonoSample(float s)
        {
            m_ring[static_cast<size_t>(m_ringWrite)] = s;
            m_ringWrite = (m_ringWrite + 1) % m_fftSize;
            if (m_ringFilled < m_fftSize)
                ++m_ringFilled;
            if (--m_fftCountdown <= 0)
            {
                m_fftCountdown = m_fftStride;
                ComputeFFT();
            }
        }

        void ComputeFFT()
        {
            if (m_ringFilled < m_fftSize)
                return;
            int idx = m_ringWrite;
            for (int i = 0; i < m_fftSize; ++i)
            {
                m_fftIn[static_cast<size_t>(i)] = m_ring[static_cast<size_t>(idx)] * m_fftWindow[static_cast<size_t>(i)];
                idx = (idx + 1) % m_fftSize;
            }
            kiss_fftr(m_fftCfg, m_fftIn.data(), m_fftOut.data());
            const int outSize = m_fftSize / 2 + 1;
            const float scalar = 1.0f / std::sqrt(static_cast<float>(m_fftSize));
            for (int i = 0; i < outSize; ++i)
            {
                const float re = m_fftOut[static_cast<size_t>(i)].r;
                const float im = m_fftOut[static_cast<size_t>(i)].i;
                const float mag = std::sqrt(re * re + im * im) * scalar;
                float &old = m_spectrum[static_cast<size_t>(i)];
                old = mag + m_kFFT[(mag < old)] * (old - mag);
            }
        }

        std::vector<float> BuildBands(int bands) const
        {
            std::vector<float> out(static_cast<size_t>(bands), 0.0f);
            if (m_spectrum.empty())
                return out;
            const int outSize = static_cast<int>(m_spectrum.size());
            const double freqMin = m_config.freqMin <= 0.0 ? 20.0 : m_config.freqMin;
            const double freqMax = (m_config.freqMax <= freqMin) ? 20000.0 : m_config.freqMax;
            const double sensitivity = (m_config.sensitivity <= 0.0) ? 35.0 : m_config.sensitivity;

            for (int i = 0; i < bands; ++i)
            {
                const double f1 = freqMin * std::pow(freqMax / freqMin, static_cast<double>(i) / static_cast<double>(bands));
                const double f2 = freqMin * std::pow(freqMax / freqMin, static_cast<double>(i + 1) / static_cast<double>(bands));
                int idx1 = static_cast<int>(f1 * 2.0 * outSize / static_cast<double>(m_sampleRate));
                int idx2 = static_cast<int>(f2 * 2.0 * outSize / static_cast<double>(m_sampleRate));
                if (idx1 < 0)
                    idx1 = 0;
                if (idx2 >= outSize)
                    idx2 = outSize - 1;
                if (idx2 < idx1)
                    idx2 = idx1;
                float maxVal = 0.0f;
                for (int k = idx1; k <= idx2; ++k)
                    maxVal = std::max(maxVal, m_spectrum[static_cast<size_t>(k)]);
                if (maxVal > 0.0f)
                {
                    const float db = 20.0f * std::log10(maxVal);
                    out[static_cast<size_t>(i)] = Clamp01(1.0f + db / static_cast<float>(sensitivity));
                }
            }
            return out;
        }

        void PollCapture()
        {
            if (!m_captureClient || !m_pwfx)
                return;
            UINT32 packetLength = 0;
            HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);

            bool hadAudio = false;
            float sumSq[2] = {0.0f, 0.0f};
            float peak[2] = {0.0f, 0.0f};
            uint64_t frameCount = 0;

            while (SUCCEEDED(hr) && packetLength > 0)
            {
                BYTE *data = nullptr;
                UINT32 frames = 0;
                DWORD flags = 0;
                hr = m_captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
                if (FAILED(hr))
                    break;

                const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
                const bool isFloat = (m_pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) || (m_pwfx->wBitsPerSample == 32);

                if (!silent && data && frames > 0)
                {
                    hadAudio = true;
                    if (isFloat)
                    {
                        const float *in = reinterpret_cast<const float *>(data);
                        for (UINT32 i = 0; i < frames; ++i)
                        {
                            const float l = in[i * m_pwfx->nChannels + 0];
                            const float r = (m_pwfx->nChannels > 1) ? in[i * m_pwfx->nChannels + 1] : l;
                            sumSq[0] += l * l;
                            sumSq[1] += r * r;
                            peak[0] = std::max(peak[0], std::fabs(l));
                            peak[1] = std::max(peak[1], std::fabs(r));
                            PushMonoSample((l + r) * 0.5f);
                        }
                    }
                    else
                    {
                        const int16_t *in = reinterpret_cast<const int16_t *>(data);
                        for (UINT32 i = 0; i < frames; ++i)
                        {
                            const float l = static_cast<float>(in[i * m_pwfx->nChannels + 0]) / 32768.0f;
                            const float r = (m_pwfx->nChannels > 1)
                                                ? static_cast<float>(in[i * m_pwfx->nChannels + 1]) / 32768.0f
                                                : l;
                            sumSq[0] += l * l;
                            sumSq[1] += r * r;
                            peak[0] = std::max(peak[0], std::fabs(l));
                            peak[1] = std::max(peak[1], std::fabs(r));
                            PushMonoSample((l + r) * 0.5f);
                        }
                    }
                    frameCount += frames;
                }

                m_captureClient->ReleaseBuffer(frames);
                hr = m_captureClient->GetNextPacketSize(&packetLength);
            }

            if (hadAudio && frameCount > 0)
            {
                const float n = static_cast<float>(frameCount);
                const float rmsNow0 = std::sqrt(sumSq[0] / n);
                const float rmsNow1 = std::sqrt(sumSq[1] / n);
                m_rms[0] = Clamp01(rmsNow0 + m_kRMS[(rmsNow0 < m_rms[0])] * (m_rms[0] - rmsNow0));
                m_rms[1] = Clamp01(rmsNow1 + m_kRMS[(rmsNow1 < m_rms[1])] * (m_rms[1] - rmsNow1));
                m_peak[0] = Clamp01(peak[0] + m_kPeak[(peak[0] < m_peak[0])] * (m_peak[0] - peak[0]));
                m_peak[1] = Clamp01(peak[1] + m_kPeak[(peak[1] < m_peak[1])] * (m_peak[1] - peak[1]));
            }
            else
            {
                m_rms[0] *= m_kRMS[1];
                m_rms[1] *= m_kRMS[1];
                m_peak[0] *= m_kPeak[1];
                m_peak[1] *= m_kPeak[1];
            }
        }
    };

    AudioLevelAnalyzer g_audioLevelAnalyzer;

    bool GetAudioLevelStats(AudioLevelStats &outStats, const AudioLevelConfig &config)
    {
        if (!g_audioLevelAnalyzer.GetStats(outStats, config))
        {
            return false;
        }
        return true;
    }

    // *****************************************************************************
    // Brightness
    // *****************************************************************************

    class BrightnessControl
    {
    public:
        explicit BrightnessControl(int displayIndex = 0) : m_displayIndex(displayIndex) {}
        ~BrightnessControl() { Cleanup(); }

        bool GetBrightness(BrightnessInfo &outInfo)
        {
            if (!Initialize())
            {
                outInfo.supported = false;
                return false;
            }

            const bool ok = (m_method == Method::LaptopIoctl) ? GetLaptopBrightness(outInfo) : false;
            if (!ok)
            {
                outInfo.supported = false;
            }
            return ok;
        }

        bool SetBrightnessPercent(int percent)
        {
            if (!Initialize())
            {
                return false;
            }
            if (m_method == Method::LaptopIoctl)
            {
                return SetLaptopBrightnessPercent(percent);
            }
            return false;
        }

    private:
        enum class Method
        {
            Unknown,
            LaptopIoctl,
            None
        };

        int m_displayIndex = 0;
        Method m_method = Method::Unknown;
        HANDLE m_lcdDevice = INVALID_HANDLE_VALUE;
        std::vector<BYTE> m_laptopLevels;
        bool m_initialized = false;

        bool Initialize()
        {
            (void)m_displayIndex;
            if (m_initialized)
                return m_method != Method::None;

            m_initialized = true;
            m_method = Method::None;

            if (InitializeLaptopIoctl())
            {
                m_method = Method::LaptopIoctl;
                return true;
            }
            return false;
        }

        bool InitializeLaptopIoctl()
        {
#ifndef IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS
#define IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x125, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
            m_lcdDevice = CreateFileW(L"\\\\.\\LCD", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if (m_lcdDevice == INVALID_HANDLE_VALUE)
            {
                return false;
            }

            BYTE levels[256] = {};
            DWORD bytesReturned = 0;
            if (!DeviceIoControl(m_lcdDevice, IOCTL_VIDEO_QUERY_SUPPORTED_BRIGHTNESS, nullptr, 0, levels, sizeof(levels), &bytesReturned, nullptr) || bytesReturned < 2)
            {
                CloseHandle(m_lcdDevice);
                m_lcdDevice = INVALID_HANDLE_VALUE;
                return false;
            }

            m_laptopLevels.assign(levels, levels + bytesReturned);
            std::sort(m_laptopLevels.begin(), m_laptopLevels.end());
            m_laptopLevels.erase(std::unique(m_laptopLevels.begin(), m_laptopLevels.end()), m_laptopLevels.end());
            return m_laptopLevels.size() >= 2;
        }

        void Cleanup()
        {
            if (m_lcdDevice != INVALID_HANDLE_VALUE)
            {
                CloseHandle(m_lcdDevice);
                m_lcdDevice = INVALID_HANDLE_VALUE;
            }
            m_laptopLevels.clear();
        }

        BYTE FindClosestLaptopLevel(BYTE raw) const
        {
            if (m_laptopLevels.empty())
                return raw;

            BYTE best = m_laptopLevels.front();
            int bestDiff = std::abs(static_cast<int>(best) - static_cast<int>(raw));
            for (BYTE level : m_laptopLevels)
            {
                const int diff = std::abs(static_cast<int>(level) - static_cast<int>(raw));
                if (diff < bestDiff)
                {
                    best = level;
                    bestDiff = diff;
                }
            }
            return best;
        }

        bool GetLaptopBrightness(BrightnessInfo &outInfo)
        {
#ifndef IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x126, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
            if (m_lcdDevice == INVALID_HANDLE_VALUE || m_laptopLevels.size() < 2)
                return false;

            BYTE values[3] = {};
            DWORD bytesReturned = 0;
            if (!DeviceIoControl(m_lcdDevice, IOCTL_VIDEO_QUERY_DISPLAY_BRIGHTNESS, nullptr, 0, values, sizeof(values), &bytesReturned, nullptr) || bytesReturned < 3)
            {
                return false;
            }

            const BYTE rawCurrent = (values[0] == 1) ? values[1] : values[2];
            const BYTE minRaw = m_laptopLevels.front();
            const BYTE maxRaw = m_laptopLevels.back();
            const BYTE normalizedRaw = FindClosestLaptopLevel(rawCurrent);

            outInfo.min = minRaw;
            outInfo.max = maxRaw;
            outInfo.current = normalizedRaw;
            outInfo.supported = true;

            if (maxRaw > minRaw)
            {
                outInfo.percent = static_cast<int>((static_cast<double>(normalizedRaw - minRaw) * 100.0) / static_cast<double>(maxRaw - minRaw));
            }
            else
            {
                outInfo.percent = 0;
            }

            if (outInfo.percent < 0)
                outInfo.percent = 0;
            if (outInfo.percent > 100)
                outInfo.percent = 100;
            return true;
        }

        bool SetLaptopBrightnessPercent(int percent)
        {
#ifndef IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS
#define IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS CTL_CODE(FILE_DEVICE_VIDEO, 0x127, METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif
            if (m_lcdDevice == INVALID_HANDLE_VALUE || m_laptopLevels.size() < 2)
                return false;

            if (percent < 0)
                percent = 0;
            if (percent > 100)
                percent = 100;

            const BYTE minRaw = m_laptopLevels.front();
            const BYTE maxRaw = m_laptopLevels.back();
            BYTE targetRaw = minRaw;
            if (maxRaw > minRaw)
            {
                targetRaw = static_cast<BYTE>(minRaw + static_cast<BYTE>((static_cast<double>(maxRaw - minRaw) * static_cast<double>(percent)) / 100.0));
            }
            targetRaw = FindClosestLaptopLevel(targetRaw);

            BYTE setValues[3] = {3, targetRaw, targetRaw};
            DWORD bytesReturned = 0;
            return DeviceIoControl(m_lcdDevice, IOCTL_VIDEO_SET_DISPLAY_BRIGHTNESS, setValues, sizeof(setValues), nullptr, 0, &bytesReturned, nullptr) == TRUE;
        }
    };

    bool GetBrightness(BrightnessInfo &outInfo, int displayIndex)
    {
        BrightnessControl control(displayIndex);
        return control.GetBrightness(outInfo);
    }

    bool SetBrightnessPercent(int percent, int displayIndex)
    {
        BrightnessControl control(displayIndex);
        return control.SetBrightnessPercent(percent);
    }

    // *****************************************************************************
    // Clipboard
    // *****************************************************************************

    bool ClipboardSetText(const std::wstring &text)
    {
        if (!OpenClipboard(nullptr))
        {
            return false;
        }
        EmptyClipboard();

        const size_t bytes = (text.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes);
        if (!hMem)
        {
            CloseClipboard();
            return false;
        }

        void *ptr = GlobalLock(hMem);
        if (!ptr)
        {
            GlobalFree(hMem);
            CloseClipboard();
            return false;
        }
        memcpy(ptr, text.c_str(), bytes);
        GlobalUnlock(hMem);

        if (!SetClipboardData(CF_UNICODETEXT, hMem))
        {
            GlobalFree(hMem);
            CloseClipboard();
            return false;
        }

        CloseClipboard();
        return true;
    }

    bool ClipboardGetText(std::wstring &outText)
    {
        outText.clear();
        if (!OpenClipboard(nullptr))
        {
            return false;
        }

        HANDLE hData = GetClipboardData(CF_UNICODETEXT);
        if (!hData)
        {
            CloseClipboard();
            return false;
        }

        const wchar_t *text = static_cast<const wchar_t *>(GlobalLock(hData));
        if (!text)
        {
            CloseClipboard();
            return false;
        }

        outText = text;
        GlobalUnlock(hData);
        CloseClipboard();
        return true;
    }

    // *****************************************************************************
    // CPU Metrics
    // *****************************************************************************

    bool GetCpuStats(CpuStats &outStats)
    {
        FILETIME idleFt{}, kernelFt{}, userFt{};
        if (!GetSystemTimes(&idleFt, &kernelFt, &userFt))
        {
            return false;
        }

        ULARGE_INTEGER idle{}, kernel{}, user{};
        idle.LowPart = idleFt.dwLowDateTime;
        idle.HighPart = idleFt.dwHighDateTime;
        kernel.LowPart = kernelFt.dwLowDateTime;
        kernel.HighPart = kernelFt.dwHighDateTime;
        user.LowPart = userFt.dwLowDateTime;
        user.HighPart = userFt.dwHighDateTime;

        if (!g_cpuInitialized)
        {
            g_lastIdleTime = idle.QuadPart;
            g_lastKernelTime = kernel.QuadPart;
            g_lastUserTime = user.QuadPart;
            g_cpuInitialized = true;
            outStats.usage = 0.0;
            return true;
        }

        const ULONGLONG idleDelta = idle.QuadPart - g_lastIdleTime;
        const ULONGLONG kernelDelta = kernel.QuadPart - g_lastKernelTime;
        const ULONGLONG userDelta = user.QuadPart - g_lastUserTime;
        const ULONGLONG totalDelta = kernelDelta + userDelta;

        g_lastIdleTime = idle.QuadPart;
        g_lastKernelTime = kernel.QuadPart;
        g_lastUserTime = user.QuadPart;

        if (totalDelta == 0)
        {
            outStats.usage = 0.0;
            return true;
        }

        double usage = (static_cast<double>(totalDelta - idleDelta) * 100.0) / static_cast<double>(totalDelta);
        if (usage < 0.0)
            usage = 0.0;
        if (usage > 100.0)
            usage = 100.0;
        outStats.usage = usage;
        return true;
    }

    // *****************************************************************************
    // Disk Metrics
    // *****************************************************************************

    bool GetDiskStats(const std::wstring &path, DiskStats &outStats)
    {
        std::wstring target = path;
        if (target.empty())
        {
            std::error_code ec;
            target = std::filesystem::current_path(ec).root_path().wstring();
            if (target.empty())
            {
                target = L"C:\\";
            }
        }

        ULARGE_INTEGER freeBytesAvailable{};
        ULARGE_INTEGER totalBytes{};
        ULARGE_INTEGER totalFreeBytes{};
        if (!GetDiskFreeSpaceExW(target.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes))
        {
            return false;
        }

        outStats.total = static_cast<double>(totalBytes.QuadPart);
        outStats.available = static_cast<double>(freeBytesAvailable.QuadPart);
        outStats.used = static_cast<double>(totalBytes.QuadPart - totalFreeBytes.QuadPart);
        outStats.percent = (outStats.total > 0.0)
                               ? static_cast<int>((outStats.used * 100.0) / outStats.total + 0.5)
                               : 0;
        return true;
    }

    // *****************************************************************************
    // Display Metrics
    // *****************************************************************************

    namespace
    {
        struct DisplayEnumContext
        {
            DisplayMetrics *out = nullptr;
        };

        BOOL CALLBACK EnumDisplayMonitorsForMetrics(HMONITOR hMonitor, HDC, LPRECT lprcMonitor, LPARAM dwData)
        {
            auto *ctx = reinterpret_cast<DisplayEnumContext *>(dwData);
            if (!ctx || !ctx->out || !lprcMonitor)
                return TRUE;

            DisplayMonitorInfo dm{};
            dm.id = static_cast<int>(ctx->out->monitors.size()) + 1;
            dm.active = true;
            dm.screen.left = lprcMonitor->left;
            dm.screen.top = lprcMonitor->top;
            dm.screen.right = lprcMonitor->right;
            dm.screen.bottom = lprcMonitor->bottom;

            MONITORINFOEXW mi{};
            mi.cbSize = sizeof(mi);
            if (GetMonitorInfoW(hMonitor, &mi))
            {
                dm.work.left = mi.rcWork.left;
                dm.work.top = mi.rcWork.top;
                dm.work.right = mi.rcWork.right;
                dm.work.bottom = mi.rcWork.bottom;
                dm.deviceName = mi.szDevice;

                DISPLAY_DEVICEW ddm{};
                ddm.cb = sizeof(ddm);
                DWORD monIdx = 0;
                while (EnumDisplayDevicesW(mi.szDevice, monIdx++, &ddm, 0))
                {
                    if ((ddm.StateFlags & DISPLAY_DEVICE_ACTIVE) && (ddm.StateFlags & DISPLAY_DEVICE_ATTACHED))
                    {
                        dm.monitorName = ddm.DeviceString;
                        break;
                    }
                    ddm.cb = sizeof(ddm);
                }

                if (mi.dwFlags & MONITORINFOF_PRIMARY)
                {
                    ctx->out->primaryIndex = static_cast<int>(ctx->out->monitors.size());
                }
            }
            else
            {
                dm.work = dm.screen;
            }

            ctx->out->monitors.push_back(std::move(dm));
            return TRUE;
        }
    } // namespace

    DisplayMetrics GetDisplayMetrics()
    {
        DisplayMetrics out;
        out.virtualLeft = GetSystemMetrics(SM_XVIRTUALSCREEN);
        out.virtualTop = GetSystemMetrics(SM_YVIRTUALSCREEN);
        out.virtualWidth = GetSystemMetrics(SM_CXVIRTUALSCREEN);
        out.virtualHeight = GetSystemMetrics(SM_CYVIRTUALSCREEN);
        out.primaryIndex = 0;

        DisplayEnumContext ctx{};
        ctx.out = &out;
        EnumDisplayMonitors(nullptr, nullptr, EnumDisplayMonitorsForMetrics, reinterpret_cast<LPARAM>(&ctx));

        return out;
    }

    // *****************************************************************************
    // Environment
    // *****************************************************************************
    std::wstring GetEnv(const std::wstring &name)
    {
        if (name.empty())
        {
            return L"";
        }

        const DWORD size = GetEnvironmentVariableW(name.c_str(), nullptr, 0);
        if (size == 0)
        {
            return L"";
        }

        std::wstring value(size, L'\0');
        DWORD written = GetEnvironmentVariableW(name.c_str(), value.data(), size);
        if (written == 0)
        {
            return L"";
        }
        if (!value.empty() && value.back() == L'\0')
        {
            value.pop_back();
        }
        return value;
    }

    std::vector<std::pair<std::wstring, std::wstring>> GetAllEnv()
    {
        std::vector<std::pair<std::wstring, std::wstring>> out;
        LPWCH block = GetEnvironmentStringsW();
        if (!block)
        {
            return out;
        }

        LPCWCH p = block;
        while (*p)
        {
            std::wstring entry(p);
            const size_t eq = entry.find(L'=');
            if (eq != std::wstring::npos && eq > 0)
            {
                out.emplace_back(entry.substr(0, eq), entry.substr(eq + 1));
            }
            p += entry.size() + 1;
        }

        FreeEnvironmentStringsW(block);
        return out;
    }
    // *****************************************************************************
    // Execute Command
    // *****************************************************************************

    bool Execute(const std::wstring &target, const std::wstring &parameters, const std::wstring &workingDir, int show)
    {
        HINSTANCE result = ShellExecuteW(
            nullptr,
            L"open",
            target.c_str(),
            parameters.empty() ? nullptr : parameters.c_str(),
            workingDir.empty() ? nullptr : workingDir.c_str(),
            show);
        return reinterpret_cast<INT_PTR>(result) > 32;
    }

    // *****************************************************************************
    // Hotkey
    // *****************************************************************************

    bool ParseHotkeyString(const std::wstring &hotkey, UINT &modifiers, UINT &vk)
    {
        modifiers = 0;
        vk = 0;

        size_t start = 0;
        while (start <= hotkey.size())
        {
            size_t plus = hotkey.find(L'+', start);
            std::wstring token = (plus == std::wstring::npos) ? hotkey.substr(start) : hotkey.substr(start, plus - start);
            token = Utils::TrimUpper(token);

            if (token == L"CTRL" || token == L"CONTROL")
                modifiers |= MOD_CONTROL;
            else if (token == L"ALT")
                modifiers |= MOD_ALT;
            else if (token == L"SHIFT")
                modifiers |= MOD_SHIFT;
            else if (token == L"WIN" || token == L"WINDOWS")
                modifiers |= MOD_WIN;
            else if (token.size() == 1 && token[0] >= L'A' && token[0] <= L'Z')
                vk = static_cast<UINT>(token[0]);
            else if (token.size() == 1 && token[0] >= L'0' && token[0] <= L'9')
                vk = static_cast<UINT>(token[0]);
            else if (token.size() >= 2 && token[0] == L'F')
            {
                int f = _wtoi(token.c_str() + 1);
                if (f >= 1 && f <= 24)
                    vk = VK_F1 + (f - 1);
            }
            else if (token == L"SPACE")
                vk = VK_SPACE;
            else if (token == L"ENTER" || token == L"RETURN")
                vk = VK_RETURN;
            else if (token == L"TAB")
                vk = VK_TAB;
            else if (token == L"ESC" || token == L"ESCAPE")
                vk = VK_ESCAPE;
            else if (token == L"BACKSPACE")
                vk = VK_BACK;
            else if (token == L"DELETE" || token == L"DEL")
                vk = VK_DELETE;
            else if (token == L"INSERT" || token == L"INS")
                vk = VK_INSERT;
            else if (token == L"HOME")
                vk = VK_HOME;
            else if (token == L"END")
                vk = VK_END;
            else if (token == L"PAGEUP" || token == L"PGUP")
                vk = VK_PRIOR;
            else if (token == L"PAGEDOWN" || token == L"PGDN")
                vk = VK_NEXT;
            else if (token == L"LEFT")
                vk = VK_LEFT;
            else if (token == L"RIGHT")
                vk = VK_RIGHT;
            else if (token == L"UP")
                vk = VK_UP;
            else if (token == L"DOWN")
                vk = VK_DOWN;

            if (plus == std::wstring::npos)
                break;
            start = plus + 1;
        }
        return vk != 0;
    }

    bool IsModifierVk(UINT vk)
    {
        return vk == VK_CONTROL || vk == VK_LCONTROL || vk == VK_RCONTROL ||
               vk == VK_MENU || vk == VK_LMENU || vk == VK_RMENU ||
               vk == VK_SHIFT || vk == VK_LSHIFT || vk == VK_RSHIFT ||
               vk == VK_LWIN || vk == VK_RWIN;
    }

    bool IsDownWithEvent(UINT checkVk, UINT eventVk, bool eventDown)
    {
        if (checkVk == VK_CONTROL)
        {
            if (eventVk == VK_LCONTROL || eventVk == VK_RCONTROL || eventVk == VK_CONTROL)
                return eventDown;
            return (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        }
        if (checkVk == VK_MENU)
        {
            if (eventVk == VK_LMENU || eventVk == VK_RMENU || eventVk == VK_MENU)
                return eventDown;
            return (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        }
        if (checkVk == VK_SHIFT)
        {
            if (eventVk == VK_LSHIFT || eventVk == VK_RSHIFT || eventVk == VK_SHIFT)
                return eventDown;
            return (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        }
        if (checkVk == VK_LWIN || checkVk == VK_RWIN)
        {
            if (eventVk == checkVk)
                return eventDown;
            return (GetAsyncKeyState(checkVk) & 0x8000) != 0;
        }
        if (eventVk == checkVk)
            return eventDown;
        return (GetAsyncKeyState(checkVk) & 0x8000) != 0;
    }

    bool IsHotkeyPressedNow(const RegisteredHotkey &hk, UINT eventVk, bool eventDown)
    {
        if ((hk.modifiers & MOD_CONTROL) && !IsDownWithEvent(VK_CONTROL, eventVk, eventDown))
            return false;
        if ((hk.modifiers & MOD_ALT) && !IsDownWithEvent(VK_MENU, eventVk, eventDown))
            return false;
        if ((hk.modifiers & MOD_SHIFT) && !IsDownWithEvent(VK_SHIFT, eventVk, eventDown))
            return false;
        if ((hk.modifiers & MOD_WIN) &&
            !IsDownWithEvent(VK_LWIN, eventVk, eventDown) &&
            !IsDownWithEvent(VK_RWIN, eventVk, eventDown))
            return false;
        return IsDownWithEvent(hk.vk, eventVk, eventDown);
    }

    LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam)
    {
        if (nCode == HC_ACTION && lParam)
        {
            const auto *kb = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
            const UINT vk = kb->vkCode;
            const bool isDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
            const bool isUp = (wParam == WM_KEYUP || wParam == WM_SYSKEYUP);
            if ((isDown || isUp) && g_hotkeyMessageWindow)
            {
                const bool relevant = IsModifierVk(vk);
                for (auto &kv : g_hotkeys)
                {
                    RegisteredHotkey &hk = kv.second;
                    if (!relevant && vk != hk.vk)
                    {
                        continue;
                    }

                    const bool nowPressed = IsHotkeyPressedNow(hk, vk, isDown);
                    if (nowPressed && !hk.pressed)
                    {
                        hk.pressed = true;
                        PostMessageW(g_hotkeyMessageWindow, WM_HOTKEY, static_cast<WPARAM>(kv.first), 0);
                    }
                    else if (!nowPressed && hk.pressed)
                    {
                        hk.pressed = false;
                        PostMessageW(g_hotkeyMessageWindow, WM_NOVADESK_HOTKEY_UP, static_cast<WPARAM>(kv.first), 0);
                    }
                }
            }
        }
        return CallNextHookEx(g_keyboardHook, nCode, wParam, lParam);
    }

    void EnsureKeyboardHook(HWND messageWindow)
    {
        g_hotkeyMessageWindow = messageWindow;
        if (!g_keyboardHook)
        {
            g_keyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandleW(nullptr), 0);
        }
    }

    void MaybeRemoveKeyboardHook()
    {
        if (g_hotkeys.empty() && g_keyboardHook)
        {
            UnhookWindowsHookEx(g_keyboardHook);
            g_keyboardHook = nullptr;
            g_hotkeyMessageWindow = nullptr;
        }
    }

    int RegisterHotkey(HWND messageWindow, const std::wstring &hotkey, int onKeyDownCallbackId, int onKeyUpCallbackId)
    {
        if (!messageWindow)
            return -1;
        UINT modifiers = 0;
        UINT vk = 0;
        if (!ParseHotkeyString(hotkey, modifiers, vk))
            return -1;

        int id = g_nextHotkeyId++;
        EnsureKeyboardHook(messageWindow);
        if (!g_keyboardHook)
            return -1;

        RegisteredHotkey entry{};
        entry.binding.onKeyDownCallbackId = onKeyDownCallbackId;
        entry.binding.onKeyUpCallbackId = onKeyUpCallbackId;
        entry.modifiers = modifiers;
        entry.vk = vk;
        g_hotkeys[id] = entry;
        return id;
    }

    bool UnregisterHotkey(HWND messageWindow, int id)
    {
        auto it = g_hotkeys.find(id);
        if (it == g_hotkeys.end())
            return false;
        (void)messageWindow;
        g_hotkeys.erase(it);
        MaybeRemoveKeyboardHook();
        return true;
    }

    void ClearHotkeys(HWND messageWindow)
    {
        (void)messageWindow;
        g_hotkeys.clear();
        MaybeRemoveKeyboardHook();
    }

    bool ResolveHotkeyMessage(int id, HotkeyBinding &outBinding)
    {
        auto it = g_hotkeys.find(id);
        if (it == g_hotkeys.end())
            return false;
        outBinding = it->second.binding;
        return true;
    }

    // *****************************************************************************
    // Json
    // *****************************************************************************

    bool IsWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    void SkipWhitespaceAndComments(const std::string &text, size_t &i, size_t end)
    {
        while (i < end)
        {
            if (IsWhitespace(text[i]))
            {
                ++i;
                continue;
            }
            if (text[i] == '/' && i + 1 < end)
            {
                if (text[i + 1] == '/')
                {
                    i += 2;
                    while (i < end && text[i] != '\n')
                    {
                        ++i;
                    }
                    continue;
                }
                if (text[i + 1] == '*')
                {
                    i += 2;
                    while (i + 1 < end && !(text[i] == '*' && text[i + 1] == '/'))
                    {
                        ++i;
                    }
                    if (i + 1 < end)
                    {
                        i += 2;
                    }
                    continue;
                }
            }
            break;
        }
    }

    bool SkipString(const std::string &text, size_t &i, size_t end)
    {
        if (i >= end || text[i] != '"')
        {
            return false;
        }
        ++i;
        while (i < end)
        {
            char c = text[i++];
            if (c == '\\')
            {
                if (i < end)
                {
                    ++i;
                }
                continue;
            }
            if (c == '"')
            {
                return true;
            }
        }
        return false;
    }

    bool FindRootObjectSpan(const std::string &text, Span &span)
    {
        size_t i = 0;
        size_t end = text.size();
        while (i < end)
        {
            SkipWhitespaceAndComments(text, i, end);
            if (i >= end)
            {
                break;
            }
            if (text[i] == '"')
            {
                if (!SkipString(text, i, end))
                {
                    return false;
                }
                continue;
            }
            if (text[i] == '{')
            {
                size_t depth = 0;
                size_t start = i;
                while (i < end)
                {
                    char c = text[i];
                    if (c == '"')
                    {
                        if (!SkipString(text, i, end))
                        {
                            return false;
                        }
                        continue;
                    }
                    if (c == '/' && i + 1 < end)
                    {
                        size_t before = i;
                        SkipWhitespaceAndComments(text, i, end);
                        if (i != before)
                        {
                            continue;
                        }
                    }
                    if (c == '{')
                    {
                        ++depth;
                    }
                    else if (c == '}')
                    {
                        --depth;
                        if (depth == 0)
                        {
                            span.start = start;
                            span.end = i + 1;
                            return true;
                        }
                    }
                    ++i;
                }
                return false;
            }
            ++i;
        }
        return false;
    }

    bool ParseValueSpan(const std::string &text, size_t &i, size_t end, Span &span)
    {
        SkipWhitespaceAndComments(text, i, end);
        if (i >= end)
        {
            return false;
        }
        span.start = i;
        char c = text[i];
        if (c == '"')
        {
            if (!SkipString(text, i, end))
            {
                return false;
            }
            span.end = i;
            return true;
        }
        if (c == '{' || c == '[')
        {
            char open = c;
            char close = (c == '{') ? '}' : ']';
            size_t depth = 0;
            while (i < end)
            {
                char ch = text[i];
                if (ch == '"')
                {
                    if (!SkipString(text, i, end))
                    {
                        return false;
                    }
                    continue;
                }
                if (ch == '/' && i + 1 < end)
                {
                    size_t before = i;
                    SkipWhitespaceAndComments(text, i, end);
                    if (i != before)
                    {
                        continue;
                    }
                }
                if (ch == open)
                {
                    ++depth;
                }
                else if (ch == close)
                {
                    --depth;
                    if (depth == 0)
                    {
                        ++i;
                        span.end = i;
                        return true;
                    }
                }
                ++i;
            }
            return false;
        }
        while (i < end)
        {
            char ch = text[i];
            if (ch == ',' || ch == '}' || ch == ']')
            {
                break;
            }
            ++i;
        }
        span.end = i;
        return true;
    }

    bool CollectObjectMembers(const std::string &text, const Span &objSpan, std::unordered_map<std::string, Span> &members, size_t &lastValueEnd)
    {
        size_t i = objSpan.start + 1;
        size_t end = objSpan.end - 1;
        lastValueEnd = objSpan.start + 1;
        while (i < end)
        {
            SkipWhitespaceAndComments(text, i, end);
            if (i >= end)
            {
                break;
            }
            if (text[i] == '}')
            {
                break;
            }
            if (text[i] != '"')
            {
                return false;
            }
            size_t keyStart = i + 1;
            if (!SkipString(text, i, end))
            {
                return false;
            }
            size_t keyEnd = i - 1;
            std::string key = text.substr(keyStart, keyEnd - keyStart);
            SkipWhitespaceAndComments(text, i, end);
            if (i >= end || text[i] != ':')
            {
                return false;
            }
            ++i;
            Span valueSpan;
            if (!ParseValueSpan(text, i, end, valueSpan))
            {
                return false;
            }
            members[key] = valueSpan;
            lastValueEnd = valueSpan.end;
            SkipWhitespaceAndComments(text, i, end);
            if (i < end && text[i] == ',')
            {
                ++i;
            }
        }
        return true;
    }

    size_t FindInsertPosition(const std::string &text, const Span &objSpan)
    {
        size_t i = objSpan.end - 1;
        while (i > objSpan.start)
        {
            size_t before = i;
            SkipWhitespaceAndComments(text, i, objSpan.end);
            if (i != before)
            {
                continue;
            }
            if (IsWhitespace(text[i]))
            {
                --i;
                continue;
            }
            break;
        }
        return objSpan.end - 1;
    }

    bool MergeObjectInText(std::string &text, Span &objSpan, const json &patch)
    {
        if (!patch.is_object())
        {
            return false;
        }

        std::string indent = "    ";
        bool hasMembers = true;
        size_t insertPos = FindInsertPosition(text, objSpan);

        for (auto &item : patch.items())
        {
            std::unordered_map<std::string, Span> members;
            size_t lastValueEnd = objSpan.start + 1;
            if (!CollectObjectMembers(text, objSpan, members, lastValueEnd))
            {
                return false;
            }

            const std::string &key = item.key();
            auto it = members.find(key);
            hasMembers = !members.empty();

            if (it != members.end())
            {
                Span span = it->second;
                if (item.value().is_object())
                {
                    size_t i = span.start;
                    SkipWhitespaceAndComments(text, i, span.end);
                    if (i < span.end && text[i] == '{')
                    {
                        Span nestedObj;
                        if (FindRootObjectSpan(text.substr(span.start, span.end - span.start), nestedObj))
                        {
                            Span adjusted;
                            adjusted.start = span.start + nestedObj.start;
                            adjusted.end = span.start + nestedObj.end;
                            size_t beforeSize = text.size();
                            if (MergeObjectInText(text, adjusted, item.value()))
                            {
                                size_t afterSize = text.size();
                                if (afterSize != beforeSize)
                                {
                                    objSpan.end += (afterSize - beforeSize);
                                }
                                continue;
                            }
                        }
                    }
                }

                std::string serializedValue = item.value().dump(4);
                size_t beforeSize = text.size();
                text.replace(span.start, span.end - span.start, serializedValue);
                size_t afterSize = text.size();
                if (afterSize != beforeSize)
                {
                    objSpan.end += (afterSize - beforeSize);
                }
            }
            else
            {
                std::string serializedValue = item.value().dump(4);
                std::string entry = (hasMembers ? "," : "") + std::string("\n") + indent + "\"" + key + "\": " + serializedValue;
                size_t beforeSize = text.size();
                text.insert(insertPos, entry);
                size_t afterSize = text.size();
                if (afterSize != beforeSize)
                {
                    objSpan.end += (afterSize - beforeSize);
                }
                insertPos += entry.size();
                hasMembers = true;
            }
            insertPos = FindInsertPosition(text, objSpan);
        }

        if (hasMembers)
        {
            if (insertPos < text.size() && text[insertPos] != '\n')
            {
                text.insert(insertPos, "\n");
            }
        }
        return true;
    }

    bool MergeIntoJsonText(std::string &text, const json &patch)
    {
        if (!patch.is_object())
        {
            return false;
        }

        Span root;
        if (!FindRootObjectSpan(text, root))
        {
            return false;
        }

        return MergeObjectInText(text, root, patch);
    }

    bool JsonReadTextFile(const std::wstring &path, std::string &outText)
    {
        outText.clear();
        std::ifstream f(std::filesystem::path(path), std::ios::binary);
        if (!f.is_open())
        {
            return false;
        }
        outText.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        return true;
    }

    bool JsonWriteTextFile(const std::wstring &path, const std::string &text)
    {
        std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
        if (!out.is_open())
        {
            return false;
        }
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
        return true;
    }

    bool JsonMergePatchFile(const std::wstring &path, const std::string &patchText)
    {
        try
        {
            const nlohmann::json patch = nlohmann::json::parse(patchText, nullptr, true, true);
            if (!patch.is_object())
            {
                return false;
            }

            std::string current;
            if (novadesk::shared::system::JsonReadTextFile(path, current) && current.find_first_not_of(" \t\r\n") != std::string::npos)
            {
                if (::novadesk::shared::system::MergeIntoJsonText(current, patch))
                {
                    return novadesk::shared::system::JsonWriteTextFile(path, current);
                }
                ::Logging::Log(::LogLevel::Warn, L"JSON merge failed; rewriting without preserving comments: %s", path.c_str());
            }

            return novadesk::shared::system::JsonWriteTextFile(path, patch.dump(4));
        }
        catch (const nlohmann::json::parse_error &e)
        {
            ::Logging::Log(::LogLevel::Error, L"JSON parse error in merge patch: %S", e.what());
            return false;
        }
        catch (...)
        {
            return false;
        }
    }

    // *****************************************************************************
    // Memory Metrics
    // *****************************************************************************

    bool GetMemoryStats(MemoryStats &outStats)
    {
        MEMORYSTATUSEX mem{};
        mem.dwLength = sizeof(mem);
        if (!GlobalMemoryStatusEx(&mem))
        {
            return false;
        }

        outStats.total = static_cast<double>(mem.ullTotalPhys);
        outStats.available = static_cast<double>(mem.ullAvailPhys);
        outStats.used = static_cast<double>(mem.ullTotalPhys - mem.ullAvailPhys);
        outStats.percent = static_cast<int>(mem.dwMemoryLoad);
        return true;
    }

    // *****************************************************************************
    // Mouse Metrics
    // *****************************************************************************

    bool GetMousePosition(MousePosition &outPos)
    {
        POINT p{};
        if (!GetCursorPos(&p))
        {
            return false;
        }
        outPos.x = static_cast<int>(p.x);
        outPos.y = static_cast<int>(p.y);
        return true;
    }

    // *****************************************************************************
    // Network Metrics
    // *****************************************************************************

    bool GetNetworkStats(NetworkStats &outStats)
    {
        ULONG size = 0;
        if (GetIfTable(nullptr, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER)
        {
            return false;
        }

        std::vector<BYTE> buffer(size);
        auto *table = reinterpret_cast<PMIB_IFTABLE>(buffer.data());
        if (GetIfTable(table, &size, FALSE) != NO_ERROR)
        {
            return false;
        }

        ULONGLONG totalIn = 0;
        ULONGLONG totalOut = 0;
        for (DWORD i = 0; i < table->dwNumEntries; ++i)
        {
            const MIB_IFROW &row = table->table[i];
            if (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL)
            {
                totalIn += row.dwInOctets;
                totalOut += row.dwOutOctets;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        double netIn = 0.0;
        double netOut = 0.0;
        if (g_lastNetworkSample != std::chrono::steady_clock::time_point::min())
        {
            const double dt = std::chrono::duration<double>(now - g_lastNetworkSample).count();
            if (dt > 0.0)
            {
                netIn = static_cast<double>(totalIn - g_lastTotalIn) / dt;
                netOut = static_cast<double>(totalOut - g_lastTotalOut) / dt;
            }
        }

        g_lastTotalIn = totalIn;
        g_lastTotalOut = totalOut;
        g_lastNetworkSample = now;

        outStats.netIn = netIn;
        outStats.netOut = netOut;
        outStats.totalIn = static_cast<double>(totalIn);
        outStats.totalOut = static_cast<double>(totalOut);
        return true;
    }

    // *****************************************************************************
    // Power
    // *****************************************************************************

    bool GetPowerStatus(PowerStatus &outStatus)
    {
        SYSTEM_POWER_STATUS sps{};
        if (!GetSystemPowerStatus(&sps))
        {
            return false;
        }

        outStatus.acline = (sps.ACLineStatus == 1) ? 1 : 0;

        int status = 4;
        if (sps.BatteryFlag & 128)
            status = 0;
        else if (sps.BatteryFlag & 8)
            status = 1;
        else if (sps.BatteryFlag & 4)
            status = 2;
        else if (sps.BatteryFlag & 2)
            status = 3;

        outStatus.status = status;
        outStatus.status2 = sps.BatteryFlag;
        outStatus.lifetime = static_cast<double>(sps.BatteryLifeTime);
        outStatus.percent = (sps.BatteryLifePercent == 255) ? 0 : sps.BatteryLifePercent;

        SYSTEM_INFO si{};
        GetSystemInfo(&si);
        const UINT cpuCount = static_cast<UINT>(si.dwNumberOfProcessors);
        outStatus.mhz = 0.0;

        if (cpuCount > 0)
        {
            auto *ppi = new PROCESSOR_POWER_INFORMATION_LOCAL[cpuCount];
            if (CallNtPowerInformation(
                    ProcessorInformation,
                    nullptr,
                    0,
                    ppi,
                    sizeof(PROCESSOR_POWER_INFORMATION_LOCAL) * cpuCount) == 0)
            {
                outStatus.mhz = static_cast<double>(ppi[0].CurrentMhz);
            }
            delete[] ppi;
        }

        outStatus.hz = outStatus.mhz * 1000000.0;
        return true;
    }

    // *****************************************************************************
    // Registry
    // *****************************************************************************

    HKEY GetRegistryRootKey(const std::wstring &root)
    {
        if (root == L"HKCU" || root == L"HKEY_CURRENT_USER")
            return HKEY_CURRENT_USER;
        if (root == L"HKLM" || root == L"HKEY_LOCAL_MACHINE")
            return HKEY_LOCAL_MACHINE;
        if (root == L"HKCR" || root == L"HKEY_CLASSES_ROOT")
            return HKEY_CLASSES_ROOT;
        if (root == L"HKU" || root == L"HKEY_USERS")
            return HKEY_USERS;
        return nullptr;
    }

    bool SplitRegistryPath(const std::wstring &fullPath, HKEY &outRoot, std::wstring &outSubKey)
    {
        const size_t p = fullPath.find(L'\\');
        if (p == std::wstring::npos)
        {
            return false;
        }
        const std::wstring rootPart = fullPath.substr(0, p);
        outSubKey = fullPath.substr(p + 1);
        outRoot = GetRegistryRootKey(rootPart);
        return outRoot != nullptr && !outSubKey.empty();
    }

    bool RegistryReadData(const std::wstring &fullPath, const std::wstring &valueName, RegistryValue &outValue)
    {
        outValue = RegistryValue{};

        HKEY root = nullptr;
        std::wstring subKey;
        if (!SplitRegistryPath(fullPath, root, subKey))
        {
            return false;
        }

        HKEY key = nullptr;
        if (RegOpenKeyExW(root, subKey.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
        {
            return false;
        }

        DWORD type = 0;
        DWORD size = 0;
        if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, nullptr, &size) != ERROR_SUCCESS)
        {
            RegCloseKey(key);
            return false;
        }

        if (type == REG_SZ || type == REG_EXPAND_SZ)
        {
            std::vector<wchar_t> buf(size / sizeof(wchar_t) + 1, L'\0');
            if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(buf.data()), &size) == ERROR_SUCCESS)
            {
                outValue.type = RegistryValueType::String;
                outValue.stringValue = buf.data();
                RegCloseKey(key);
                return true;
            }
        }
        else if (type == REG_DWORD)
        {
            DWORD value = 0;
            DWORD dwordSize = sizeof(value);
            if (RegQueryValueExW(key, valueName.c_str(), nullptr, &type, reinterpret_cast<LPBYTE>(&value), &dwordSize) == ERROR_SUCCESS)
            {
                outValue.type = RegistryValueType::Number;
                outValue.numberValue = static_cast<double>(value);
                RegCloseKey(key);
                return true;
            }
        }

        RegCloseKey(key);
        return false;
    }

    bool RegistryWriteString(const std::wstring &fullPath, const std::wstring &valueName, const std::wstring &value)
    {
        HKEY root = nullptr;
        std::wstring subKey;
        if (!SplitRegistryPath(fullPath, root, subKey))
        {
            return false;
        }

        HKEY key = nullptr;
        if (RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        {
            return false;
        }

        const auto *raw = reinterpret_cast<const BYTE *>(value.c_str());
        const DWORD rawSize = static_cast<DWORD>((value.size() + 1) * sizeof(wchar_t));
        const bool ok = (RegSetValueExW(key, valueName.c_str(), 0, REG_SZ, raw, rawSize) == ERROR_SUCCESS);
        RegCloseKey(key);
        return ok;
    }

    bool RegistryWriteNumber(const std::wstring &fullPath, const std::wstring &valueName, double value)
    {
        HKEY root = nullptr;
        std::wstring subKey;
        if (!SplitRegistryPath(fullPath, root, subKey))
        {
            return false;
        }

        HKEY key = nullptr;
        if (RegCreateKeyExW(root, subKey.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_WRITE, nullptr, &key, nullptr) != ERROR_SUCCESS)
        {
            return false;
        }

        const DWORD dword = static_cast<DWORD>(value);
        const bool ok = (RegSetValueExW(
                             key,
                             valueName.c_str(),
                             0,
                             REG_DWORD,
                             reinterpret_cast<const BYTE *>(&dword),
                             sizeof(dword)) == ERROR_SUCCESS);
        RegCloseKey(key);
        return ok;
    }

    // *****************************************************************************
    // Wallpaper
    // *****************************************************************************

    bool SetWallpaper(const std::wstring &imagePath, const std::wstring &style)
    {
        const wchar_t *wallpaperStyle = L"10";
        const wchar_t *tileWallpaper = L"0";
        if (style == L"fill")
        {
            wallpaperStyle = L"10";
            tileWallpaper = L"0";
        }
        else if (style == L"fit")
        {
            wallpaperStyle = L"6";
            tileWallpaper = L"0";
        }
        else if (style == L"stretch")
        {
            wallpaperStyle = L"2";
            tileWallpaper = L"0";
        }
        else if (style == L"tile")
        {
            wallpaperStyle = L"0";
            tileWallpaper = L"1";
        }
        else if (style == L"center")
        {
            wallpaperStyle = L"0";
            tileWallpaper = L"0";
        }
        else if (style == L"span")
        {
            wallpaperStyle = L"22";
            tileWallpaper = L"0";
        }

        HKEY key = nullptr;
        const LONG openRes = RegOpenKeyExW(HKEY_CURRENT_USER, L"Control Panel\\Desktop", 0, KEY_SET_VALUE, &key);
        if (openRes == ERROR_SUCCESS)
        {
            RegSetValueExW(
                key,
                L"WallpaperStyle",
                0,
                REG_SZ,
                reinterpret_cast<const BYTE *>(wallpaperStyle),
                static_cast<DWORD>((wcslen(wallpaperStyle) + 1) * sizeof(wchar_t)));
            RegSetValueExW(
                key,
                L"TileWallpaper",
                0,
                REG_SZ,
                reinterpret_cast<const BYTE *>(tileWallpaper),
                static_cast<DWORD>((wcslen(tileWallpaper) + 1) * sizeof(wchar_t)));
            RegCloseKey(key);
        }

        return SystemParametersInfoW(
                   SPI_SETDESKWALLPAPER,
                   0,
                   const_cast<wchar_t *>(imagePath.c_str()),
                   SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE) == TRUE;
    }

    bool GetCurrentWallpaperPath(std::wstring &outPath)
    {
        outPath.assign(MAX_PATH, L'\0');
        const BOOL ok = SystemParametersInfoW(
            SPI_GETDESKWALLPAPER,
            static_cast<UINT>(outPath.size()),
            outPath.data(),
            0);
        if (!ok)
        {
            outPath.clear();
            return false;
        }
        outPath.resize(wcslen(outPath.c_str()));
        return !outPath.empty();
    }

    // *****************************************************************************
    // Webfetch
    // *****************************************************************************

    bool WebFetch(const std::wstring &url, std::string &outData)
    {
        outData.clear();
        if (url.empty())
        {
            return false;
        }

        const bool isHttp = (url.rfind(L"http://", 0) == 0 || url.rfind(L"https://", 0) == 0);
        if (!isHttp)
        {
            std::wstring path = url;
            if (url.rfind(L"file://", 0) == 0)
            {
                path = url.substr(7);
                if (!path.empty() && path[0] == L'/' && path.size() > 2 && path[2] == L':')
                {
                    path = path.substr(1);
                }
            }

            std::ifstream f(std::filesystem::path(path), std::ios::binary);
            if (!f.is_open())
            {
                return false;
            }
            outData.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
            return true;
        }

        HINTERNET hInternet = InternetOpenW(L"Novadesk WebFetch", INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, 0);
        if (!hInternet)
        {
            return false;
        }

        DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
        if (url.rfind(L"https://", 0) == 0)
        {
            flags |= INTERNET_FLAG_SECURE;
        }
        HINTERNET hUrl = InternetOpenUrlW(hInternet, url.c_str(), nullptr, 0, flags, 0);
        if (!hUrl)
        {
            InternetCloseHandle(hInternet);
            return false;
        }

        char buffer[4096];
        DWORD bytesRead = 0;
        while (InternetReadFile(hUrl, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0)
        {
            outData.append(buffer, bytesRead);
        }

        InternetCloseHandle(hUrl);
        InternetCloseHandle(hInternet);
        return true;
    }

} // namespace novadesk::shared::system
