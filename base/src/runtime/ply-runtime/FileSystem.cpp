/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                           ┃
┃    ╱   ╱╲    Plywood Multimedia Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/                  ┃
┃    └──┴┴┴┘                                         ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32
#include <ply-runtime/string/WString.h>
#include <shellapi.h>
#else
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#endif

namespace ply {

ThreadLocal<FSResult> FileSystemIface::last_result_;
FileSystem_t FileSystem;

//  ▄▄    ▄▄        ▄▄▄  ▄▄
//  ██ ▄▄ ██  ▄▄▄▄   ██  ██  ▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ▀█▄██▄█▀  ▄▄▄██  ██  ██▄█▀  ██▄▄██ ██  ▀▀
//   ██▀▀██  ▀█▄▄██ ▄██▄ ██ ▀█▄ ▀█▄▄▄  ██
//

void FileSystemWalker::visit(StringView dir_path) {
    this->triple.dir_path = dir_path;
    this->triple.dir_names.clear();
    this->triple.files.clear();
    for (FileInfo& info : this->fs->list_dir(dir_path)) {
        if (info.is_dir) {
            this->triple.dir_names.append(std::move(info.name));
        } else {
            this->triple.files.append(std::move(info));
        }
    }
}

void FileSystemWalker::Iterator::operator++() {
    if (!this->walker->triple.dir_names.is_empty()) {
        StackItem& item = this->walker->stack.append();
        item.path = std::move(this->walker->triple.dir_path);
        item.dir_names = std::move(this->walker->triple.dir_names);
        item.dir_index = 0;
    } else {
        this->walker->triple.dir_path.clear();
        this->walker->triple.dir_names.clear();
        this->walker->triple.files.clear();
    }
    while (!this->walker->stack.is_empty()) {
        StackItem& item = this->walker->stack.back();
        if (item.dir_index < item.dir_names.num_items()) {
            this->walker->visit(this->walker->fs->path_format().join(
                item.path, item.dir_names[item.dir_index]));
            item.dir_index++;
            return;
        }
        this->walker->stack.pop();
    }
    // End of walk
    PLY_ASSERT(this->walker->triple.dir_path.is_empty());
}

//  ▄▄▄▄   ▄▄▄
//   ██   ██    ▄▄▄▄   ▄▄▄▄  ▄▄▄▄
//   ██  ▀██▀   ▄▄▄██ ██    ██▄▄██
//  ▄██▄  ██   ▀█▄▄██ ▀█▄▄▄ ▀█▄▄▄
//

FileSystemWalker FileSystemIface::walk(StringView top, u32 flags) {
    FileSystemWalker walker;
    walker.fs = this;
    walker.flags = flags;
    walker.visit(top);
    return walker;
}

FSResult FileSystemIface::make_dirs(StringView path) {
    if (path == this->path_format().get_drive_letter(path)) {
        return this->set_last_result(FSResult::OK);
    }
    ExistsResult er = this->exists(path);
    if (er == ExistsResult::Directory) {
        return this->set_last_result(FSResult::AlreadyExists);
    } else if (er == ExistsResult::File) {
        return this->set_last_result(FSResult::AccessDenied);
    } else {
        auto split = this->path_format().split(path);
        if (!split.first.is_empty() && !split.second.is_empty()) {
            FSResult r = make_dirs(split.first);
            if (r != FSResult::OK && r != FSResult::AlreadyExists)
                return r;
        }
        return this->make_dir(path);
    }
}

InStream FileSystemIface::open_stream_for_read(StringView path) {
    return this->open_pipe_for_read(path);
}

OutStream FileSystemIface::open_stream_for_write(StringView path) {
    return this->open_pipe_for_write(path);
}

InStream FileSystemIface::open_text_for_read(StringView path,
                                             const TextFormat& text_format) {
    if (InStream in = this->open_stream_for_read(path))
        return text_format.create_importer(std::move(in));
    return {};
}

InStream FileSystemIface::open_text_for_read_autodetect(StringView path,
                                                        TextFormat* out_format) {
    if (InStream in = this->open_stream_for_read(path)) {
        TextFormat text_format = TextFormat::autodetect(in);
        if (out_format) {
            *out_format = text_format;
        }
        return text_format.create_importer(std::move(in));
    }
    return {};
}

String FileSystemIface::load_binary(StringView path) {
    String result;
    Owned<InPipe> in_pipe = this->open_pipe_for_read(path);
    if (in_pipe) {
        u64 file_size = in_pipe->get_file_size();
        // Files >= 4GB cannot be loaded this way:
        result.resize(safe_demote<u32>(file_size));
        in_pipe->read({result.bytes, result.num_bytes});
    }
    return result;
}

String FileSystemIface::load_text(StringView path, const TextFormat& text_format) {
    if (InStream in = this->open_text_for_read(path, text_format))
        return in.read_remaining_contents();
    return {};
}

String FileSystemIface::load_text_autodetect(StringView path, TextFormat* out_format) {
    if (InStream in = this->open_text_for_read_autodetect(path, out_format)) {
        return in.read_remaining_contents();
    }
    return {};
}

OutStream FileSystemIface::open_text_for_write(StringView path,
                                               const TextFormat& text_format) {
    if (OutStream out = this->open_stream_for_write(path))
        return {text_format.create_exporter(std::move(out)).release(), true};
    return {};
}

FSResult FileSystemIface::make_dirs_and_save_binary_if_different(StringView path,
                                                                 StringView view) {
    // Load existing contents.
    // FIXME: We shouldn't need to load the whole file in memory first. Instead, compare
    // as we go.
    String existing_contents = this->load_binary(path);
    FSResult existing_result = this->last_result();
    if (existing_result == FSResult::OK && existing_contents == view) {
        return this->set_last_result(FSResult::Unchanged);
    }
    if (existing_result != FSResult::OK && existing_result != FSResult::NotFound) {
        return existing_result;
    }
    existing_contents.clear();

    // Create intermediate directories
    FSResult result = this->make_dirs(this->path_format().split(path).first);
    if (result != FSResult::OK && result != FSResult::AlreadyExists) {
        return result;
    }

    // Save file
    // FIXME: Write to temporary file first, then rename atomically
    Owned<OutPipe> out_pipe = this->open_pipe_for_write(path);
    result = this->last_result();
    if (result != FSResult::OK) {
        return result;
    }
    out_pipe->write(view);
    return result;
}

FSResult FileSystemIface::make_dirs_and_save_text_if_different(
    StringView path, StringView str_contents, const TextFormat& text_format) {
    Owned<OutPipe> out = text_format.create_exporter(MemOutStream{});
    out->write(str_contents);
    out->flush();
    String raw_contents = out->get_tail_pipe()->child_stream.move_to_string();
    return this->make_dirs_and_save_binary_if_different(path, raw_contents);
}

#if PLY_TARGET_WIN32

//  ▄▄    ▄▄ ▄▄            ▄▄
//  ██ ▄▄ ██ ▄▄ ▄▄▄▄▄   ▄▄▄██  ▄▄▄▄  ▄▄    ▄▄  ▄▄▄▄
//  ▀█▄██▄█▀ ██ ██  ██ ██  ██ ██  ██ ██ ██ ██ ▀█▄▄▄
//   ██▀▀██  ██ ██  ██ ▀█▄▄██ ▀█▄▄█▀  ██▀▀██   ▄▄▄█▀
//

#define PLY_FSWIN32_ALLOW_UKNOWN_ERRORS 0

PLY_INLINE double windows_to_posix_time(const FILETIME& file_time) {
    return (u64(file_time.dwHighDateTime) << 32 | file_time.dwLowDateTime) /
               10000000.0 -
           11644473600.0;
}

void file_info_from_data(FileInfo* info, WIN32_FIND_DATAW find_data, u32 flags) {
    info->name = from_wstring(find_data.cFileName);
    info->is_dir = (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (flags & FileSystemIface::WithSizes) {
        info->file_size = u64(find_data.nFileSizeHigh) << 32 | find_data.nFileSizeLow;
    }
    if (flags & FileSystemIface::WithTimes) {
        info->creation_time = windows_to_posix_time(find_data.ftCreationTime);
        info->access_time = windows_to_posix_time(find_data.ftLastAccessTime);
        info->modification_time = windows_to_posix_time(find_data.ftLastWriteTime);
    }
}

Array<FileInfo> FileSystem_t::list_dir(StringView path, u32 flags) {
    Array<FileInfo> result;
    HANDLE h_find = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW find_data;

    String pattern = WindowsPath.join(path, "*");
    h_find = FindFirstFileW(win32_path_arg(pattern), &find_data);
    if (h_find == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME: {
                this->set_last_result(FSResult::NotFound);
                return result;
            }
            case ERROR_ACCESS_DENIED: {
                this->set_last_result(FSResult::AccessDenied);
                return result;
            }
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                this->set_last_result(FSResult::Unknown);
                return result;
            }
        }
    }

