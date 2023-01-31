/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime.h>
#include <ply-runtime/io/OutStream.h>
#include <ply-runtime/container/Tuple.h>

namespace ply {

struct Path_t {
    // On Windows:
    //  \ is the default separator
    //  / and \ are both recognized as separators
    //  (eg. CMake-style path such as "C:/path/to/file" can be manipulated as a Windows path)
    //  Drive letters are recognized when joining paths
    //  Absolute paths must begin with a drive letter
    //  (FIXME: Should also recognize UNC paths, eg. \\server\share, as absolute)
    //
    // On POSIX:
    //  / is the only separator
    //  Windows-style drive letters are treated like any other path component

    bool isWindows = false;

    PLY_INLINE bool isSepByte(char c) const {
        return c == '/' || (this->isWindows && c == '\\');
    }

    PLY_INLINE char sepByte() const {
        return this->isWindows ? '\\' : '/';
    }

    PLY_INLINE bool hasDriveLetter(StringView path) const {
        if (!this->isWindows)
            return false;
        return path.numBytes >= 2 && isAsciiLetter(path.bytes[0]) && path.bytes[1] == ':';
    }

    PLY_INLINE StringView getDriveLetter(StringView path) const {
        return hasDriveLetter(path) ? path.left(2) : StringView{};
    }

    PLY_INLINE bool isAbsolute(StringView path) const {
        if (this->isWindows) {
            return path.numBytes >= 3 && this->hasDriveLetter(path) && this->isSepByte(path[2]);
        } else {
            return path.numBytes >= 1 && this->isSepByte(path[0]);
        }
    }

    PLY_INLINE bool isRelative(StringView path) const {
        return !this->hasDriveLetter(path) && path.numBytes > 0 && !this->isSepByte(path[0]);
    }

    PLY_DLL_ENTRY Tuple<StringView, StringView> split(StringView path) const;
    PLY_DLL_ENTRY Array<StringView> splitFull(StringView path) const;
    PLY_DLL_ENTRY Tuple<StringView, StringView> splitExt(StringView path) const;
    PLY_DLL_ENTRY String joinArray(ArrayView<const StringView> components) const;

    template <typename... StringViews>
    PLY_INLINE String join(StringViews&&... pathComponentArgs) const {
        FixedArray<StringView, sizeof...(StringViews)> components{
            std::forward<StringViews>(pathComponentArgs)...};
        return joinArray(components);
    }

    PLY_DLL_ENTRY String makeRelative(StringView ancestor, StringView descendant) const;
    PLY_DLL_ENTRY HybridString from(const Path_t& srcFormat, StringView srcPath) const;

    PLY_INLINE bool endsWithSep(StringView path) {
        return path.numBytes > 0 && this->isSepByte(path.back());
    }
};

struct WString;
WString win32PathArg(StringView path, bool allowExtended = true);

extern Path_t Path;
extern Path_t WindowsPath;
extern Path_t PosixPath;

} // namespace ply
