/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

/**
 * @file framework.h
 * @brief Precompiled header for the Novadesk application.
 *
 * Includes standard Windows headers and C runtime libraries.
 */

#pragma once

#include "targetver.h"

/// Exclude rarely-used Windows headers for faster compilation.
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

// C Runtime Headers
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
