/*━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━┓
┃     ____                                    ┃
┃    ╱   ╱╲    Plywood C++ Development Kit    ┃
┃   ╱___╱╭╮╲   https://plywood.dev/           ┃
┃    └──┴┴┴┘                                  ┃
┗━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━*/

#pragma once
#include <ply-runtime.h>
#include <ply-runtime/Path.h>
#include <ply-runtime/container/Tuple.h>
#include <ply-runtime/container/Owned.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/io/text/TextFormat.h>

namespace ply {

enum class FSResult {
    Unknown = 0,
    NotFound,
    Locked,
    AccessDenied,
    OK,
    AlreadyExists,
    Unchanged,
};

enum class ExistsResult {
    NotFound,
    File,
    Directory,
};

struct FileInfo {
    FSResult result = FSResult::Unknown; // Result of getFileInfo()
    String name;
    bool isDir = false;
    u64 fileSize = 0;            // Size of the file in bytes
    double creationTime = 0;     // The file's POSIX creation time
    double accessTime = 0;       // The file's POSIX access time
    double modificationTime = 0; // The file's POSIX modification time
};

struct WalkTriple {
    String dirPath;
    Array<String> dirNames;
    Array<FileInfo> files;
};

struct FileSystemIface;

class FileSystemWalker {
private:
    struct StackItem {
        String path;
        Array<String> dirNames;
        u32 dirIndex;
    };

    WalkTriple triple;
    Array<StackItem> stack;
    FileSystemIface* fs = nullptr;
    u32 flags = 0;

    friend struct FileSystemIface;
    void visit(StringView dirPath);

public:
    PLY_INLINE FileSystemWalker() = default;
    PLY_NO_INLINE FileSystemWalker(FileSystemWalker&&) = default;

    // Range-for support:
    struct Iterator {
        FileSystemWalker* walker;
        PLY_INLINE WalkTriple& operator*() {
            return this->walker->triple;
        }
        void operator++();
        PLY_INLINE bool operator!=(const Iterator&) const {
            return !this->walker->triple.dirPath.isEmpty();
        }
    };
    PLY_INLINE Iterator begin() {
        return {this};
    }
    PLY_INLINE Iterator end() {
        return {this};
    }
};

// FileSystemIface will allow us to create virtual filesystems

struct FileSystemIface {
    static const u32 WithSizes = 0x1;
    static const u32 WithTimes = 0x2;

    static ThreadLocal<FSResult> lastResult_;

    static PLY_INLINE FSResult setLastResult(FSResult result) {
        FileSystemIface::lastResult_.store(result);
        return result;
    }
    static PLY_INLINE FSResult lastResult() {
        return FileSystemIface::lastResult_.load();
    }

    virtual ~FileSystemIface() {
    }
    virtual Path_t pathFormat() = 0;
    virtual FSResult setWorkingDirectory(StringView path) = 0;
    virtual String getWorkingDirectory() = 0;
    virtual ExistsResult exists(StringView path) = 0;
    virtual Array<FileInfo> listDir(StringView path,
                                    u32 flags = WithSizes | WithTimes) = 0;
    virtual FSResult makeDir(StringView path) = 0;
    virtual FSResult moveFile(StringView srcPath, StringView dstPath) = 0;
    virtual FSResult deleteFile(StringView path) = 0;
    virtual FSResult removeDirTree(StringView dirPath) = 0;
    virtual FileInfo getFileInfo(StringView path) = 0; // Doesn't set FileInfo::name
    virtual Owned<InPipe> openPipeForRead(StringView path) = 0;
    virtual Owned<OutPipe> openPipeForWrite(StringView path) = 0;

    PLY_INLINE bool isDir(StringView path) {
        return this->exists(path) == ExistsResult::Directory;
    }

    FileSystemWalker walk(StringView top, u32 flags = WithSizes | WithTimes);
    FSResult makeDirs(StringView path);
    InStream openStreamForRead(StringView path);
    OutStream openStreamForWrite(StringView path);
    InStream openTextForRead(StringView path,
                             const TextFormat& format = TextFormat::default_utf8());
    InStream openTextForReadAutodetect(StringView path,
                                       TextFormat* out_format = nullptr);
    OutStream openTextForWrite(StringView path,
                               const TextFormat& format = TextFormat::default_utf8());
    String loadBinary(StringView path);
    String loadText(StringView path, const TextFormat& format);
    String loadTextAutodetect(StringView path, TextFormat* out_format = nullptr);
    FSResult makeDirsAndSaveBinaryIfDifferent(StringView path, StringView contents);
    FSResult makeDirsAndSaveTextIfDifferent(
        StringView path, StringView strContents,
        const TextFormat& format = TextFormat::default_utf8());
};

struct FileSystem_t : FileSystemIface {
#if PLY_TARGET_WIN32
    // ReadWriteLock used to mitigate data race issues with SetCurrentDirectoryW:
    // https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setcurrentdirectory
    ReadWriteLock workingDirLock;

    // Direct access to Windows handles:
    HANDLE openHandleForRead(StringView path);
    HANDLE openHandleForWrite(StringView path);
    FileInfo getFileInfo(HANDLE handle);
#endif

    virtual Array<FileInfo> listDir(StringView path,
                                    u32 flags = WithSizes | WithTimes) override;
    virtual FSResult makeDir(StringView path) override;
    virtual Path_t pathFormat() override;
    virtual String getWorkingDirectory() override;
    virtual FSResult setWorkingDirectory(StringView path) override;
    virtual ExistsResult exists(StringView path) override;
    virtual Owned<InPipe> openPipeForRead(StringView path) override;
    virtual Owned<OutPipe> openPipeForWrite(StringView path) override;
    virtual FSResult moveFile(StringView srcPath, StringView dstPath) override;
    virtual FSResult deleteFile(StringView path) override;
    virtual FSResult removeDirTree(StringView dirPath) override;
    virtual FileInfo getFileInfo(StringView path) override;

    virtual ~FileSystem_t() {
    }
};

extern FileSystem_t FileSystem;

} // namespace ply
