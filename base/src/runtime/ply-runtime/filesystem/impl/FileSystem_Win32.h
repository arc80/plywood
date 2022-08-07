/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/filesystem/FileSystem.h>
#include <ply-runtime/thread/RWLock.h>

namespace ply {

struct FileSystem_Win32 : FileSystem {
    struct DirImpl : Directory::Impl {
        HANDLE hFind = INVALID_HANDLE_VALUE;
        WIN32_FIND_DATAW findData;

        static void destructImpl(Directory::Impl*);
        static FSResult nextImpl(Directory::Impl*);

        PLY_INLINE DirImpl() {
            this->destruct = destructImpl;
            this->next = nextImpl;
        }
        ~DirImpl() = delete; // Should be deleted through parent class
        FSResult begin(StringView path);
    };

    // This RWLock is used to mitigate data race issues with SetCurrentDirectoryW:
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
    RWLock workingDirLock;

    // More direct access:
    static HANDLE openHandleForRead(StringView path);
    static HANDLE openHandleForWrite(StringView path);
    static void getFileStatus(HANDLE handle, FileStatus& status);

    // FileSystem::Funcs implementations:
    static Directory listDir(FileSystem*, StringView path, u32 flags);
    static FSResult makeDir(FileSystem*, StringView path);
    static String getWorkingDirectory(FileSystem* fs_);
    static FSResult setWorkingDirectory(FileSystem* fs_, StringView path);
    static ExistsResult exists(FileSystem*, StringView path);
    static Owned<InPipe> openPipeForRead(FileSystem*, StringView path);
    static Owned<OutPipe> openPipeForWrite(FileSystem*, StringView path);
    static FSResult moveFile(FileSystem*, StringView srcPath, StringView dstPath);
    static FSResult deleteFile(FileSystem*, StringView path);
    static FSResult removeDirTree(FileSystem*, StringView dirPath);
    static FileStatus getFileStatus(FileSystem*, StringView path);

    FileSystem_Win32();
};

PLY_DLL_ENTRY FileSystem* FileSystem_Win32_get();

} // namespace ply
