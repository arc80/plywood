/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/thread/ThreadLocal.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/filesystem/Path.h>
#include <ply-runtime/container/Array.h>
#include <ply-runtime/container/Buffer.h>
#include <ply-runtime/container/Tuple.h>
#include <ply-runtime/container/Owned.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/io/InStream.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/io/text/TextFormat.h>

// clang-format off
#if !defined(PLY_IMPL_FILESYSTEM_GET)
    #if PLY_TARGET_WIN32
        #define PLY_IMPL_FILESYSTEM_NATIVE FileSystem_Win32_native
    #elif PLY_TARGET_POSIX
        #define PLY_IMPL_FILESYSTEM_NATIVE FileSystem_POSIX_native
    #else
        #error "*** Unable to select a default FileSystem implementation ***"
    #endif
#endif
// clang-format on

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

struct FileStatus {
    FSResult result = FSResult::Unknown; // Result of getFileStatus()
    u64 fileSize = 0;                    // Size of the file in bytes
    double creationTime = 0;             // The file's POSIX creation time
    double accessTime = 0;               // The file's POSIX access time
    double modificationTime = 0;         // The file's POSIX modification time
};

struct DirectoryEntry {
    String name; // FIXME: Make this a HybridString
    bool isDir = false;
    u64 fileSize = 0;            // Size of the file in bytes
    double creationTime = 0;     // The file's POSIX creation time
    double accessTime = 0;       // The file's POSIX access time
    double modificationTime = 0; // The file's POSIX modification time
};

struct Directory {
    struct Impl {
        void (*destruct)(Impl* impl) = nullptr;
        FSResult (*next)(Impl* impl) = nullptr;
        u32 flags = 0;
        DirectoryEntry entry;
        PLY_INLINE ~Impl() {
            destruct(this);
        }
    };
    Owned<Impl> impl;

    struct Iterator {
        Impl* impl;
        PLY_INLINE DirectoryEntry& operator*() {
            return this->impl->entry;
        }
        PLY_INLINE void operator++() {
            this->impl->next(this->impl);
        }
        PLY_INLINE bool operator!=(const Iterator&) const {
            return !this->impl->entry.name.isEmpty();
        }
    };

    PLY_INLINE Directory(Impl* impl) : impl{impl} {
    }
    PLY_INLINE Directory(Directory&& other) : impl{std::move(other.impl)} {
    }
    PLY_INLINE Iterator begin() {
        return {impl};
    }
    PLY_INLINE Iterator end() {
        return {impl};
    }
};

enum class ExistsResult {
    NotFound,
    File,
    Directory,
};

struct WalkTriple {
    struct FileInfo {
        String name;
        u64 fileSize = 0;            // Size of the file in bytes
        double creationTime = 0;     // The file's POSIX creation time
        double accessTime = 0;       // The file's POSIX access time
        double modificationTime = 0; // The file's POSIX modification time
    };

    String dirPath;
    Array<String> dirNames;
    Array<FileInfo> files;
};

struct FileSystem;

PLY_DLL_ENTRY FileSystem* PLY_IMPL_FILESYSTEM_NATIVE();

//------------------------------------------------------------------------------------------------
/*!
`FileSystem` is a filesystem interface.

You can access the native filesystem by calling `FileSystem::native()`.

A `FileSystem` object expects either Windows paths or POSIX-style paths, depending on its
implementation. The `NativePath` class always manipulate paths in the appropriate format for the
native filesystem. When working with an abstract `FileSystem` where the path format is unknown,
`pathFormat()` will return a `PathFormat` object suitable for manipulating paths in the expected
format. See [Manipulating Paths](ManipulatingPaths) for more information.

You can create a virtual filesystem using `createVirtualFileSystem()`, or even implement your own.
Virtual filesystems don't necessarily support all the member functions in the `FileSystem`
interface.

## Result Codes

Most `FileSystem` member functions update an internal result code. This result code is stored in a
thread-local variable of type `FSResult`. Some member functions, such as `makeDir()`, return the
result code explicitly. Other member functions, such as `loadTextAutodetect()`, do not return the
result code explicitly. You can always access the last result code by calling `lastResult()`.

Note that, because the internal result code is thread-local, it's shared across all `FileSystem`
instances. For example, an operation on a virtual filesystem will update the result code for the
native filesystem, and vice versa.

In some cases, a function may set the result code to `Unknown` instead of one of the result codes
documented here. Such cases mean that Plywood needs to be updated to recognize the underlying error.
*/
struct FileSystem {
    static const u32 WithSizes = 0x1;
    static const u32 WithTimes = 0x2;

