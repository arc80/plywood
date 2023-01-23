/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ArrayView.h>
#include <string> // for char_traits

namespace ply {

struct String;
struct HybridString;
template <typename>
class Array;

namespace fmt {
template <typename>
struct TypeParser;
template <typename>
struct FormatParser;
} // namespace fmt

PLY_INLINE bool isWhite(char cp) {
    return (cp == ' ') || (cp == '\t') || (cp == '\r') || (cp == '\n');
}

PLY_INLINE bool isAsciiLetter(char cp) {
    return (cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z');
}

PLY_INLINE bool isDecimalDigit(char cp) {
    return (cp >= '0' && cp <= '9');
}

//------------------------------------------------------------------------------------------------
/*!
A `StringView` references an immutable range of memory. It consists of a pointer `bytes` and an
integer `numBytes`. A `StringView` does not own the memory it points to, and no heap memory is freed
when the `StringView` is destroyed. The caller is responible for ensuring that memory referenced by
`StringView` remains valid for the lifetime of the `StringView` object.

`StringView` objects often reference UTF-8-encoded text, but this isn't a requirement. The memory
referenced by a `StringView` can hold any kind of data.

Even though `StringView` objects aren't required to contain text, they provide many member functions
to help manipulate text, such as `trim()`, `splitByte()` and `upperAsc()`. These text manipulation
functions are generally intended to work with UTF-8-encoding strings, but most of them will also
work with any 8-bit format compatible with ASCII, such as ISO 8859-1 or Windows-1252.

When a `StringView` object does contain text, the text string is generally _not_ null-terminated. If
you need a null-terminated string (for example, to pass to a third-party library), you must
construct a string that includes the null terminator byte yourself. The null terminator then counts
towards the total number of bytes in `numBytes`. A convenience function `withNullTerminator()` is
provided for this.

Several `StringView` functions accept byte offset arguments, such as `subStr()`, `left()` and
`right()`. Be aware that when a `StringView` contains UTF-8-encoded text, byte offsets are not
necessarily the same as the number of characters (Unicode points) encoded by the string. For more
information, see [Unicode Support](Unicode).
*/
struct StringView {
    /*!
    The first byte in the immutable memory range.
    */
    const char* bytes = nullptr;

    /*!
    The number of bytes in the immutable memory range.
    */
    u32 numBytes = 0;

    /*!
    Constructs an empty `StringView`.
    */
    PLY_INLINE StringView() = default;

    /*!
    \beginGroup
    Constructs a `StringView` from a null-terminated string. The string memory is expected to remain
    valid for the lifetime of the `StringView`. Note that the null terminator character does not
    count towards `numBytes`. For example, `StringView{"hello"}` results in `numBytes` equal to 5.

    When the argument is a C-style string literal, compilers are able to compute `numBytes` at
    compile time if optimization is enabled.

    The second form of this constructor exists in order to support C++20 compilers, where the type
    of UTF-8 string literals such as `u8"hello"` has been changed from `const char*` (before C++20)
    to `const char8_t*` (after C++20). `StringView` simply interprets such literals as `const
    char*`.
    */
    StringView(const char* s)
        : bytes{s}, numBytes{(u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[numBytes] == 0); // Sanity check; numBytes must fit within 32-bit field
    }

    template <typename U, u32 N,
              std::enable_if_t<std::is_same<U, std::decay_t<decltype(*u8"")>>::value, bool> = false>
    StringView(const U (&s)[N]) : bytes{s}, numBytes{N} {
    }
    /*!
    \endGroup
    */

    /*!
    Constructs a `StringView` from a single byte. `c` is expected to remain valid for the lifetime
    of the `StringView`.
    */
    template <typename U, typename = std::enable_if_t<std::is_same<U, char>::value>>
    PLY_INLINE StringView(const U& c) : bytes{&c}, numBytes{1} {
    }

    /*!
    Constructs a `StringView` explicitly from the arguments. The string memory is expected to remain
    valid for the lifetime of the `StringView`.
    */
    PLY_INLINE StringView(const char* bytes, u32 numBytes) : bytes{bytes}, numBytes{numBytes} {
    }

    /*!
    Returns a `StringView` referencing an immutable range of memory between two pointers. The number
    of bytes in the memory range is given by `endByte` - `startByte`, and `endByte` is considered a
    pointer to the first byte _after_ the memory range.
    */
    static PLY_INLINE StringView fromRange(const char* startByte, const char* endByte) {
        return {startByte, safeDemote<u32>(endByte - startByte)};
    }

    /*!
    Subscript operator with runtime bounds checking.
    */
    PLY_INLINE const char& operator[](u32 index) const {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }

    /*!
    Reverse subscript operator with runtime bound checking. Expects a negative index. `-1` returns
    the last byte of the given string; `-2` returns the second-last byte, etc.
    */
    PLY_INLINE const char& back(s32 ofs = -1) const {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
    }

    /*!
    Returns `bytes + numBytes`. This pointer is considered to point to the first byte _after_ the
    memory range.
    */
    PLY_INLINE const char* end() const {
        return this->bytes + this->numBytes;
    }

    /*!
    Moves `bytes` forward and subtracts the given number of bytes from `numBytes`.
    */
    PLY_INLINE void offsetHead(u32 numBytes) {
        PLY_ASSERT(numBytes <= this->numBytes);
        this->bytes += numBytes;
        this->numBytes -= numBytes;
    }

    /*!
    Advances the end of the memory range by `ofs` bytes while keeping the start of the memory range
    unchanged.
    */
    PLY_INLINE void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= this->numBytes);
        this->numBytes += ofs;
    }

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

