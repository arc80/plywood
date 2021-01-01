/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#include <ply-runtime/Precomp.h>

#if PLY_TARGET_POSIX

#include <ply-runtime/filesystem/impl/FileSystem_POSIX.h>
#include <ply-runtime/io/impl/Pipe_FD.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>

#define PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS 0

namespace ply {

PLY_NO_INLINE void FileSystem_POSIX::DirImpl::destructImpl(ply::Directory::Impl* dirImpl_) {
    FileSystem_POSIX::DirImpl* dirImpl = static_cast<FileSystem_POSIX::DirImpl*>(dirImpl_);
    if (dirImpl->dir) {
        int rc = closedir(dirImpl->dir);
        PLY_ASSERT(rc == 0);
        PLY_UNUSED(rc);
    }
}

PLY_NO_INLINE FSResult FileSystem_POSIX::DirImpl::begin(const StringView path) {
    this->dirPath = path.withoutNullTerminator();
    this->dir = opendir(path.withNullTerminator().bytes);
    if (!this->dir) {
        this->entry = {};
        switch (errno) {
            case ENOENT:
                return FileSystem::setLastResult(FSResult::NotFound);
            case EACCES:
                return FileSystem::setLastResult(FSResult::AccessDenied);
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        }
    }
    return nextImpl(this);
}

PLY_NO_INLINE FSResult FileSystem_POSIX::DirImpl::nextImpl(ply::Directory::Impl* dirImpl_) {
    FileSystem_POSIX::DirImpl* dirImpl = static_cast<FileSystem_POSIX::DirImpl*>(dirImpl_);
    for (;;) {
        errno = 0;
        struct dirent* rde = readdir(dirImpl->dir);
        dirImpl->entry = {};
        if (!rde) {
            if (errno == 0) {
                return FileSystem::setLastResult(FSResult::OK);
            } else {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                return FileSystem::setLastResult(FSResult::Unknown);
            }
        } else {
            dirImpl->entry.name = rde->d_name;

            // d_type is not POSIX, but it exists on OSX and Linux.
            if (rde->d_type == DT_REG) {
                dirImpl->entry.isDir = false;
            } else if (rde->d_type == DT_DIR) {
                if (rde->d_name[0] == '.') {
                    if (rde->d_name[1] == 0 || (rde->d_name[1] == '.' && rde->d_name[2] == 0))
                        continue;
                }
                dirImpl->entry.isDir = true;
            }

            if (dirImpl->flags != 0) {
                // Get additional information requested by flags
                String joinedPath = PosixPath::join(dirImpl->dirPath, dirImpl->entry.name);
                struct stat buf;
                int rc = stat(joinedPath.withNullTerminator().bytes, &buf);
                if (rc != 0) {
                    switch (errno) {
                        case ENOENT: {
                            goto skipEntry;
                        }
                        default: {
                            PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                            FileSystem::setLastResult(FSResult::Unknown);
                            break;
                        }
                    }
                } else {
                    if (!dirImpl->entry.isDir && (dirImpl->flags & FileSystem::WithSizes) != 0) {
                        dirImpl->entry.fileSize = buf.st_size;
                    }
                    if ((dirImpl->flags & FileSystem::WithTimes) != 0) {
                        dirImpl->entry.creationTime = buf.st_ctime;
                        dirImpl->entry.accessTime = buf.st_atime;
                        dirImpl->entry.modificationTime = buf.st_mtime;
                    }
                }
            }

            return FileSystem::setLastResult(FSResult::OK);
        }
    skipEntry:;
    }
}

PLY_NO_INLINE Directory FileSystem_POSIX::listDir(FileSystem*, const StringView path, u32 flags) {
    FileSystem_POSIX::DirImpl* dirImpl = new FileSystem_POSIX::DirImpl;
    dirImpl->flags = flags;
    dirImpl->begin(path);
    return {dirImpl};
}

PLY_NO_INLINE FSResult FileSystem_POSIX::makeDir(FileSystem*, const StringView path) {
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

PLY_NO_INLINE FSResult FileSystem_POSIX::setWorkingDirectory(FileSystem*, const StringView path) {
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

PLY_NO_INLINE String FileSystem_POSIX::getWorkingDirectory(FileSystem*) {
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

PLY_NO_INLINE ExistsResult FileSystem_POSIX::exists(FileSystem*, const StringView path) {
    struct stat buf;
    int rc = stat(path.withNullTerminator().bytes, &buf);
    if (rc == 0)
        return (buf.st_mode & S_IFMT) == S_IFDIR ? ExistsResult::Directory : ExistsResult::File;
    if (errno != ENOENT) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
    }
    return ExistsResult::NotFound;
}

PLY_NO_INLINE int FileSystem_POSIX::openFDForRead(const StringView path) {
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

PLY_NO_INLINE Owned<InPipe> FileSystem_POSIX::openPipeForRead(FileSystem*, const StringView path) {
    int fd = openFDForRead(path);
    if (fd == -1)
        return nullptr;
    return new InPipe_FD{fd};
}

PLY_NO_INLINE int FileSystem_POSIX::openFDForWrite(const StringView path) {
    int fd = open(path.withNullTerminator().bytes, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC,
                  mode_t(0644));
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

PLY_NO_INLINE Owned<OutPipe> FileSystem_POSIX::openPipeForWrite(FileSystem*, const StringView path) {
    int fd = openFDForWrite(path);
    if (fd == -1)
        return nullptr;
    return new OutPipe_FD{fd};
}

PLY_NO_INLINE FSResult FileSystem_POSIX::moveFile(FileSystem*, const StringView srcPath,
                                                  const StringView dstPath) {
    int rc = rename(srcPath.withNullTerminator().bytes, dstPath.withNullTerminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::setLastResult(FSResult::Unknown);
    }
    return FileSystem::setLastResult(FSResult::OK);
}

PLY_NO_INLINE FSResult FileSystem_POSIX::deleteFile(FileSystem*, const StringView path) {
    int rc = unlink(path.withNullTerminator().bytes);
    if (rc != 0) {
        PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
        return FileSystem::setLastResult(FSResult::Unknown);
    }
    return FileSystem::setLastResult(FSResult::OK);
}

PLY_NO_INLINE FSResult FileSystem_POSIX::removeDirTree(FileSystem* fs, const StringView dirPath) {   
    for (const DirectoryEntry& dirEntry : fs->listDir(dirPath)) {
        String joined = PosixPath::join(dirPath, dirEntry.name);
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

PLY_NO_INLINE FileStatus FileSystem_POSIX::getFileStatus(FileSystem*, const StringView path) {
    FileStatus status;
    struct stat buf;
    int rc = stat(path.withNullTerminator().bytes, &buf);
    if (rc != 0) {
        switch (errno) {
            case ENOENT: {
                status.result = FileSystem::setLastResult(FSResult::NotFound);
                break;
            }
            default: {
                PLY_ASSERT(PLY_FSPOSIX_ALLOW_UNKNOWN_ERRORS);
                FileSystem::setLastResult(FSResult::Unknown);
                break;
            }
        }
    } else {
        status.result = FileSystem::setLastResult(FSResult::OK);
        status.fileSize = buf.st_size;
        status.creationTime = buf.st_ctime;
        status.accessTime = buf.st_atime;
        status.modificationTime = buf.st_mtime;
    }
    return status;
}

FileSystem::Funcs FileSystemFuncs_POSIX = {
    {false},
    FileSystem_POSIX::listDir,
    FileSystem_POSIX::makeDir,
    FileSystem_POSIX::setWorkingDirectory,
    FileSystem_POSIX::getWorkingDirectory,
    FileSystem_POSIX::exists,
    FileSystem_POSIX::openPipeForRead,
    FileSystem_POSIX::openPipeForWrite,
    FileSystem_POSIX::moveFile,
    FileSystem_POSIX::deleteFile,
    FileSystem_POSIX::removeDirTree,
    FileSystem_POSIX::getFileStatus,
};

PLY_INLINE FileSystem_POSIX::FileSystem_POSIX() : FileSystem{&FileSystemFuncs_POSIX} {
}

PLY_NO_INLINE FileSystem* FileSystem_POSIX_native() {
    static FileSystem_POSIX fs;
    return &fs;
}

} // namespace ply

#endif // PLY_TARGET_POSIX
