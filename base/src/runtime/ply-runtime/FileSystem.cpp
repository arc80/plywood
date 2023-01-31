/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#include <ply-runtime/Precomp.h>
#include <ply-runtime/FileSystem.h>
#include <ply-runtime/io/Pipe.h>

#if PLY_TARGET_WIN32
#include <ply-runtime/string/WString.h>
#include <ply-runtime/string/TextEncoding.h>
#include <shellapi.h>
#else
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#endif

namespace ply {

ThreadLocal<FSResult> FileSystemIface::lastResult_;
FileSystem_t FileSystem;

//  ▄▄    ▄▄        ▄▄▄  ▄▄
//  ██ ▄▄ ██  ▄▄▄▄   ██  ██  ▄▄  ▄▄▄▄  ▄▄▄▄▄
//  ▀█▄██▄█▀  ▄▄▄██  ██  ██▄█▀  ██▄▄██ ██  ▀▀
//   ██▀▀██  ▀█▄▄██ ▄██▄ ██ ▀█▄ ▀█▄▄▄  ██
//

void FileSystemWalker::visit(StringView dirPath) {
    this->triple.dirPath = dirPath;
    this->triple.dirNames.clear();
    this->triple.files.clear();
    for (FileInfo& info : this->fs->listDir(dirPath)) {
        if (info.isDir) {
            this->triple.dirNames.append(std::move(info.name));
        } else {
            this->triple.files.append(std::move(info));
        }
    }
}

void FileSystemWalker::Iterator::operator++() {
    if (!this->walker->triple.dirNames.isEmpty()) {
        StackItem& item = this->walker->stack.append();
        item.path = std::move(this->walker->triple.dirPath);
        item.dirNames = std::move(this->walker->triple.dirNames);
        item.dirIndex = 0;
    } else {
        this->walker->triple.dirPath.clear();
        this->walker->triple.dirNames.clear();
        this->walker->triple.files.clear();
    }
    while (!this->walker->stack.isEmpty()) {
        StackItem& item = this->walker->stack.back();
        if (item.dirIndex < item.dirNames.numItems()) {
            this->walker->visit(this->walker->fs->pathFormat().join(
                item.path, item.dirNames[item.dirIndex]));
            item.dirIndex++;
            return;
        }
        this->walker->stack.pop();
    }
    // End of walk
    PLY_ASSERT(this->walker->triple.dirPath.isEmpty());
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

FSResult FileSystemIface::makeDirs(StringView path) {
    if (path == this->pathFormat().getDriveLetter(path)) {
        return this->setLastResult(FSResult::OK);
    }
    ExistsResult er = this->exists(path);
    if (er == ExistsResult::Directory) {
        return this->setLastResult(FSResult::AlreadyExists);
    } else if (er == ExistsResult::File) {
        return this->setLastResult(FSResult::AccessDenied);
    } else {
        auto split = this->pathFormat().split(path);
        if (!split.first.isEmpty() && !split.second.isEmpty()) {
            FSResult r = makeDirs(split.first);
            if (r != FSResult::OK && r != FSResult::AlreadyExists)
                return r;
        }
        return this->makeDir(path);
    }
}

InStream FileSystemIface::openStreamForRead(StringView path) {
    return this->openPipeForRead(path);
}

OutStream FileSystemIface::openStreamForWrite(StringView path) {
    return this->openPipeForWrite(path);
}

InStream FileSystemIface::openTextForRead(StringView path,
                                          const TextFormat& textFormat) {
    if (InStream in = this->openStreamForRead(path))
        return textFormat.createImporter(std::move(in));
    return {};
}

InStream FileSystemIface::openTextForReadAutodetect(StringView path,
                                                    TextFormat* out_format) {
    if (InStream in = this->openStreamForRead(path)) {
        TextFormat textFormat = TextFormat::autodetect(in);
        if (out_format) {
            *out_format = textFormat;
        }
        return textFormat.createImporter(std::move(in));
    }
    return {};
}

String FileSystemIface::loadBinary(StringView path) {
    String result;
    Owned<InPipe> inPipe = this->openPipeForRead(path);
    if (inPipe) {
        u64 fileSize = inPipe->get_file_size();
        // Files >= 4GB cannot be loaded this way:
        result.resize(safeDemote<u32>(fileSize));
        inPipe->read({result.bytes, result.numBytes});
    }
    return result;
}

String FileSystemIface::loadText(StringView path, const TextFormat& textFormat) {
    if (InStream in = this->openTextForRead(path, textFormat))
        return in.read_remaining_contents();
    return {};
}

String FileSystemIface::loadTextAutodetect(StringView path, TextFormat* out_format) {
    if (InStream in = this->openTextForReadAutodetect(path, out_format)) {
        return in.read_remaining_contents();
    }
    return {};
}

OutStream FileSystemIface::openTextForWrite(StringView path,
                                            const TextFormat& textFormat) {
    if (OutStream out = this->openStreamForWrite(path))
        return {textFormat.createExporter(std::move(out)).release(),
                true};
    return {};
}

FSResult FileSystemIface::makeDirsAndSaveBinaryIfDifferent(StringView path,
                                                           StringView view) {
    // Load existing contents.
    // FIXME: We shouldn't need to load the whole file in memory first. Instead, compare
    // as we go.
    String existingContents = this->loadBinary(path);
    FSResult existingResult = this->lastResult();
    if (existingResult == FSResult::OK && existingContents == view) {
        return this->setLastResult(FSResult::Unchanged);
    }
    if (existingResult != FSResult::OK && existingResult != FSResult::NotFound) {
        return existingResult;
    }
    existingContents.clear();

    // Create intermediate directories
    FSResult result = this->makeDirs(this->pathFormat().split(path).first);
    if (result != FSResult::OK && result != FSResult::AlreadyExists) {
        return result;
    }

    // Save file
    // FIXME: Write to temporary file first, then rename atomically
    Owned<OutPipe> outPipe = this->openPipeForWrite(path);
    result = this->lastResult();
    if (result != FSResult::OK) {
        return result;
    }
    outPipe->write(view);
    return result;
}

FSResult FileSystemIface::makeDirsAndSaveTextIfDifferent(StringView path,
                                                         StringView strContents,
                                                         const TextFormat& textFormat) {
    Owned<OutPipe> out = textFormat.createExporter(MemOutStream{});
    out->write(strContents);
    out->flush();
    String rawContents = out->get_tail_pipe()->child_stream.moveToString();
    return this->makeDirsAndSaveBinaryIfDifferent(path, rawContents);
}

#if PLY_TARGET_WIN32

//  ▄▄    ▄▄ ▄▄            ▄▄
//  ██ ▄▄ ██ ▄▄ ▄▄▄▄▄   ▄▄▄██  ▄▄▄▄  ▄▄    ▄▄  ▄▄▄▄
//  ▀█▄██▄█▀ ██ ██  ██ ██  ██ ██  ██ ██ ██ ██ ▀█▄▄▄
//   ██▀▀██  ██ ██  ██ ▀█▄▄██ ▀█▄▄█▀  ██▀▀██   ▄▄▄█▀
//

#define PLY_FSWIN32_ALLOW_UKNOWN_ERRORS 0

PLY_INLINE double windowsToPosixTime(const FILETIME& fileTime) {
    return (u64(fileTime.dwHighDateTime) << 32 | fileTime.dwLowDateTime) / 10000000.0 -
           11644473600.0;
}

void fileInfoFromData(FileInfo* info, WIN32_FIND_DATAW findData, u32 flags) {
    info->name = fromWString(findData.cFileName);
    info->isDir = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (flags & FileSystemIface::WithSizes) {
        info->fileSize = u64(findData.nFileSizeHigh) << 32 | findData.nFileSizeLow;
    }
    if (flags & FileSystemIface::WithTimes) {
        info->creationTime = windowsToPosixTime(findData.ftCreationTime);
        info->accessTime = windowsToPosixTime(findData.ftLastAccessTime);
        info->modificationTime = windowsToPosixTime(findData.ftLastWriteTime);
    }
}

Array<FileInfo> FileSystem_t::listDir(StringView path, u32 flags) {
    Array<FileInfo> result;
    HANDLE hFind = INVALID_HANDLE_VALUE;
    WIN32_FIND_DATAW findData;

    String pattern = WindowsPath.join(path, "*");
    hFind = FindFirstFileW(win32PathArg(pattern), &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME: {
                this->setLastResult(FSResult::NotFound);
                return result;
            }
            case ERROR_ACCESS_DENIED: {
                this->setLastResult(FSResult::AccessDenied);
                return result;
            }
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                this->setLastResult(FSResult::Unknown);
                return result;
            }
        }
    }

