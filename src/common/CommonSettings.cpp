#include "CommonSettings.h"

#include "AppPaths.h"
#include "NovadeskPaths.h"
#include "../third_party/json/json.hpp"

#include <filesystem>
#include <fstream>
#include <set>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {
const std::set<std::string> kSupported = {
    "ar", "bg", "zh-Hans", "zh-Hant", "hr", "cs", "da", "nl", "en",
    "et", "fi", "fr", "de", "el", "he", "hu", "id", "it", "ja",
    "ko", "lv", "lt", "ms", "nb", "pl", "pt-BR", "pt-PT", "ro",
    "ru", "sr-Cyrl", "sr-Latn", "sk", "sl", "es", "sv", "th", "tr",
    "uk", "vi"};

bool IsPortable() {
  const fs::path exeDir = AppPaths::GetExeDir();
  if (exeDir.empty()) return false;
  std::error_code ec;
  return fs::exists(exeDir / L"common_settings.json", ec) ||
         fs::exists(exeDir / L"manage_novadesk_settings.json", ec);
}

json Load() {
  std::ifstream input(fs::path(CommonSettings::GetPath()), std::ios::binary);
  if (!input.is_open()) return json::object();
  try {
    json value;
    input >> value;
    return value.is_object() ? value : json::object();
  } catch (...) {
    return json::object();
  }
}
} // namespace

namespace CommonSettings {
std::wstring GetDirectory() {
  if (IsPortable()) return AppPaths::GetExeDir().wstring();
  const fs::path appData = NovadeskPaths::GetAppDataDir();
  return appData.empty() ? AppPaths::GetExeDir().wstring() : appData.wstring();
}

std::wstring GetPath() { return (fs::path(GetDirectory()) / L"common_settings.json").wstring(); }

std::string GetUiLanguage() {
  const json value = Load();
  if (value.contains("uiLanguage") && value["uiLanguage"].is_string()) {
    const std::string language = value["uiLanguage"].get<std::string>();
    if (kSupported.count(language)) return language;
  }
  return "en";
}

bool SetUiLanguage(const std::string &language) {
  if (!kSupported.count(language)) return false;
  json value = Load();
  value["uiLanguage"] = language;
  const fs::path target(GetPath());
  std::error_code ec;
  fs::create_directories(target.parent_path(), ec);
  const fs::path temporary = target.wstring() + L".tmp";
  {
    std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) return false;
    output << value.dump(2) << '\n';
    if (!output.good()) return false;
  }
  fs::rename(temporary, target, ec);
  if (!ec) return true;
  fs::remove(target, ec);
  ec.clear();
  fs::rename(temporary, target, ec);
  return !ec;
}
} // namespace CommonSettings