    while (true) {
        FileInfo info;
        file_info_from_data(&info, find_data, flags);
        if (info.name != "." && info.name != "..") {
            result.append(std::move(info));
        }

        BOOL rc = FindNextFileW(h_find, &find_data);
        if (!rc) {
            DWORD err = GetLastError();
            switch (err) {
                case ERROR_NO_MORE_FILES: {
                    this->set_last_result(FSResult::OK);
                    return result;
                }
                case ERROR_FILE_INVALID: {
                    this->set_last_result(FSResult::NotFound);
                    return result;
                }
                default: {
                    PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                    this->set_last_result(FSResult::Unknown);
                    return result;
                }
            }
        }
    }
}

FSResult FileSystem_t::make_dir(StringView path) {
    BOOL rc = CreateDirectoryW(win32_path_arg(path), NULL);
    if (rc) {
        return this->set_last_result(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_ALREADY_EXISTS:
                return this->set_last_result(FSResult::AlreadyExists);
            case ERROR_ACCESS_DENIED:
                return this->set_last_result(FSResult::AccessDenied);
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return this->set_last_result(FSResult::Unknown);
            }
        }
    }
}

Path_t FileSystem_t::path_format() {
    return Path;
}

FSResult FileSystem_t::set_working_directory(StringView path) {
    BOOL rc;
    {
        // This ReadWriteLock is used to mitigate data race issues with
        // SetCurrentDirectoryW:
        // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
        ExclusiveLockGuard<ReadWriteLock> guard{this->working_dir_lock};
        rc = SetCurrentDirectoryW(win32_path_arg(path));
    }
    if (rc) {
        return this->set_last_result(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_PATH_NOT_FOUND:
                return this->set_last_result(FSResult::NotFound);
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return this->set_last_result(FSResult::Unknown);
            }
        }
    }
}

