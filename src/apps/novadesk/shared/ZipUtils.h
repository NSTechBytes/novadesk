#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace novadesk::shared
{
    struct ZipCompressOptions
    {
        int compressionLevel = 6; // 0 (store) to 9 (best)
        bool overwrite = true;
    };

    struct ZipExtractOptions
    {
        bool overwrite = true;
        std::vector<std::string> selectedEntries; // Empty means extract all
    };

    struct ZipEntryInfo
    {
        std::string name;
        uint64_t size = 0;
        uint64_t compressedSize = 0;
        uint32_t crc = 0;
        bool isDirectory = false;
    };

    bool CompressToZip(const std::filesystem::path &sourcePath,
                       const std::filesystem::path &destZipPath,
                       const ZipCompressOptions &opts,
                       std::string &errorOut);

    bool ExtractFromZip(const std::filesystem::path &zipPath,
                        const std::filesystem::path &destDirPath,
                        const ZipExtractOptions &opts,
                        std::string &errorOut);

    bool ListZipEntries(const std::filesystem::path &zipPath,
                        std::vector<ZipEntryInfo> &outEntries,
                        std::string &errorOut);

    bool ReadZipEntryContent(const std::filesystem::path &zipPath,
                             const std::string &entryName,
                             std::string &outContent,
                             std::string &errorOut);
}
