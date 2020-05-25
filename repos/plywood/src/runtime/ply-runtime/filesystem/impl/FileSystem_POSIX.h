/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/filesystem/FileSystem.h>
#include <dirent.h>

namespace ply {

struct FileSystem_POSIX : FileSystem {
    struct DirImpl : Directory::Impl {
        String dirPath;
        DIR* dir = nullptr;

        static void destructImpl(Directory::Impl*);
        static FSResult nextImpl(Directory::Impl*);

        PLY_INLINE DirImpl() {
            this->destruct = destructImpl;
            this->next = nextImpl;
        }
        ~DirImpl() = delete; // Should be deleted through parent class
        FSResult begin(StringView path);
    };

    // More direct access:
    static int openFDForRead(StringView path);
    static int openFDForWrite(StringView path);

    // FileSystem::Funcs implementations:
    static Directory listDir(FileSystem*, StringView path, u32 flags);
    static FSResult makeDir(FileSystem*, StringView path);
    static String getWorkingDirectory(FileSystem*);
    static FSResult setWorkingDirectory(FileSystem* fs_, StringView path);
    static ExistsResult exists(FileSystem*, StringView path);
    static Owned<InPipe> openPipeForRead(FileSystem*, StringView path);
    static Owned<OutPipe> openPipeForWrite(FileSystem*, StringView path);
    static FSResult moveFile(FileSystem*, StringView srcPath, StringView dstPath);
    static FSResult deleteFile(FileSystem*, StringView path);
    static FSResult removeDirTree(FileSystem*, StringView dirPath);
    static FileStatus getFileStatus(FileSystem*, StringView path);

    FileSystem_POSIX();
};

} // namespace ply
