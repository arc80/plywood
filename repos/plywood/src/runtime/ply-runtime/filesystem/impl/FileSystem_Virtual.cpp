/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>
#include <ply-runtime/filesystem/FileSystem.h>
#include <ply-runtime/io/Pipe.h>

#if 0
namespace ply {
namespace details {

struct FileSystem_Virtual : FileSystem {
    FileSystem* targetFS = nullptr;
    String targetRoot;

    FileSystem_Virtual(const PathFormat& pathFmt);

    PLY_NO_INLINE String convertToTargetPath(const StringView path) const {
        if (targetRoot.isEmpty()) {
            return targetFS->pathFmt.convertFrom(pathFmt, path).view();
        } else {
            // If there's a targetRoot, only accept relative paths
            // FIXME: should also detect and prohibit ".."
            PLY_ASSERT(!pathFmt.isAbsolute(path));
            return targetFS->pathFmt.join(targetRoot, targetFS->pathFmt.convertFrom(pathFmt, path));
        }
    }

    static PLY_NO_INLINE Directory listDir(FileSystem* fs_, const StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->listDir(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE OpenResult makeDirImpl(FileSystem* fs_, const StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->makeDir(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE String getWorkingDirectoryImpl(const FileSystem* fs_) {
        PLY_ASSERT(0); // Unsupported
        return {};
    }

    static PLY_NO_INLINE ExistsResult existsImpl(const FileSystem* fs_, const StringView path) {
        const FileSystem_Virtual* fs = static_cast<const FileSystem_Virtual*>(fs_);
        return fs->targetFS->exists(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE Tuple<Owned<InPipe>, OpenResult> openPipeForReadImpl(FileSystem* fs_,
                                                                              const StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->openPipeForRead(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE Tuple<Owned<InStream>, OpenResult> openForReadImpl(FileSystem* fs_,
                                                                            const StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->openForRead(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE Tuple<Owned<OutPipe>, OpenResult> openPipeForWriteImpl(FileSystem* fs_,
                                                                                const StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->openPipeForWrite(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE Tuple<Owned<OutStream>, OpenResult> openForWriteImpl(FileSystem* fs_,
                                                                              const StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->openForWrite(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE OpenResult replaceFileImpl(FileSystem* fs_, const StringView dstPath,
                                                    const StringView srcPath) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->moveFile(fs->convertToTargetPath(dstPath),
                                         fs->convertToTargetPath(srcPath));
    }

    static PLY_NO_INLINE OpenResult deleteFileImpl(FileSystem* fs_, const StringView path) {
        FileSystem_Virtual* fs = static_cast<FileSystem_Virtual*>(fs_);
        return fs->targetFS->deleteFile(fs->convertToTargetPath(path));
    }

    static PLY_NO_INLINE Tuple<FileStatus, OpenResult> getFileStatusImpl(const FileSystem* fs_,
                                                                         const StringView path) {
        const FileSystem_Virtual* fs = static_cast<const FileSystem_Virtual*>(fs_);
        return fs->targetFS->getFileStatus(fs->convertToTargetPath(path));
    }
};

// clang-format off
FileSystem::Funcs FileSystem_Virtual_Funcs = {
    FileSystem_Virtual::listDir,
    FileSystem_Virtual::makeDirImpl,
    FileSystem_Virtual::getWorkingDirectoryImpl,
    FileSystem_Virtual::existsImpl,
    FileSystem_Virtual::openPipeForReadImpl,
    FileSystem_Virtual::openForReadImpl,
    FileSystem_Virtual::openPipeForWriteImpl,
    FileSystem_Virtual::openForWriteImpl,
    FileSystem_Virtual::replaceFileImpl,
    FileSystem_Virtual::deleteFileImpl,
    FileSystem_Virtual::getFileStatusImpl,
};
// clang-format on

PLY_INLINE FileSystem_Virtual::FileSystem_Virtual(const PathFormat& pathFmt)
    : FileSystem{&FileSystem_Virtual_Funcs, pathFmt} {
}

} // namespace details

Owned<FileSystem> createVirtualFileSystem(FileSystem* targetFS, const PathFormat& pathFmt,
                                          const StringView targetRoot) {
    details::FileSystem_Virtual* fs = new details::FileSystem_Virtual{pathFmt};
    fs->targetFS = targetFS;
    fs->targetRoot = targetRoot;
    return fs;
}

} // namespace ply
#endif
