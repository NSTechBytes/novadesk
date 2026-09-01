/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */
 
#include "FSModule.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "../../shared/PathUtils.h"
#include "../../shared/Utils.h"
#include "../../shared/ZipUtils.h"
#include "../engine/JSEngine.h"

namespace novadesk::scripting::quickjs
{
    namespace
    {
        namespace fs = std::filesystem;

        constexpr size_t kFsReadFileMaxBytes = 64u * 1024u * 1024u; // 64 MB

        std::wstring ResolveFsPath(JSContext *ctx, JSValueConst v)
        {
            const char *s = JS_ToCString(ctx, v);
            if (!s)
                return L"";
            std::wstring p = Utils::ToWString(s);
            JS_FreeCString(ctx, s);
            if (p.empty())
                return L"";
            if (!PathUtils::IsPathRelative(p))
                return PathUtils::NormalizePath(p);
            return PathUtils::ResolvePath(p, JSEngine::GetEntryScriptDir());
        }

        JSValue JsFsReadFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.readFile(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            std::error_code ec;
            auto fileSize = fs::file_size(fs::path(path), ec);
            if (ec || fileSize > kFsReadFileMaxBytes)
                return JS_ThrowTypeError(ctx, "fs.readFile: file too large (max 64 MB)");
            std::ifstream in(fs::path(path), std::ios::binary);
            if (!in.is_open())
                return JS_NULL;
            std::string data;
            data.resize(static_cast<size_t>(fileSize));
            in.read(data.data(), static_cast<std::streamsize>(fileSize));
            return JS_NewStringLen(ctx, data.data(), data.size());
        }

        JSValue JsFsWriteFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.writeFile(path, data[, append])");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            size_t len = 0;
            const char *data = JS_ToCStringLen(ctx, &len, argv[1]);
            if (!data)
                return JS_EXCEPTION;
            const bool append = (argc > 2) ? (JS_ToBool(ctx, argv[2]) != 0) : false;
            std::ofstream out(fs::path(path), std::ios::binary | (append ? std::ios::app : std::ios::trunc));
            bool ok = out.is_open();
            if (ok)
            {
                out.write(data, static_cast<std::streamsize>(len));
                ok = !out.fail();
            }
            JS_FreeCString(ctx, data);
            return JS_NewBool(ctx, ok ? 1 : 0);
        }

