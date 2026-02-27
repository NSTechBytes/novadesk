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

#if ((__cplusplus >= 202002L) || defined(__cpp_impl_coroutine)) && __has_include(<winrt/base.h>) && __has_include(<winrt/Windows.Media.Control.h>) && __has_include(<winrt/Windows.Storage.Streams.h>)
#define NOVADESK_HAS_WINRT_NOWPLAYING 1
#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Media.h>
#include <winrt/Windows.Media.Control.h>
#endif

#include "../domain/DesktopManager.h"
#include "PathUtils.h"
#include "Utils.h"
#include "../../../third_party/kiss_fft130/kiss_fftr.h"

#pragma comment(lib, "Winmm.lib")
#pragma comment(lib, "PowrProf.lib")
#pragma comment(lib, "Iphlpapi.lib")
#pragma comment(lib, "runtimeobject.lib")

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

ULONGLONG g_lastIdleTime = 0;
ULONGLONG g_lastKernelTime = 0;
ULONGLONG g_lastUserTime = 0;
bool g_cpuInitialized = false;

ULONGLONG g_lastTotalIn = 0;
ULONGLONG g_lastTotalOut = 0;
std::chrono::steady_clock::time_point g_lastNetworkSample = std::chrono::steady_clock::time_point::min();

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

struct ComInit {
    HRESULT hr = E_FAIL;
    ComInit() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
    ~ComInit() { if (SUCCEEDED(hr)) CoUninitialize(); }
    bool Ok() const { return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE; }
};

std::wstring ToLowerCopy(const std::wstring& s) {
    std::wstring out = s;
    std::transform(out.begin(), out.end(), out.begin(), ::towlower);
    return out;
}

std::wstring FileNameFromPath(const std::wstring& path) {
    size_t pos = path.find_last_of(L"\\/");
    return (pos == std::wstring::npos) ? path : path.substr(pos + 1);
}

std::wstring GetProcessPathByPid(DWORD pid) {
    std::wstring result;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) return result;
    wchar_t buf[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameW(process, 0, buf, &size)) {
        result = buf;
    }
    CloseHandle(process);
    return result;
}

bool EnsureAppIconDir(std::wstring& outDir) {
    std::wstring baseDir = PathUtils::GetAppDataPath();
    if (baseDir.empty()) return false;
    outDir = baseDir + L"AppIcons\\";
    CreateDirectoryW(baseDir.c_str(), nullptr);
    CreateDirectoryW(outDir.c_str(), nullptr);
    return true;
}

bool SetForMatchingSessions(DWORD pid, const std::wstring& processName, bool byPid, float* setVolume, bool* setMute) {
    ComInit com;
    if (!com.Ok()) return false;

    IMMDeviceEnumerator* deviceEnum = nullptr;
    IMMDevice* device = nullptr;
    IAudioSessionManager2* manager2 = nullptr;
    IAudioSessionEnumerator* sessionEnum = nullptr;
    bool anySet = false;
    std::wstring target = byPid ? L"" : ToLowerCopy(processName);

    do {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnum);
        if (FAILED(hr) || !deviceEnum) break;
        hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
        if (FAILED(hr) || !device) break;
        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&manager2);
        if (FAILED(hr) || !manager2) break;
        hr = manager2->GetSessionEnumerator(&sessionEnum);
        if (FAILED(hr) || !sessionEnum) break;

        int count = 0;
        hr = sessionEnum->GetCount(&count);
        if (FAILED(hr)) break;

        for (int i = 0; i < count; ++i) {
            IAudioSessionControl* control = nullptr;
            IAudioSessionControl2* control2 = nullptr;
            ISimpleAudioVolume* simpleVol = nullptr;

            if (FAILED(sessionEnum->GetSession(i, &control)) || !control) continue;
            if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&control2)) || !control2) {
                control->Release();
                continue;
            }
            if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVol)) || !simpleVol) {
                control2->Release();
                control->Release();
                continue;
            }

            DWORD curPid = 0;
            control2->GetProcessId(&curPid);
            bool match = false;
            if (byPid) {
                match = (curPid == pid);
            } else {
                std::wstring curName = ToLowerCopy(FileNameFromPath(GetProcessPathByPid(curPid)));
                match = (curName == target);
            }

            if (match) {
                if (setVolume) simpleVol->SetMasterVolume(*setVolume, nullptr);
                if (setMute) simpleVol->SetMute(*setMute ? TRUE : FALSE, nullptr);
                anySet = true;
            }

            simpleVol->Release();
            control2->Release();
            control->Release();
        }
    } while (false);

    if (sessionEnum) sessionEnum->Release();
    if (manager2) manager2->Release();
    if (device) device->Release();
    if (deviceEnum) deviceEnum->Release();
    return anySet;
}

float Clamp01(float v) {
    if (v < 0.0f) return 0.0f;
    if (v > 1.0f) return 1.0f;
    return v;
}

class AudioLevelAnalyzer {
public:
    ~AudioLevelAnalyzer() { Release(); }