String FileSystem_t::get_working_directory() {
    u32 num_units_with_null_term = MAX_PATH + 1;
    for (;;) {
        WString win32_path = WString::allocate(num_units_with_null_term);
        DWORD rc;
        {
            // This ReadWriteLock is used to mitigate data race issues with
            // SetCurrentDirectoryW:
            // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
            SharedLockGuard<ReadWriteLock> guard{this->working_dir_lock};
            rc = GetCurrentDirectoryW(num_units_with_null_term,
                                      (LPWSTR) win32_path.units);
        }
        if (rc == 0) {
            PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
            this->set_last_result(FSResult::Unknown);
            return {};
        }
        PLY_ASSERT(rc != num_units_with_null_term);
        if (rc < num_units_with_null_term) {
            // GetCurrentDirectoryW: If the function succeeds, the return value
            // specifies the number of characters that are written to the buffer, not
            // including the terminating null character.
            WStringView truncated_win32_path = {win32_path.units, rc};
            if (truncated_win32_path.num_units >= 4 &&
                truncated_win32_path.raw_bytes().left(8) ==
                    StringView{(const char*) L"\\\\?\\", 8}) {
                // Drop leading "\\\\?\\":
                truncated_win32_path.units += 4;
                truncated_win32_path.num_units -= 4;
            }
            this->set_last_result(FSResult::OK);
            return from_wstring(truncated_win32_path);
        }
        // GetCurrentDirectoryW: If the buffer that is pointed to by lp_buffer is not
        // large enough, the return value specifies the required size of the buffer, in
        // characters, including the null-terminating character.
        num_units_with_null_term = rc;
    }
}

ExistsResult FileSystem_t::exists(StringView path) {
    // FIXME: Do something sensible when passed "C:" and other drive letters
    DWORD attribs = GetFileAttributesW(win32_path_arg(path));
    if (attribs == INVALID_FILE_ATTRIBUTES) {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME: {
                return ExistsResult::NotFound;
            }
            default: {
#if PLY_WITH_ASSERTS
                PLY_DEBUG_BREAK(); // Unrecognized error
#endif
                return ExistsResult::NotFound;
            }
        }
    } else if ((attribs & FILE_ATTRIBUTE_DIRECTORY) != 0) {
        return ExistsResult::Directory;
    } else {
        return ExistsResult::File;
    }
}

