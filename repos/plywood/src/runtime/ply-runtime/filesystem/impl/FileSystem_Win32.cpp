/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_WIN32

#include <ply-runtime/filesystem/impl/FileSystem_Win32.h>
#include <ply-runtime/io/impl/Pipe_Win32.h>
#include <ply-runtime/io/text/TextConverter.h>
#include <shellapi.h>

#define PLY_FSWIN32_ALLOW_UKNOWN_ERRORS 0

namespace ply {

PLY_NO_INLINE void FileSystem_Win32::DirImpl::destructImpl(ply::Directory::Impl* dirImpl_) {
    FileSystem_Win32::DirImpl* dirImpl = static_cast<FileSystem_Win32::DirImpl*>(dirImpl_);
    if (dirImpl->hFind != INVALID_HANDLE_VALUE) {
        BOOL rc = FindClose(dirImpl->hFind);
        PLY_UNUSED(rc);
    }
}

PLY_INLINE double windowsToPosixTime(const FILETIME& fileTime) {
    return (u64(fileTime.dwHighDateTime) << 32 | fileTime.dwLowDateTime) / 10000000.0 -
           11644473600.0;
}

PLY_NO_INLINE void dirEntryFromFindData(FileSystem_Win32::DirImpl* dirImpl) {
    dirImpl->entry = {};
    WStringView fnView{dirImpl->findData.cFileName};
    dirImpl->entry.name = TextConverter::convert<UTF8, UTF16_Native>(fnView.stringView());
    dirImpl->entry.isDir = (dirImpl->findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (dirImpl->flags & FileSystem::WithSizes) {
        dirImpl->entry.fileSize =
            u64(dirImpl->findData.nFileSizeHigh) << 32 | dirImpl->findData.nFileSizeLow;
    }
    if (dirImpl->flags & FileSystem::WithTimes) {
        dirImpl->entry.creationTime = windowsToPosixTime(dirImpl->findData.ftCreationTime);
        dirImpl->entry.accessTime = windowsToPosixTime(dirImpl->findData.ftLastAccessTime);
        dirImpl->entry.modificationTime = windowsToPosixTime(dirImpl->findData.ftLastWriteTime);
    }
}

PLY_NO_INLINE FSResult FileSystem_Win32::DirImpl::begin(StringView path) {
    String pattern = WindowsPath::join(path, "*");
    this->hFind = FindFirstFileW(win32PathArg(pattern), &this->findData);
    if (this->hFind == INVALID_HANDLE_VALUE) {
        this->entry = {};
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME: {
                return FileSystem::setLastResult(FSResult::NotFound);
            }
            case ERROR_ACCESS_DENIED: {
                return FileSystem::setLastResult(FSResult::AccessDenied);
            }
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    } else {
        dirEntryFromFindData(this);
        while (this->entry.name == "." || this->entry.name == "..") {
            nextImpl(this);
        }
        return FileSystem::setLastResult(FSResult::OK);
    }
}

PLY_NO_INLINE FSResult FileSystem_Win32::DirImpl::nextImpl(ply::Directory::Impl* dirImpl_) {
    FileSystem_Win32::DirImpl* dirImpl = static_cast<FileSystem_Win32::DirImpl*>(dirImpl_);
    BOOL rc = FindNextFileW(dirImpl->hFind, &dirImpl->findData);
    if (rc) {
        dirEntryFromFindData(dirImpl);
        return FileSystem::setLastResult(FSResult::OK);
    } else {
        dirImpl->entry = {};
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_NO_MORE_FILES: {
                return FileSystem::setLastResult(FSResult::OK);
            }
            case ERROR_FILE_INVALID: {
                return FileSystem::setLastResult(FSResult::NotFound);
            }
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    }
}

PLY_NO_INLINE Directory FileSystem_Win32::listDir(FileSystem*, StringView path, u32 flags) {
    FileSystem_Win32::DirImpl* dirImpl = new FileSystem_Win32::DirImpl;
    dirImpl->flags = flags;
    dirImpl->begin(path);
    return {dirImpl};
}

PLY_NO_INLINE FSResult FileSystem_Win32::makeDir(FileSystem*, StringView path) {
    BOOL rc = CreateDirectoryW(win32PathArg(path), NULL);
    if (rc) {
        return FileSystem::setLastResult(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_ALREADY_EXISTS:
                return FileSystem::setLastResult(FSResult::AlreadyExists);
            case ERROR_ACCESS_DENIED:
                return FileSystem::setLastResult(FSResult::AccessDenied);
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    }
}

PLY_NO_INLINE FSResult FileSystem_Win32::setWorkingDirectory(FileSystem* fs_, StringView path) {
    FileSystem_Win32* fs = static_cast<FileSystem_Win32*>(fs_);
    BOOL rc;
    {
        // This RWLock is used to mitigate data race issues with SetCurrentDirectoryW:
        // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
        ExclusiveLockGuard<RWLock> guard{fs->workingDirLock};
        rc = SetCurrentDirectoryW(win32PathArg(path));
    }
    if (rc) {
        return FileSystem::setLastResult(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        switch (err) {
            case ERROR_PATH_NOT_FOUND:
                return FileSystem::setLastResult(FSResult::NotFound);
            default: {
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    }
}

PLY_NO_INLINE String FileSystem_Win32::getWorkingDirectory(FileSystem* fs_) {
    FileSystem_Win32* fs = static_cast<FileSystem_Win32*>(fs_);
    u32 numUnitsWithNullTerm = MAX_PATH + 1;
    for (;;) {
        WString win32Path = WString::allocate(numUnitsWithNullTerm);
        DWORD rc;
        {
            // This RWLock is used to mitigate data race issues with SetCurrentDirectoryW:
            // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
            SharedLockGuard<RWLock> guard{fs->workingDirLock};
            rc = GetCurrentDirectoryW(numUnitsWithNullTerm, (LPWSTR) win32Path.units);
        }
        if (rc == 0) {
            PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
            FileSystem::setLastResult(FSResult::Unknown);
            return {};
        }
        PLY_ASSERT(rc != numUnitsWithNullTerm);
        if (rc < numUnitsWithNullTerm) {
            // GetCurrentDirectoryW: If the function succeeds, the return value specifies the
            // number of characters that are written to the buffer, not including the
            // terminating null character.
            WStringView truncatedWin32Path = {win32Path.units, rc};
            if (truncatedWin32Path.numUnits >= 4 &&
                truncatedWin32Path.stringView().left(8) == StringView{(const char*) L"\\\\?\\", 8}) {
                // Drop leading "\\\\?\\":
                truncatedWin32Path.units += 4;
                truncatedWin32Path.numUnits -= 4;
            }
            FileSystem::setLastResult(FSResult::OK);
            return TextConverter::convert<UTF8, UTF16_Native>(truncatedWin32Path.stringView());
        }
        // GetCurrentDirectoryW: If the buffer that is pointed to by lpBuffer is not large
        // enough, the return value specifies the required size of the buffer, in characters,
        // including the null-terminating character.
        numUnitsWithNullTerm = rc;
    }
}

PLY_NO_INLINE ExistsResult FileSystem_Win32::exists(FileSystem*, StringView path) {
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

PLY_NO_INLINE HANDLE FileSystem_Win32::openHandleForRead(StringView path) {
    // Should this use FILE_SHARE_DELETE or FILE_SHARE_WRITE?
    HANDLE handle = CreateFileW(win32PathArg(path), GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        FileSystem::setLastResult(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME:
                FileSystem::setLastResult(FSResult::NotFound);
                break;

            case ERROR_SHARING_VIOLATION:
                FileSystem::setLastResult(FSResult::Locked);
                break;

            case ERROR_ACCESS_DENIED:
                FileSystem::setLastResult(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                FileSystem::setLastResult(FSResult::Unknown);
                break;
        }
    }
    return handle;
}

PLY_NO_INLINE Owned<InPipe> FileSystem_Win32::openPipeForRead(FileSystem*, StringView path) {
    HANDLE handle = openHandleForRead(path);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    return new InPipe_Win32{handle};
}

PLY_NO_INLINE HANDLE FileSystem_Win32::openHandleForWrite(StringView path) {
    // FIXME: Needs graceful handling of ERROR_SHARING_VIOLATION
    // Should this use FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE?
    HANDLE handle = CreateFileW(win32PathArg(path), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle != INVALID_HANDLE_VALUE) {
        FileSystem::setLastResult(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        switch (error) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_INVALID_NAME:
                FileSystem::setLastResult(FSResult::NotFound);
                break;

            case ERROR_SHARING_VIOLATION:
                FileSystem::setLastResult(FSResult::Locked);
                break;

            case ERROR_ACCESS_DENIED:
                FileSystem::setLastResult(FSResult::AccessDenied);
                break;

            default:
                PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
                FileSystem::setLastResult(FSResult::Unknown);
                break;
        }
    }
    return handle;
}

PLY_NO_INLINE Owned<OutPipe> FileSystem_Win32::openPipeForWrite(FileSystem*, StringView path) {
    HANDLE handle = openHandleForWrite(path);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    return new OutPipe_Win32{handle};
}

PLY_NO_INLINE FSResult FileSystem_Win32::moveFile(FileSystem*, StringView srcPath,
                                                  StringView dstPath) {
    BOOL rc = MoveFileExW(win32PathArg(srcPath), win32PathArg(dstPath), MOVEFILE_REPLACE_EXISTING);
    if (rc) {
        return FileSystem::setLastResult(FSResult::OK);
    } else {
        DWORD error = GetLastError();
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        return FileSystem::setLastResult(FSResult::Unknown);
    }
}

PLY_NO_INLINE FSResult FileSystem_Win32::deleteFile(FileSystem*, StringView path) {
    BOOL rc = DeleteFileW(win32PathArg(path));
    if (rc) {
        return FileSystem::setLastResult(FSResult::OK);
    } else {
        DWORD err = GetLastError();
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        return FileSystem::setLastResult(FSResult::Unknown);
    }
}

PLY_NO_INLINE FSResult FileSystem_Win32::removeDirTree(FileSystem* fs, StringView dirPath) {
    HybridString absPath = dirPath;
    if (!WindowsPath::isAbsolute(dirPath)) {
        absPath = WindowsPath::normalize(fs->funcs->getWorkingDirectory(fs), dirPath);
    }
    MemOutStream mout;
    StringView srcView = absPath.view();
    TextConverter::create<UTF16_Native, UTF8>().writeTo(&mout, &srcView, true);
    mout << StringView{"\0\0\0\0", 4}; // double null terminated
    WString wstr = WString::moveFromString(mout.moveToString());
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

PLY_NO_INLINE void FileSystem_Win32::getFileStatus(HANDLE handle, FileStatus& status) {
    FILETIME creationTime = {0, 0};
    FILETIME lastAccessTime = {0, 0};
    FILETIME lastWriteTime = {0, 0};
    BOOL rc = GetFileTime(handle, &creationTime, &lastAccessTime, &lastWriteTime);
    if (rc) {
        status.creationTime = windowsToPosixTime(creationTime);
        status.accessTime = windowsToPosixTime(lastAccessTime);
        status.modificationTime = windowsToPosixTime(lastWriteTime);
    } else {
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        status.result = FSResult::Unknown;
    }

    LARGE_INTEGER fileSize;
    rc = GetFileSizeEx(handle, &fileSize);
    if (rc) {
        status.fileSize = fileSize.QuadPart;
    } else {
        PLY_ASSERT(PLY_FSWIN32_ALLOW_UKNOWN_ERRORS);
        status.result = FSResult::Unknown;
    }

    FileSystem::setLastResult(status.result);
}

PLY_NO_INLINE FileStatus FileSystem_Win32::getFileStatus(FileSystem*, StringView path) {
    FileStatus status;
    HANDLE handle = openHandleForRead(path);
    status.result = FileSystem::lastResult_.load();
    if (handle != INVALID_HANDLE_VALUE) {
        getFileStatus(handle, status);
        BOOL rc = CloseHandle(handle);
        PLY_ASSERT(rc != 0);
        PLY_UNUSED(rc);
    }
    return status;
}

FileSystem::Funcs FileSystemFuncs_Win32 = {
    {true},
    FileSystem_Win32::listDir,
    FileSystem_Win32::makeDir,
    FileSystem_Win32::setWorkingDirectory,
    FileSystem_Win32::getWorkingDirectory,
    FileSystem_Win32::exists,
    FileSystem_Win32::openPipeForRead,
    FileSystem_Win32::openPipeForWrite,
    FileSystem_Win32::moveFile,
    FileSystem_Win32::deleteFile,
    FileSystem_Win32::removeDirTree,
    FileSystem_Win32::getFileStatus,
};

PLY_INLINE FileSystem_Win32::FileSystem_Win32() : FileSystem{&FileSystemFuncs_Win32} {
}

PLY_NO_INLINE FileSystem* FileSystem_Win32_native() {
    static FileSystem_Win32 fs;
    return &fs;
}

} // namespace ply

#endif // PLY_TARGET_WIN32