    while (true) {
        FileInfo info;
        fileInfoFromData(&info, findData, flags);
        if (info.name != "." && info.name != "..") {
            result.append(std::move(info));
        }

        BOOL rc = FindNextFileW(hFind, &findData);
        if (!rc) {
            DWORD err = GetLastError();
            switch (err) {
                case ERROR_NO_MORE_FILES: {
                    this->setLastResult(FSResult::OK);
                    return result;
                }
                case ERROR_FILE_INVALID: {
                    this->setLastResult(FSResult::NotFound);
                    return result;
                }
                default: {
                    PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                    this->setLastResult(FSResult::Unknown);
                    return result;
                }
            }
        }
    }
}

FSResult FileSystem_t::makeDir(StringView path) {
    BOOL rc = CreateDirectoryW(win32PathArg(path), NULL);
    if (rc) {
        return this->setLastResult(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_ALREADY_EXISTS:
                return this->setLastResult(FSResult::AlreadyExists);
            case ERROR_ACCESS_DENIED:
                return this->setLastResult(FSResult::AccessDenied);
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return this->setLastResult(FSResult::Unknown);
            }
        }
    }
}

Path_t FileSystem_t::pathFormat() {
    return Path;
}

FSResult FileSystem_t::setWorkingDirectory(StringView path) {
    BOOL rc;
    {
        // This ReadWriteLock is used to mitigate data race issues with SetCurrentDirectoryW:
        // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
        ExclusiveLockGuard<ReadWriteLock> guard{this->workingDirLock};
        rc = SetCurrentDirectoryW(win32PathArg(path));
    }
    if (rc) {
        return this->setLastResult(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_PATH_NOT_FOUND:
                return this->setLastResult(FSResult::NotFound);
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return this->setLastResult(FSResult::Unknown);
            }
        }
    }
}

