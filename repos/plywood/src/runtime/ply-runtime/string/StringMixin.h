/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringView.h>

namespace ply {

template <typename Derived>
struct StringMixin {
    /*!
    Parse the given string directly as `Type`. Whitepsace is trimmed from the beginning and end of
    the string before parsing occurs. If the string cannot be parsed, or if the string is not
    completely consumed by the parse operation, `defaultValue` is returned.

        StringView{"123"}.to<u32>();    // returns 123
        StringView{" 123 "}.to<u32>();  // returns 123
        StringView{"abc"}.to<u32>();    // returns 0
        StringView{"123a"}.to<u32>();   // returns 0
        StringView{""}.to<u32>();       // returns 0
        StringView{""}.to<s32>(-1);     // returns -1

    This function uses `StringViewReader` internally. If you need to distinguish between a
    successful parse and an unsuccessful one, create and use a `StringViewReader` object directly.
    */
    template <typename T>
    PLY_INLINE T to(const T& defaultValue = subst::createDefault<T>()) const {
        return static_cast<const Derived*>(this)->view().to(defaultValue);
    }

    /*!
    Explicit conversion to `bool`. Returns `true` if the length of the string is greater than 0.
    Allows you to use a `String` object inside an `if` condition.

        if (str) {
            ...
        }
    */
    PLY_INLINE explicit operator bool() const {
        return (bool) static_cast<const Derived*>(this)->view();
    }

    /*!
    Returns `true` if the length of the string is 0.
    */
    PLY_INLINE bool isEmpty() const {
        return static_cast<const Derived*>(this)->view().isEmpty();
    }

    /*!
    \beginGroup
    Returns a substring that starts at the offset given by `start`. The optional `numBytes` argument
    determines the length of the substring in bytes. If `numBytes` is not specified, the substring
    continues to the end of the string.
    */
    PLY_INLINE StringView subStr(u32 start) const {
        return static_cast<const Derived*>(this)->view().subStr(start);
    }
    PLY_INLINE StringView subStr(u32 start, u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().subStr(start, numBytes);
    }
    /*!
    \endGroup
    */

    /*!
    Returns a substring that contains only the first `numBytes` bytes of the string.
    */
    PLY_INLINE StringView left(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().left(numBytes);
    }

    /*!
    Returns a substring with the last `numBytes` bytes of the string omitted.
    */
    PLY_INLINE StringView shortenedBy(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().shortenedBy(numBytes);
    }

    /*!
    Returns a substring that contains only the last `numBytes` bytes of the string.
    */
    PLY_INLINE StringView right(u32 numBytes) const {
        return static_cast<const Derived*>(this)->view().right(numBytes);
    }

    /*!
    Returns `true` if the string precedes `other` in sorted order. Equivalent to `compare(*this,
    other) < 0`.
    */
    PLY_INLINE bool operator<(StringView other) const {
        return static_cast<const Derived*>(this)->view() < other;
    }

    /*!
    \beginGroup
    Returns `true` if the string contents are identical (or not identical) when compared
    byte-for-byte.
    */
    PLY_INLINE bool operator==(StringView src) const {
        return static_cast<const Derived*>(this)->view() == src;
    }
    PLY_INLINE bool operator!=(StringView src) const {
        return static_cast<const Derived*>(this)->view() != src;
    }
    /*!
    \endGroup
    */

    /*!
    Returns a new `String` containing the concatenation of two `StringViews`.
    */
    PLY_INLINE String operator+(StringView other) const;

    /*!
    Returns a new `String` containing the contents of the `StringView` repeated `count` times.

        StringView{'*'};    // returns "**********"
    */
    PLY_INLINE String operator*(u32 count) const;

    /*!
    Returns the offset of the first occurence of `matchByte` in the string, or `-1` if not found.
    The search begins at the offset specified by `startPos`. This function can find ASCII codes in
    UTF-8 encoded strings, since ASCII codes are always encoded as a single byte in UTF-8.
    */
    PLY_INLINE s32 findByte(char matchByte, u32 startPos = 0) const {
        return static_cast<const Derived*>(this)->view().findByte(matchByte, startPos);
    }

    /*!
    A template function that returns the offset of the first byte for which `matchFunc` returns
    `true`, or `-1` if none. The search begins at the offset specified by `startPos`. This function
    can be used to find a whitespace character by calling `findByte(isWhite)`.
    */
    template <typename MatchFuncOrChar>
    PLY_INLINE s32 findByte(const MatchFuncOrChar& matchFuncOrByte, u32 startPos = 0) const {
        return static_cast<const Derived*>(this)->view().findByte(matchFuncOrByte, startPos);
    }