    This function uses `ViewInStream` internally. If you need to distinguish between a successful
    parse and an unsuccessful one, create and use a `ViewInStream` object directly.
    */
    template <typename T>
    PLY_NO_INLINE T to(const T& defaultValue = subst::createDefault<T>()) const;

    /*!
    Explicit conversion to `bool`. Returns `true` if the length of the string is greater than 0.
    Allows you to use a `String` object inside an `if` condition.

        if (str) {
            ...
        }
    */
    PLY_INLINE explicit operator bool() const {
        return this->numBytes != 0;
    }

    /*!
    Returns `true` if the length of the string is 0.
    */
    PLY_INLINE bool isEmpty() const {
        return this->numBytes == 0;
    }

    /*!
    \beginGroup
    Returns a substring that starts at the offset given by `start`. The optional `numBytes` argument
    determines the length of the substring in bytes. If `numBytes` is not specified, the substring
    continues to the end of the string.
    */
    PLY_INLINE StringView subStr(u32 start) const {
        PLY_ASSERT(start <= numBytes);
        return {this->bytes + start, this->numBytes - start};
    }
    PLY_INLINE StringView subStr(u32 start, u32 numBytes) const {
        PLY_ASSERT(start <= this->numBytes);
        PLY_ASSERT(start + numBytes <= this->numBytes);
        return {this->bytes + start, numBytes};
    }
    /*!
    \endGroup
    */

    /*!
    Returns `true` if `curByte` points to a byte inside the memory range.
    */
    PLY_INLINE bool contains(const char* curByte) const {
        return uptr(curByte - this->bytes) <= this->numBytes;
    }

    /*!
    Returns a substring that contains only the first `numBytes` bytes of the string.
    */
    PLY_INLINE StringView left(u32 numBytes) const {
        PLY_ASSERT(numBytes <= this->numBytes);
        return {this->bytes, numBytes};
    }

    /*!
    Returns a substring with the last `numBytes` bytes of the string omitted.
    */
    PLY_INLINE StringView shortenedBy(u32 numBytes) const {
        PLY_ASSERT(numBytes <= this->numBytes);
        return {this->bytes, this->numBytes - numBytes};
    }

    /*!
    Returns a substring that contains only the last `numBytes` bytes of the string.
    */
    PLY_INLINE StringView right(u32 numBytes) const {
        PLY_ASSERT(numBytes <= this->numBytes);
        return {this->bytes + this->numBytes - numBytes, numBytes};
    }

    /*!
    Returns the offset of the first occurence of `matchByte` in the string, or `-1` if not found.
    The search begins at the offset specified by `startPos`. This function can find ASCII codes in
    UTF-8 encoded strings, since ASCII codes are always encoded as a single byte in UTF-8.
    */
    PLY_INLINE s32 findByte(char matchByte, u32 startPos = 0) const {
        for (u32 i = startPos; i < this->numBytes; i++) {
            if (this->bytes[i] == matchByte)
                return i;
        }
        return -1;
    }

    /*!
    A template function that returns the offset of the first byte for which `matchFunc` returns
    `true`, or `-1` if none. The search begins at the offset specified by `startPos`. This function
    can be used to find a whitespace character by calling `findByte(isWhite)`.
    */
    template <typename MatchFunc>
    PLY_INLINE s32 findByte(const MatchFunc& matchFunc, u32 startPos = 0) const {
        for (u32 i = startPos; i < this->numBytes; i++) {
            if (matchFunc(this->bytes[i]))
                return i;
        }
        return -1;
    }

