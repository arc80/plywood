/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/Array.h>
#include <ply-runtime/io/text/StringWriter.h>
#include <ply-runtime/container/Tuple.h>

namespace ply {

struct PathFormat {
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

    static constexpr char FwdSlash = '/';
    static constexpr char BackSlash = '\\';

    bool isWindows = false;

    PLY_INLINE bool isSepByte(char c) const {
        return c == '/' || (this->isWindows && c == '\\');
    }
    PLY_INLINE const char& sepByte() const {
        return this->isWindows ? BackSlash : FwdSlash;
    }
    PLY_INLINE bool hasDriveLetter(const StringView path) const {
        if (!this->isWindows)
            return false;
        return path.numBytes >= 2 && isAsciiLetter(path.bytes[0]) && path.bytes[1] == ':';
    }
    PLY_INLINE StringView getDriveLetter(const StringView path) const {
        return hasDriveLetter(path) ? path.left(2) : StringView{};
    }
    PLY_INLINE bool isAbsolute(const StringView path) const {
        if (this->isWindows) {
            return path.numBytes >= 3 && this->hasDriveLetter(path) && this->isSepByte(path[2]);
        } else {
            return path.numBytes >= 1 && this->isSepByte(path[0]);
        }
    }
    PLY_INLINE bool isRelative(const StringView path) const {
        return !this->hasDriveLetter(path) && path.numBytes > 0 && !this->isSepByte(path[0]);
    }
    PLY_DLL_ENTRY Tuple<StringView, StringView> split(const StringView path) const;
    PLY_DLL_ENTRY Array<StringView> splitFull(const StringView path) const;
    PLY_DLL_ENTRY Tuple<StringView, StringView> splitExt(const StringView path) const;
    PLY_DLL_ENTRY String joinAndNormalize(ArrayView<const StringView> components) const;
    template <typename... StringViews>
    PLY_INLINE String join(StringViews&&... pathComponentArgs) const {
        FixedArray<StringView, sizeof...(StringViews)> components{
            std::forward<StringViews>(pathComponentArgs)...};
        return joinAndNormalize(components.view());
    }
    template <typename... StringViews>
    PLY_INLINE String normalize(StringViews&&... pathComponentArgs) const {
        FixedArray<StringView, sizeof...(StringViews)> components{
            std::forward<StringViews>(pathComponentArgs)...};
        return joinAndNormalize(components.view());
    }
    PLY_DLL_ENTRY String makeRelative(const StringView ancestor, const StringView descendant) const;
    PLY_DLL_ENTRY HybridString from(const PathFormat& srcFormat, const StringView srcPath) const;
    template <typename OtherTraits>
    PLY_INLINE HybridString from(const StringView path) const {
        return from(OtherTraits::format(), path);
    }
    PLY_INLINE bool endsWithSep(const StringView path) {
        return path.numBytes > 0 && this->isSepByte(path.back());
    }
};

struct WString;
WString win32PathArg(const StringView path, bool allowExtended = true);

template <bool IsWindows>
struct Path {
    static PLY_INLINE bool isSepByte(char u) {
        return PathFormat{IsWindows}.isSepByte(u);
    }
    static PLY_INLINE bool hasDriveLetter(const StringView path) {
        return PathFormat{IsWindows}.hasDriveLetter(path);
    }
    static PLY_INLINE StringView getDriveLetter(const StringView path) {
        return PathFormat{IsWindows}.getDriveLetter(path);
    }
    static PLY_INLINE bool isAbsolute(const StringView path) {
        return PathFormat{IsWindows}.isAbsolute(path);
    }
    static PLY_INLINE PathFormat format() {
        return PathFormat{IsWindows};
    }
    static PLY_INLINE Array<StringView> splitFull(const StringView path) {
        return PathFormat{IsWindows}.splitFull(path);
    }
    static PLY_INLINE Tuple<StringView, StringView> split(const StringView path) {
        return PathFormat{IsWindows}.split(path);
    }
    template <typename... StringViews>
    static PLY_INLINE String join(StringViews&&... pathComponentArgs) {
        FixedArray<StringView, sizeof...(StringViews)> components{
            std::forward<StringViews>(pathComponentArgs)...};
        return PathFormat{IsWindows}.joinAndNormalize(components.view());
    }
    static PLY_INLINE String makeRelative(const StringView ancestor, const StringView descendant) {
        return PathFormat{IsWindows}.makeRelative(ancestor, descendant);
    }
    template <typename... StringViews>
    static PLY_INLINE String normalize(StringViews&&... pathComponentArgs) {
        FixedArray<StringView, sizeof...(StringViews)> components{
            std::forward<StringViews>(pathComponentArgs)...};
        return PathFormat{IsWindows}.joinAndNormalize(components.view());
    }
    static PLY_INLINE bool isNormalized(const StringView path) {
        return path == normalize(path);
    }
    static PLY_INLINE HybridString from(const PathFormat& otherFmt, const StringView path) {
        return PathFormat{IsWindows}.from(otherFmt, path);
    }
    template <typename OtherPath>
    static PLY_INLINE HybridString from(const StringView path) {
        return PathFormat{IsWindows}.from(OtherPath::format(), path);
    }
    static PLY_INLINE Tuple<StringView, StringView> splitExt(const StringView path) {
        return PathFormat{IsWindows}.splitExt(path);
    }
    static PLY_INLINE bool endsWithSep(const StringView path) {
        return path.numBytes > 0 && PathFormat{IsWindows}.isSepByte(path.back());
    }
};

using PosixPath = Path<false>;
using WindowsPath = Path<true>;

#if PLY_TARGET_POSIX
using NativePath = Path<false>;
#elif PLY_TARGET_WIN32
using NativePath = Path<true>;
#else
#error "*** Unable to select NativePath ***"
#endif

} // namespace ply
