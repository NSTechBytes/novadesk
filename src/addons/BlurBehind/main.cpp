/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include <NovadeskAPI/novadesk_addon.h>

#include "ApiState.h"
#include "ArgParser.h"
#include "BlurBehind.h"
#include "WinVersion.h"
#include "WindowStyler.h"

#include <Windows.h>
#include <unordered_map>

// ============================================================================
// Globals
// ============================================================================

static const NovadeskHostAPI *g_Host = nullptr;

// Per-HWND saved config used by JsToggle to restore the previous state.
static std::unordered_map<HWND, BB::Config> g_toggleCache;

// ============================================================================
// Internal helpers
// ============================================================================

/// Apply a full BB::Config to a window, performing version-gated fallbacks.
static void ApplyConfig(const BB::Config &cfg) {
  HWND hwnd = cfg.hwnd;
  if (!hwnd || !IsWindow(hwnd)) return;

  BB::Accent accent = cfg.accent;

  // Acrylic requires Win11; fall back to Blur on Win10.
  if (accent == BB::Accent::ACRYLIC && !WinVersion::IsWin11()) {
    accent = BB::Accent::BLUR;
  }

  // Reset accent, then apply desired state.
  WindowStyler::SetAccent(hwnd, BB::Accent::DEFAULT, BB::Effect::DEFAULT);
  if (accent != BB::Accent::DEFAULT) {
    WindowStyler::SetAccent(hwnd, accent, cfg.effect);
  }

  // Corner (Win11+)
  WindowStyler::SetCorner(hwnd, cfg.corner);

  // Stroke (Win11+)
  WindowStyler::SetStroke(hwnd, cfg.stroke);
}

// ============================================================================
// JavaScript Binding Functions
// ============================================================================

// ----------------------------------------------------------------------------
// apply(hwnd, type?, corner?)
// apply(hwnd, configObject)
// ----------------------------------------------------------------------------
static int JsApply(novadesk_context ctx) {
  const int top = g_Host->GetTop(ctx);

  // If first arg is an object → config-object form.
  if (top >= 1 && g_Host->IsObject(ctx, 0)) {
    BB::Config cfg = ArgParser::ParseConfig(g_Host, ctx, 0);
    // hwnd might be the second arg if not inside the object.
    if (!cfg.hwnd && top >= 2) cfg.hwnd = ArgParser::ParseHwnd(g_Host, ctx, 1);
    if (!cfg.hwnd) {
      g_Host->ThrowError(ctx, "apply(config): hwnd is missing or invalid");
      return 0;
    }
    if (cfg.disabled) {
      WindowStyler::Disable(cfg.hwnd);
    } else {
      ApplyConfig(cfg);
    }
    g_Host->PushBool(ctx, 1);
    return 1;
  }

  // Positional form: apply(hwnd, type?, corner?)
  int hwndIdx = -1;
  HWND hwnd = ArgParser::FindHwnd(g_Host, ctx, &hwndIdx);
  if (!hwnd) {
    g_Host->ThrowError(ctx, "apply(hwnd, type?, corner?): invalid hwnd");
    return 0;
  }

  int base = (hwndIdx >= 0) ? hwndIdx : 0;

  // Second positional arg might itself be a config object.
  if (top > base + 1 && g_Host->IsObject(ctx, base + 1)) {
    BB::Config cfg = ArgParser::ParseConfig(g_Host, ctx, base + 1);
    cfg.hwnd = hwnd;
    ApplyConfig(cfg);
    g_Host->PushBool(ctx, 1);
    return 1;
  }

  BB::Config cfg;
  cfg.hwnd   = hwnd;
  cfg.accent = ArgParser::ParseAccent(g_Host, ctx, base + 1, BB::Accent::BLUR);
  cfg.corner = ArgParser::ParseCorner(g_Host, ctx, base + 2, BB::Corner::DEFAULT);

  ApplyConfig(cfg);
  g_Host->PushBool(ctx, 1);
  return 1;
}

// ----------------------------------------------------------------------------
// disable(hwnd)
// ----------------------------------------------------------------------------
static int JsDisable(novadesk_context ctx) {
  int hwndIdx = -1;
  HWND hwnd = ArgParser::FindHwnd(g_Host, ctx, &hwndIdx);
  if (!hwnd) {
    g_Host->ThrowError(ctx, "disable(hwnd): invalid hwnd");
    return 0;
  }
  WindowStyler::Disable(hwnd);
  WindowStyler::SetCorner(hwnd, BB::Corner::DEFAULT);
  g_toggleCache.erase(hwnd);
  g_Host->PushBool(ctx, 1);
  return 1;
}

// ----------------------------------------------------------------------------
// setCorner(hwnd, corner)
// ----------------------------------------------------------------------------
static int JsSetCorner(novadesk_context ctx) {
  int hwndIdx = -1;
  HWND hwnd = ArgParser::FindHwnd(g_Host, ctx, &hwndIdx);
  if (!hwnd) {
    g_Host->ThrowError(ctx, "setCorner(hwnd, corner): invalid hwnd");
    return 0;
  }
  int base = (hwndIdx >= 0) ? hwndIdx : 0;
  BB::Corner corner = ArgParser::ParseCorner(g_Host, ctx, base + 1, BB::Corner::ROUND);
  WindowStyler::SetCorner(hwnd, corner);
  g_Host->PushBool(ctx, 1);
  return 1;
}