    /*!
    \beginGroup
    Reverse `findByte` functions. Returns the offset of the last byte in the string that matches the
    first argument (if passed a `char`) or for which the first argument returns true (if passed a
    function). The optional `startPos` argument specifies an offset at which to begin the search.
    */
    PLY_INLINE s32 rfindByte(char matchByte, u32 startPos) const {
        s32 i = startPos;
        for (; i >= 0; i--) {
            if (this->bytes[i] == matchByte)
                break;
        }
        return i;
    }
    template <typename MatchFunc>
    PLY_INLINE s32 rfindByte(const MatchFunc& matchFunc, u32 startPos) const {
        s32 i = startPos;
        for (; i >= 0; i--) {
            if (matchFunc(this->bytes[i]))
                break;
        }
        return i;
    }
    /*!
    \endGroup
    */
    template <typename MatchFuncOrChar>
    PLY_INLINE s32 rfindByte(const MatchFuncOrChar& matchFuncOrByte) const {
        return this->rfindByte(matchFuncOrByte, this->numBytes - 1);
    }

    /*!
    Returns `true` if the string starts with `arg`.
    */
    PLY_DLL_ENTRY bool startsWith(StringView arg) const;

    /*!
    Returns `true` if the string ends with `arg`.
    */
    PLY_DLL_ENTRY bool endsWith(StringView arg) const;

    /*!
    \beginGroup
    Returns a substring with leading and/or trailing bytes removed. Bytes are removed if `true` is
    returned when passed to `matchFunc`. These functions can be used to trim whitespace characters
    from a UTF-8 string, for example by calling `trim(isWhite)`, since whitespace characters are
    each encoded as a single byte in UTF-8.

    `ltrim()` trims leading bytes only, `rtrim()` trims trailing bytes only, and `trim()` trims both
    leading and trailing bytes.
    */
    PLY_DLL_ENTRY StringView trim(bool (*matchFunc)(char) = isWhite, bool left = true,
                                  bool right = true) const;
    PLY_INLINE StringView ltrim(bool (*matchFunc)(char) = isWhite) const {
        return this->trim(matchFunc, true, false);
    }
    PLY_INLINE StringView rtrim(bool (*matchFunc)(char) = isWhite) const {
        return this->trim(matchFunc, false, true);
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
    PLY_DLL_ENTRY String join(ArrayView<const StringView> comps) const;

    /*!
    Returns a list of the words in the given string using `sep` as a delimiter byte.
    */
    PLY_DLL_ENTRY Array<StringView> splitByte(char sep) const;

    /*!
    Returns a new `String` with all lowercase ASCII characters converted to uppercase. This function
    works with UTF-8 strings. Also works with any 8-bit text encoding compatible with ASCII.
    */
    PLY_DLL_ENTRY String upperAsc() const;

    /*!
    Returns a new `String` with all uppercase ASCII characters converted to lowercase. This function
    works with UTF-8 strings. Also works with any 8-bit text encoding compatible with ASCII.
    */
    PLY_DLL_ENTRY String lowerAsc() const;

    /*!
    Returns a new `String` with the bytes reversed. This function is really only suitable when you
    know that all characters contained in the string are encoded in a single byte.
    */
    PLY_DLL_ENTRY String reversedBytes() const;

    /*!
    Returns a new `String` with each byte passed through the provided `filterFunc`. It's safe to
    call this function on UTF-8 encoded strings as long as `filterFunc` leaves byte values greater
    than or equal to 128 unchanged. Therefore, this function is mainly suitable for filtering ASCII
    codes.
    */
    PLY_DLL_ENTRY String filterBytes(char (*filterFunc)(char)) const;

    /*!
    Returns `true` if the last byte in the string is a zero byte.
    */
    PLY_INLINE bool includesNullTerminator() const {
        return (this->numBytes > 0) ? (this->bytes[this->numBytes - 1] == 0) : false;
    }

    /*!
    If the last byte of the given string is not a zero byte, this function allocates memory for a
    new string, copies the contents of the given string to it, appends a zero byte and returns the
    new string. In that case, the new string's `numBytes` will be one greater than the `numBytes` of
    the original string. If the last byte of the given string is already a zero byte, a view of the
    given string is returned and no new memory is allocated.
    */
    PLY_DLL_ENTRY HybridString withNullTerminator() const;

    /*!
    If the last byte of the given string is not a zero byte, returns a view of the given string. If
    the last byte of the given string is a zero byte, returns a substring with the last byte
    omitted.
    */
    PLY_DLL_ENTRY StringView withoutNullTerminator() const;
};

/*!
\addToClass StringView
Returns:
* `-1` if `a` precedes `b` in sorted order
* `0` if the strings are equal
* `1` if `a` follows `b` in sorted order
Strings are sorted by comparing the unsigned value of each byte. If one of the strings contains
the other as a prefix, the shorter string comes first in sorted order.
*/
PLY_DLL_ENTRY s32 compare(StringView a, StringView b);

/*!
\beginGroup
Comparison functions. `a` < `b` if `a` precedes `b` in sorted order.
*/
PLY_INLINE bool operator==(StringView a, StringView b) {
    return compare(a, b) == 0;
}
PLY_INLINE bool operator!=(StringView a, StringView b) {
    return compare(a, b) != 0;
}
PLY_INLINE bool operator<(StringView a, StringView b) {
    return compare(a, b) < 0;
}
PLY_INLINE bool operator<=(StringView a, StringView b) {
    return compare(a, b) <= 0;
}
PLY_INLINE bool operator>(StringView a, StringView b) {
    return compare(a, b) > 0;
}
PLY_INLINE bool operator>=(StringView a, StringView b) {
    return compare(a, b) >= 0;
}
/*!
\endGroup
*/

/*!
Returns a new `String` containing the concatenation of two `StringViews`.
*/
PLY_DLL_ENTRY String operator+(StringView a, StringView b);

/*!
Returns a new `String` containing the contents of the given `StringView` repeated `count` times.

    StringView{'*'} * 10;    // returns "**********"
*/
PLY_DLL_ENTRY String operator*(StringView str, u32 count);

struct MutStringView {
    /*!
    The first byte in the mutable memory range.
    */
    char* bytes = nullptr;