    bool GetStats(AudioLevelStats& outStats, const AudioLevelConfig& config) {
        if (!EnsureInitialized(config)) {
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

    IMMDeviceEnumerator* m_enumerator = nullptr;
    IMMDevice* m_device = nullptr;
    IAudioClient* m_audioClient = nullptr;
    IAudioCaptureClient* m_captureClient = nullptr;
    WAVEFORMATEX* m_pwfx = nullptr;

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

    bool SameConfig(const AudioLevelConfig& c) const {
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

    int NormalizeFFTSize(int n) const {
        if (n < 2) n = 1024;
        if (n % 2 != 0) ++n;
        return n;
    }

    int ClampMs(int ms) const { return (ms <= 0) ? 1 : ms; }

    float CalcSmoothCoeff(int ms) const {
        const double sr = static_cast<double>(m_sampleRate <= 0 ? 48000 : m_sampleRate);
        const double t = static_cast<double>(ClampMs(ms)) * 0.001;
        return static_cast<float>(std::exp(std::log10(0.01) / (sr * t)));
    }

    void RecalculateCoeffs() {
        m_kRMS[0] = CalcSmoothCoeff(m_config.rmsAttack);
        m_kRMS[1] = CalcSmoothCoeff(m_config.rmsDecay);
        m_kPeak[0] = CalcSmoothCoeff(m_config.peakAttack);
        m_kPeak[1] = CalcSmoothCoeff(m_config.peakDecay);

        const double fftRate = static_cast<double>(m_sampleRate <= 0 ? 48000 : m_sampleRate) /
            static_cast<double>(m_fftStride <= 0 ? 1 : m_fftStride);
        const auto fftCoeff = [&](int ms) -> float {
            const double t = static_cast<double>(ClampMs(ms)) * 0.001;
            return static_cast<float>(std::exp(std::log10(0.01) / (fftRate * t)));
        };
        m_kFFT[0] = fftCoeff(m_config.fftAttack);
        m_kFFT[1] = fftCoeff(m_config.fftDecay);
    }

    bool EnsureInitialized(const AudioLevelConfig& config) {
        if (m_initialized && SameConfig(config)) return true;
        if (m_initialized) Release();
        m_config = config;

        m_fftSize = NormalizeFFTSize(m_config.fftSize);
        m_fftOverlap = m_config.fftOverlap;
        if (m_fftOverlap < 0) m_fftOverlap = m_fftSize / 2;
        if (m_fftOverlap >= m_fftSize) m_fftOverlap = m_fftSize / 2;
        m_fftStride = m_fftSize - m_fftOverlap;
        if (m_fftStride <= 0) m_fftStride = m_fftSize / 2;
        m_fftCountdown = m_fftStride;

        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&m_enumerator));
        if (FAILED(hr) || !m_enumerator) return false;

        EDataFlow dataFlow = (m_config.port == "input") ? eCapture : eRender;
        if (!m_config.deviceId.empty()) hr = m_enumerator->GetDevice(m_config.deviceId.c_str(), &m_device);
        else hr = m_enumerator->GetDefaultAudioEndpoint(dataFlow, eMultimedia, &m_device);
        if (FAILED(hr) || !m_device) return false;

        hr = m_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, reinterpret_cast<void**>(&m_audioClient));
        if (FAILED(hr) || !m_audioClient) return false;

        hr = m_audioClient->GetMixFormat(&m_pwfx);
        if (FAILED(hr) || !m_pwfx) return false;

        m_sampleRate = static_cast<int>(m_pwfx->nSamplesPerSec);
        m_channels = static_cast<int>(m_pwfx->nChannels);
        if (m_channels < 1) m_channels = 1;
        if (m_channels > 2) m_channels = 2;

        DWORD flags = (dataFlow == eRender) ? AUDCLNT_STREAMFLAGS_LOOPBACK : 0;
        hr = m_audioClient->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, 0, 0, m_pwfx, nullptr);
        if (FAILED(hr)) return false;

        hr = m_audioClient->GetService(__uuidof(IAudioCaptureClient), reinterpret_cast<void**>(&m_captureClient));
        if (FAILED(hr) || !m_captureClient) return false;

        hr = m_audioClient->Start();
        if (FAILED(hr)) return false;

        m_fftCfg = kiss_fftr_alloc(m_fftSize, 0, nullptr, nullptr);
        if (!m_fftCfg) return false;

        m_ring.assign(static_cast<size_t>(m_fftSize), 0.0f);
        m_fftWindow.resize(static_cast<size_t>(m_fftSize));
        m_fftIn.resize(static_cast<size_t>(m_fftSize));
        m_fftOut.resize(static_cast<size_t>(m_fftSize));
        m_spectrum.assign(static_cast<size_t>(m_fftSize / 2 + 1), 0.0f);

        for (int i = 0; i < m_fftSize; ++i) {
            m_fftWindow[static_cast<size_t>(i)] = 0.5f * (1.0f - std::cos(2.0f * 3.1415926535f * i / static_cast<float>(m_fftSize - 1)));
        }

