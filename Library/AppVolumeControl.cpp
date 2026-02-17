/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "AppVolumeControl.h"
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <algorithm>

namespace {
    struct ComInit {
        HRESULT hr = E_FAIL;
        ComInit() { hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED); }
        ~ComInit() { if (SUCCEEDED(hr)) CoUninitialize(); }
        bool Ok() const { return SUCCEEDED(hr) || hr == RPC_E_CHANGED_MODE; }
    };

    std::wstring ToLowerCopy(const std::wstring& s)
    {
        std::wstring out = s;
        std::transform(out.begin(), out.end(), out.begin(), ::towlower);
        return out;
    }

    std::wstring FileNameFromPath(const std::wstring& path)
    {
        size_t pos = path.find_last_of(L"\\/");
        return (pos == std::wstring::npos) ? path : path.substr(pos + 1);
    }

    std::wstring GetProcessNameByPid(DWORD pid)
    {
        std::wstring result;
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (!hProcess) return result;

        wchar_t buf[MAX_PATH] = {};
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameW(hProcess, 0, buf, &size)) {
            result = FileNameFromPath(buf);
        }
        CloseHandle(hProcess);
        return result;
    }
}

bool AppVolumeControl::ListSessions(std::vector<SessionInfo>& sessions)
{
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

            DWORD pid = 0;
            control2->GetProcessId(&pid);

            AudioSessionState state;
            if (FAILED(control->GetState(&state)) || state != AudioSessionStateActive) {
                simpleVol->Release();
                control2->Release();
                control->Release();
                continue;
            }

            float vol = 0.0f;
            BOOL mute = FALSE;
            simpleVol->GetMasterVolume(&vol);
            simpleVol->GetMute(&mute);

            LPWSTR displayName = nullptr;
            std::wstring display;
            if (SUCCEEDED(control->GetDisplayName(&displayName)) && displayName) {
                display = displayName;
                CoTaskMemFree(displayName);
            }

            SessionInfo info;
            info.pid = pid;
            info.processName = GetProcessNameByPid(pid);
            info.displayName = display;
            info.volume = vol;
            info.muted = mute != FALSE;
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

bool AppVolumeControl::GetByPid(DWORD pid, float& outVolume, bool& outMuted)
{
    std::vector<SessionInfo> sessions;
    if (!ListSessions(sessions)) return false;

    double sum = 0.0;
    int count = 0;
    bool mutedAny = false;
    for (const auto& s : sessions) {
        if (s.pid == pid) {
            sum += s.volume;
            mutedAny = mutedAny || s.muted;
            ++count;
        }
    }
    if (count == 0) return false;

    outVolume = (float)(sum / (double)count);
    outMuted = mutedAny;
    return true;
}

bool AppVolumeControl::GetByProcessName(const std::wstring& processName, float& outVolume, bool& outMuted)
{
    std::vector<SessionInfo> sessions;
    if (!ListSessions(sessions)) return false;

    std::wstring target = ToLowerCopy(processName);
    double sum = 0.0;
    int count = 0;
    bool mutedAny = false;
    for (const auto& s : sessions) {
        if (ToLowerCopy(s.processName) == target) {
            sum += s.volume;
            mutedAny = mutedAny || s.muted;
            ++count;
        }
    }
    if (count == 0) return false;

    outVolume = (float)(sum / (double)count);
    outMuted = mutedAny;
    return true;
}

static bool SetForMatchingSessions(DWORD pid, const std::wstring& processName, bool byPid, float* setVolume, bool* setMute)
{
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
                std::wstring name = ToLowerCopy(GetProcessNameByPid(curPid));
                match = (name == target);
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

bool AppVolumeControl::SetVolumeByPid(DWORD pid, float volume01)
{
    if (volume01 < 0.0f) volume01 = 0.0f;
    if (volume01 > 1.0f) volume01 = 1.0f;
    return SetForMatchingSessions(pid, L"", true, &volume01, nullptr);
}

bool AppVolumeControl::SetVolumeByProcessName(const std::wstring& processName, float volume01)
{
    if (volume01 < 0.0f) volume01 = 0.0f;
    if (volume01 > 1.0f) volume01 = 1.0f;
    return SetForMatchingSessions(0, processName, false, &volume01, nullptr);
}

bool AppVolumeControl::SetMuteByPid(DWORD pid, bool mute)
{
    return SetForMatchingSessions(pid, L"", true, nullptr, &mute);
}

bool AppVolumeControl::SetMuteByProcessName(const std::wstring& processName, bool mute)
{
    return SetForMatchingSessions(0, processName, false, nullptr, &mute);
}