    /*!
    The number of bytes in the mutable memory range.
    */
    u32 numBytes = 0;

    /*!
    Constructs an empty `MutStringView`.
    */
    PLY_INLINE MutStringView() = default;

    /*!
    Constructs a `MutStringView` explicitly from the arguments. The string memory is expected to
    remain valid for the lifetime of the `MutStringView`.
    */
    PLY_INLINE MutStringView(char* bytes, u32 numBytes) : bytes{bytes}, numBytes{numBytes} {
    }

    char* end() {
        return this->bytes + this->numBytes;
    }

    /*!
    Returns a `MutStringView` referencing an mutable range of memory between two pointers. The
    number of bytes in the memory range is given by `endByte` - `startByte`, and `endByte` is
    considered a pointer to the first byte _after_ the memory range.
    */
    static PLY_INLINE MutStringView fromRange(char* startByte, char* endByte) {
        return {startByte, safeDemote<u32>(endByte - startByte)};
    }

    /*!
    Conversion operator. Makes `MutStringView` implicitly convertible to `StringView`.
    */
    PLY_INLINE operator const StringView&() const {
        return reinterpret_cast<const StringView&>(*this);
    }

    /*!
    Moves `bytes` forward and subtracts the given number of bytes from `numBytes`.
    */
    PLY_INLINE void offsetHead(u32 numBytes) {
        PLY_ASSERT(numBytes <= this->numBytes);
        this->bytes += numBytes;
        this->numBytes -= numBytes;
    }

    /*!
    Advances the end of the memory range by `ofs` bytes while keeping the start of the memory range
    unchanged.
    */
    PLY_INLINE void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= this->numBytes);
        this->numBytes += ofs;
    }
};

template <typename T>
PLY_INLINE ArrayView<const T> ArrayView<T>::from(StringView view) {
    u32 numItems = view.numBytes / sizeof(T); // Divide by constant is fast
    return {(const T*) view.bytes, numItems};
}

template <typename T>
PLY_INLINE ArrayView<T> ArrayView<T>::from(MutStringView view) {
    u32 numItems = view.numBytes / sizeof(T); // Divide by constant is fast
    return {(T*) view.bytes, numItems};
}

template <typename T>
PLY_INLINE StringView ArrayView<T>::stringView() const {
    return {(const char*) items, safeDemote<u32>(numItems * sizeof(T))};
}

template <typename T>
PLY_INLINE MutStringView ArrayView<T>::mutableStringView() {
    return {(char*) items, safeDemote<u32>(numItems * sizeof(T))};
}

namespace subst {
template <typename T>
PLY_INLINE void destructViewAs(StringView view) {
    subst::destructArray<T>((T*) view.bytes, view.numBytes / (u32) sizeof(T));
}
} // namespace subst

} // namespace ply
