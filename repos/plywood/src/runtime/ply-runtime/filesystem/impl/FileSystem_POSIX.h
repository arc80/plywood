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
        FSResult begin(const StringView path);
    };

    // More direct access:
    static int openFDForRead(const StringView path);
    static int openFDForWrite(const StringView path);

    // FileSystem::Funcs implementations:
    static Directory listDir(FileSystem*, const StringView path, u32 flags);
    static FSResult makeDir(FileSystem*, const StringView path);
    static String getWorkingDirectory(FileSystem*);
    static FSResult setWorkingDirectory(FileSystem* fs_, const StringView path);
    static ExistsResult exists(FileSystem*, const StringView path);
    static Owned<InPipe> openPipeForRead(FileSystem*, const StringView path);
    static Owned<OutPipe> openPipeForWrite(FileSystem*, const StringView path);
    static FSResult moveFile(FileSystem*, const StringView srcPath, const StringView dstPath);
    static FSResult deleteFile(FileSystem*, const StringView path);
    static FSResult removeDirTree(FileSystem*, const StringView dirPath);
    static FileStatus getFileStatus(FileSystem*, const StringView path);

    FileSystem_POSIX();
};

} // namespace ply
