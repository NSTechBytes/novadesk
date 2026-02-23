#include "Utils.h"

#include <fstream>
#include <sstream>

namespace novadesk::shared::utils {
std::string ReadTextFile(const std::filesystem::path& path) {
    std::ifstream in(path, std::ios::in | std::ios::binary);
    if (!in) {
        return {};
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    return buffer.str();
}
}  // namespace novadesk::shared::utils
