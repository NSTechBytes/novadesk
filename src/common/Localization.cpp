#include "Localization.h"

#include "AppPaths.h"
#include "CommonSettings.h"
#include "../third_party/json/json.hpp"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <unordered_map>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace {
std::string g_language = "en";
bool g_rtl = false;
std::unordered_map<std::string, std::wstring> g_strings;
const std::wstring kMissing;
const std::vector<Localization::Language> kLanguages = {
    {"ar", L"\u0627\u0644\u0639\u0631\u0628\u064a\u0629", true}, {"bg", L"\u0411\u044a\u043b\u0433\u0430\u0440\u0441\u043a\u0438", false}, {"zh-Hans", L"\u7b80\u4f53\u4e2d\u6587", false}, {"zh-Hant", L"\u7e41\u9ad4\u4e2d\u6587", false},
    {"hr", L"Hrvatski", false}, {"cs", L"\u010ce\u0161tina", false}, {"da", L"Dansk", false}, {"nl", L"Nederlands", false}, {"en", L"English", false},
    {"et", L"Eesti", false}, {"fi", L"Suomi", false}, {"fr", L"Fran\u00e7ais", false}, {"de", L"Deutsch", false}, {"el", L"\u0395\u03bb\u03bb\u03b7\u03bd\u03b9\u03ba\u03ac", false},
    {"he", L"\u05e2\u05d1\u05e8\u05d9\u05ea", true}, {"hu", L"Magyar", false}, {"id", L"Bahasa Indonesia", false}, {"it", L"Italiano", false}, {"ja", L"\u65e5\u672c\u8a9e", false},
    {"ko", L"\ud55c\uad6d\uc5b4", false}, {"lv", L"Latvie\u0161u", false}, {"lt", L"Lietuvi\u0173", false}, {"ms", L"Bahasa Melayu", false}, {"nb", L"Norsk bokm\u00e5l", false},
    {"pl", L"Polski", false}, {"pt-BR", L"Portugu\u00eas (Brasil)", false}, {"pt-PT", L"Portugu\u00eas (Portugal)", false}, {"ro", L"Rom\u00e2n\u0103", false},
    {"ru", L"\u0420\u0443\u0441\u0441\u043a\u0438\u0439", false}, {"sr-Cyrl", L"\u0421\u0440\u043f\u0441\u043a\u0438 (\u045b\u0438\u0440\u0438\u043b\u0438\u0446\u0430)", false}, {"sr-Latn", L"Srpski (latinica)", false}, {"sk", L"Sloven\u010dina", false},
    {"sl", L"Sloven\u0161\u010dina", false}, {"es", L"Espa\u00f1ol", false}, {"sv", L"Svenska", false}, {"th", L"\u0e44\u0e17\u0e22", false}, {"tr", L"T\u00fcrk\u00e7e", false},
    {"uk", L"\u0423\u043a\u0440\u0430\u0457\u043d\u0441\u044c\u043a\u0430", false}, {"vi", L"Ti\u1ebfng Vi\u1ec7t", false}};

std::wstring Utf8ToWide(const std::string &value) {
  if (value.empty()) return {};
  const int count = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
  std::wstring output(count, L'\0');
  if (count > 0) MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), output.data(), count);
  return output;
}
void LoadCatalog(const fs::path &path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.is_open()) return;
  try {
    json catalog;
    input >> catalog;
    if (!catalog.is_object()) return;
    for (auto it = catalog.begin(); it != catalog.end(); ++it)
      if (it.value().is_string()) g_strings[it.key()] = Utf8ToWide(it.value().get<std::string>());
  } catch (...) {}
}
} // namespace

namespace Localization {
void Initialize() {
  g_strings.clear();
  g_language = CommonSettings::GetUiLanguage();
  g_rtl = false;
  for (const Language &language : kLanguages)
    if (g_language == language.tag) g_rtl = language.rtl;
  // GDI's process-default RTL layout also changes DirectWrite text placement
  // in custom-rendered windows.  Those windows mirror their own geometry, so
  // applying it here produces a second transform and clipped text.  Native
  // title bars that need mirroring set it on their own window instead.
  SetProcessDefaultLayout(0);
  std::vector<fs::path> localeDirectories = {
      AppPaths::GetExeDir() / L"locales", fs::current_path() / L"locales"};
  fs::path parent = fs::current_path();
  while (parent.has_parent_path()) {
    // Project-root catalogs support launching each application directly from
    // its build directory, while installed applications use exeDir/locales.
    localeDirectories.push_back(parent / L"locales");
    const fs::path next = parent.parent_path();
    if (next == parent) break;
    parent = next;
  }
  for (const auto &locales : localeDirectories) {
    if (!fs::exists(locales / L"en.json")) continue;
    LoadCatalog(locales / L"en.json");
    if (g_language != "en") LoadCatalog(locales / (Utf8ToWide(g_language) + L".json"));
    break;
  }
}
const std::wstring &Text(const char *key) {
  const auto it = g_strings.find(key ? key : "");
  return it == g_strings.end() ? kMissing : it->second;
}
const std::wstring &TextOr(const char *key, const wchar_t *fallback) {
  const std::wstring &translated = Text(key);
  if (!translated.empty()) return translated;
  static thread_local std::wstring fallbackText;
  fallbackText = fallback ? fallback : L"";
  return fallbackText;
}
const std::string &LanguageTag() { return g_language; }
bool IsRtl() { return g_rtl; }
const std::vector<Language> &SupportedLanguages() { return kLanguages; }
} // namespace Localization