    struct Walk {
        struct Impl {
            void (*destruct)(Impl* impl) = nullptr;
            void (*next)(Impl* impl) = nullptr;
            WalkTriple triple;
            PLY_INLINE ~Impl() {
                destruct(this);
            }
        };
        Owned<Impl> impl;

        struct Iterator {
            Impl* impl;
            PLY_INLINE WalkTriple& operator*() {
                return this->impl->triple;
            }
            PLY_INLINE void operator++() {
                this->impl->next(impl);
            }
            PLY_INLINE bool operator!=(const Iterator&) const {
                return !this->impl->triple.dirPath.isEmpty();
            }
        };

        PLY_INLINE Walk(Impl* impl) : impl{impl} {
        }
        PLY_INLINE Walk(Walk&& other) : impl{std::move(other.impl)} {
        }
        PLY_INLINE Iterator begin() {
            return {this->impl};
        }
        PLY_INLINE Iterator end() {
            return {this->impl};
        }
    };

    struct Funcs {
        PathFormat pathFmt;
        Directory (*listDir)(FileSystem* fs, StringView path, u32 flags) = nullptr;
        FSResult (*makeDir)(FileSystem* fs, StringView path) = nullptr;
        FSResult (*setWorkingDirectory)(FileSystem* fs_, StringView path) = nullptr;
        String (*getWorkingDirectory)(FileSystem* fs) = nullptr;
        ExistsResult (*exists)(FileSystem* fs, StringView path) = nullptr;
        Owned<InPipe> (*openPipeForRead)(FileSystem* fs, StringView path) = nullptr;
        Owned<OutPipe> (*openPipeForWrite)(FileSystem* fs, StringView path) = nullptr;
        FSResult (*moveFile)(FileSystem* fs, StringView srcPath, StringView dstPath) = nullptr;
        FSResult (*deleteFile)(FileSystem* fs, StringView path) = nullptr;
        FSResult (*removeDirTree)(FileSystem* fs, StringView dirPath) = nullptr;
        FileStatus (*getFileStatus)(FileSystem* fs, StringView path) = nullptr;
    };

    static ThreadLocal<FSResult> lastResult_;

    Funcs* funcs = nullptr;

    PLY_INLINE FileSystem(Funcs* funcs) : funcs{funcs} {
    }

    /*!
    Returns a `FileSystem` object that interacts with the native filesystem.
    */
    PLY_INLINE static FileSystem* native() {
        return PLY_IMPL_FILESYSTEM_NATIVE();
    }

    /*!
    Returns a `PathFormat` describing the filesystem's expected path format. This object can be used
    to [manipulate paths](ManipulatingPaths) in the expected format.
    */
    PLY_INLINE PathFormat pathFormat() const {
        return this->funcs->pathFmt;
    }

    /*!
    Sets the working directory of the process in the target filesystem. May not be supported in
    virtual filesystems.

    This function updates the internal result code. The result code is also returned directly.
    Expected result codes are `OK` or `NotFound`.
    */
    PLY_INLINE FSResult setWorkingDirectory(StringView path) {
        return this->funcs->setWorkingDirectory(this, path);
    }

    /*!
    Gets the working directory of the process in the target filesystem.

    This function updates the internal result code. The expected result code is `OK`.
    */
    PLY_INLINE String getWorkingDirectory() {
        return this->funcs->getWorkingDirectory(this);
    }

