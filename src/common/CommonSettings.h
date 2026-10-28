#pragma once

#include <string>

// Settings shared by Novadesk, its manager, and the NDPKG installer.
namespace CommonSettings {
std::wstring GetDirectory();
std::wstring GetPath();
// Stable identifier for this installation's shared settings location.
std::wstring GetInstallationScopeId();
std::string GetUiLanguage();
bool SetUiLanguage(const std::string &language);

/// Preferred Windows UI theme: "system", "dark", or "light".
std::string GetWindowsTheme();
bool SetWindowsTheme(const std::string &theme);
} // namespace CommonSettings