String FileSystem_t::getWorkingDirectory() {
    u32 numUnitsWithNullTerm = MAX_PATH + 1;
    for (;;) {
        WString win32Path = WString::allocate(numUnitsWithNullTerm);
        DWORD rc;
        {
            // This ReadWriteLock is used to mitigate data race issues with
            // SetCurrentDirectoryW:
            // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
            SharedLockGuard<ReadWriteLock> guard{this->workingDirLock};
            rc = GetCurrentDirectoryW(numUnitsWithNullTerm, (LPWSTR) win32Path.units);
        }
        if (rc == 0) {
            PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
            this->setLastResult(FSResult::Unknown);
            return {};
        }
        PLY_ASSERT(rc != numUnitsWithNullTerm);
        if (rc < numUnitsWithNullTerm) {
            // GetCurrentDirectoryW: If the function succeeds, the return value
            // specifies the number of characters that are written to the buffer, not
            // including the terminating null character.
            WStringView truncatedWin32Path = {win32Path.units, rc};
            if (truncatedWin32Path.numUnits >= 4 &&
                truncatedWin32Path.raw_bytes().left(8) ==
                    StringView{(const char*) L"\\\\?\\", 8}) {
                // Drop leading "\\\\?\\":
                truncatedWin32Path.units += 4;
                truncatedWin32Path.numUnits -= 4;
            }
            this->setLastResult(FSResult::OK);
            return fromWString(truncatedWin32Path);
        }
        // GetCurrentDirectoryW: If the buffer that is pointed to by lpBuffer is not
        // large enough, the return value specifies the required size of the buffer, in
        // characters, including the null-terminating character.
        numUnitsWithNullTerm = rc;
    }
}