    /*!
    Returns the last result code.
    */
    PLY_INLINE FSResult lastResult() const {
        return FileSystem::lastResult_.load();
    }

    /*!
    Sets the last result code.
    */
    static PLY_INLINE FSResult setLastResult(FSResult result) {
        FileSystem::lastResult_.store(result);
        return result;
    }

    /*!
    Checks whether a file or directory with the specified pathname exists. The return value is one
    of `File`, `Directory` or `NotFound`.

    This function does not update the internal result code.
    */
    PLY_INLINE ExistsResult exists(StringView path) {
        return this->funcs->exists(this, path);
    }

    /*!
    Returns `true` if `path` specifies the name of a directory. Shorthand for `exists(path) ==
    ExistsResult::Directory`.

    This function does not update the internal result code.
    */
    PLY_INLINE bool isDir(StringView path) {
        return this->funcs->exists(this, path) == ExistsResult::Directory;
    }

    /*!
    Returns a `Directory` object. The `Directory` object can be used in a range-based for loop to
    enumerate the contents of a directory. Such a range-based for loop iterates over a sequence of
    `DirectoryEntry` objects. It is recommended to iterate by reference in order to avoid copying
    the `DirectoryEntry` object. Each `DirectoryEntry` takes the following form:

        struct DirectoryEntry {
            String name;
            bool isDir;
            u64 fileSize;           // Only valid if WithSizes was specified
            double creationTime;    // Only valid if WithTimes was specified
            double accessTime;
            double modificationTime;
        };

    The entries are returned in an arbitrary order, and the special directory entries `"."` and
    `".."` are not enumerated.

    The `flags` argument accepts a bitwise-or of zero or more of the following values:

    * `FileSystem::WithSizes`
    * `FileSystem::WithFlags`

    The `fileSize` member of `DirectoryEntry` is only valid if `WithSizes` was set. The
    `creationTime`, `accessTime` and `modificationTime` members are only valid if `WithTimes` was
    set. Using these flags is likely faster than calling `getFileStatus()` on the individual files,
    especially on Windows. Conversely, if the additional information is not needed, it might be
    faster to omit these flags on some platforms.

    The following example outputs a list of non-directory files in the current directory:

        StringWriter sw = StdOut::text();
        for (const DirectoryEntry& entry : FileSystem::native()->listDir(".", 0)) {
            if (!entry.isDir) {
                sw << entry.name << '\n';
            }
        }

    The initial call to `listDir()` updates the internal result code. Expected result codes are
    `OK`, `NotFound` or `AccessDenied`. Each iteration of the loop updates the internal result code
    as well. Expected result codes are `OK` or `NotFound` (if, for example, the volume was removed
    during iteration).
    */
    PLY_INLINE Directory listDir(StringView path, u32 flags = WithSizes | WithTimes) {
        return this->funcs->listDir(this, path, flags);
    }

