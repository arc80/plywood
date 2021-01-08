/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Array.h>
#include <ply-runtime/container/BufferView.h>
#include <ply-runtime/container/Hash.h>
#include <ply-runtime/container/Tuple.h>
#include <string> // for char_traits

namespace ply {

struct String;
struct HybridString;

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
A `StringView` references an immutable range of memory that is generally intended (but not required)
to contain UTF-8-encoded text. It consists of a pointer `bytes` and an integer `numBytes`. Many
`StringView` functions also work with text in any 8-bit format compatible with ASCII, such as ISO
8859-1 or Windows-1252. A `StringView` does not own the memory it points to, and no heap memory is
freed when the `StringView` is destroyed.

The `StringView` class is similar to `ConstBufferView` except that it provides member functions
suitable for string manipulation, such as `trim()`, `split()` and concatenation using the `+`
operator.

`StringView` objects are not implicitly convertible to or from `ConstBufferView`, but they can be
explicitly converted using `bufferView()` or `fromBufferView()`.

`StringView` objects are not guaranteed to be null-terminated. If you need a null-terminated string
(for example, to pass to a third-party library), you must construct a string that includes the null
terminator byte yourself. The null terminator then counts towards the total number of bytes in
`numBytes`. A convenience function `withNullTerminator()` is provided for this.

Strictly speaking, `StringView` objects are not _required_ to contain text, though many member
functions expect it. Internally, a `StringView` is simply a reference to an immutable sequence of
bytes. The main reason to prefer a variable of type `StringView` over `ConstBufferView` is to
express the intention for the memory range to contain text and/or for the convenience of having the
`StringView` member functions available.

Several `StringView` functions work directly with byte offsets, such as `subStr()`, `left()` and
`right()`. Be aware that when a string is encoded in UTF-8, byte offsets are not necessarily the
same as the number of characters (Unicode points) encoded by the string. If the string contains
multibyte characters, it is the caller's responsibility to pass byte offsets that begin and end at
character boundaries.

For more information, see [Unicode Support](Unicode).

[FIXME: Mention caller is responsible for lifetime of the underlying data.]
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

    If you have a C-style string literal, and you want the null terminator character included in the
    `StringView`, use `StringView::fromBufferView("hello")` instead. In that case, the null
    terminator character counts towards the number of bytes in `numBytes`, making `numBytes` equal
    to 6 in this example.

    The second form of this constructor exists in order to support C++20 compilers, where the type
    of UTF-8 string literals such as `u8"hello"` has been changed from `const char*` (before C++20)
    to `const char8_t*` (after C++20). `StringView` simply interprets such literals as `const
    char*`.
    */
    PLY_INLINE StringView(const char* s)
        : bytes{s}, numBytes{(u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[numBytes] == 0); // Sanity check; numBytes must fit within 32-bit field
    }

    template <typename U, std::size_t N,
              std::enable_if_t<std::is_same<U, std::decay_t<decltype(*u8"")>>::value, bool> = false>
    PLY_INLINE StringView(const U (&s)[N]) : StringView((const char*) s) {
    }
    /*!
    \endGroup
    */

    /*!
    Constructs a `StringView` from a single byte. `c` is expected to remain valid for the lifetime
    of the `StringView`.
    */
    PLY_INLINE StringView(const char& c) : bytes{&c}, numBytes{1} {
    }

    /*!
    Constructs a `StringView` explicitly from the arguments. The string memory is expected to remain
    valid for the lifetime of the `StringView`.
    */
    PLY_INLINE StringView(const char* bytes, u32 numBytes) : bytes{bytes}, numBytes{numBytes} {
    }

    /*!
    \beginGroup
    Explicitly convert the `StringView` to a `ConstBufferView`.
    */
    PLY_INLINE ConstBufferView& bufferView() {
        return reinterpret_cast<ConstBufferView&>(*this);
    }
    PLY_INLINE const ConstBufferView& bufferView() const {
        return reinterpret_cast<const ConstBufferView&>(*this);
    }
    /*!
    \endGroup
    */

    /*!
    Explicitly convert a `ConstBufferView` to a `StringView`.
    */
    static PLY_INLINE const StringView& fromBufferView(const ConstBufferView& binView) {
        return reinterpret_cast<const StringView&>(binView);
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
    Returns:
    * `-1` if `str0` precedes `str1` in sorted order
    * `0` if the strings are equal
    * `1` if `str0` follows `str1` in sorted order
    Strings are sorted by comparing the unsigned value of each byte. If one of the strings contains
    the other as a prefix, the shorter string comes first in sorted order.
    */
    PLY_DLL_ENTRY friend s32 compare(StringView str0, StringView str1);

    /*!
    Returns `true` if the string precedes `other` in sorted order. Equivalent to `compare(*this,
    other) < 0`.
    */
    PLY_INLINE bool operator<(StringView other) const {
        return compare(*this, other) < 0;
    }

    /*!
    \beginGroup
    Returns `true` if the string contents are identical (or not identical) when compared
    byte-for-byte.
    */
    PLY_INLINE bool operator==(StringView src) const {
        return this->bufferView() == src.bufferView();
    }
    PLY_INLINE bool operator!=(StringView src) const {
        return !(this->bufferView() == src.bufferView());
    }
    /*!
    \endGroup
    */

    /*!
    Returns a new `String` containing the concatenation of two `StringViews`.
    */
    PLY_DLL_ENTRY String operator+(StringView other) const;

    /*!
    Returns a new `String` containing the contents of the `StringView` repeated `count` times.

        StringView{'*'};    // returns "**********"
    */
    PLY_DLL_ENTRY String operator*(u32 count) const;

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
    PLY_DLL_ENTRY StringView trim(bool (*matchFunc)(char), bool left = true,
                                  bool right = true) const;
    PLY_INLINE StringView ltrim(bool (*matchFunc)(char)) const {
        return this->trim(matchFunc, true, false);
    }
    PLY_INLINE StringView rtrim(bool (*matchFunc)(char)) const {
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

    // FIXME: Make sure this documentation string is OK
    /*!
    Returns a new `String` with UTF-8 characters reversed. For example,
    `StringView{"`&#x1f60b;&#x1f37a;&#x1f355;`"}.reversedUTF8()` returns
    `"`&#x1f355;&#x1f37a;&#x1f60b;`"`.
    */
    PLY_DLL_ENTRY String reversedUTF8() const;

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

// Note: char* will be implicitly converted to StringView here:
PLY_INLINE Hasher& operator<<(Hasher& h, StringView str) {
    return h << str.bufferView();
}

} // namespace ply
