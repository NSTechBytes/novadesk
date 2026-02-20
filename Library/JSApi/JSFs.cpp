/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "JSFs.h"
#include "JSUtils.h"
#include "../Utils.h"
#include "../Logging.h"
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <vector>
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <chrono>

namespace JSApi {
    namespace fs = std::filesystem;

    struct DirHandle {
        std::vector<std::wstring> entries;
        size_t index = 0;
    };

    static std::unordered_map<int, FILE*> s_FileHandles;
    static std::unordered_map<int, DirHandle> s_DirHandles;
    static int s_NextFileHandle = 3;
    static int s_NextDirHandle = -1;

    static std::wstring ResolveFsPath(duk_context* ctx, const std::wstring& path) {
        return ResolveScriptPath(ctx, path);
    }

    static void PushStatObject(duk_context* ctx, const fs::path& p, bool lstatMode) {
        duk_push_object(ctx);

        std::error_code ec;
        fs::file_status st = lstatMode ? fs::symlink_status(p, ec) : fs::status(p, ec);
        if (ec || !fs::exists(st)) {
            duk_push_boolean(ctx, false); duk_put_prop_string(ctx, -2, "exists");
            duk_push_number(ctx, 0); duk_put_prop_string(ctx, -2, "size");
            duk_push_boolean(ctx, false); duk_put_prop_string(ctx, -2, "isFile");
            duk_push_boolean(ctx, false); duk_put_prop_string(ctx, -2, "isDirectory");
            duk_push_boolean(ctx, false); duk_put_prop_string(ctx, -2, "isSymlink");
            duk_push_number(ctx, 0); duk_put_prop_string(ctx, -2, "mtimeMs");
            duk_push_number(ctx, 0); duk_put_prop_string(ctx, -2, "atimeMs");
            duk_push_number(ctx, 0); duk_put_prop_string(ctx, -2, "ctimeMs");
            duk_push_number(ctx, 0); duk_put_prop_string(ctx, -2, "mode");
            return;
        }

        duk_push_boolean(ctx, true); duk_put_prop_string(ctx, -2, "exists");
        duk_push_boolean(ctx, fs::is_regular_file(st)); duk_put_prop_string(ctx, -2, "isFile");
        duk_push_boolean(ctx, fs::is_directory(st)); duk_put_prop_string(ctx, -2, "isDirectory");
        duk_push_boolean(ctx, fs::is_symlink(st)); duk_put_prop_string(ctx, -2, "isSymlink");

        uintmax_t size = 0;
        if (fs::is_regular_file(st)) {
            size = fs::file_size(p, ec);
            if (ec) size = 0;
        }
        duk_push_number(ctx, (double)size); duk_put_prop_string(ctx, -2, "size");

        auto mtime = fs::last_write_time(p, ec);
        double mtimeMs = 0;
        if (!ec) {
            using namespace std::chrono;
            auto sctp = time_point_cast<milliseconds>(mtime - fs::file_time_type::clock::now() + system_clock::now());
            mtimeMs = (double)sctp.time_since_epoch().count();
        }
        duk_push_number(ctx, mtimeMs); duk_put_prop_string(ctx, -2, "mtimeMs");
        duk_push_number(ctx, mtimeMs); duk_put_prop_string(ctx, -2, "atimeMs");
        duk_push_number(ctx, mtimeMs); duk_put_prop_string(ctx, -2, "ctimeMs");

        duk_push_number(ctx, (double)static_cast<unsigned int>(st.permissions())); duk_put_prop_string(ctx, -2, "mode");
    }

