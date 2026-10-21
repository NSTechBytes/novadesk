#pragma once

#include <string>
#include <vector>

namespace Localization {
struct Language {
  const char *tag;
  const wchar_t *nativeName;
  bool rtl;
};

void Initialize();
const std::wstring &Text(const char *key);
const std::wstring &TextOr(const char *key, const wchar_t *fallback);
const std::string &LanguageTag();
bool IsRtl();
const std::vector<Language> &SupportedLanguages();
} // namespace Localization
