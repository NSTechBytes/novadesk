/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <cstdint>

namespace novadesk::shared {

/**
 * @brief Configuration options for zip compression operations.
 */
struct ZipCompressOptions {
  int compressionLevel = 6; ///< Compression level from 0 (store) to 9 (best).
  bool overwrite = true;    ///< Whether to overwrite an existing destination file.
};

/**
 * @brief Configuration options for zip extraction operations.
 */
struct ZipExtractOptions {
  bool overwrite = true; ///< Whether to overwrite existing files during extraction.
  std::vector<std::string> selectedEntries; ///< Specific entries to extract; empty means extract all.
};

/**
 * @brief Metadata information for a single entry within a zip archive.
 */
struct ZipEntryInfo {
  std::string name;            ///< The entry name/path within the archive.
  uint64_t size = 0;           ///< Uncompressed size of the entry in bytes.
  uint64_t compressedSize = 0; ///< Compressed size of the entry in bytes.
  uint32_t crc = 0;            ///< CRC32 checksum of the entry.
  bool isDirectory = false;    ///< True if this entry represents a directory.
};

/**
 * @brief Compresses a source directory or single file into a .zip archive.
 *
 * @param sourcePath Path to the file or directory to compress (absolute or relative).
 * @param destZipPath Output file path for the resulting archive.
 * @param opts Compression configuration (level, overwrite flag).
 * @param errorOut Receives an error message if compression fails.
 *
 * @return True if compression completed successfully; false otherwise.
 *
 * @note Recursively traverses subdirectories and preserves relative folder hierarchy.
 */
bool CompressToZip(const std::filesystem::path &sourcePath,
                   const std::filesystem::path &destZipPath,
                   const ZipCompressOptions &opts, std::string &errorOut);

/**
 * @brief Extracts all or selected entries from a .zip archive to a directory.
 *
 * @param zipPath Path to the .zip archive to extract.
 * @param destDirPath Destination directory where files will be extracted.
 * @param opts Extraction configuration (overwrite flag, selected entries).
 * @param errorOut Receives an error message if extraction fails.
 *
 * @return True if extraction completed successfully; false otherwise.
 *
 * @warning Protects against Zip Slip vulnerabilities by rejecting entries with ".." paths.
 */
bool ExtractFromZip(const std::filesystem::path &zipPath,
                    const std::filesystem::path &destDirPath,
                    const ZipExtractOptions &opts, std::string &errorOut);

/**
 * @brief Lists all entries in a zip archive with their metadata.
 *
 * @param zipPath Path to the .zip archive to inspect.
 * @param outEntries Vector that receives the list of entry information.
 * @param errorOut Receives an error message if the operation fails.
 *
 * @return True if the archive was read successfully; false otherwise.
 */
bool ListZipEntries(const std::filesystem::path &zipPath,
                    std::vector<ZipEntryInfo> &outEntries,
                    std::string &errorOut);

/**
 * @brief Reads the content of a single entry from a zip archive into a string.
 *
 * @param zipPath Path to the .zip archive.
 * @param entryName Name of the entry to read (case-sensitive).
 * @param outContent String that receives the decompressed content.
 * @param errorOut Receives an error message if the operation fails.
 *
 * @return True if the entry was found and read successfully; false otherwise.
 */
bool ReadZipEntryContent(const std::filesystem::path &zipPath,
                         const std::string &entryName, std::string &outContent,
                         std::string &errorOut);
} // namespace novadesk::shared