    /*!
    Similar to `os.walk()` in Python. Returns a `FileSystem::Walk` object. The `FileSystem::Walk`
    object can be used in a range-based for loop to enumerate the contents of a directory tree. Such
    a range-based for loop iterates over a sequence of `WalkTriple` objects, with one `WalkTriple`
    object for each directory in the tree rooted at `top`. It is recommended to iterate by reference
    in order to avoid copying the `WalkTriple` object. Each `WalkTriple` takes the following form:

        struct WalkTriple {
            struct FileInfo {
                String name;
                u64 fileSize;           // Only valid if WithSizes was specified
                double creationTime;    // Only valid if WithTimes was specified
                double accessTime;
                double modificationTime;
            };

            String dirPath;
            Array<String> dirNames;
            Array<FileInfo> files;
        };

    `dirPath` contains the path to the directory. `dirNames` is a list of the names of the
    subdirectories in `dirPath` excluding `"."` and `".."`. `files` is a list of
    `WalkTriple::FileInfo` objects for the non-directory files in `dirPath`.

    On each iteration of the loop, the caller can modify the `dirNames` list in-place, and `walk()`
    will only recurse into the subdirectories whose names remain in `dirNames`. This can be used to
    prune the search, impose a specific order of visiting, or inform `walk()` about directories that
    the caller creates or renames.

    The `flags` argument accepts a bitwise-or of zero or more of the following values:

    * `FileSystem::WithSizes`
    * `FileSystem::WithFlags`

    The `fileSize` member of `WalkTriple::FileInfo` is only valid if `WithSizes` was set. The
    `creationTime`, `accessTime` and `modificationTime` members are only valid if `WithTimes` was
    set. Using these flags is likely faster than calling `getFileStatus()` on the individual files,
    especially on Windows. Conversely, if the additional information is not needed, it might be
    faster to omit these flags on some platforms.

    The following example displays the number of bytes taken by non-directory files in each
    directory under the starting directory, except that it doesn't look under any subdirectory that
    starts with a dot `"."`:

        StringWriter sw = StdOut::text();
        for (WalkTriple& triple : fs->walk(".", FileSystem::WithSizes)) {
            // Calculate the number of bytes taken by non-directory files
            u64 sum = 0;
            for (const WalkTriple::FileInfo& file : triple.files) {
                sum += file.fileSize;
            }
            sw.format("{}: {} bytes\n", triple.dirPath, sum);

            // Prune subdirectories that start with "."
            for (u32 i = 0; i < triple.dirNames.numItems(); i++) {
                if (triple.dirNames[i].startsWith(".")) {
                    triple.dirNames.erase(i);
                    i--;
                }
            }
        }

    The initial call to `walk()` updates the internal result code. Expected result codes are `OK`,
    `NotFound` or `AccessDenied`. Each iteration of the loop updates the internal result code as
    well. Expected result codes are `OK`, `NotFound` (if, for example, the volume was removed during
    iteration) or `AccessDenied`.
    */
    PLY_DLL_ENTRY Walk walk(StringView top, u32 flags = WithSizes | WithTimes);

    /*!
    Creates a new directory. The parent directory must already exist.

    This function updates the internal result code. The result code is also returned directly.
    Expected result codes are `OK`, `AlreadyExists` or `AccessDenied`.
    */
    PLY_INLINE FSResult makeDir(StringView path) {
        return this->funcs->makeDir(this, path);
    }

    /*!
    Recursive directory creation function. Like `makeDir()`, but creates all intermediate-level
    directories needed to contain the leaf directory. Similar to `os.makedirs()` in Python.

    This function updates the internal result code. The result code is also returned directly.
    Expected result codes are `OK` or `AccessDenied`.
    */
    PLY_DLL_ENTRY FSResult makeDirs(StringView path);

    /*!
    Renames the file at `srcPath` to `dstPath`. If `dstPath` already exists and is a file, it will
    be silently replaced.

    This function updates the internal result code. The result code is also returned directly.
    */
    PLY_INLINE FSResult moveFile(StringView srcPath, StringView dstPath) {
        return this->funcs->moveFile(this, srcPath, dstPath);
    }

    /*!
    Deletes the file at `path`.

    This function updates the internal result code. The result code is also returned directly.
    */
    PLY_INLINE FSResult deleteFile(StringView path) {
        return this->funcs->deleteFile(this, path);
    }

    /*!
    Removes the directory tree at `path`. The specified directory and all of its contents are
    deleted recursively.

    This function updates the internal result code. The result code is also returned directly.
    Expected result codes are `OK` or `AccessDenied`.
    */
    PLY_INLINE FSResult removeDirTree(StringView dirPath) {
        return this->funcs->removeDirTree(this, dirPath);
    }

    /*!
    Returns a `FileStatus` object for the file located at `path`. `FileStatus` contains the
    following members:

        struct FileStatus {
            FSResult result;            // Result of getFileStatus()
            u64 fileSize;               // Size of the file in bytes
            double creationTime;        // The file's POSIX creation time
            double accessTime;          // The file's POSIX access time
            double modificationTime;    // The file's POSIX modification time
        };

    `creationTime`, `accessTime` and `modificationTime` are expressed in POSIX time, which is the
    number of seconds since January 1, 1970.

    This function updates the internal result code. The result code is also returned in the `result`
    member. Expected result codes are `OK` or `NotFound`.
    */
    PLY_INLINE FileStatus getFileStatus(StringView path) {
        return this->funcs->getFileStatus(this, path);
    }

