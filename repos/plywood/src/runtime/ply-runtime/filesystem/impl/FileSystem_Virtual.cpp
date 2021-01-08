/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/filesystem/FileSystem.h>
#include <ply-runtime/io/Pipe.h>

namespace ply {

struct FileSystem_Virtual : FileSystem {
    FileSystem* targetFS = nullptr;
    String targetRoot;

    FileSystem_Virtual();

    PLY_NO_INLINE String convertToTargetPath(StringView path) const {
        // FIXME: should also detect and prohibit ".."
        PathFormat pathFmt = this->targetFS->pathFormat();
        PLY_ASSERT(!pathFmt.isAbsolute(path));
        return pathFmt.join(this->targetRoot, PosixPath::from(pathFmt, path));
    }

    static PLY_NO_INLINE Directory listDir(FileSystem* fs_, StringView path, u32 flags) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->listDir(fs->convertToTargetPath(path), flags);
    }

    static PLY_NO_INLINE FSResult makeDir(FileSystem* fs_, StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->makeDir(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE FSResult setWorkingDirectory(FileSystem*, StringView) {
        PLY_ASSERT(0); // Unsupported
        return FSResult::NotFound;
    }

    static PLY_NO_INLINE String getWorkingDirectory(FileSystem*) {
        PLY_ASSERT(0); // Unsupported
        return {};
    }

    static PLY_NO_INLINE ExistsResult exists(FileSystem* fs_, StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->exists(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE Owned<InPipe> openPipeForRead(FileSystem* fs_, StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->openPipeForRead(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE Owned<OutPipe> openPipeForWrite(FileSystem* fs_, StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->openPipeForWrite(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE FSResult moveFile(FileSystem* fs_, StringView dstPath,
                                           StringView srcPath) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->moveFile(fs->convertToTargetPath(dstPath),
                                      fs->convertToTargetPath(srcPath));
    }

    static PLY_NO_INLINE FSResult deleteFile(FileSystem* fs_, StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->deleteFile(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE FSResult removeDirTree(FileSystem* fs_, StringView dirPath) {   
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->removeDirTree(fs->convertToTargetPath(dirPath));
    }

    static PLY_NO_INLINE FileStatus getFileStatus(FileSystem* fs_, StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->getFileStatus(fs->convertToTargetPath(path));
    }
};

FileSystem::Funcs FileSystemFuncs_Virtual = {
    {false},
    FileSystem_Virtual::listDir,
    FileSystem_Virtual::makeDir,
    FileSystem_Virtual::setWorkingDirectory,
    FileSystem_Virtual::getWorkingDirectory,
    FileSystem_Virtual::exists,
    FileSystem_Virtual::openPipeForRead,
    FileSystem_Virtual::openPipeForWrite,
    FileSystem_Virtual::moveFile,
    FileSystem_Virtual::deleteFile,
    FileSystem_Virtual::removeDirTree,
    FileSystem_Virtual::getFileStatus,
};

PLY_INLINE FileSystem_Virtual::FileSystem_Virtual() : FileSystem{&FileSystemFuncs_Virtual} {
}

Owned<FileSystem> FileSystem::createVirtual(StringView rootPath) {
    FileSystem_Virtual* fs = new FileSystem_Virtual;
    fs->targetFS = this;
    fs->targetRoot = rootPath;
    return fs;
}

} // namespace ply
