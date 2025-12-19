/* Copyright (C) 2026 Novadesk Project 
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __NOVADESK_LOGGING_H__
#define __NOVADESK_LOGGING_H__

#include <windows.h>
#include <string>

enum class LogLevel
{
    Info,
    Error,
    Debug
};

class Logging
{
public:
    /*
    ** Log a message to the debug output.
    ** Supports formatted output similar to printf.
    ** Messages are prefixed with [INFO], [ERROR], or [DEBUG] based on level.
    */
    static void Log(LogLevel level, const wchar_t* format, ...);
};

#endif