    /*!
    Returns an `InPipe` that reads the raw contents of the specified file, or `nullptr` if the file
    could not be opened.

    This function updates the internal result code. Expected result codes are `OK`, `NotFound`,
    `AccessDenied` or `Locked`.

    `InPipe` is a low-level class that doesn't perform any application-level buffering. If you plan
    to read small amounts of data at a time, such when parsing a text or binary file format, it is
    suggested to wrap the returned `InPipe` in an `InStream`. The `openStreamForRead()` convenience
    function is provided for this.
    */
    PLY_INLINE Owned<InPipe> openPipeForRead(StringView path) {
        return this->funcs->openPipeForRead(this, path);
    }

    /*!
    Returns an `OutPipe` that writes raw data to the specified file, or `nullptr` if the file could
    not be opened.

    This function updates the internal result code. Expected result codes are `OK`, `NotFound`,
    `AccessDenied` or `Locked`.

    `OutPipe` is a low-level class that doesn't perform any application-level buffering. If you plan
    to write small amounts of data at a time, such when writing a text or binary file format, it is
    suggested to wrap the returned `OutPipe` in an `OutStream`. The `openStreamForWrite()`
    convenience function is provided for this.
    */
    PLY_INLINE Owned<OutPipe> openPipeForWrite(StringView path) {
        return this->funcs->openPipeForWrite(this, path);
    }

    /*!
    Returns an `InStream` that reads the raw contents of the specified file, or `nullptr` if the
    file could not be opened.

    This function updates the internal result code. Expected result codes are `OK`, `NotFound` or
    `Locked`.
    */
    PLY_DLL_ENTRY Owned<InStream> openStreamForRead(StringView path);

    /*!
    Returns an `OutStream` that writes raw data to the specified file, or `nullptr` if the file
    could not be opened.

    This function updates the internal result code. Expected result codes are `OK`, `NotFound`,
    `AccessDenied` or `Locked`.
    */
    PLY_DLL_ENTRY Owned<OutStream> openStreamForWrite(StringView path);

    /*!
    Returns a `StringReader` that reads raw data from the specified text file and converts it to
    UTF-8 with Unix-style newlines, or `nullptr` if the file could not be opened. The text file is
    expected to have the file format described by `textFormat`. Any byte order mark (BOM) in the
    file is skipped. See `TextFormat` for more information on supported text file formats.

    This function updates the internal result code. Expected result codes are `OK`, `NotFound`,
    `AccessDenied` or `Locked`.

    The `openTextForRead()` function is equivalent to the following:

        Owned<InStream> ins = fs->openStreamForRead(path);
        if (!ins)
            return nullptr;
        return textFormat.createImporter(std::move(ins));
    */
    PLY_DLL_ENTRY Owned<StringReader> openTextForRead(StringView path,
                                                      const TextFormat& textFormat);

    /*!
    Returns a `Tuple`. The first tuple item is a `StringReader` that reads raw data from the
    specified text file and converts it to UTF-8 with Unix-style newlines, or `nullptr` if the file
    could not be opened. The text file format is detected automatically and returned as the second
    tuple item. Any byte order mark (BOM) in the text file is skipped. See `TextFormat` for more
    information on supported text file formats.

    This function updates the internal result code. Expected result codes are `OK`, `NotFound`,
    `AccessDenied` or `Locked`.

    The `openTextForReadAutodetect()` function is equivalent to the following:

        Owned<InStream> ins = fs->openStreamForRead(path);
        if (!ins)
            return {nullptr, TextFormat{}};
        TextFormat textFormat = TextFormat::autodetect(ins);
        return {textFormat.createImporter(std::move(ins)), textformat};
    */
    PLY_DLL_ENTRY Tuple<Owned<StringReader>, TextFormat> openTextForReadAutodetect(StringView path);

