/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "ZipUtils.h"

#include <fstream>
#include <iostream>
#include <algorithm>
#include <unordered_set>

#include "zip.h"
#include "unzip.h"

namespace novadesk::shared {
namespace fs = std::filesystem;

// ============================================================================
// Internal Helpers
// ============================================================================

namespace {
/**
 * @brief Adds a single file to an open zip archive handle.
 *
 * @param zf Open zip file handle.
 * @param filePath Path to the file on disk.
 * @param zipEntryPath Entry name within the archive.
 * @param level Compression level (0 = store, 1-9 = deflate).
 *
 * @return True if the file was added successfully.
 */
bool AddFileToZipHandle(zipFile zf, const fs::path &filePath,
                        const std::string &zipEntryPath, int level) {
  std::ifstream in(filePath, std::ios::binary);
  if (!in.is_open())
    return false;

  zip_fileinfo zi = {};
  if (zipOpenNewFileInZip64(zf, zipEntryPath.c_str(), &zi, nullptr, 0, nullptr,
                            0, nullptr, level == 0 ? 0 : Z_DEFLATED, level,
                            1) != ZIP_OK) {
    return false;
  }

  // Stream file contents in 64KB chunks to avoid loading entire file into
  // memory
  std::vector<char> buffer(64 * 1024);
  while (in) {
    in.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
    const std::streamsize got = in.gcount();
    if (got <= 0)
      break;
    if (zipWriteInFileInZip(zf, buffer.data(), static_cast<unsigned int>(got)) <
        0) {
      zipCloseFileInZip(zf);
      return false;
    }
  }

  return zipCloseFileInZip(zf) == ZIP_OK;
}

/**
 * @brief Adds a directory entry to an open zip archive handle.
 *
 * @param zf Open zip file handle.
 * @param zipDirPath Directory path within the archive (will get trailing '/').
 *
 * @return True if the directory entry was added successfully.
 */
bool AddDirectoryEntryToZipHandle(zipFile zf, const std::string &zipDirPath) {
  zip_fileinfo zi = {};
  std::string dirEntry = zipDirPath;
  if (!dirEntry.empty() && dirEntry.back() != '/')
    dirEntry += '/';

  if (zipOpenNewFileInZip64(zf, dirEntry.c_str(), &zi, nullptr, 0, nullptr, 0,
                            nullptr, 0, 0, 0) != ZIP_OK) {
    return false;
  }
  return zipCloseFileInZip(zf) == ZIP_OK;
}
} // namespace

// ============================================================================
// Public API Implementation
// ============================================================================

bool CompressToZip(const fs::path &sourcePath, const fs::path &destZipPath,
                   const ZipCompressOptions &opts, std::string &errorOut) {
  std::error_code ec;
  if (!fs::exists(sourcePath, ec) || ec) {
    errorOut = "Source path does not exist.";
    return false;
  }

  if (fs::exists(destZipPath, ec) && !opts.overwrite) {
    errorOut = "Destination zip file already exists and overwrite is false.";
    return false;
  }

  // Ensure parent directory exists before creating the zip
  if (destZipPath.has_parent_path()) {
    fs::create_directories(destZipPath.parent_path(), ec);
  }

  const int level = std::clamp(opts.compressionLevel, 0, 9);
  const std::string destStr = destZipPath.string();

  zipFile zf = zipOpen64(destStr.c_str(), APPEND_STATUS_CREATE);
  if (!zf) {
    errorOut = "Failed to create or open destination zip file.";
    return false;
  }

  bool success = true;

  if (fs::is_directory(sourcePath, ec)) {
    // Recursively iterate directory and add each entry to the archive
    for (const auto &entry : fs::recursive_directory_iterator(
             sourcePath, fs::directory_options::skip_permission_denied, ec)) {
      if (ec)
        continue;

      fs::path relPath = fs::relative(entry.path(), sourcePath, ec);
      if (ec)
        continue;

      std::string rel = relPath.generic_string();
      if (rel.empty())
        continue;

      if (entry.is_directory()) {
        if (!AddDirectoryEntryToZipHandle(zf, rel)) {
          success = false;
          errorOut = "Failed to add directory entry: " + rel;
          break;
        }
      } else if (entry.is_regular_file()) {
        if (!AddFileToZipHandle(zf, entry.path(), rel, level)) {
          success = false;
          errorOut = "Failed to add file: " + rel;
          break;
        }
      }
    }
  } else if (fs::is_regular_file(sourcePath, ec)) {
    const std::string filename = sourcePath.filename().generic_string();
    if (!AddFileToZipHandle(zf, sourcePath, filename, level)) {
      success = false;
      errorOut = "Failed to compress single file: " + filename;
    }
  } else {
    errorOut = "Source path is neither a regular file nor a directory.";
    success = false;
  }

  if (zipClose(zf, nullptr) != ZIP_OK) {
    if (success) {
      errorOut = "Failed to finalize and close zip file.";
      success = false;
    }
  }

  return success;
}

bool ExtractFromZip(const fs::path &zipPath, const fs::path &destDirPath,
                    const ZipExtractOptions &opts, std::string &errorOut) {
  std::error_code ec;
  if (!fs::exists(zipPath, ec) || ec) {
    errorOut = "Zip archive does not exist.";
    return false;
  }

  fs::create_directories(destDirPath, ec);

  const std::string zipStr = zipPath.string();
  unzFile uf = unzOpen64(zipStr.c_str());
  if (!uf) {
    errorOut = "Failed to open zip archive.";
    return false;
  }

  auto closeUnz = [&]() { unzClose(uf); };

  int rc = unzGoToFirstFile(uf);
  if (rc == UNZ_END_OF_LIST_OF_FILE) {
    closeUnz();
    return true;
  }
  if (rc != UNZ_OK) {
    closeUnz();
    errorOut = "Failed to read entries from zip archive.";
    return false;
  }

  // Build filter set for selective extraction
  std::unordered_set<std::string> filterSet;
  for (const auto &item : opts.selectedEntries) {
    std::string norm = item;
    std::replace(norm.begin(), norm.end(), '\\', '/');
    filterSet.insert(norm);
  }

  const bool hasFilter = !filterSet.empty();

  do {
    unz_file_info64 fileInfo = {};
    char rawName[1024] = {};
    rc = unzGetCurrentFileInfo64(uf, &fileInfo, rawName, sizeof(rawName),
                                 nullptr, 0, nullptr, 0);
    if (rc != UNZ_OK) {
      closeUnz();
      errorOut = "Failed to read entry info from zip.";
      return false;
    }

    std::string entryName(rawName);
    std::replace(entryName.begin(), entryName.end(), '\\', '/');

    // Skip entries not in the filter set
    if (hasFilter && filterSet.find(entryName) == filterSet.end()) {
      continue;
    }

    // WARNING: Reject entries with ".." to prevent Zip Slip path traversal
    // attacks
    if (entryName.find("..") != std::string::npos) {
      continue;
    }

    fs::path targetPath = destDirPath / fs::path(entryName);
    const bool isDir = !entryName.empty() && entryName.back() == '/';

    if (isDir) {
      fs::create_directories(targetPath, ec);
    } else {
      if (targetPath.has_parent_path()) {
        fs::create_directories(targetPath.parent_path(), ec);
      }

      // Skip existing files when overwrite is disabled
      if (fs::exists(targetPath, ec) && !opts.overwrite) {
        continue;
      }

      rc = unzOpenCurrentFile(uf);
      if (rc != UNZ_OK) {
        closeUnz();
        errorOut = "Failed to open entry in zip: " + entryName;
        return false;
      }

      std::ofstream out(targetPath, std::ios::binary | std::ios::trunc);
      if (!out.is_open()) {
        unzCloseCurrentFile(uf);
        closeUnz();
        errorOut =
            "Failed to create target file on disk: " + targetPath.string();
        return false;
      }

      // Stream extracted data in 64KB chunks
      std::vector<char> buffer(64 * 1024);
      bool readError = false;
      while (true) {
        const int bytes = unzReadCurrentFile(
            uf, buffer.data(), static_cast<unsigned int>(buffer.size()));
        if (bytes < 0) {
          readError = true;
          break;
        }
        if (bytes == 0)
          break;
        out.write(buffer.data(), bytes);
      }

      unzCloseCurrentFile(uf);

      if (readError) {
        closeUnz();
        errorOut = "Error while extracting file: " + entryName;
        return false;
      }
    }
  } while (unzGoToNextFile(uf) == UNZ_OK);

  closeUnz();
  return true;
}

bool ListZipEntries(const fs::path &zipPath,
                    std::vector<ZipEntryInfo> &outEntries,
                    std::string &errorOut) {
  std::error_code ec;
  if (!fs::exists(zipPath, ec) || ec) {
    errorOut = "Zip archive does not exist.";
    return false;
  }

  const std::string zipStr = zipPath.string();
  unzFile uf = unzOpen64(zipStr.c_str());
  if (!uf) {
    errorOut = "Failed to open zip archive.";
    return false;
  }

  int rc = unzGoToFirstFile(uf);
  if (rc == UNZ_END_OF_LIST_OF_FILE) {
    unzClose(uf);
    return true;
  }
  if (rc != UNZ_OK) {
    unzClose(uf);
    errorOut = "Failed to read entries from zip archive.";
    return false;
  }

  // Iterate through all entries and collect metadata
  do {
    unz_file_info64 fileInfo = {};
    char rawName[1024] = {};
    rc = unzGetCurrentFileInfo64(uf, &fileInfo, rawName, sizeof(rawName),
                                 nullptr, 0, nullptr, 0);
    if (rc != UNZ_OK) {
      unzClose(uf);
      errorOut = "Failed to read entry metadata from zip.";
      return false;
    }

    std::string entryName(rawName);
    std::replace(entryName.begin(), entryName.end(), '\\', '/');

    ZipEntryInfo info;
    info.name = entryName;
    info.size = fileInfo.uncompressed_size;
    info.compressedSize = fileInfo.compressed_size;
    info.crc = static_cast<uint32_t>(fileInfo.crc);
    info.isDirectory = !entryName.empty() && entryName.back() == '/';

    outEntries.push_back(info);
  } while (unzGoToNextFile(uf) == UNZ_OK);

  unzClose(uf);
  return true;
}

bool ReadZipEntryContent(const fs::path &zipPath, const std::string &entryName,
                         std::string &outContent, std::string &errorOut) {
  std::error_code ec;
  if (!fs::exists(zipPath, ec) || ec) {
    errorOut = "Zip archive does not exist.";
    return false;
  }

  const std::string zipStr = zipPath.string();
  unzFile uf = unzOpen64(zipStr.c_str());
  if (!uf) {
    errorOut = "Failed to open zip archive.";
    return false;
  }

  std::string target = entryName;
  std::replace(target.begin(), target.end(), '\\', '/');

  // Locate the specific entry by name
  if (unzLocateFile(uf, target.c_str(), 2) != UNZ_OK) {
    unzClose(uf);
    errorOut = "Entry not found in zip archive: " + entryName;
    return false;
  }

  unz_file_info64 fileInfo = {};
  char rawName[1024] = {};
  if (unzGetCurrentFileInfo64(uf, &fileInfo, rawName, sizeof(rawName), nullptr,
                              0, nullptr, 0) != UNZ_OK) {
    unzClose(uf);
    errorOut = "Failed to read entry info.";
    return false;
  }

  if (unzOpenCurrentFile(uf) != UNZ_OK) {
    unzClose(uf);
    errorOut = "Failed to open entry for reading.";
    return false;
  }

  // Pre-allocate output buffer based on uncompressed size
  outContent.clear();
  if (fileInfo.uncompressed_size > 0) {
    outContent.resize(static_cast<size_t>(fileInfo.uncompressed_size));
    int bytes = unzReadCurrentFile(
        uf, outContent.data(), static_cast<unsigned int>(outContent.size()));
    if (bytes < 0) {
      unzCloseCurrentFile(uf);
      unzClose(uf);
      outContent.clear();
      errorOut = "Failed to read entry content.";
      return false;
    }
  }

  unzCloseCurrentFile(uf);
  unzClose(uf);
  return true;
}
} // namespace novadesk::shared