ExistsResult FileSystem_t::exists(StringView path) {
    // FIXME: Do something sensible when passed "C:" and other drive letters
    DWORD attribs = GetFileAttributesW(win32PathArg(path));
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

HANDLE FileSystem_t::openHandleForRead(StringView path) {
    // Should this use FILE_SHARE_DELETE or FILE_SHARE_WRITE?
    HANDLE handle = CreateFileW(win32PathArg(path), GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        this->setLastResult(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME:
                this->setLastResult(FSResult::NotFound);
                break;

            case ERROR_SHARING_VIOLATION:
                this->setLastResult(FSResult::Locked);
                break;

            case ERROR_ACCESS_DENIED:
                this->setLastResult(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                this->setLastResult(FSResult::Unknown);
                break;
        }
    }
    return handle;
}

Owned<InPipe> FileSystem_t::openPipeForRead(StringView path) {
    HANDLE handle = openHandleForRead(path);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    return new InPipe_Handle{handle};
}

HANDLE FileSystem_t::openHandleForWrite(StringView path) {
    // FIXME: Needs graceful handling of ERROR_SHARING_VIOLATION
    // Should this use FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE?
    HANDLE handle = CreateFileW(win32PathArg(path), GENERIC_WRITE, 0, NULL,
                                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        this->setLastResult(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME:
                this->setLastResult(FSResult::NotFound);
                break;

            case ERROR_SHARING_VIOLATION:
                this->setLastResult(FSResult::Locked);
                break;

            case ERROR_ACCESS_DENIED:
                this->setLastResult(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                this->setLastResult(FSResult::Unknown);
                break;
        }
    }
    return handle;
}

Owned<OutPipe> FileSystem_t::openPipeForWrite(StringView path) {
    HANDLE handle = openHandleForWrite(path);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    return new OutPipe_Handle{handle};
}

FSResult FileSystem_t::moveFile(StringView srcPath, StringView dstPath) {
    BOOL rc = MoveFileExW(win32PathArg(srcPath), win32PathArg(dstPath),
                          MOVEFILE_REPLACE_EXISTING);
    if (rc) {
        return this->setLastResult(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        return this->setLastResult(FSResult::Unknown);
    }
}

FSResult FileSystem_t::deleteFile(StringView path) {
    BOOL rc = DeleteFileW(win32PathArg(path));
    if (rc) {
        return this->setLastResult(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        return this->setLastResult(FSResult::Unknown);
    }
}

FSResult FileSystem_t::removeDirTree(StringView dirPath) {
    HybridString absPath = dirPath;
    if (!WindowsPath.isAbsolute(dirPath)) {
        absPath = WindowsPath.join(this->getWorkingDirectory(), dirPath);
    }
    OutPipe_ConvertUnicode out{MemOutStream{}, UTF16_LE};
    out.write(absPath.view());
    out.child_stream << StringView{"\0\0\0\0", 4}; // double null terminated
    WString wstr = WString::moveFromString(out.child_stream.moveToString());
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

FileInfo FileSystem_t::getFileInfo(HANDLE handle) {
    FileInfo info;
    FILETIME creationTime = {0, 0};
    FILETIME lastAccessTime = {0, 0};
    FILETIME lastWriteTime = {0, 0};
    BOOL rc = GetFileTime(handle, &creationTime, &lastAccessTime, &lastWriteTime);
    if (rc) {
        info.creationTime = windowsToPosixTime(creationTime);
        info.accessTime = windowsToPosixTime(lastAccessTime);
        info.modificationTime = windowsToPosixTime(lastWriteTime);
    } else {
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        info.result = FSResult::Unknown;
    }

    LARGE_INTEGER fileSize;
    rc = GetFileSizeEx(handle, &fileSize);
    if (rc) {
        info.fileSize = fileSize.QuadPart;
    } else {
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        info.result = FSResult::Unknown;
    }

    info.result = FSResult::OK;
    this->setLastResult(FSResult::OK);
    return info;
}

FileInfo FileSystem_t::getFileInfo(StringView path) {
    HANDLE handle = this->openHandleForRead(path);
    if (handle == INVALID_HANDLE_VALUE) {
        FileInfo info;
        info.result = this->lastResult();
        return info;
    }

    FileInfo info = this->getFileInfo(handle);
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

Array<FileInfo> FileSystem_t::listDir(StringView path, u32 flags) {
    Array<FileInfo> result;

    DIR* dir = opendir(path.withNullTerminator().bytes);
    if (!dir) {
        switch (errno) {
            case ENOENT: {
                this->setLastResult(FSResult::NotFound);
                return result;
            }
            case EACCES: {
                this->setLastResult(FSResult::AccessDenied);
                return result;
            }
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                this->setLastResult(FSResult::Unknown);
                return result;
            }
        }
    }

    while (true) {
        errno = 0;
        struct dirent* rde = readdir(dirImpl->dir);
        if (!rde) {
            if (errno == 0) {
                this->setLastResult(FSResult::OK);
            } else {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                this->setLastResult(FSResult::Unknown);
            }
            break;
        }

        FileInfo info;
        info.name = rde->d_name;

        // d_type is not POSIX, but it exists on OSX and Linux.
        if (rde->d_type == DT_REG) {
            info.isDir = false;
        } else if (rde->d_type == DT_DIR) {
            if (rde->d_name[0] == '.') {
                if (rde->d_name[1] == 0 ||
                    (rde->d_name[1] == '.' && rde->d_name[2] == 0))
                    continue;
            }
            info.isDir = true;
        }

        if (dirImpl->flags != 0) {
            // Get additional information requested by flags
            String joinedPath = PosixPath.join(path, info.name);
            struct stat buf;
            int rc = stat(joinedPath.withNullTerminator().bytes, &buf);
            if (rc != 0) {
                if (errno == ENOENT)
                    continue;
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::setLastResult(FSResult::Unknown);
                break;
            }

            if (!info.isDir && (dirImpl->flags & FileSystem::WithSizes) != 0) {
                info.fileSize = buf.st_size;
            }
            if ((dirImpl->flags & FileSystem::WithTimes) != 0) {
                info.creationTime = buf.st_ctime;
                info.accessTime = buf.st_atime;
                info.modificationTime = buf.st_mtime;
            }
        }

        result.append(std::move(info));
    }

    closedir(dirImpl->dir);
    return result;
}

FSResult FileSystem_t::makeDir(StringView path) {
    int rc = mkdir(path.withNullTerminator().bytes, mode_t(0755));
    if (rc == 0) {
        return FileSystem::setLastResult(FSResult::OK);
    } else {
        switch (errno) {
            case EEXIST:
            case EISDIR: {
                return FileSystem::setLastResult(FSResult::AlreadyExists);
            }
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    }
}

FSResult FileSystem_t::setWorkingDirectory(StringView path) {
    int rc = chdir(path.withNullTerminator().bytes);
    if (rc == 0) {
        return FileSystem::setLastResult(FSResult::OK);
    } else {
        switch (errno) {
            case ENOENT:
                return FileSystem::setLastResult(FSResult::NotFound);
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    }
}

PLY_NO_INLINE String FileSystem_t::getWorkingDirectory() {
    u32 numUnitsWithNullTerm = PATH_MAX + 1;
    String path = String::allocate(numUnitsWithNullTerm);
    for (;;) {
        char* rs = getcwd(path.bytes, numUnitsWithNullTerm);
        if (rs) {
            s32 len = path.findByte('\0');
            PLY_ASSERT(len >= 0);
            path.resize(len);
            FileSystem::setLastResult(FSResult::OK);
            return path;
        } else {
            switch (errno) {
                case ERANGE: {
                    numUnitsWithNullTerm *= 2;
                    path.resize(numUnitsWithNullTerm);
                    break;
                }
                default: {
                    PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                    FileSystem::setLastResult(FSResult::Unknown);
                    return {};
                }
            }
        }
    }
}

PLY_NO_INLINE ExistsResult FileSystem_t::exists(StringView path) {
    struct stat buf;
    int rc = stat(path.withNullTerminator().bytes, &buf);
    if (rc == 0)
        return (buf.st_mode & S_IFMT) == S_IFDIR ? ExistsResult::Directory
                                                 : ExistsResult::File;
    if (errno != ENOENT) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
    }
    return ExistsResult::NotFound;
}

PLY_NO_INLINE int FileSystem_t::openFDForRead(StringView path) {
    int fd = open(path.withNullTerminator().bytes, O_RDONLY | O_CLOEXEC);
    if (fd != -1) {
        FileSystem::setLastResult(FSResult::OK);
    } else {
        switch (errno) {
            case ENOENT:
                FileSystem::setLastResult(FSResult::NotFound);
                break;

            case EACCES:
                FileSystem::setLastResult(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::setLastResult(FSResult::Unknown);
                break;
        }
    }
    return fd;
}

PLY_NO_INLINE Owned<InPipe> FileSystem_t::openPipeForRead(StringView path) {
    int fd = openFDForRead(path);
    if (fd == -1)
        return nullptr;
    return new InPipe_FD{fd};
}

PLY_NO_INLINE int FileSystem_t::openFDForWrite(StringView path) {
    int fd = open(path.withNullTerminator().bytes,
                  O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode_t(0644));
    if (fd != -1) {
        FileSystem::setLastResult(FSResult::OK);
    } else {
        switch (errno) {
            case ENOENT:
                FileSystem::setLastResult(FSResult::NotFound);
                break;

            case EACCES:
                FileSystem::setLastResult(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::setLastResult(FSResult::Unknown);
                break;
        }
    }
    return fd;
}

PLY_NO_INLINE Owned<OutPipe> FileSystem_t::openPipeForWrite(StringView path) {
    int fd = openFDForWrite(path);
    if (fd == -1)
        return nullptr;
    return new OutPipe_FD{fd};
}

PLY_NO_INLINE FSResult FileSystem_t::moveFile(StringView srcPath, StringView dstPath) {
    int rc =
        rename(srcPath.withNullTerminator().bytes, dstPath.withNullTerminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::setLastResult(FSResult::Unknown);
    }
    return FileSystem::setLastResult(FSResult::OK);
}

PLY_NO_INLINE FSResult FileSystem_t::deleteFile(StringView path) {
    int rc = unlink(path.withNullTerminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::setLastResult(FSResult::Unknown);
    }
    return FileSystem::setLastResult(FSResult::OK);
}

PLY_NO_INLINE FSResult FileSystem_t::removeDirTree(FileSystem* fs, StringView dirPath) {
    for (const DirectoryEntry& dirEntry : fs->listDir(dirPath)) {
        String joined = PosixPath.join(dirPath, dirEntry.name);
        if (dirEntry.isDir) {
            FSResult fsResult = fs->removeDirTree(joined);
            if (fsResult != FSResult::OK) {
                return fsResult;
            }
        } else {
            int rc = unlink(joined.withNullTerminator().bytes);
            if (rc != 0) {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    }
    int rc = rmdir(dirPath.withNullTerminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::setLastResult(FSResult::Unknown);
    }
    return FileSystem::setLastResult(FSResult::OK);
}

PLY_NO_INLINE FileInfo FileSystem_t::getFileStatus(StringView path) {
    FileInfo info;
    struct stat buf;
    int rc = stat(path.withNullTerminator().bytes, &buf);
    if (rc != 0) {
        switch (errno) {
            case ENOENT: {
                info.result = FileSystem::setLastResult(FSResult::NotFound);
                break;
            }
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::setLastResult(FSResult::Unknown);
                break;
            }
        }
    } else {
        info.result = FileSystem::setLastResult(FSResult::OK);
        info.fileSize = buf.st_size;
        info.creationTime = buf.st_ctime;
        info.accessTime = buf.st_atime;
        info.modificationTime = buf.st_mtime;
    }
    return info;
}

#endif

} // namespace ply