    /*!
    Returns a `Buffer` containing the raw contents of the specified file, or an empty `Buffer` if
    the file could not be opened.

    To check if the file was opened successfuly, call `lastResult()`. Expected result codes are
    `OK`, `NotFound`, `AccessDenied` or `Locked`.
    */
    PLY_DLL_ENTRY Buffer loadBinary(StringView path);

    /*!
    Returns a `String` containing the contents of the specified text file converted to UTF-8 with
    Unix-style newlines, or an empty `String` if the file could not be opened. The text file is
    expected to have the file format described by `textFormat`. Any byte order mark (BOM) in the
    text file is skipped. See `TextFormat` for more information on supported text file formats.

    To check if the file was opened successfuly, call `lastResult()`. Expected result codes are
    `OK`, `NotFound`, `AccessDenied` or `Locked`.
    */
    PLY_DLL_ENTRY String loadText(StringView path, const TextFormat& textFormat);

    /*!
    Returns a `Tuple`. The first tuple item is a `String` containing the contents of the specified
    text file converted to UTF-8 with Unix-style newlines, or an empty `String` if the file could
    not be opened. The text file format is detected automatically and returned as the second tuple
    item. Any byte order mark (BOM) in the text file is skipped. See `TextFormat` for more
    information on supported text file formats.

    To check if the file was opened successfuly, call `lastResult()`. Expected result codes are
    `OK`, `NotFound`, `AccessDenied` or `Locked`.
    */
    PLY_DLL_ENTRY Tuple<String, TextFormat> loadTextAutodetect(StringView path);

    /*!
    Returns a `StringWriter` that writes text to the specified text file in the specified format, or
    `nullptr` if the file could not be opened. The `StringWriter` expects UTF-8-encoded text. The
    `StringWriter` accepts both Windows and Unix-style newlines; all newlines will be converted to
    the format described by `textFormat`. See `TextFormat` for more information on supported text
    file formats.

    This function updates the internal result code. Expected result codes are `OK`, `NotFound`,
    `AccessDenied` or `Locked`.

    The `openTextForWrite()` function is equivalent to the following:

        Owned<OutStream> outs = fs->openTextForWrite(path);
        if (!outs)
            return nullptr;
        return TextFormat::createExporter(std::move(outs), textFormat);
    */
    Owned<StringWriter> openTextForWrite(StringView path, const TextFormat& textFormat);

    /*!
    First, this function tries to load the raw contents of the specified file. If the load succeeds
    and its raw contents match `contents` exactly, the function returns `Unchanged`. Otherwise, if
    the parent directories of `path` don't exist, it attempts to create them. If that succeeds, it
    saves `contents` to a temporary file in the same folder as `path`. If that succeeds, it renames
    the temporary file to `path`, replacing any original contents.

    In all cases, this function updates the internal result code. The result code is also returned
    directly.
    */
    PLY_DLL_ENTRY FSResult makeDirsAndSaveBinaryIfDifferent(StringView path,
                                                            ConstBufferView contents);

    /*!
    First, this function converts `strContents` to a raw memory buffer using the text file format
    specified by `textFormat`. Then, it tries to load the raw contents of the specified file. If the
    load succeeds and its raw contents match the contents of the raw memory buffer exactly, the
    function returns `Unchanged`. Otherwise, if the parent directories of `path` don't exist, it
    attempts to create them. If that succeeds, it saves the raw memory buffer to a temporary file in
    the same folder as `path`. If that succeeds, it renames the temporary file to `path`, replacing
    any original contents. See `TextFormat` for more information on supported text file formats.

    In all cases, this function updates the internal result code. The result code is also returned
    directly.
    */
    PLY_DLL_ENTRY FSResult makeDirsAndSaveTextIfDifferent(StringView path, StringView strContents,
                                                          const TextFormat& textFormat);
};

} // namespace ply