// ----------------------------------------------------------------------------
// setEffect(hwnd, effect)
// ----------------------------------------------------------------------------
static int JsSetEffect(novadesk_context ctx) {
  int hwndIdx = -1;
  HWND hwnd = ArgParser::FindHwnd(g_Host, ctx, &hwndIdx);
  if (!hwnd) {
    g_Host->ThrowError(ctx, "setEffect(hwnd, effect): invalid hwnd");
    return 0;
  }
  int base = (hwndIdx >= 0) ? hwndIdx : 0;
  BB::Effect effect = ArgParser::ParseEffect(g_Host, ctx, base + 1, BB::Effect::DEFAULT);
  WindowStyler::SetAccent(hwnd, BB::Accent::BLUR, effect);
  g_Host->PushBool(ctx, 1);
  return 1;
}

// ----------------------------------------------------------------------------
// setStroke(hwnd, color|"hidden"|"visible")
// ----------------------------------------------------------------------------
static int JsSetStroke(novadesk_context ctx) {
  int hwndIdx = -1;
  HWND hwnd = ArgParser::FindHwnd(g_Host, ctx, &hwndIdx);
  if (!hwnd) {
    g_Host->ThrowError(ctx, "setStroke(hwnd, color): invalid hwnd");
    return 0;
  }
  int base = (hwndIdx >= 0) ? hwndIdx : 0;
  BB::Stroke stroke = ArgParser::ParseStroke(g_Host, ctx, base + 1, BB::Stroke::VISIBLE);
  WindowStyler::SetStroke(hwnd, stroke);
  g_Host->PushBool(ctx, 1);
  return 1;
}

// ----------------------------------------------------------------------------
// toggle(hwnd)
// Toggles blur on/off. Stores/restores the previous config per HWND.
// ----------------------------------------------------------------------------
static int JsToggle(novadesk_context ctx) {
  int hwndIdx = -1;
  HWND hwnd = ArgParser::FindHwnd(g_Host, ctx, &hwndIdx);
  if (!hwnd) {
    g_Host->ThrowError(ctx, "toggle(hwnd): invalid hwnd");
    return 0;
  }

  auto it = g_toggleCache.find(hwnd);
  if (it != g_toggleCache.end()) {
    // Blur is currently disabled — restore saved config.
    ApplyConfig(it->second);
    g_toggleCache.erase(it);
    g_Host->PushBool(ctx, 1); // true = blur ON
  } else {
    // Blur is currently on — disable and save a default config so we can restore.
    BB::Config saved;
    saved.hwnd   = hwnd;
    saved.accent = BB::Accent::BLUR;
    saved.corner = BB::Corner::DEFAULT;
    g_toggleCache[hwnd] = saved;
    WindowStyler::Disable(hwnd);
    g_Host->PushBool(ctx, 0); // false = blur OFF
  }
  return 1;
}

// ----------------------------------------------------------------------------
// isSupported(feature) → bool
// feature: "blur" | "acrylic" | "corner" | "stroke"
// ----------------------------------------------------------------------------
static int JsIsSupported(novadesk_context ctx) {
  if (g_Host->GetTop(ctx) < 1 || !g_Host->IsString(ctx, 0)) {
    g_Host->PushBool(ctx, 0);
    return 1;
  }
  const char *raw = g_Host->GetString(ctx, 0);
  if (!raw) { g_Host->PushBool(ctx, 0); return 1; }

  std::string s(raw);
  for (char &c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

  bool result = false;
  if (s == "blur" || s == "blurbehind")     result = WinVersion::IsWin10();
  else if (s == "acrylic")                  result = WinVersion::IsWin11();
  else if (s == "corner")                   result = WinVersion::IsWin11();
  else if (s == "stroke" || s == "border")  result = WinVersion::IsWin11();

  g_Host->PushBool(ctx, result ? 1 : 0);
  return 1;
}

// ============================================================================
// Addon Initialization
// ============================================================================

NOVADESK_ADDON_INIT(ctx, hMsgWnd, host) {
  (void)hMsgWnd;
  g_Host = host;

  WinVersion::Initialize();
  ApiState::Initialize();

  novadesk::Addon addon(ctx, host);
  addon.RegisterString("name",    "BlurBehind");
  addon.RegisterString("version", "3.0.0");

  // ── Existing API ─────────────────────────────────────────────────────────
  addon.RegisterFunction("apply",     JsApply,     4);
  addon.RegisterFunction("disable",   JsDisable,   1);
  addon.RegisterFunction("setCorner", JsSetCorner, 2);

  // ── Extended API ─────────────────────────────────────────────────────────
  addon.RegisterFunction("setEffect",   JsSetEffect,   2);
  addon.RegisterFunction("setStroke",   JsSetStroke,   2);
  addon.RegisterFunction("toggle",      JsToggle,      1);
  addon.RegisterFunction("isSupported", JsIsSupported, 1);

  // ── Capability flags object ───────────────────────────────────────────────
  addon.RegisterObject("supports", [](novadesk::Addon &s) {
    s.RegisterBool("blur",    WinVersion::IsWin10());
    s.RegisterBool("acrylic", WinVersion::IsWin11());
    s.RegisterBool("corner",  WinVersion::IsWin11());
    s.RegisterBool("stroke",  WinVersion::IsWin11());
  });
}

NOVADESK_ADDON_UNLOAD() {
  g_toggleCache.clear();
  ApiState::Finalize();
}
