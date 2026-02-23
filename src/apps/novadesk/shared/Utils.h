#pragma once

#include <filesystem>
#include <string>

namespace novadesk::shared::utils {
std::string ReadTextFile(const std::filesystem::path& path);
}  // namespace novadesk::shared::utils
