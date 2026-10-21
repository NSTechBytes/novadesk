#pragma once

#include <string>

// Settings shared by Novadesk, its manager, and the NDPKG installer.
namespace CommonSettings {
std::wstring GetDirectory();
std::wstring GetPath();
std::string GetUiLanguage();
bool SetUiLanguage(const std::string &language);
} // namespace CommonSettings