HANDLE FileSystem_t::open_handle_for_read(StringView path) {
    // Should this use FILE_SHARE_DELETE or FILE_SHARE_WRITE?
    HANDLE handle = CreateFileW(win32_path_arg(path), GENERIC_READ, FILE_SHARE_READ,
                                NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        this->set_last_result(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME:
                this->set_last_result(FSResult::NotFound);
                break;

            case ERROR_SHARING_VIOLATION:
                this->set_last_result(FSResult::Locked);
                break;

            case ERROR_ACCESS_DENIED:
                this->set_last_result(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                this->set_last_result(FSResult::Unknown);
                break;
        }
    }
    return handle;
}

Owned<InPipe> FileSystem_t::open_pipe_for_read(StringView path) {
    HANDLE handle = open_handle_for_read(path);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    return new InPipe_Handle{handle};
}

HANDLE FileSystem_t::open_handle_for_write(StringView path) {
    // FIXME: Needs graceful handling of ERROR_SHARING_VIOLATION
    // Should this use FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE?
    HANDLE handle = CreateFileW(win32_path_arg(path), GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        this->set_last_result(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME:
                this->set_last_result(FSResult::NotFound);
                break;

            case ERROR_SHARING_VIOLATION:
                this->set_last_result(FSResult::Locked);
                break;

            case ERROR_ACCESS_DENIED:
                this->set_last_result(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                this->set_last_result(FSResult::Unknown);
                break;
        }
    }
    return handle;
}

Owned<OutPipe> FileSystem_t::open_pipe_for_write(StringView path) {
    HANDLE handle = open_handle_for_write(path);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    return new OutPipe_Handle{handle};
}

FSResult FileSystem_t::move_file(StringView src_path, StringView dst_path) {
    BOOL rc = MoveFileExW(win32_path_arg(src_path), win32_path_arg(dst_path),
                          MOVEFILE_REPLACE_EXISTING);
    if (rc) {
        return this->set_last_result(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        return this->set_last_result(FSResult::Unknown);
    }
}

FSResult FileSystem_t::delete_file(StringView path) {
    BOOL rc = DeleteFileW(win32_path_arg(path));
    if (rc) {
        return this->set_last_result(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        return this->set_last_result(FSResult::Unknown);
    }
}

FSResult FileSystem_t::remove_dir_tree(StringView dir_path) {
    HybridString abs_path = dir_path;
    if (!WindowsPath.is_absolute(dir_path)) {
        abs_path = WindowsPath.join(this->get_working_directory(), dir_path);
    }
    OutPipe_ConvertUnicode out{MemOutStream{}, UTF16_LE};
    out.write(abs_path.view());
    out.child_stream << StringView{"\0\0\0\0", 4}; // double null terminated
    WString wstr = WString::move_from_string(out.child_stream.move_to_string());
    SHFILEOPSTRUCTW shfo;
    memset(&shfo, 0, sizeof(shfo));
    shfo.hwnd = NULL;
    shfo.wFunc = FO_DELETE;
    shfo.pFrom = wstr;
    shfo.pTo = NULL;
    shfo.fFlags = FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION;
    shfo.fAnyOperationsAborted = FALSE;
    shfo.hNameMappings = NULL;
    shfo.lpszProgressTitle = NULL;
    int rc = SHFileOperationW(&shfo);
    return (rc == 0) ? FSResult::OK : FSResult::AccessDenied;
}

FileInfo FileSystem_t::get_file_info(HANDLE handle) {
    FileInfo info;
    FILETIME creation_time = {0, 0};
    FILETIME last_access_time = {0, 0};
    FILETIME last_write_time = {0, 0};
    BOOL rc = GetFileTime(handle, &creation_time, &last_access_time, &last_write_time);
    if (rc) {
        info.creation_time = windows_to_posix_time(creation_time);
        info.access_time = windows_to_posix_time(last_access_time);
        info.modification_time = windows_to_posix_time(last_write_time);
    } else {
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        info.result = FSResult::Unknown;
    }

    LARGE_INTEGER file_size;
    rc = GetFileSizeEx(handle, &file_size);
    if (rc) {
        info.file_size = file_size.QuadPart;
    } else {
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        info.result = FSResult::Unknown;
    }

    info.result = FSResult::OK;
    this->set_last_result(FSResult::OK);
    return info;
}

FileInfo FileSystem_t::get_file_info(StringView path) {
    HANDLE handle = this->open_handle_for_read(path);
    if (handle == INVALID_HANDLE_VALUE) {
        FileInfo info;
        info.result = this->last_result();
        return info;
    }

    FileInfo info = this->get_file_info(handle);
    CloseHandle(handle);
    return info;
}

#elif PLY_TARGET_POSIX

//  ▄▄▄▄▄   ▄▄▄▄   ▄▄▄▄  ▄▄▄▄ ▄▄  ▄▄
//  ██  ██ ██  ██ ██  ▀▀  ██  ▀█▄▄█▀
//  ██▀▀▀  ██  ██  ▀▀▀█▄  ██   ▄██▄
//  ██     ▀█▄▄█▀ ▀█▄▄█▀ ▄██▄ ██  ██
//

#define PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS 0

Array<FileInfo> FileSystem_t::list_dir(StringView path, u32 flags) {
    Array<FileInfo> result;

    DIR* dir = opendir(path.with_null_terminator().bytes);
    if (!dir) {
        switch (errno) {
            case ENOENT: {
                this->set_last_result(FSResult::NotFound);
                return result;
            }
            case EACCES: {
                this->set_last_result(FSResult::AccessDenied);
                return result;
            }
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                this->set_last_result(FSResult::Unknown);
                return result;
            }
        }
    }

    while (true) {
        errno = 0;
        struct dirent* rde = readdir(dir_impl->dir);
        if (!rde) {
            if (errno == 0) {
                this->set_last_result(FSResult::OK);
            } else {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                this->set_last_result(FSResult::Unknown);
            }
            break;
        }

        FileInfo info;
        info.name = rde->d_name;

        // d_type is not POSIX, but it exists on OSX and Linux.
        if (rde->d_type == DT_REG) {
            info.is_dir = false;
        } else if (rde->d_type == DT_DIR) {
            if (rde->d_name[0] == '.') {
                if (rde->d_name[1] == 0 ||
                    (rde->d_name[1] == '.' && rde->d_name[2] == 0))
                    continue;
            }
            info.is_dir = true;
        }

        if (dir_impl->flags != 0) {
            // Get additional information requested by flags
            String joined_path = PosixPath.join(path, info.name);
            struct stat buf;
            int rc = stat(joined_path.with_null_terminator().bytes, &buf);
            if (rc != 0) {
                if (errno == ENOENT)
                    continue;
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::set_last_result(FSResult::Unknown);
                break;
            }

            if (!info.is_dir && (dir_impl->flags & FileSystem::WithSizes) != 0) {
                info.file_size = buf.st_size;
            }
            if ((dir_impl->flags & FileSystem::WithTimes) != 0) {
                info.creation_time = buf.st_ctime;
                info.access_time = buf.st_atime;
                info.modification_time = buf.st_mtime;
            }
        }

        result.append(std::move(info));
    }

    closedir(dir_impl->dir);
    return result;
}

FSResult FileSystem_t::make_dir(StringView path) {
    int rc = mkdir(path.with_null_terminator().bytes, mode_t(0755));
    if (rc == 0) {
        return FileSystem::set_last_result(FSResult::OK);
    } else {
        switch (errno) {
            case EEXIST:
            case EISDIR: {
                return FileSystem::set_last_result(FSResult::AlreadyExists);
            }
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::set_last_result(FSResult::Unknown);
            }
        }
    }
}

FSResult FileSystem_t::set_working_directory(StringView path) {
    int rc = chdir(path.with_null_terminator().bytes);
    if (rc == 0) {
        return FileSystem::set_last_result(FSResult::OK);
    } else {
        switch (errno) {
            case ENOENT:
                return FileSystem::set_last_result(FSResult::NotFound);
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::set_last_result(FSResult::Unknown);
            }
        }
    }
}

PLY_NO_INLINE String FileSystem_t::get_working_directory() {
    u32 num_units_with_null_term = PATH_MAX + 1;
    String path = String::allocate(num_units_with_null_term);
    for (;;) {
        char* rs = getcwd(path.bytes, num_units_with_null_term);
        if (rs) {
            s32 len = path.find_byte('\0');
            PLY_ASSERT(len >= 0);
            path.resize(len);
            FileSystem::set_last_result(FSResult::OK);
            return path;
        } else {
            switch (errno) {
                case ERANGE: {
                    num_units_with_null_term *= 2;
                    path.resize(num_units_with_null_term);
                    break;
                }
                default: {
                    PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                    FileSystem::set_last_result(FSResult::Unknown);
                    return {};
                }
            }
        }
    }
}

PLY_NO_INLINE ExistsResult FileSystem_t::exists(StringView path) {
    struct stat buf;
    int rc = stat(path.with_null_terminator().bytes, &buf);
    if (rc == 0)
        return (buf.st_mode & S_IFMT) == S_IFDIR ? ExistsResult::Directory
                                                 : ExistsResult::File;
    if (errno != ENOENT) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
    }
    return ExistsResult::NotFound;
}

