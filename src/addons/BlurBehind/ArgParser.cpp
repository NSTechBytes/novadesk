/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ArgParser.h"
#include "BlurBehind.h"

#include <cctype>
#include <cstdlib>
#include <cstring>
#include <string>

// ============================================================================
// Internal helpers
// ============================================================================

namespace {

/// Convert a C-string to lowercase (ASCII only).
std::string ToLower(const char *s) {
  if (!s)
    return {};
  std::string out(s);
  for (char &c : out)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

/// Convert a std::string to lowercase (ASCII only).
std::string ToLower(const std::string &s) {
  std::string out(s);
  for (char &c : out)
    c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return out;
}

/// Trim leading/trailing ASCII whitespace.
std::string Trim(const std::string &s) {
  const char *ws = " \t\r\n";
  const size_t b = s.find_first_not_of(ws);
  if (b == std::string::npos)
    return {};
  const size_t e = s.find_last_not_of(ws);
  return s.substr(b, e - b + 1);
}

/// True if every character in s is an ASCII decimal digit.
bool IsAllDigits(const std::string &s) {
  if (s.empty())
    return false;
  for (char c : s)
    if (c < '0' || c > '9')
      return false;
  return true;
}

/// True if every character in s is a hex digit (0-9, a-f, A-F).
bool IsAllHex(const std::string &s) {
  if (s.empty())
    return false;
  for (char c : s)
    if (!std::isxdigit(static_cast<unsigned char>(c)))
      return false;
  return true;
}

/// Parse a HWND handle value from a raw string.
uintptr_t ParseHandleString(const char *raw) {
  if (!raw)
    return 0;
  std::string s = Trim(std::string(raw));
  if (s.empty())
    return 0;

  // 0x-prefixed hex
  if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
    return static_cast<uintptr_t>(_strtoui64(s.c_str(), nullptr, 16));

  // Zero-padded hex handles from the host (>= 8 chars, all hex digits)
  if (s.size() >= 8 && IsAllHex(s))
    return static_cast<uintptr_t>(_strtoui64(s.c_str(), nullptr, 16));

  if (IsAllDigits(s))
    return static_cast<uintptr_t>(_strtoui64(s.c_str(), nullptr, 10));

  return 0;
}

/// Parse a stroke colour from a #RRGGBB / 0xRRGGBB / RRGGBB hex string.
BB::Stroke ParseStrokeHex(const std::string &s) {
  std::string hex = s;
  if (!hex.empty() && hex[0] == '#')
    hex = hex.substr(1);
  if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
    hex = hex.substr(2);

  if (!IsAllHex(hex) || (hex.size() != 6 && hex.size() != 8))
    return BB::Stroke::VISIBLE;

  if (hex.size() == 8)
    hex = hex.substr(2); // if AARRGGBB, drop alpha

  unsigned long rgb = std::strtoul(hex.c_str(), nullptr, 16);
  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = (rgb >> 0) & 0xFF;
  COLORREF cr = RGB(r, g, b);
  return static_cast<BB::Stroke>(cr);
}

/// Lookup in any unordered_map<string,T>; returns defVal on miss.
template <typename T>
T Lookup(const std::unordered_map<std::string, T> &map, const std::string &key,
         T defVal) {
  auto it = map.find(key);
  return (it != map.end()) ? it->second : defVal;
}

} // anonymous namespace

// ============================================================================
// Public implementation
// ============================================================================

namespace ArgParser {

HWND ParseHwnd(const NovadeskHostAPI *host, novadesk_context ctx, int idx) {
  uintptr_t parsed = 0;
  if (host->IsString(ctx, idx)) {
    parsed = ParseHandleString(host->GetString(ctx, idx));
  } else if (host->IsNumber(ctx, idx)) {
    double d = host->GetNumber(ctx, idx);
    if (d > 0)
      parsed = static_cast<uintptr_t>(d);
  }
  if (parsed == 0)
    return nullptr;
  HWND hw = reinterpret_cast<HWND>(parsed);
  return IsWindow(hw) ? hw : nullptr;
}

HWND FindHwnd(const NovadeskHostAPI *host, novadesk_context ctx, int *outIdx) {
  const int top = host->GetTop(ctx);
  const int maxScan = (top < 4) ? top : 4;
  for (int i = 0; i < maxScan; ++i) {
    HWND hw = ParseHwnd(host, ctx, i);
    if (hw) {
      if (outIdx)
        *outIdx = i;
      return hw;
    }
  }
  if (outIdx)
    *outIdx = -1;
  return nullptr;
}

BB::Accent ParseAccent(const NovadeskHostAPI *host, novadesk_context ctx,
                       int idx, BB::Accent defVal) {
  if (!host->IsString(ctx, idx))
    return defVal;
  const char *s = host->GetString(ctx, idx);
  if (!s || !*s)
    return defVal;
  return Lookup(BB::Maps::AccentMap(), ToLower(s), defVal);
}

BB::Effect ParseEffect(const NovadeskHostAPI *host, novadesk_context ctx,
                       int idx, BB::Effect defVal) {
  if (!host->IsString(ctx, idx))
    return defVal;
  const char *s = host->GetString(ctx, idx);
  if (!s || !*s)
    return defVal;
  return Lookup(BB::Maps::EffectMap(), ToLower(s), defVal);
}

BB::Corner ParseCorner(const NovadeskHostAPI *host, novadesk_context ctx,
                       int idx, BB::Corner defVal) {
  if (!host->IsString(ctx, idx))
    return defVal;
  const char *s = host->GetString(ctx, idx);
  if (!s || !*s)
    return defVal;
  return Lookup(BB::Maps::CornerMap(), ToLower(s), defVal);
}

BB::Stroke ParseStroke(const NovadeskHostAPI *host, novadesk_context ctx,
                       int idx, BB::Stroke defVal) {
  if (!host->IsString(ctx, idx))
    return defVal;
  const char *s = host->GetString(ctx, idx);
  if (!s || !*s)
    return defVal;

  std::string lo = ToLower(Trim(s));
  if (lo == "hidden" || lo == "none" || lo == "0")
    return BB::Stroke::HIDDEN;
  if (lo == "visible" || lo == "default")
    return BB::Stroke::VISIBLE;

  // Hex colour
  BB::Stroke hex = ParseStrokeHex(lo);
  return hex;
}

BB::Config ParseConfig(const NovadeskHostAPI *host, novadesk_context ctx,
                       int idx) {
  BB::Config cfg;

  // Helper: read a string property from the object and return it (or "").
  auto getStr = [&](const char *name) -> std::string {
    std::string result;
    if (host->GetProperty(ctx, idx, name)) {
      if (host->IsString(ctx, -1)) {
        const char *v = host->GetString(ctx, -1);
        if (v)
          result = v;
      }
      host->Pop(ctx);
    } else {
      host->Pop(ctx);
    }
    return result;
  };

  auto getBool = [&](const char *name) -> int { // -1=absent, 0=false, 1=true
    int result = -1;
    if (host->GetProperty(ctx, idx, name)) {
      if (host->IsBool(ctx, -1))
        result = host->GetBool(ctx, -1) ? 1 : 0;
      else if (host->IsNumber(ctx, -1))
        result = (host->GetNumber(ctx, -1) != 0.0) ? 1 : 0;
      host->Pop(ctx);
    } else {
      host->Pop(ctx);
    }
    return result;
  };

  // hwnd — optional inside object; may be passed separately
  {
    if (host->GetProperty(ctx, idx, "hwnd")) {
      uintptr_t parsed = 0;
      if (host->IsNumber(ctx, -1)) {
        double d = host->GetNumber(ctx, -1);
        if (d > 0)
          parsed = static_cast<uintptr_t>(d);
      } else if (host->IsString(ctx, -1)) {
        parsed = ParseHandleString(host->GetString(ctx, -1));
      }
      if (parsed) {
        HWND hw = reinterpret_cast<HWND>(parsed);
        if (IsWindow(hw))
          cfg.hwnd = hw;
      }
      host->Pop(ctx);
    } else {
      host->Pop(ctx);
    }
  }

  // type / accent
  {
    std::string s = getStr("type");
    if (s.empty())
      s = getStr("accent");
    if (!s.empty()) {
      std::string lo = ToLower(s);
      cfg.accent = Lookup(BB::Maps::AccentMap(), lo, cfg.accent);
    }
  }

  // effect
  {
    std::string s = getStr("effect");
    if (!s.empty())
      cfg.effect = Lookup(BB::Maps::EffectMap(), ToLower(s), cfg.effect);
  }

  // corner
  {
    std::string s = getStr("corner");
    if (!s.empty())
      cfg.corner = Lookup(BB::Maps::CornerMap(), ToLower(s), cfg.corner);
  }

  // stroke / border
  {
    std::string s = getStr("stroke");
    if (s.empty())
      s = getStr("border");
    if (!s.empty()) {
      std::string lo = ToLower(Trim(s));
      if (lo == "hidden" || lo == "none")
        cfg.stroke = BB::Stroke::HIDDEN;
      else if (lo == "visible")
        cfg.stroke = BB::Stroke::VISIBLE;
      else
        cfg.stroke = ParseStrokeHex(lo);
    }
  }

  // disabled
  {
    int b = getBool("disabled");
    if (b >= 0)
      cfg.disabled = (b == 1);
  }

  return cfg;
}

} // namespace ArgParser
