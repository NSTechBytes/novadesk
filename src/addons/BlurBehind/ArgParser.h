/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include "BlurBehind.h"
#include <NovadeskAPI/novadesk_addon.h>

// ============================================================================
// ArgParser — parse JS arguments from the Novadesk host context
// ============================================================================

namespace ArgParser {

/// Parse a window handle from argument at index idx.
/// Accepts: hex string (0x… or zero-padded), decimal string, or JS number.
HWND ParseHwnd(const NovadeskHostAPI *host, novadesk_context ctx, int idx);

/// Scan the first 4 arguments for a valid HWND; returns the argument index
/// via outIdx (or -1 if not found).
HWND FindHwnd(const NovadeskHostAPI *host, novadesk_context ctx, int *outIdx);

/// Parse Accent enum from a string argument at idx.
BB::Accent ParseAccent(const NovadeskHostAPI *host, novadesk_context ctx, int idx,
                       BB::Accent defVal = BB::Accent::BLUR);

/// Parse Effect enum from a string argument at idx.
BB::Effect ParseEffect(const NovadeskHostAPI *host, novadesk_context ctx, int idx,
                       BB::Effect defVal = BB::Effect::DEFAULT);

/// Parse Corner enum from a string argument at idx.
BB::Corner ParseCorner(const NovadeskHostAPI *host, novadesk_context ctx, int idx,
                       BB::Corner defVal = BB::Corner::DEFAULT);

/// Parse Stroke from "hidden" / "visible" token or 0xRRGGBB hex at idx.
BB::Stroke ParseStroke(const NovadeskHostAPI *host, novadesk_context ctx, int idx,
                       BB::Stroke defVal = BB::Stroke::VISIBLE);

/// Parse a full BB::Config from a JS object at argument index idx.
/// Reads properties: hwnd, type/accent, effect, corner, stroke, disabled.
BB::Config ParseConfig(const NovadeskHostAPI *host, novadesk_context ctx, int idx);

} // namespace ArgParser