        RecalculateCoeffs();
        m_initialized = true;
        return true;
    }

    void Release() {
        if (m_audioClient) m_audioClient->Stop();
        if (m_captureClient) { m_captureClient->Release(); m_captureClient = nullptr; }
        if (m_audioClient) { m_audioClient->Release(); m_audioClient = nullptr; }
        if (m_device) { m_device->Release(); m_device = nullptr; }
        if (m_enumerator) { m_enumerator->Release(); m_enumerator = nullptr; }
        if (m_pwfx) { CoTaskMemFree(m_pwfx); m_pwfx = nullptr; }
        if (m_fftCfg) { free(m_fftCfg); m_fftCfg = nullptr; }
        m_initialized = false;
    }

    void PushMonoSample(float s) {
        m_ring[static_cast<size_t>(m_ringWrite)] = s;
        m_ringWrite = (m_ringWrite + 1) % m_fftSize;
        if (m_ringFilled < m_fftSize) ++m_ringFilled;
        if (--m_fftCountdown <= 0) { m_fftCountdown = m_fftStride; ComputeFFT(); }
    }

    void ComputeFFT() {
        if (m_ringFilled < m_fftSize) return;
        int idx = m_ringWrite;
        for (int i = 0; i < m_fftSize; ++i) {
            m_fftIn[static_cast<size_t>(i)] = m_ring[static_cast<size_t>(idx)] * m_fftWindow[static_cast<size_t>(i)];
            idx = (idx + 1) % m_fftSize;
        }
        kiss_fftr(m_fftCfg, m_fftIn.data(), m_fftOut.data());
        const int outSize = m_fftSize / 2 + 1;
        const float scalar = 1.0f / std::sqrt(static_cast<float>(m_fftSize));
        for (int i = 0; i < outSize; ++i) {
            const float re = m_fftOut[static_cast<size_t>(i)].r;
            const float im = m_fftOut[static_cast<size_t>(i)].i;
            const float mag = std::sqrt(re * re + im * im) * scalar;
            float& old = m_spectrum[static_cast<size_t>(i)];
            old = mag + m_kFFT[(mag < old)] * (old - mag);
        }
    }

    std::vector<float> BuildBands(int bands) const {
        std::vector<float> out(static_cast<size_t>(bands), 0.0f);
        if (m_spectrum.empty()) return out;
        const int outSize = static_cast<int>(m_spectrum.size());
        const double freqMin = m_config.freqMin <= 0.0 ? 20.0 : m_config.freqMin;
        const double freqMax = (m_config.freqMax <= freqMin) ? 20000.0 : m_config.freqMax;
        const double sensitivity = (m_config.sensitivity <= 0.0) ? 35.0 : m_config.sensitivity;

        for (int i = 0; i < bands; ++i) {
            const double f1 = freqMin * std::pow(freqMax / freqMin, static_cast<double>(i) / static_cast<double>(bands));
            const double f2 = freqMin * std::pow(freqMax / freqMin, static_cast<double>(i + 1) / static_cast<double>(bands));
            int idx1 = static_cast<int>(f1 * 2.0 * outSize / static_cast<double>(m_sampleRate));
            int idx2 = static_cast<int>(f2 * 2.0 * outSize / static_cast<double>(m_sampleRate));
            if (idx1 < 0) idx1 = 0;
            if (idx2 >= outSize) idx2 = outSize - 1;
            if (idx2 < idx1) idx2 = idx1;
            float maxVal = 0.0f;
            for (int k = idx1; k <= idx2; ++k) maxVal = std::max(maxVal, m_spectrum[static_cast<size_t>(k)]);
            if (maxVal > 0.0f) {
                const float db = 20.0f * std::log10(maxVal);
                out[static_cast<size_t>(i)] = Clamp01(1.0f + db / static_cast<float>(sensitivity));
            }
        }
        return out;
    }

    void PollCapture() {
        if (!m_captureClient || !m_pwfx) return;
        UINT32 packetLength = 0;
        HRESULT hr = m_captureClient->GetNextPacketSize(&packetLength);

        bool hadAudio = false;
        float sumSq[2] = {0.0f, 0.0f};
        float peak[2] = {0.0f, 0.0f};
        uint64_t frameCount = 0;

        while (SUCCEEDED(hr) && packetLength > 0) {
            BYTE* data = nullptr;
            UINT32 frames = 0;
            DWORD flags = 0;
            hr = m_captureClient->GetBuffer(&data, &frames, &flags, nullptr, nullptr);
            if (FAILED(hr)) break;

            const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
            const bool isFloat = (m_pwfx->wFormatTag == WAVE_FORMAT_IEEE_FLOAT) || (m_pwfx->wBitsPerSample == 32);

            if (!silent && data && frames > 0) {
                hadAudio = true;
                if (isFloat) {
                    const float* in = reinterpret_cast<const float*>(data);
                    for (UINT32 i = 0; i < frames; ++i) {
                        const float l = in[i * m_pwfx->nChannels + 0];
                        const float r = (m_pwfx->nChannels > 1) ? in[i * m_pwfx->nChannels + 1] : l;
                        sumSq[0] += l * l;
                        sumSq[1] += r * r;
                        peak[0] = std::max(peak[0], std::fabs(l));
                        peak[1] = std::max(peak[1], std::fabs(r));
                        PushMonoSample((l + r) * 0.5f);
                    }
                } else {
                    const int16_t* in = reinterpret_cast<const int16_t*>(data);
                    for (UINT32 i = 0; i < frames; ++i) {
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

        if (hadAudio && frameCount > 0) {
            const float n = static_cast<float>(frameCount);
            const float rmsNow0 = std::sqrt(sumSq[0] / n);
            const float rmsNow1 = std::sqrt(sumSq[1] / n);
            m_rms[0] = Clamp01(rmsNow0 + m_kRMS[(rmsNow0 < m_rms[0])] * (m_rms[0] - rmsNow0));
            m_rms[1] = Clamp01(rmsNow1 + m_kRMS[(rmsNow1 < m_rms[1])] * (m_rms[1] - rmsNow1));
            m_peak[0] = Clamp01(peak[0] + m_kPeak[(peak[0] < m_peak[0])] * (m_peak[0] - peak[0]));
            m_peak[1] = Clamp01(peak[1] + m_kPeak[(peak[1] < m_peak[1])] * (m_peak[1] - peak[1]));
        } else {
            m_rms[0] *= m_kRMS[1];
            m_rms[1] *= m_kRMS[1];
            m_peak[0] *= m_kPeak[1];
            m_peak[1] *= m_kPeak[1];
        }
    }
};

AudioLevelAnalyzer g_audioLevelAnalyzer;

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

bool GetCpuStats(CpuStats& outStats) {
    FILETIME idleFt{}, kernelFt{}, userFt{};
    if (!GetSystemTimes(&idleFt, &kernelFt, &userFt)) {
        return false;
    }

    ULARGE_INTEGER idle{}, kernel{}, user{};
    idle.LowPart = idleFt.dwLowDateTime;
    idle.HighPart = idleFt.dwHighDateTime;
    kernel.LowPart = kernelFt.dwLowDateTime;
    kernel.HighPart = kernelFt.dwHighDateTime;
    user.LowPart = userFt.dwLowDateTime;
    user.HighPart = userFt.dwHighDateTime;

    if (!g_cpuInitialized) {
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

    if (totalDelta == 0) {
        outStats.usage = 0.0;
        return true;
    }

    double usage = (static_cast<double>(totalDelta - idleDelta) * 100.0) / static_cast<double>(totalDelta);
    if (usage < 0.0) usage = 0.0;
    if (usage > 100.0) usage = 100.0;
    outStats.usage = usage;
    return true;
}

bool GetMemoryStats(MemoryStats& outStats) {
    MEMORYSTATUSEX mem{};
    mem.dwLength = sizeof(mem);
    if (!GlobalMemoryStatusEx(&mem)) {
        return false;
    }

    outStats.total = static_cast<double>(mem.ullTotalPhys);
    outStats.available = static_cast<double>(mem.ullAvailPhys);
    outStats.used = static_cast<double>(mem.ullTotalPhys - mem.ullAvailPhys);
    outStats.percent = static_cast<int>(mem.dwMemoryLoad);
    return true;
}

bool GetNetworkStats(NetworkStats& outStats) {
    ULONG size = 0;
    if (GetIfTable(nullptr, &size, FALSE) != ERROR_INSUFFICIENT_BUFFER) {
        return false;
    }

    std::vector<BYTE> buffer(size);
    auto* table = reinterpret_cast<PMIB_IFTABLE>(buffer.data());
    if (GetIfTable(table, &size, FALSE) != NO_ERROR) {
        return false;
    }

    ULONGLONG totalIn = 0;
    ULONGLONG totalOut = 0;
    for (DWORD i = 0; i < table->dwNumEntries; ++i) {
        const MIB_IFROW& row = table->table[i];
        if (row.dwOperStatus == IF_OPER_STATUS_OPERATIONAL) {
            totalIn += row.dwInOctets;
            totalOut += row.dwOutOctets;
        }
    }

    const auto now = std::chrono::steady_clock::now();
    double netIn = 0.0;
    double netOut = 0.0;
    if (g_lastNetworkSample != std::chrono::steady_clock::time_point::min()) {
        const double dt = std::chrono::duration<double>(now - g_lastNetworkSample).count();
        if (dt > 0.0) {
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

bool GetMousePosition(MousePosition& outPos) {
    POINT p{};
    if (!GetCursorPos(&p)) {
        return false;
    }
    outPos.x = static_cast<int>(p.x);
    outPos.y = static_cast<int>(p.y);
    return true;
}

bool GetDiskStats(const std::wstring& path, DiskStats& outStats) {
    std::wstring target = path;
    if (target.empty()) {
        std::error_code ec;
        target = std::filesystem::current_path(ec).root_path().wstring();
        if (target.empty()) {
            target = L"C:\\";
        }
    }

    ULARGE_INTEGER freeBytesAvailable{};
    ULARGE_INTEGER totalBytes{};
    ULARGE_INTEGER totalFreeBytes{};
    if (!GetDiskFreeSpaceExW(target.c_str(), &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
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

bool GetAudioLevelStats(AudioLevelStats& outStats, const AudioLevelConfig& config) {
    if (!g_audioLevelAnalyzer.GetStats(outStats, config)) {
        return false;
    }
    return true;
}

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

bool AppVolumeListSessions(std::vector<AppVolumeSessionInfo>& sessions) {
    sessions.clear();
    ComInit com;
    if (!com.Ok()) return false;

    IMMDeviceEnumerator* deviceEnum = nullptr;
    IMMDevice* device = nullptr;
    IAudioSessionManager2* manager2 = nullptr;
    IAudioSessionEnumerator* sessionEnum = nullptr;
    bool success = false;

    do {
        HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&deviceEnum);
        if (FAILED(hr) || !deviceEnum) break;

        hr = deviceEnum->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
        if (FAILED(hr) || !device) break;

        hr = device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&manager2);
        if (FAILED(hr) || !manager2) break;

        hr = manager2->GetSessionEnumerator(&sessionEnum);
        if (FAILED(hr) || !sessionEnum) break;

        int count = 0;
        hr = sessionEnum->GetCount(&count);
        if (FAILED(hr)) break;

        for (int i = 0; i < count; ++i) {
            IAudioSessionControl* control = nullptr;
            IAudioSessionControl2* control2 = nullptr;
            ISimpleAudioVolume* simpleVol = nullptr;

            if (FAILED(sessionEnum->GetSession(i, &control)) || !control) continue;
            if (FAILED(control->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&control2)) || !control2) {
                control->Release();
                continue;
            }
            if (FAILED(control->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleVol)) || !simpleVol) {
                control2->Release();
                control->Release();
                continue;
            }

            AudioSessionState state{};
            if (FAILED(control->GetState(&state)) || state != AudioSessionStateActive) {
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
            peak = 0.0f;

            LPWSTR displayName = nullptr;
            std::wstring display;
            if (SUCCEEDED(control->GetDisplayName(&displayName)) && displayName) {
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

            if (!info.filePath.empty()) {
                std::wstring iconDir;
                if (EnsureAppIconDir(iconDir)) {
                    info.iconPath = iconDir + L"pid_" + std::to_wstring(pid) + L"_48.ico";
                    if (!Utils::ExtractFileIconToIco(info.filePath, info.iconPath, 48)) {
                        info.iconPath.clear();
                    }
                }
            }

            sessions.push_back(info);

            simpleVol->Release();
            control2->Release();
            control->Release();
        }

        success = true;
    } while (false);

    if (sessionEnum) sessionEnum->Release();
    if (manager2) manager2->Release();
    if (device) device->Release();
    if (deviceEnum) deviceEnum->Release();
    return success;
}

bool AppVolumeGetByPid(uint32_t pid, float& outVolume, bool& outMuted, float& outPeak) {
    std::vector<AppVolumeSessionInfo> sessions;
    if (!AppVolumeListSessions(sessions)) return false;

    double sum = 0.0;
    int count = 0;
    bool mutedAny = false;
    double peakMax = 0.0;
    for (const auto& s : sessions) {
        if (s.pid == pid) {
            sum += s.volume;
            mutedAny = mutedAny || s.muted;
            if (s.peak > peakMax) peakMax = s.peak;
            ++count;
        }
    }
    if (count == 0) return false;

    outVolume = static_cast<float>(sum / static_cast<double>(count));
    outMuted = mutedAny;
    outPeak = static_cast<float>(peakMax);
    return true;
}

bool AppVolumeGetByProcessName(const std::wstring& processName, float& outVolume, bool& outMuted, float& outPeak) {
    std::vector<AppVolumeSessionInfo> sessions;
    if (!AppVolumeListSessions(sessions)) return false;

    std::wstring target = ToLowerCopy(processName);
    double sum = 0.0;
    int count = 0;
    bool mutedAny = false;
    double peakMax = 0.0;
    for (const auto& s : sessions) {
        if (ToLowerCopy(s.processName) == target) {
            sum += s.volume;
            mutedAny = mutedAny || s.muted;
            if (s.peak > peakMax) peakMax = s.peak;
            ++count;
        }
    }
    if (count == 0) return false;

    outVolume = static_cast<float>(sum / static_cast<double>(count));
    outMuted = mutedAny;
    outPeak = static_cast<float>(peakMax);
    return true;
}

bool AppVolumeSetVolumeByPid(uint32_t pid, float volume01) {
    if (volume01 < 0.0f) volume01 = 0.0f;
    if (volume01 > 1.0f) volume01 = 1.0f;
    return SetForMatchingSessions(static_cast<DWORD>(pid), L"", true, &volume01, nullptr);
}

bool AppVolumeSetVolumeByProcessName(const std::wstring& processName, float volume01) {
    if (volume01 < 0.0f) volume01 = 0.0f;
    if (volume01 > 1.0f) volume01 = 1.0f;
    return SetForMatchingSessions(0, processName, false, &volume01, nullptr);
}

bool AppVolumeSetMuteByPid(uint32_t pid, bool mute) {
    return SetForMatchingSessions(static_cast<DWORD>(pid), L"", true, nullptr, &mute);
}

bool AppVolumeSetMuteByProcessName(const std::wstring& processName, bool mute) {
    return SetForMatchingSessions(0, processName, false, nullptr, &mute);
}

#if NOVADESK_HAS_WINRT_NOWPLAYING
namespace {
using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Media;
using namespace winrt::Windows::Media::Control;
using namespace winrt::Windows::Storage::Streams;

enum class NowPlayingAction {
    Play,
    Pause,
    PlayPause,
    Stop,
    Next,
    Previous,
    SetPosition,
    SetShuffle,
    ToggleShuffle,
    SetRepeat
};

struct NowPlayingActionItem {
    NowPlayingAction action{};
    int value = 0;
    bool flag = false;
};

class NowPlayingController {
public:
    NowPlayingController() {
        m_worker = std::thread(&NowPlayingController::WorkerMain, this);
    }
    ~NowPlayingController() {
        {
            std::lock_guard<std::mutex> lock(m_actionMutex);
            m_stopWorker = true;
        }
        m_actionSignal.notify_all();
        if (m_worker.joinable()) m_worker.join();
    }

    NowPlayingStats ReadStats() {
        std::lock_guard<std::mutex> lock(m_statsMutex);
        return m_stats;
    }

    bool Play() { Queue(NowPlayingAction::Play); return true; }
    bool Pause() { Queue(NowPlayingAction::Pause); return true; }
    bool PlayPause() { Queue(NowPlayingAction::PlayPause); return true; }
    bool Stop() { Queue(NowPlayingAction::Stop); return true; }
    bool Next() { Queue(NowPlayingAction::Next); return true; }
    bool Previous() { Queue(NowPlayingAction::Previous); return true; }
    bool SetPosition(int value, bool isPercent) { Queue(NowPlayingAction::SetPosition, value, isPercent); return true; }
    bool SetShuffle(bool enabled) { Queue(NowPlayingAction::SetShuffle, 0, enabled); return true; }
    bool ToggleShuffle() { Queue(NowPlayingAction::ToggleShuffle); return true; }
    bool SetRepeat(int mode) { Queue(NowPlayingAction::SetRepeat, mode, false); return true; }

private:
    std::mutex m_statsMutex;
    NowPlayingStats m_stats{};

    std::mutex m_actionMutex;
    std::queue<NowPlayingActionItem> m_actions;
    std::condition_variable m_actionSignal;
    bool m_stopWorker = false;
    std::thread m_worker;

    GlobalSystemMediaTransportControlsSessionManager m_manager{ nullptr };
    std::wstring m_coverPath;
    std::wstring m_coverTrackKey;
    std::chrono::steady_clock::time_point m_localPosTime{};
    double m_localPosSec = 0.0;
    bool m_hasLocalPos = false;

    static int ToSeconds(TimeSpan span) {
        return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(span).count());
    }

    std::wstring BuildTrackKey(const NowPlayingStats& s) {
        return s.player + L"|" + s.artist + L"|" + s.album + L"|" + s.title;
    }

    std::wstring EnsureCoverPath() {
        if (!m_coverPath.empty()) return m_coverPath;
        std::wstring base = PathUtils::GetAppDataPath();
        if (base.empty()) return L"";
        std::wstring dir = base + L"NowPlaying\\";
        CreateDirectoryW(base.c_str(), nullptr);
        CreateDirectoryW(dir.c_str(), nullptr);
        m_coverPath = dir + L"cover.jpg";
        return m_coverPath;
    }

    bool SaveThumbnail(IRandomAccessStreamReference const& thumb) {
        try {
            const std::wstring path = EnsureCoverPath();
            if (path.empty()) return false;
            auto stream = thumb.OpenReadAsync().get();
            uint64_t size = stream.Size();
            if (size == 0 || size > (16ULL * 1024ULL * 1024ULL)) return false;

            Buffer buffer((uint32_t)size);
            auto rb = stream.ReadAsync(buffer, (uint32_t)size, InputStreamOptions::None).get();
            uint32_t len = rb.Length();
            if (len == 0) return false;
            DataReader reader = DataReader::FromBuffer(rb);
            std::vector<uint8_t> bytes(len);
            reader.ReadBytes(bytes);
            std::ofstream out(std::filesystem::path(path), std::ios::binary | std::ios::trunc);
            if (!out.is_open()) return false;
            out.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            return true;
        } catch (...) {
            return false;
        }
    }

    GlobalSystemMediaTransportControlsSession Session() {
        if (m_manager == nullptr) return nullptr;
        try { return m_manager.GetCurrentSession(); } catch (...) { return nullptr; }
    }

    void Queue(NowPlayingAction action, int value = 0, bool flag = false) {
        std::lock_guard<std::mutex> lock(m_actionMutex);
        if (m_stopWorker) return;
        m_actions.push(NowPlayingActionItem{ action, value, flag });
        m_actionSignal.notify_one();
    }

    void ProcessActions(std::queue<NowPlayingActionItem>& pending) {
        while (!pending.empty()) {
            auto action = pending.front();
            pending.pop();
            try {
                auto session = Session();
                if (session == nullptr) continue;
                switch (action.action) {
                case NowPlayingAction::Play: session.TryPlayAsync(); break;
                case NowPlayingAction::Pause: session.TryPauseAsync(); break;
                case NowPlayingAction::PlayPause: session.TryTogglePlayPauseAsync(); break;
                case NowPlayingAction::Stop: session.TryStopAsync(); break;
                case NowPlayingAction::Next: session.TrySkipNextAsync(); break;
                case NowPlayingAction::Previous: session.TrySkipPreviousAsync(); break;
                case NowPlayingAction::SetPosition: {
                    auto timeline = session.GetTimelineProperties();
                    int duration = 0;
                    if (timeline != nullptr) duration = (std::max)(0, ToSeconds(timeline.EndTime()) - ToSeconds(timeline.StartTime()));
                    int sec = action.flag ? ((duration > 0) ? (duration * action.value / 100) : 0) : action.value;
                    sec = (std::max)(0, (std::min)(duration > 0 ? duration : sec, sec));
                    auto ticks = TimeSpan(std::chrono::seconds(sec)).count();
                    session.TryChangePlaybackPositionAsync(ticks);
                    break;
                }
                case NowPlayingAction::SetShuffle:
                    session.TryChangeShuffleActiveAsync(action.flag);
                    break;
                case NowPlayingAction::ToggleShuffle: {
                    auto pb = session.GetPlaybackInfo();
                    bool curr = false;
                    if (pb != nullptr) {
                        auto sh = pb.IsShuffleActive();
                        if (sh != nullptr) curr = sh.Value();
                    }
                    session.TryChangeShuffleActiveAsync(!curr);
                    break;
                }
                case NowPlayingAction::SetRepeat: {
                    MediaPlaybackAutoRepeatMode mode = MediaPlaybackAutoRepeatMode::None;
                    if (action.value == 1) mode = MediaPlaybackAutoRepeatMode::Track;
                    else if (action.value == 2) mode = MediaPlaybackAutoRepeatMode::List;
                    session.TryChangeAutoRepeatModeAsync(mode);
                    break;
                }
                }
            } catch (...) {}
        }
    }

    void RefreshStats() {
        NowPlayingStats stats{};
        auto prev = ReadStats();
        auto nowSteady = std::chrono::steady_clock::now();

        auto session = Session();
        if (session != nullptr) {
            stats.available = true;
            stats.status = 1;
            try { stats.player = session.SourceAppUserModelId().c_str(); } catch (...) {}
            try {
                auto props = session.TryGetMediaPropertiesAsync().get();
                if (props != nullptr) {
                    stats.artist = props.Artist().c_str();
                    stats.album = props.AlbumTitle().c_str();
                    stats.title = props.Title().c_str();
                    std::wstring trackKey = BuildTrackKey(stats);
                    auto thumb = props.Thumbnail();
                    if (thumb != nullptr) {
                        if (trackKey != m_coverTrackKey || m_coverPath.empty()) {
                            if (SaveThumbnail(thumb)) m_coverTrackKey = trackKey;
                        }
                        stats.thumbnail = m_coverPath;
                    } else {
                        m_coverTrackKey.clear();
                    }
                }
            } catch (...) {}
            try {
                auto tl = session.GetTimelineProperties();
                if (tl != nullptr) {
                    stats.duration = (std::max)(0, ToSeconds(tl.EndTime()) - ToSeconds(tl.StartTime()));
                    stats.position = (std::max)(0, ToSeconds(tl.Position()));
                }
            } catch (...) {}
            try {
                auto pb = session.GetPlaybackInfo();
                if (pb != nullptr) {
                    auto ps = pb.PlaybackStatus();
                    if (ps == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing) stats.state = 1;
                    else if (ps == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Paused) stats.state = 2;
                    else stats.state = 0;
                    auto sh = pb.IsShuffleActive();
                    if (sh != nullptr) stats.shuffle = sh.Value();
                    auto rp = pb.AutoRepeatMode();
                    if (rp != nullptr) stats.repeat = rp.Value() != MediaPlaybackAutoRepeatMode::None;
                }
            } catch (...) {}

            if (stats.state == 1 && stats.duration > 0) {
                try {
                    auto tl = session.GetTimelineProperties();
                    if (tl != nullptr) {
                        auto last = tl.LastUpdatedTime().time_since_epoch();
                        auto now = winrt::clock::now().time_since_epoch();
                        int elapsed = (int)std::chrono::duration_cast<std::chrono::seconds>(now - last).count();
                        if (elapsed > 0 && elapsed <= 3) stats.position += elapsed;
                    }
                } catch (...) {}

                bool sameTrack =
                    !prev.title.empty() &&
                    prev.player == stats.player &&
                    prev.artist == stats.artist &&
                    prev.album == stats.album &&
                    prev.title == stats.title &&
                    prev.duration == stats.duration;
                if (!sameTrack || !m_hasLocalPos) {
                    m_localPosSec = (double)stats.position;
                    m_localPosTime = nowSteady;
                    m_hasLocalPos = true;
                } else {
                    double elapsedLocal = std::chrono::duration<double>(nowSteady - m_localPosTime).count();
                    if (elapsedLocal > 0.0 && elapsedLocal < 5.0) m_localPosSec += elapsedLocal;
                    m_localPosTime = nowSteady;
                    int predicted = (int)m_localPosSec;
                    if (stats.duration > 0) predicted = (std::min)(predicted, stats.duration);
                    if (stats.position < predicted || (stats.duration > 0 && stats.position >= stats.duration && predicted < stats.duration)) {
                        stats.position = predicted;
                    } else if (stats.position > predicted + 1) {
                        m_localPosSec = (double)stats.position;
                        m_localPosTime = nowSteady;
                    }
                }
            } else {
                m_hasLocalPos = false;
            }
            if (stats.duration > 0) {
                stats.position = (std::min)(stats.position, stats.duration);
                stats.progress = (std::min)(100, (stats.position * 100) / stats.duration);
            }
        }

        std::lock_guard<std::mutex> lock(m_statsMutex);
        m_stats = stats;
    }

    void WorkerMain() {
        HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
        bool uninit = SUCCEEDED(hr);
        auto nextRefresh = std::chrono::steady_clock::now();
        while (true) {
            std::queue<NowPlayingActionItem> pending;
            {
                std::unique_lock<std::mutex> lock(m_actionMutex);
                m_actionSignal.wait_until(lock, nextRefresh, [&] { return m_stopWorker || !m_actions.empty(); });
                if (m_stopWorker) break;
                std::swap(pending, m_actions);
            }
            if (m_manager == nullptr) {
                try { m_manager = GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get(); }
                catch (...) { m_manager = nullptr; }
            }
            ProcessActions(pending);
            auto now = std::chrono::steady_clock::now();
            if (now >= nextRefresh) {
                RefreshStats();
                nextRefresh = now + std::chrono::milliseconds(500);
            }
        }
        if (uninit) RoUninitialize();
    }
};

NowPlayingController& GetNowPlayingController() {
    static NowPlayingController s;
    return s;
}
} // namespace
#endif

bool GetNowPlayingStats(NowPlayingStats& outStats) {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    outStats = GetNowPlayingController().ReadStats();
    return true;
#else
    outStats = NowPlayingStats{};
    return true;
#endif
}

bool NowPlayingPlay() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().Play();
#else
    return false;
#endif
}

bool NowPlayingPause() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().Pause();
#else
    return false;
#endif
}

bool NowPlayingPlayPause() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().PlayPause();
#else
    return false;
#endif
}

bool NowPlayingStop() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().Stop();
#else
    return false;
#endif
}

bool NowPlayingNext() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().Next();
#else
    return false;
#endif
}

bool NowPlayingPrevious() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().Previous();
#else
    return false;
#endif
}

bool NowPlayingSetPosition(int value, bool isPercent) {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().SetPosition(value, isPercent);
#else
    (void)value;
    (void)isPercent;
    return false;
#endif
}

bool NowPlayingSetShuffle(bool enabled) {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().SetShuffle(enabled);
#else
    (void)enabled;
    return false;
#endif
}

bool NowPlayingToggleShuffle() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().ToggleShuffle();
#else
    return false;
#endif
}

