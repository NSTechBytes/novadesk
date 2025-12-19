#pragma once
#include <string>

namespace Utils {
    // Convert std::string (UTF-8) to std::wstring (wide char)
    std::wstring ToWString(const std::string& str);
}
