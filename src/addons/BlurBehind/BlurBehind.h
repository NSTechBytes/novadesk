/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <Windows.h>
#include <cstdint>
#include <string>
#include <unordered_map>

// ============================================================================
// Namespace BB — BlurBehind Types & Enumerations
// ============================================================================

namespace BB {

// ----------------------------------------------------------------------------
// Accent — window background composition mode (SetWindowCompositionAttribute)
// ----------------------------------------------------------------------------
enum class Accent : uint8_t {
  DEFAULT = 0, // No accent applied (transparent / disabled).
  BLUR = 3,    // Gaussian blur (works on Win10+).
  ACRYLIC = 4, // Acrylic material — blur + noise (Win11+).
};

// ----------------------------------------------------------------------------
// Effect — additional composition effect flags (ORed into Flags field)
// ----------------------------------------------------------------------------
enum class Effect : uint8_t {
  DEFAULT = 0,    // No extra effect.
  LUMINANCE = 2,  // High-contrast luminance boost (improves text legibility).
  FULLSCREEN = 4, // Allow frosted-glass in full-screen mode.
  BOTH = 6,       // Luminance + fullscreen combined.
};

// ----------------------------------------------------------------------------
// Corner — window corner rounding (DWMWA_WINDOW_CORNER_PREFERENCE, Win11+)
// ----------------------------------------------------------------------------
enum class Corner : uint8_t {
  DEFAULT = 1,            // System default (rounds on Win11, square on Win10).
  ROUND = 2,              // Round corners — 8 px radius.
  ROUND_SMALL = 3,        // Small round corners — 4 px radius.
  ROUND_SMALL_SHADOW = 4, // Small radius with soft shadow.
};

// ----------------------------------------------------------------------------
// Stroke — window border / stroke colour (DWMWA_BORDER_COLOR, Win11+)
// ----------------------------------------------------------------------------
enum class Stroke : uint32_t {
  VISIBLE = 0xFFFFFFFF, // 1 px system border (default).
  HIDDEN = 0xFFFFFFFE,  // No border / invisible stroke.
};

// ============================================================================
// Config — aggregated per-call state
// ============================================================================

struct Config {
  HWND hwnd = nullptr;
  Accent accent = Accent::BLUR;
  Effect effect = Effect::DEFAULT;
  Corner corner = Corner::DEFAULT;
  Stroke stroke = Stroke::VISIBLE;
  bool disabled = false;
};

// ============================================================================
// Lookup Maps — string → enum (lower-case keys)
// ============================================================================

namespace Maps {

inline const std::unordered_map<std::string, Accent> &AccentMap() {
  static const std::unordered_map<std::string, Accent> m = {
      {"0", Accent::DEFAULT},        {"none", Accent::DEFAULT},
      {"disabled", Accent::DEFAULT}, {"default", Accent::DEFAULT},
      {"3", Accent::BLUR},           {"blur", Accent::BLUR},
      {"blurbehind", Accent::BLUR},  {"4", Accent::ACRYLIC},
      {"acrylic", Accent::ACRYLIC},
  };
  return m;
}

inline const std::unordered_map<std::string, Effect> &EffectMap() {
  static const std::unordered_map<std::string, Effect> m = {
      {"0", Effect::DEFAULT},           {"none", Effect::DEFAULT},
      {"default", Effect::DEFAULT},     {"1", Effect::LUMINANCE},
      {"luminance", Effect::LUMINANCE}, {"highcontrast", Effect::LUMINANCE},
      {"2", Effect::FULLSCREEN},        {"fullscreen", Effect::FULLSCREEN},
      {"cover", Effect::FULLSCREEN},    {"3", Effect::BOTH},
      {"both", Effect::BOTH},           {"combined", Effect::BOTH},
  };
  return m;
}

inline const std::unordered_map<std::string, Corner> &CornerMap() {
  static const std::unordered_map<std::string, Corner> m = {
      {"0", Corner::DEFAULT},
      {"none", Corner::DEFAULT},
      {"default", Corner::DEFAULT},
      {"1", Corner::ROUND},
      {"round", Corner::ROUND},
      {"2", Corner::ROUND_SMALL},
      {"roundsmall", Corner::ROUND_SMALL},
      {"small", Corner::ROUND_SMALL},
      {"3", Corner::ROUND_SMALL_SHADOW},
      {"roundsmallshadow", Corner::ROUND_SMALL_SHADOW},
      {"roundws", Corner::ROUND_SMALL_SHADOW},
  };
  return m;
}

} // namespace Maps
} // namespace BB