PLY_NO_INLINE int FileSystem_t::open_fdfor_read(StringView path) {
    int fd = open(path.with_null_terminator().bytes, O_RDONLY | O_CLOEXEC);
    if (fd != -1) {
        FileSystem::set_last_result(FSResult::OK);
    } else {
        switch (errno) {
            case ENOENT:
                FileSystem::set_last_result(FSResult::NotFound);
                break;

            case EACCES:
                FileSystem::set_last_result(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::set_last_result(FSResult::Unknown);
                break;
        }
    }
    return fd;
}

PLY_NO_INLINE Owned<InPipe> FileSystem_t::open_pipe_for_read(StringView path) {
    int fd = open_fdfor_read(path);
    if (fd == -1)
        return nullptr;
    return new InPipe_FD{fd};
}

PLY_NO_INLINE int FileSystem_t::open_fdfor_write(StringView path) {
    int fd = open(path.with_null_terminator().bytes,
                  O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode_t(0644));
    if (fd != -1) {
        FileSystem::set_last_result(FSResult::OK);
    } else {
        switch (errno) {
            case ENOENT:
                FileSystem::set_last_result(FSResult::NotFound);
                break;

            case EACCES:
                FileSystem::set_last_result(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::set_last_result(FSResult::Unknown);
                break;
        }
    }
    return fd;
}

PLY_NO_INLINE Owned<OutPipe> FileSystem_t::open_pipe_for_write(StringView path) {
    int fd = open_fdfor_write(path);
    if (fd == -1)
        return nullptr;
    return new OutPipe_FD{fd};
}

PLY_NO_INLINE FSResult FileSystem_t::move_file(StringView src_path,
                                               StringView dst_path) {
    int rc = rename(src_path.with_null_terminator().bytes,
                    dst_path.with_null_terminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::set_last_result(FSResult::Unknown);
    }
    return FileSystem::set_last_result(FSResult::OK);
}

PLY_NO_INLINE FSResult FileSystem_t::delete_file(StringView path) {
    int rc = unlink(path.with_null_terminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::set_last_result(FSResult::Unknown);
    }
    return FileSystem::set_last_result(FSResult::OK);
}

PLY_NO_INLINE FSResult FileSystem_t::remove_dir_tree(FileSystem* fs,
                                                     StringView dir_path) {
    for (const DirectoryEntry& dir_entry : fs->list_dir(dir_path)) {
        String joined = PosixPath.join(dir_path, dir_entry.name);
        if (dir_entry.is_dir) {
            FSResult fs_result = fs->remove_dir_tree(joined);
            if (fs_result != FSResult::OK) {
                return fs_result;
            }
        } else {
            int rc = unlink(joined.with_null_terminator().bytes);
            if (rc != 0) {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::set_last_result(FSResult::Unknown);
            }
        }
    }
    int rc = rmdir(dir_path.with_null_terminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::set_last_result(FSResult::Unknown);
    }
    return FileSystem::set_last_result(FSResult::OK);
}

PLY_NO_INLINE FileInfo FileSystem_t::get_file_status(StringView path) {
    FileInfo info;
    struct stat buf;
    int rc = stat(path.with_null_terminator().bytes, &buf);
    if (rc != 0) {
        switch (errno) {
            case ENOENT: {
                info.result = FileSystem::set_last_result(FSResult::NotFound);
                break;
            }
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::set_last_result(FSResult::Unknown);
                break;
            }
        }
    } else {
        info.result = FileSystem::set_last_result(FSResult::OK);
        info.file_size = buf.st_size;
        info.creation_time = buf.st_ctime;
        info.access_time = buf.st_atime;
        info.modification_time = buf.st_mtime;
    }
    return info;
}

#endif

// ┏━━━━━━━━━━━━━━━━━━━━━━┓
// ┃  get_workspace_path  ┃
// ┗━━━━━━━━━━━━━━━━━━━━━━┛
String get_workspace_path() {
    String path = FileSystem.get_working_directory();
    static const StringView file_name = "workspace-settings.pylon";
    String settings_path;

    // Search each parent directory for workspace-settings.pylon:
    while (true) {
        settings_path = Path.join(path, file_name);
        if (FileSystem.exists(settings_path) == ExistsResult::File)
            break;
        String next_dir = Path.split(path).first;
        if (path == next_dir) {
            // We've reached the topmost directory.
            Error.log("Can't locate {}", file_name);
            exit(1);
        }
        path = next_dir;
    }

    return path;
}

} // namespace ply