    static duk_ret_t js_fs_read_file(duk_context* ctx) {
        if (duk_get_top(ctx) < 1 || !duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::wstring p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        std::ifstream in(p, std::ios::binary);
        if (!in.is_open()) { duk_push_null(ctx); return 1; }
        std::string data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        duk_push_lstring(ctx, data.data(), data.size());
        return 1;
    }

    static duk_ret_t js_fs_write_file(duk_context* ctx) {
        if (duk_get_top(ctx) < 2 || !duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::wstring p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        std::string data = duk_safe_to_string(ctx, 1);
        std::ofstream out(p, std::ios::binary | std::ios::trunc);
        bool ok = out.is_open();
        if (ok) out.write(data.data(), (std::streamsize)data.size());
        duk_push_boolean(ctx, ok);
        return 1;
    }

    static duk_ret_t js_fs_append_file(duk_context* ctx) {
        if (duk_get_top(ctx) < 2 || !duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::wstring p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        std::string data = duk_safe_to_string(ctx, 1);
        std::ofstream out(p, std::ios::binary | std::ios::app);
        bool ok = out.is_open();
        if (ok) out.write(data.data(), (std::streamsize)data.size());
        duk_push_boolean(ctx, ok);
        return 1;
    }

    static duk_ret_t js_fs_exists(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        bool ok = fs::exists(ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))), ec);
        duk_push_boolean(ctx, ok && !ec);
        return 1;
    }

    static duk_ret_t js_fs_mkdir(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        bool recursive = (duk_get_top(ctx) > 1 && duk_is_boolean(ctx, 1)) ? (duk_get_boolean(ctx, 1) != 0) : false;
        std::error_code ec;
        fs::path p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        bool ok = recursive ? fs::create_directories(p, ec) : fs::create_directory(p, ec);
        if (!ok && fs::exists(p, ec)) ok = true;
        duk_push_boolean(ctx, ok && !ec);
        return 1;
    }

    static duk_ret_t js_fs_readdir(duk_context* ctx) {
        if (duk_get_top(ctx) < 1) return DUK_RET_TYPE_ERROR;
        duk_push_array(ctx);

        if (duk_is_number(ctx, 0)) {
            int hid = duk_get_int(ctx, 0);
            auto it = s_DirHandles.find(hid);
            if (it == s_DirHandles.end()) return 1;
            duk_uarridx_t idx = 0;
            while (it->second.index < it->second.entries.size()) {
                duk_push_string(ctx, Utils::ToString(it->second.entries[it->second.index]).c_str());
                duk_put_prop_index(ctx, -2, idx++);
                it->second.index++;
            }
            return 1;
        }

        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        fs::path p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        std::error_code ec;
        duk_uarridx_t idx = 0;
        for (auto& entry : fs::directory_iterator(p, ec)) {
            duk_push_string(ctx, Utils::ToString(entry.path().filename().wstring()).c_str());
            duk_put_prop_index(ctx, -2, idx++);
        }
        return 1;
    }

    static duk_ret_t js_fs_opendir(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        fs::path p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        std::error_code ec;
        DirHandle dh;
        for (auto& entry : fs::directory_iterator(p, ec)) {
            dh.entries.push_back(entry.path().filename().wstring());
        }
        if (ec) { duk_push_null(ctx); return 1; }
        int hid = s_NextDirHandle--;
        s_DirHandles[hid] = std::move(dh);
        duk_push_int(ctx, hid);
        return 1;
    }

    static duk_ret_t js_fs_open(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::wstring p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        std::wstring mode = L"rb";
        if (duk_get_top(ctx) > 1 && duk_is_string(ctx, 1)) mode = Utils::ToWString(duk_get_string(ctx, 1));
        FILE* f = nullptr;
        _wfopen_s(&f, p.c_str(), mode.c_str());
        if (!f) { duk_push_int(ctx, -1); return 1; }
        int fd = s_NextFileHandle++;
        s_FileHandles[fd] = f;
        duk_push_int(ctx, fd);
        return 1;
    }