        JSValue JsFsExists(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.exists(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_NewBool(ctx, 0);
            std::error_code ec;
            return JS_NewBool(ctx, fs::exists(fs::path(path), ec) && !ec ? 1 : 0);
        }

        JSValue JsFsMkdir(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.mkdir(path[, recursive])");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            const bool recursive = (argc > 1) ? (JS_ToBool(ctx, argv[1]) != 0) : true;
            std::error_code ec;
            bool ok = recursive ? fs::create_directories(fs::path(path), ec) : fs::create_directory(fs::path(path), ec);
            if (!ok && fs::exists(fs::path(path), ec))
                ok = true;
            return JS_NewBool(ctx, ok && !ec ? 1 : 0);
        }

        JSValue JsFsReaddir(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.readdir(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            JSValue arr = JS_NewArray(ctx);
            std::error_code ec;
            uint32_t i = 0;
            for (const auto &e : fs::directory_iterator(fs::path(path), ec))
            {
                if (ec)
                    break;
                const std::string name = Utils::ToString(e.path().filename().wstring());
                JS_SetPropertyUint32(ctx, arr, i++, JS_NewString(ctx, name.c_str()));
            }
            return arr;
        }

        JSValue JsFsUnlink(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.unlink(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            std::error_code ec;
            bool ok = fs::remove(fs::path(path), ec);
            return JS_NewBool(ctx, ok && !ec ? 1 : 0);
        }

        JSValue JsFsRename(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.rename(from, to)");
            const std::wstring from = ResolveFsPath(ctx, argv[0]);
            const std::wstring to = ResolveFsPath(ctx, argv[1]);
            if (from.empty() || to.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            std::error_code ec;
            fs::rename(fs::path(from), fs::path(to), ec);
            return JS_NewBool(ctx, !ec ? 1 : 0);
        }

        JSValue JsFsCopyFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.copyFile(from, to[, overwrite])");
            const std::wstring from = ResolveFsPath(ctx, argv[0]);
            const std::wstring to = ResolveFsPath(ctx, argv[1]);
            if (from.empty() || to.empty())
                return JS_ThrowTypeError(ctx, "invalid path");
            const bool overwrite = (argc > 2) ? (JS_ToBool(ctx, argv[2]) != 0) : true;
            std::error_code ec;
            const bool ok = fs::copy_file(
                fs::path(from),
                fs::path(to),
                overwrite ? fs::copy_options::overwrite_existing : fs::copy_options::none,
                ec);
            return JS_NewBool(ctx, ok && !ec ? 1 : 0);
        }

        JSValue JsFsStat(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.stat(path)");
            const std::wstring path = ResolveFsPath(ctx, argv[0]);
            if (path.empty())
                return JS_NULL;
            std::error_code ec;
            const fs::path p(path);
            const fs::file_status st = fs::symlink_status(p, ec);
            if (ec || !fs::exists(st))
                return JS_NULL;

            JSValue out = JS_NewObject(ctx);
            JS_SetPropertyStr(ctx, out, "isFile", JS_NewBool(ctx, fs::is_regular_file(st) ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "isDirectory", JS_NewBool(ctx, fs::is_directory(st) ? 1 : 0));
            JS_SetPropertyStr(ctx, out, "isSymlink", JS_NewBool(ctx, fs::is_symlink(st) ? 1 : 0));
            uintmax_t size = 0;
            if (fs::is_regular_file(st))
            {
                size = fs::file_size(p, ec);
                if (ec)
                    size = 0;
            }
            JS_SetPropertyStr(ctx, out, "size", JS_NewFloat64(ctx, static_cast<double>(size)));
            JS_SetPropertyStr(ctx, out, "mode", JS_NewInt32(ctx, static_cast<int32_t>(st.permissions())));
            return out;
        }

        JSValue JsFsZip(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.zip(sourcePath, destinationZipPath[, options])");
            const std::wstring src = ResolveFsPath(ctx, argv[0]);
            const std::wstring dst = ResolveFsPath(ctx, argv[1]);
            if (src.empty() || dst.empty())
                return JS_ThrowTypeError(ctx, "invalid path");

            novadesk::shared::ZipCompressOptions opts;
            if (argc > 2 && JS_IsObject(argv[2]))
            {
                JSValue compVal = JS_GetPropertyStr(ctx, argv[2], "compressionLevel");
                if (!JS_IsUndefined(compVal) && !JS_IsNull(compVal))
                {
                    int32_t lvl = 6;
                    JS_ToInt32(ctx, &lvl, compVal);
                    opts.compressionLevel = lvl;
                }
                JS_FreeValue(ctx, compVal);

                JSValue overwVal = JS_GetPropertyStr(ctx, argv[2], "overwrite");
                if (!JS_IsUndefined(overwVal) && !JS_IsNull(overwVal))
                {
                    opts.overwrite = (JS_ToBool(ctx, overwVal) != 0);
                }
                JS_FreeValue(ctx, overwVal);
            }

            std::string error;
            bool ok = novadesk::shared::CompressToZip(fs::path(src), fs::path(dst), opts, error);
            return JS_NewBool(ctx, ok ? 1 : 0);
        }

        JSValue JsFsUnzip(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.unzip(zipPath, destinationDir[, options])");
            const std::wstring zipPath = ResolveFsPath(ctx, argv[0]);
            const std::wstring destDir = ResolveFsPath(ctx, argv[1]);
            if (zipPath.empty() || destDir.empty())
                return JS_ThrowTypeError(ctx, "invalid path");

            novadesk::shared::ZipExtractOptions opts;
            if (argc > 2 && JS_IsObject(argv[2]))
            {
                JSValue overwVal = JS_GetPropertyStr(ctx, argv[2], "overwrite");
                if (!JS_IsUndefined(overwVal) && !JS_IsNull(overwVal))
                {
                    opts.overwrite = (JS_ToBool(ctx, overwVal) != 0);
                }
                JS_FreeValue(ctx, overwVal);

                JSValue entriesVal = JS_GetPropertyStr(ctx, argv[2], "entries");
                if (JS_IsArray(entriesVal))
                {
                    uint32_t len = 0;
                    JSValue lenVal = JS_GetPropertyStr(ctx, entriesVal, "length");
                    JS_ToUint32(ctx, &len, lenVal);
                    JS_FreeValue(ctx, lenVal);

                    for (uint32_t i = 0; i < len; ++i)
                    {
                        JSValue item = JS_GetPropertyUint32(ctx, entriesVal, i);
                        const char *s = JS_ToCString(ctx, item);
                        if (s)
                        {
                            opts.selectedEntries.push_back(s);
                            JS_FreeCString(ctx, s);
                        }
                        JS_FreeValue(ctx, item);
                    }
                }
                JS_FreeValue(ctx, entriesVal);
            }

            std::string error;
            bool ok = novadesk::shared::ExtractFromZip(fs::path(zipPath), fs::path(destDir), opts, error);
            return JS_NewBool(ctx, ok ? 1 : 0);
        }

        JSValue JsFsListZip(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 1)
                return JS_ThrowTypeError(ctx, "fs.listZip(zipPath)");
            const std::wstring zipPath = ResolveFsPath(ctx, argv[0]);
            if (zipPath.empty())
                return JS_ThrowTypeError(ctx, "invalid path");

            std::vector<novadesk::shared::ZipEntryInfo> entries;
            std::string error;
            bool ok = novadesk::shared::ListZipEntries(fs::path(zipPath), entries, error);
            if (!ok)
                return JS_NULL;

            JSValue arr = JS_NewArray(ctx);
            for (uint32_t i = 0; i < static_cast<uint32_t>(entries.size()); ++i)
            {
                JSValue obj = JS_NewObject(ctx);
                JS_SetPropertyStr(ctx, obj, "name", JS_NewString(ctx, entries[i].name.c_str()));
                JS_SetPropertyStr(ctx, obj, "isDirectory", JS_NewBool(ctx, entries[i].isDirectory ? 1 : 0));
                JS_SetPropertyStr(ctx, obj, "size", JS_NewFloat64(ctx, static_cast<double>(entries[i].size)));
                JS_SetPropertyStr(ctx, obj, "compressedSize", JS_NewFloat64(ctx, static_cast<double>(entries[i].compressedSize)));
                JS_SetPropertyStr(ctx, obj, "crc", JS_NewInt64(ctx, entries[i].crc));
                JS_SetPropertyUint32(ctx, arr, i, obj);
            }
            return arr;
        }

        JSValue JsFsReadZipFile(JSContext *ctx, JSValueConst, int argc, JSValueConst *argv)
        {
            if (argc < 2)
                return JS_ThrowTypeError(ctx, "fs.readZipFile(zipPath, entryName)");
            const std::wstring zipPath = ResolveFsPath(ctx, argv[0]);
            if (zipPath.empty())
                return JS_ThrowTypeError(ctx, "invalid path");

            const char *entryStr = JS_ToCString(ctx, argv[1]);
            if (!entryStr)
                return JS_ThrowTypeError(ctx, "invalid entry name");
            std::string entryName(entryStr);
            JS_FreeCString(ctx, entryStr);

            std::string content;
            std::string error;
            bool ok = novadesk::shared::ReadZipEntryContent(fs::path(zipPath), entryName, content, error);
            if (!ok)
                return JS_NULL;

            return JS_NewStringLen(ctx, content.data(), content.size());
        }
    } // namespace

    static int FsModuleInit(JSContext *ctx, JSModuleDef *m)
    {
        JS_SetModuleExport(ctx, m, "readFile", JS_NewCFunction(ctx, JsFsReadFile, "readFile", 1));
        JS_SetModuleExport(ctx, m, "writeFile", JS_NewCFunction(ctx, JsFsWriteFile, "writeFile", 3));
        JS_SetModuleExport(ctx, m, "exists", JS_NewCFunction(ctx, JsFsExists, "exists", 1));
        JS_SetModuleExport(ctx, m, "mkdir", JS_NewCFunction(ctx, JsFsMkdir, "mkdir", 2));
        JS_SetModuleExport(ctx, m, "readdir", JS_NewCFunction(ctx, JsFsReaddir, "readdir", 1));
        JS_SetModuleExport(ctx, m, "unlink", JS_NewCFunction(ctx, JsFsUnlink, "unlink", 1));
        JS_SetModuleExport(ctx, m, "rename", JS_NewCFunction(ctx, JsFsRename, "rename", 2));
        JS_SetModuleExport(ctx, m, "copyFile", JS_NewCFunction(ctx, JsFsCopyFile, "copyFile", 3));
        JS_SetModuleExport(ctx, m, "stat", JS_NewCFunction(ctx, JsFsStat, "stat", 1));

        JS_SetModuleExport(ctx, m, "zip", JS_NewCFunction(ctx, JsFsZip, "zip", 3));
        JS_SetModuleExport(ctx, m, "createZip", JS_NewCFunction(ctx, JsFsZip, "createZip", 3));
        JS_SetModuleExport(ctx, m, "unzip", JS_NewCFunction(ctx, JsFsUnzip, "unzip", 3));
        JS_SetModuleExport(ctx, m, "extractZip", JS_NewCFunction(ctx, JsFsUnzip, "extractZip", 3));
        JS_SetModuleExport(ctx, m, "listZip", JS_NewCFunction(ctx, JsFsListZip, "listZip", 1));
        JS_SetModuleExport(ctx, m, "readZipEntries", JS_NewCFunction(ctx, JsFsListZip, "readZipEntries", 1));
        JS_SetModuleExport(ctx, m, "readZipFile", JS_NewCFunction(ctx, JsFsReadZipFile, "readZipFile", 2));
        return 0;
    }

    JSModuleDef *EnsureFsModule(JSContext *ctx, const char *moduleName)
    {
        JSModuleDef *m = JS_NewCModule(ctx, moduleName, FsModuleInit);
        if (!m)
            return nullptr;
        // Register all exports. Failures here are extremely rare
        // (OOM for the export name string). The module is already
        // registered with the context by JS_NewCModule, so returning
        // nullptr on partial failure would orphan it — the caller has
        // no handle to finalize it. Always return the module so it
        // is cleaned up normally when the context is destroyed.
        JS_AddModuleExport(ctx, m, "readFile");
        JS_AddModuleExport(ctx, m, "writeFile");
        JS_AddModuleExport(ctx, m, "exists");
        JS_AddModuleExport(ctx, m, "mkdir");
        JS_AddModuleExport(ctx, m, "readdir");
        JS_AddModuleExport(ctx, m, "unlink");
        JS_AddModuleExport(ctx, m, "rename");
        JS_AddModuleExport(ctx, m, "copyFile");
        JS_AddModuleExport(ctx, m, "stat");

        JS_AddModuleExport(ctx, m, "zip");
        JS_AddModuleExport(ctx, m, "createZip");
        JS_AddModuleExport(ctx, m, "unzip");
        JS_AddModuleExport(ctx, m, "extractZip");
        JS_AddModuleExport(ctx, m, "listZip");
        JS_AddModuleExport(ctx, m, "readZipEntries");
        JS_AddModuleExport(ctx, m, "readZipFile");
        return m;
    }
} // namespace novadesk::scripting::quickjs
