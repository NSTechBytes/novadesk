/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "Logging.h"
#include <cstdio>
#include <cstdarg>
#include <filesystem>
#include <fstream>
#include <cstring>
#include <ctime>
#include <chrono>
#include <vector>
#include "PathUtils.h"

static WORD GetConsoleColorForLevel(LogLevel level)
{
    switch (level)
    {
    case LogLevel::Debug: return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE; // White
    case LogLevel::Info:  return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY; // Cyan
    case LogLevel::Warn:  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY; // Yellow
    case LogLevel::Error: return FOREGROUND_RED | FOREGROUND_INTENSITY; // Bright Red
    default:              return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }
}

// Static member initialization
bool Logging::s_ConsoleEnabled = true;
bool Logging::s_FileEnabled = false;
std::wstring Logging::s_LogFilePath = L"";
LogLevel Logging::s_MinLevel = LogLevel::Info;

/*
** Log a message to the debug output and/or file.
** Supports formatted output similar to printf.
** Messages are prefixed with [INFO], [ERROR], or [DEBUG] based on level.
*/

void Logging::Log(LogLevel level, const wchar_t* format, ...)
{
    // Check if this log level should be displayed
    if (level < s_MinLevel) return;
    if (!s_ConsoleEnabled && !s_FileEnabled) return;

    va_list args;
    va_start(args, format);

    va_list argsCopy;
    va_copy(argsCopy, args);
    int len = vswprintf(nullptr, 0, format, argsCopy);
    va_end(argsCopy);
    if (len < 0) {
        va_end(args);
        return;
    }

    std::vector<wchar_t> buffer(len + 1);
    vswprintf(buffer.data(), buffer.size(), format, args);
    va_end(args);

    const wchar_t* levelStr = L"";
    switch (level)
    {
    case LogLevel::Info:  levelStr = L"[LOG]"; break;
    case LogLevel::Warn:  levelStr = L"[WARN]"; break;
    case LogLevel::Error: levelStr = L"[ERROR]"; break;
    case LogLevel::Debug: levelStr = L"[DEBUG]"; break;
    }

    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    tm timeInfo;
#if defined(_MSC_VER)
    localtime_s(&timeInfo, &now_c);
#elif defined(__STDC_LIB_EXT1__)
    localtime_s(&now_c, &timeInfo);
#else
    tm* tmp = localtime(&now_c);
    if (tmp) {
        timeInfo = *tmp;
    } else {
        memset(&timeInfo, 0, sizeof(timeInfo));
    }
#endif

    wchar_t timestamp[64];
    swprintf(timestamp, 64, L"[%04d-%02d-%02d %02d:%02d:%02d.%03lld]",
        timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
        timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec, ms.count());

    // Prepare final output string dynamically
    // format: timestamp + " [" + productName + "] " + levelStr + " " + buffer + "\n"
    std::wstring productName = PathUtils::GetProductName();
    std::wstring line = std::wstring(timestamp) + L" [" + productName + L"] " + levelStr + L" " + buffer.data() + L"\n";

    // Output to console (debug output)
    if (s_ConsoleEnabled)
    {
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        CONSOLE_SCREEN_BUFFER_INFO csbi = {};
        bool canColor = (hOut != INVALID_HANDLE_VALUE) && GetConsoleScreenBufferInfo(hOut, &csbi);
        if (canColor)
        {
            SetConsoleTextAttribute(hOut, GetConsoleColorForLevel(level));
        }

        OutputDebugStringW(line.c_str());
        wprintf(L"%ls", line.c_str());
        fflush(stdout);

        if (canColor)
        {
            SetConsoleTextAttribute(hOut, csbi.wAttributes);
        }
    }

    // Output to file
    if (s_FileEnabled && !s_LogFilePath.empty())
    {
        std::wofstream logFile{ std::filesystem::path(s_LogFilePath), std::ios::app };
        if (logFile.is_open())
        {
            logFile << line;
            logFile.close();
        }
    }
}

/*
** Enable or disable console (debug output) logging.
*/

void Logging::SetConsoleLogging(bool enable)
{
    s_ConsoleEnabled = enable;
}

/*
** Enable or disable file logging with an option to clear the file.
** If filePath is empty, file logging is disabled.
*/
void Logging::SetFileLogging(const std::wstring& filePath, bool clearFile)
{
    s_LogFilePath = filePath;
    s_FileEnabled = !filePath.empty();

    if (s_FileEnabled && clearFile)
    {
        // Truncate the file
        std::wofstream logFile{ std::filesystem::path(s_LogFilePath), std::ios::trunc };
        if (logFile.is_open())
        {
            logFile.close();
        }
    }
}

/*
** Set the minimum log level for output.
** Messages below this level will be ignored.
*/
void Logging::SetLogLevel(LogLevel minLevel)
{
    s_MinLevel = minLevel;
}