    static duk_ret_t js_fs_close(duk_context* ctx) {
        if (!duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int fd = duk_get_int(ctx, 0);
        if (fd < 0) {
            bool ok = s_DirHandles.erase(fd) > 0;
            duk_push_boolean(ctx, ok);
            return 1;
        }
        auto it = s_FileHandles.find(fd);
        if (it == s_FileHandles.end()) { duk_push_boolean(ctx, false); return 1; }
        fclose(it->second);
        s_FileHandles.erase(it);
        duk_push_boolean(ctx, true);
        return 1;
    }

    static duk_ret_t js_fs_read(duk_context* ctx) {
        if (duk_get_top(ctx) < 2 || !duk_is_number(ctx, 0) || !duk_is_number(ctx, 1)) return DUK_RET_TYPE_ERROR;
        int fd = duk_get_int(ctx, 0);
        int len = duk_get_int(ctx, 1);
        auto it = s_FileHandles.find(fd);
        if (it == s_FileHandles.end() || len <= 0) { duk_push_string(ctx, ""); return 1; }
        if (duk_get_top(ctx) > 2 && duk_is_number(ctx, 2)) {
            _fseeki64(it->second, duk_get_int(ctx, 2), SEEK_SET);
        }
        std::string data((size_t)len, '\0');
        size_t n = fread(data.data(), 1, (size_t)len, it->second);
        duk_push_lstring(ctx, data.data(), n);
        return 1;
    }

    static duk_ret_t js_fs_write(duk_context* ctx) {
        if (duk_get_top(ctx) < 2 || !duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int fd = duk_get_int(ctx, 0);
        std::string data = duk_safe_to_string(ctx, 1);
        auto it = s_FileHandles.find(fd);
        if (it == s_FileHandles.end()) { duk_push_int(ctx, 0); return 1; }
        if (duk_get_top(ctx) > 2 && duk_is_number(ctx, 2)) {
            _fseeki64(it->second, duk_get_int(ctx, 2), SEEK_SET);
        }
        size_t n = fwrite(data.data(), 1, data.size(), it->second);
        duk_push_int(ctx, (int)n);
        return 1;
    }

    static duk_ret_t js_fs_unlink(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        bool ok = fs::remove(ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))), ec);
        duk_push_boolean(ctx, ok && !ec);
        return 1;
    }

    static duk_ret_t js_fs_rename(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_string(ctx, 1)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        fs::rename(ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))),
                   ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 1))), ec);
        duk_push_boolean(ctx, !ec);
        return 1;
    }

    static duk_ret_t js_fs_copy_file(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_string(ctx, 1)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        bool ok = fs::copy_file(
            ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))),
            ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 1))),
            fs::copy_options::overwrite_existing, ec);
        duk_push_boolean(ctx, ok && !ec);
        return 1;
    }

    static duk_ret_t js_fs_truncate(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_number(ctx, 1)) return DUK_RET_TYPE_ERROR;
        std::wstring p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        FILE* f = nullptr;
        _wfopen_s(&f, p.c_str(), L"r+b");
        if (!f) { duk_push_boolean(ctx, false); return 1; }
        int rc = _chsize_s(_fileno(f), (long long)duk_get_number(ctx, 1));
        fclose(f);
        duk_push_boolean(ctx, rc == 0);
        return 1;
    }

    static duk_ret_t js_fs_rmdir(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        bool ok = fs::remove(ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))), ec);
        duk_push_boolean(ctx, ok && !ec);
        return 1;
    }

    static duk_ret_t js_fs_rm(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        bool recursive = (duk_get_top(ctx) > 1 && duk_is_boolean(ctx, 1)) ? (duk_get_boolean(ctx, 1) != 0) : true;
        std::error_code ec;
        fs::path p = ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0)));
        bool ok = false;
        if (recursive) ok = fs::remove_all(p, ec) > 0;
        else ok = fs::remove(p, ec);
        duk_push_boolean(ctx, ok && !ec);
        return 1;
    }

    static duk_ret_t js_fs_mkdtemp(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::wstring prefix = Utils::ToWString(duk_get_string(ctx, 0));
        fs::path base = fs::temp_directory_path();
        std::error_code ec;
        for (int i = 0; i < 100; ++i) {
            auto t = std::chrono::steady_clock::now().time_since_epoch().count();
            fs::path p = base / (prefix + std::to_wstring((long long)t + i));
            if (fs::create_directory(p, ec)) {
                duk_push_string(ctx, Utils::ToString(p.wstring()).c_str());
                return 1;
            }
            if (ec) ec.clear();
        }
        duk_push_null(ctx);
        return 1;
    }

    static duk_ret_t js_fs_stat(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        PushStatObject(ctx, ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))), false);
        return 1;
    }

    static duk_ret_t js_fs_lstat(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        PushStatObject(ctx, ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))), true);
        return 1;
    }

    static duk_ret_t js_fs_fstat(duk_context* ctx) {
        if (!duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int fd = duk_get_int(ctx, 0);
        auto it = s_FileHandles.find(fd);
        if (it == s_FileHandles.end()) { duk_push_null(ctx); return 1; }
        struct _stat64 st = {};
        if (_fstat64(_fileno(it->second), &st) != 0) { duk_push_null(ctx); return 1; }
        duk_push_object(ctx);
        duk_push_boolean(ctx, true); duk_put_prop_string(ctx, -2, "exists");
        duk_push_boolean(ctx, (st.st_mode & _S_IFREG) != 0); duk_put_prop_string(ctx, -2, "isFile");
        duk_push_boolean(ctx, (st.st_mode & _S_IFDIR) != 0); duk_put_prop_string(ctx, -2, "isDirectory");
        duk_push_number(ctx, (double)st.st_size); duk_put_prop_string(ctx, -2, "size");
        duk_push_number(ctx, (double)st.st_mtime * 1000.0); duk_put_prop_string(ctx, -2, "mtimeMs");
        duk_push_number(ctx, (double)st.st_atime * 1000.0); duk_put_prop_string(ctx, -2, "atimeMs");
        duk_push_number(ctx, (double)st.st_ctime * 1000.0); duk_put_prop_string(ctx, -2, "ctimeMs");
        duk_push_number(ctx, (double)st.st_mode); duk_put_prop_string(ctx, -2, "mode");
        return 1;
    }

    static duk_ret_t js_fs_access(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int mode = 0;
        if (duk_get_top(ctx) > 1 && duk_is_number(ctx, 1)) mode = duk_get_int(ctx, 1);
        int rc = _waccess(ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))).c_str(), mode);
        duk_push_boolean(ctx, rc == 0);
        return 1;
    }

    static duk_ret_t js_fs_utimes(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_number(ctx, 2)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        double mtimeMs = duk_get_number(ctx, 2);
        using namespace std::chrono;
        auto tp = system_clock::time_point(milliseconds((long long)mtimeMs));
        auto ftp = fs::file_time_type::clock::now() + (tp - system_clock::now());
        fs::last_write_time(ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))), ftp, ec);
        duk_push_boolean(ctx, !ec);
        return 1;
    }

    static duk_ret_t js_fs_fsync(duk_context* ctx) {
        if (!duk_is_number(ctx, 0)) return DUK_RET_TYPE_ERROR;
        int fd = duk_get_int(ctx, 0);
        auto it = s_FileHandles.find(fd);
        if (it == s_FileHandles.end()) { duk_push_boolean(ctx, false); return 1; }
        int rc = fflush(it->second);
        duk_push_boolean(ctx, rc == 0);
        return 1;
    }

    static duk_ret_t js_fs_ftruncate(duk_context* ctx) {
        if (!duk_is_number(ctx, 0) || !duk_is_number(ctx, 1)) return DUK_RET_TYPE_ERROR;
        int fd = duk_get_int(ctx, 0);
        auto it = s_FileHandles.find(fd);
        if (it == s_FileHandles.end()) { duk_push_boolean(ctx, false); return 1; }
        int rc = _chsize_s(_fileno(it->second), (long long)duk_get_number(ctx, 1));
        duk_push_boolean(ctx, rc == 0);
        return 1;
    }

    static duk_ret_t js_fs_link(duk_context* ctx) {
        if (!duk_is_string(ctx, 0) || !duk_is_string(ctx, 1)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        fs::create_hard_link(
            ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))),
            ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 1))), ec);
        duk_push_boolean(ctx, !ec);
        return 1;
    }

    static duk_ret_t js_fs_realpath(duk_context* ctx) {
        if (!duk_is_string(ctx, 0)) return DUK_RET_TYPE_ERROR;
        std::error_code ec;
        fs::path p = fs::weakly_canonical(ResolveFsPath(ctx, Utils::ToWString(duk_get_string(ctx, 0))), ec);
        if (ec) { duk_push_null(ctx); return 1; }
        duk_push_string(ctx, Utils::ToString(p.wstring()).c_str());
        return 1;
    }

    void PushFsModule(duk_context* ctx) {
        duk_push_object(ctx);

        duk_push_c_function(ctx, js_fs_read_file, DUK_VARARGS); duk_put_prop_string(ctx, -2, "readFile");
        duk_push_c_function(ctx, js_fs_write_file, DUK_VARARGS); duk_put_prop_string(ctx, -2, "writeFile");
        duk_push_c_function(ctx, js_fs_open, DUK_VARARGS); duk_put_prop_string(ctx, -2, "open");
        duk_push_c_function(ctx, js_fs_close, 1); duk_put_prop_string(ctx, -2, "close");
        duk_push_c_function(ctx, js_fs_mkdir, DUK_VARARGS); duk_put_prop_string(ctx, -2, "mkdir");
        duk_push_c_function(ctx, js_fs_opendir, 1); duk_put_prop_string(ctx, -2, "opendir");
        duk_push_c_function(ctx, js_fs_exists, 1); duk_put_prop_string(ctx, -2, "exists");
        duk_push_c_function(ctx, js_fs_append_file, 2); duk_put_prop_string(ctx, -2, "appendFile");
        duk_push_c_function(ctx, js_fs_unlink, 1); duk_put_prop_string(ctx, -2, "unlink");
        duk_push_c_function(ctx, js_fs_rename, 2); duk_put_prop_string(ctx, -2, "rename");
        duk_push_c_function(ctx, js_fs_copy_file, 2); duk_put_prop_string(ctx, -2, "copyFile");
        duk_push_c_function(ctx, js_fs_truncate, 2); duk_put_prop_string(ctx, -2, "truncate");
        duk_push_c_function(ctx, js_fs_readdir, 1); duk_put_prop_string(ctx, -2, "readdir");
        duk_push_c_function(ctx, js_fs_rmdir, 1); duk_put_prop_string(ctx, -2, "rmdir");
        duk_push_c_function(ctx, js_fs_rm, DUK_VARARGS); duk_put_prop_string(ctx, -2, "rm");
        duk_push_c_function(ctx, js_fs_mkdtemp, 1); duk_put_prop_string(ctx, -2, "mkdtemp");
        duk_push_c_function(ctx, js_fs_stat, 1); duk_put_prop_string(ctx, -2, "stat");
        duk_push_c_function(ctx, js_fs_lstat, 1); duk_put_prop_string(ctx, -2, "lstat");
        duk_push_c_function(ctx, js_fs_fstat, 1); duk_put_prop_string(ctx, -2, "fstat");
        duk_push_c_function(ctx, js_fs_access, DUK_VARARGS); duk_put_prop_string(ctx, -2, "access");
        duk_push_c_function(ctx, js_fs_utimes, 3); duk_put_prop_string(ctx, -2, "utimes");
        duk_push_c_function(ctx, js_fs_read, DUK_VARARGS); duk_put_prop_string(ctx, -2, "read");
        duk_push_c_function(ctx, js_fs_write, DUK_VARARGS); duk_put_prop_string(ctx, -2, "write");
        duk_push_c_function(ctx, js_fs_fsync, 1); duk_put_prop_string(ctx, -2, "fsync");
        duk_push_c_function(ctx, js_fs_ftruncate, 2); duk_put_prop_string(ctx, -2, "ftruncate");
        duk_push_c_function(ctx, js_fs_link, 2); duk_put_prop_string(ctx, -2, "link");
        duk_push_c_function(ctx, js_fs_realpath, 1); duk_put_prop_string(ctx, -2, "realpath");
        duk_push_object(ctx);
        duk_push_int(ctx, 0); duk_put_prop_string(ctx, -2, "F_OK");
        duk_push_int(ctx, 4); duk_put_prop_string(ctx, -2, "R_OK");
        duk_push_int(ctx, 2); duk_put_prop_string(ctx, -2, "W_OK");
        duk_push_int(ctx, 1); duk_put_prop_string(ctx, -2, "X_OK");
        duk_put_prop_string(ctx, -2, "constants");
    }

    void BindFsMethods(duk_context* ctx) {
        PushFsModule(ctx);
        duk_put_prop_string(ctx, -2, "fs");
    }
}
