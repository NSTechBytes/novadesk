/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Logging.h"
#include <cstdio>
#include <cstdarg>
#include <fstream>
#include <ctime>
#include <chrono>

// Static member initialization
bool Logging::s_ConsoleEnabled = true;
bool Logging::s_FileEnabled = false;
std::wstring Logging::s_LogFilePath = L"";
LogLevel Logging::s_MinLevel = LogLevel::Info;

void Logging::Log(LogLevel level, const wchar_t* format, ...)
{
    // Check if this log level should be displayed
    if (level < s_MinLevel) return;
    if (!s_ConsoleEnabled && !s_FileEnabled) return;

    va_list args;
    va_start(args, format);
    wchar_t buffer[1024];
    vswprintf_s(buffer, format, args);
    va_end(args);

    const wchar_t* levelStr = L"";
    switch (level)
    {
    case LogLevel::Info:  levelStr = L"[LOG]"; break;
    case LogLevel::Error: levelStr = L"[ERROR]"; break;
    case LogLevel::Debug: levelStr = L"[DEBUG]"; break;
    }

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    tm timeInfo;
    localtime_s(&timeInfo, &now_c);
    
    wchar_t timestamp[32];
    swprintf_s(timestamp, L"[%04d-%02d-%02d %02d:%02d:%02d.%03lld]",
        timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
        timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, ms.count());

    wchar_t output[1200];
    swprintf_s(output, L"%s [Novadesk] %s %s\n", timestamp, levelStr, buffer);

    // Output to console (debug output)
    if (s_ConsoleEnabled)
    {
        OutputDebugStringW(output);
    }

    // Output to file
    if (s_FileEnabled && !s_LogFilePath.empty())
    {
        std::wofstream logFile(s_LogFilePath, std::ios::app);
        if (logFile.is_open())
        {
            logFile << output;
            logFile.close();
        }
    }
}

void Logging::SetConsoleLogging(bool enable)
{
    s_ConsoleEnabled = enable;
}

void Logging::SetFileLogging(const std::wstring& filePath)
{
    s_LogFilePath = filePath;
    s_FileEnabled = !filePath.empty();
}

void Logging::SetLogLevel(LogLevel minLevel)
{
    s_MinLevel = minLevel;
}