    /*!
    \beginGroup
    Reverse `findByte` functions. Returns the offset of the last byte in the string that matches the
    first argument (if passed a `char`) or for which the first argument returns true (if passed a
    function). The optional `startPos` argument specifies an offset at which to begin the search.
    */
    template <typename MatchFuncOrChar>
    PLY_INLINE s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte, u32 startPos) const {
        return static_cast<const Derived*>(this)->view().rfindByte(matchFuncOrByte, startPos);
    }
    template <typename MatchFuncOrChar>
    PLY_INLINE s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte) const {
        return static_cast<const Derived*>(this)->view().rfindByte(matchFuncOrByte,
                                                                   this->numBytes - 1);
    }
    /*!
    \endGroup
    */

    /*!
    Returns `true` if the string starts with `arg`.
    */
    PLY_INLINE bool startsWith(StringView arg) const {
        return static_cast<const Derived*>(this)->view().startsWith(arg);
    }

    /*!
    Returns `true` if the string ends with `arg`.
    */
    PLY_INLINE bool endsWith(StringView arg) const {
        return static_cast<const Derived*>(this)->view().endsWith(arg);
    }

    /*!
    \beginGroup
    Returns a substring with leading and/or trailing bytes removed. Bytes are removed if `true` is
    returned when passed to `matchFunc`. These functions can be used to trim whitespace characters
    from a UTF-8 string, for example by calling `trim(isWhite)`, since whitespace characters are
    each encoded as a single byte in UTF-8.

    `ltrim()` trims leading bytes only, `rtrim()` trims trailing bytes only, and `trim()` trims both
    leading and trailing bytes.
    */
    PLY_INLINE StringView trim(bool (*matchFunc)(char) = isWhite, bool left = true,
                               bool right = true) const {
        return static_cast<const Derived*>(this)->view().trim(matchFunc, left, right);
    }
    PLY_INLINE StringView ltrim(bool (*matchFunc)(char) = isWhite) const {
        return static_cast<const Derived*>(this)->view().ltrim(matchFunc);
    }
    PLY_INLINE StringView rtrim(bool (*matchFunc)(char) = isWhite) const {
        return static_cast<const Derived*>(this)->view().rtrim(matchFunc);
    }
    /*!
    \endGroup
    */

    /*!
    Returns a new `String` containing every item in `comps` concatenated together, with the given
    string used as a separator. For example:

        StringView{", "}.join({"a", "b", "c"}); // returns "a, b, c"
        StringView{""}.join({"a", "b", "c"});   // returns "abc"
    */
    PLY_INLINE String join(ArrayView<const StringView> comps) const;

    /*!
    Returns a list of the words in the given string using `sep` as a delimiter byte.
    */
    PLY_INLINE Array<StringView> splitByte(char sep) const {
        return static_cast<const Derived*>(this)->view().splitByte(sep);
    }

    /*!
    Returns a new `String` with all lowercase ASCII characters converted to uppercase. This function
    works with UTF-8 strings. Also works with any 8-bit text encoding compatible with ASCII.
    */
    PLY_INLINE String upperAsc() const;

    /*!
    Returns a new `String` with all uppercase ASCII characters converted to lowercase. This function
    works with UTF-8 strings. Also works with any 8-bit text encoding compatible with ASCII.
    */
    PLY_INLINE String lowerAsc() const;

    /*!
    Returns a new `String` with the bytes reversed. This function is really only suitable when you
    know that all characters contained in the string are encoded in a single byte.
    */
    PLY_INLINE String reversedBytes() const;

    // FIXME: Make sure this documentation string is OK
    /*!
    Returns a new `String` with UTF-8 characters reversed. For example,
    `StringView{"`&#x1f60b;&#x1f37a;&#x1f355;`"}.reversedUTF8()` returns
    `"`&#x1f355;&#x1f37a;&#x1f60b;`"`.
    */
    PLY_INLINE String reversedUTF8() const;

    /*!
    Returns a new `String` with each byte passed through the provided `filterFunc`. It's safe to
    call this function on UTF-8 encoded strings as long as `filterFunc` leaves byte values greater
    than or equal to 128 unchanged. Therefore, this function is mainly suitable for filtering ASCII
    codes.
    */
    PLY_INLINE String filterBytes(char (*filterFunc)(char)) const;

    /*!
    Returns `true` if the last byte in the string is a zero byte.
    */
    PLY_INLINE bool includesNullTerminator() const {
        return static_cast<const Derived*>(this)->view().includesNullTerminator();
    }

    /*!
    If the last byte of the given string is not a zero byte, this function allocates memory for a
    new string, copies the contents of the given string to it, appends a zero byte and returns the
    new string. In that case, the new string's `numBytes` will be one greater than the `numBytes` of
    the original string. If the last byte of the given string is already a zero byte, a view of the
    given string is returned and no new memory is allocated.
    */
    PLY_INLINE HybridString withNullTerminator() const;

    /*!
    If the last byte of the given string is not a zero byte, returns a view of the given string. If
    the last byte of the given string is a zero byte, returns a substring with the last byte
    omitted.
    */
    PLY_INLINE StringView withoutNullTerminator() const {
        return static_cast<const Derived*>(this)->view().withoutNullTerminator();
    }

    /*!
    Feeds the contents of the given string to a hash function.
    */
    template <typename Hasher>
    PLY_INLINE void appendTo(Hasher& hasher) const {
        static_cast<const Derived*>(this)->view().appendTo(hasher);
    }
};

} // namespace ply