bool NowPlayingSetRepeat(int mode) {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return GetNowPlayingController().SetRepeat(mode);
#else
    (void)mode;
    return false;
#endif
}

std::string NowPlayingBackend() {
#if NOVADESK_HAS_WINRT_NOWPLAYING
    return "winrt";
#else
    return "disabled";
#endif
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

std::wstring PathJoin(const std::vector<std::wstring>& parts) {
    std::filesystem::path out;
    for (const auto& part : parts) {
        out /= std::filesystem::path(part);
    }
    return out.lexically_normal().wstring();
}

std::wstring PathBasename(const std::wstring& path, const std::wstring& ext) {
    std::filesystem::path p(path);
    std::wstring base = p.filename().wstring();
    if (!ext.empty() && base.size() >= ext.size() &&
        base.compare(base.size() - ext.size(), ext.size(), ext) == 0) {
        base.resize(base.size() - ext.size());
    }
    return base;
}

std::wstring PathDirname(const std::wstring& path) {
    std::filesystem::path p(path);
    std::wstring dir = p.parent_path().wstring();
    if (dir.empty()) {
        dir = L".";
    }
    return dir;
}

std::wstring PathExtname(const std::wstring& path) {
    return std::filesystem::path(path).extension().wstring();
}

bool PathIsAbsolute(const std::wstring& path) {
    return std::filesystem::path(path).is_absolute();
}

std::wstring PathNormalize(const std::wstring& path) {
    return std::filesystem::path(path).lexically_normal().wstring();
}

std::wstring PathRelative(const std::wstring& from, const std::wstring& to) {
    return std::filesystem::path(to).lexically_relative(std::filesystem::path(from)).wstring();
}

PathParts PathParse(const std::wstring& path) {
    std::filesystem::path p(path);
    PathParts out;
    out.root = p.root_path().wstring();
    out.dir = p.parent_path().wstring();
    out.base = p.filename().wstring();
    out.ext = p.extension().wstring();
    out.name = p.stem().wstring();
    return out;
}

std::wstring PathFormat(const PathParts& parts) {
    std::wstring base = parts.base;
    if (base.empty()) {
        base = parts.name + parts.ext;
    }
    std::filesystem::path out = parts.dir.empty() ? std::filesystem::path(base) : (std::filesystem::path(parts.dir) / base);
    return out.lexically_normal().wstring();
}

}  // namespace novadesk::shared::system
