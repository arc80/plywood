/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/string/StringMixin.h>
#include <ply-runtime/Heap.h>

namespace ply {

struct HybridString;

//------------------------------------------------------------------------------------------------
/*!
A `String` object owns a single block of memory allocated on the heap. The memory block is freed
when the `String` is destroyed.

`String` objects often contain UTF-8-encoded text, but this is isn't a requirement. In general,
a `String` can hold any kind of data. For example, `FileSystem::loadBinary` returns raw file
contents in a `String` object.

Even though `String` objects aren't required to contain text, they provide many member functions to
help manipulate text, such as `trim()`, `splitByte()` and `upperAsc()`. These text manipulation
functions are generally intended to work with UTF-8-encoding strings, but most of them will also
work with any 8-bit format compatible with ASCII, such as ISO 8859-1 or Windows-1252.

When a `String` object does contain text, the text string is generally _not_ null-terminated. If you
need a null-terminated string (for example, to pass to a third-party library), you must add a null
terminator byte to the string yourself. The null terminator then counts towards the number of bytes
in `numBytes`. A convenience function `withNullTerminator()` is provided for this.

Several `String` functions accept byte offset arguments, such as `subStr()`, `left()` and `right()`.
Be aware that when a `StringView` contains UTF-8-encoded text, byte offsets are not necessarily the
same as the number of characters (Unicode points) encoded by the string. For more information, see
[Unicode Support](Unicode).
*/
struct String : StringMixin<String> {
    using View = StringView;

    /*!
    The bytes.
    */
    char* bytes = nullptr;

    /*!
    The number of bytes.
    */
    u32 numBytes = 0;

    /*!
    Constructs an empty string.
    */
    PLY_INLINE String() = default;

    /*!
    \beginGroup
    Each of these constructors constructs a copy of its argument. A new memory block is allocated on
    the heap and the contents of the argument are copied into it.
    */
    PLY_DLL_ENTRY String(StringView other);
    PLY_INLINE String(const String& other) : String{other.view()} {
    }
    PLY_INLINE String(const char* s) : String{StringView{s}} {
    }
    template <typename U, typename = std::enable_if_t<std::is_same<U, char>::value>>
    PLY_INLINE String(const U& u) : String{StringView{&u, 1}} {
    }
    /*!
    \endGroup
    */

    /*!
    Move constructor. The `other` is reset to an empty string.
    */
    PLY_INLINE String(String&& other) : bytes{other.bytes}, numBytes{other.numBytes} {
        other.bytes = nullptr;
        other.numBytes = 0;
    }

    /*!
    Conditionally moves or copies from `other`. If `other` owns its memory, the new `String` takes
    ownership of that memory and `other` is reset to an empty string. If `other` does not own its
    memory, a new memory block is allocated on the heap and the contents of `other` are copied into
    it.
    */
    PLY_DLL_ENTRY String(HybridString&& other);

    PLY_INLINE ~String() {
        if (this->bytes) {
            Heap.free(this->bytes);
        }
    }

    /*!
    Copy assignment operator. If this `String` already owns a memory block, it is destroyed. A new
    memory block is allocated on the heap and the contents of `other` are copied into it.
    */
    template <typename = void>
    PLY_INLINE void operator=(StringView other) {
        char* bytesToFree = this->bytes;
        new (this) String{other};
        if (bytesToFree) {
            Heap.free(bytesToFree);
        }
    }

    /*!
    Move assignment operator. If this `String` already owns a memory block, it is destroyed. `other`
    is reset to an empty string.
    */
    PLY_INLINE void operator=(const String& other) {
        this->~String();
        new (this) String{other.view()};
    }
    PLY_INLINE void operator=(String&& other) {
        this->~String();
        new (this) String{std::move(other)};
    }

    /*!
    Conversion operator. Makes `String` implicitly convertible to `StringView`.
    */
    PLY_INLINE operator const StringView&() const {
        return reinterpret_cast<const StringView&>(*this);
    }

    /*!
    Explicitly creates a `StringView`. No new memory is allocated by this function.
    */
    PLY_INLINE const StringView& view() const {
        return reinterpret_cast<const StringView&>(*this);
    }

    /*!
    If this `String` owns a memory block, it is destroyed and the `String` is reset to an empty
    string.
    */
    PLY_INLINE void clear() {
        if (this->bytes) {
            Heap.free(this->bytes);
        }
        this->bytes = nullptr;
        this->numBytes = 0;
    }

    /*!
    Appends the contents of `other` to this string. This function performs a heap reallocation each
    time it is called. If you wish to concatenate several strings together, consider using a
    `MemOutStream` instead, then convert to `String` at the final step by calling
    `MemOutStream::moveToString()`.
    */
    PLY_INLINE void operator+=(StringView other) {
        *this = this->view() + other;
    }

    /*!
    Returns a `String` that owns a newly allocated block of memory. The contents of the memory block
    are uninitialized. Note that this is a `static` function; the new `String` is returned
    explicitly.

        String str = String::allocate(100);
    */
    PLY_DLL_ENTRY static String allocate(u32 numBytes);

    /*!
    Resize the owned memory block. If `numBytes` is greater than the current size, new space added
    to the memory block is left uninitialized.
    */
    PLY_DLL_ENTRY void resize(u32 numBytes);

    /*!
    Returns a `String` that takes ownership of the memory block at `bytes`. This memory block must
    have been allocated on the heap.
    */
    static PLY_INLINE String adopt(char* bytes, u32 numBytes) {
        String str;
        str.bytes = bytes;
        str.numBytes = numBytes;
        return str;
    }

    /*!
    Returns a pointer to the owned memory block while releasing ownership. The `String` is reset to
    an empty string.
    */
    PLY_INLINE char* release() {
        char* r = this->bytes;
        this->bytes = nullptr;
        this->numBytes = 0;
        return r;
    }

    /*!
    Template function that expands the format string `fmt` using the given arguments and returns a
    new `String` containing the result.

        String str = String::format("The answer is {}.\n", 42);

    For more information, see [Converting Values to Text](ConvertingValuesToText).
    */
    template <typename... Args>
    static PLY_INLINE String format(StringView fmt, const Args&... args);

    /*!
    Template function that converts the provided argument to a string.

        String s = String::from(123.456);

    For more information, see [Converting Values to Text](ConvertingValuesToText).
    */
    template <typename T>
    static PLY_INLINE String from(const T& value);

    /*!
    \beginGroup
    Subscript operators with runtime bounds checking.
    */
    PLY_INLINE const char& operator[](u32 index) const {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }
    PLY_INLINE char& operator[](u32 index) {
        PLY_ASSERT(index < this->numBytes);
        return this->bytes[index];
    }
    /*!
    \endGroup
    */

    /*!
    \beginGroup
    Reverse subscript operators with runtime bounds checking. `ofs` must be less than zero. By
    default, returns the last byte in the string. Equivalent to `str[str.numBytes + ofs]`.
    */
    PLY_INLINE const char& back(s32 ofs = -1) const {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
    }
    PLY_INLINE char& back(s32 ofs = -1) {
        PLY_ASSERT(u32(-ofs - 1) < this->numBytes);
        return this->bytes[this->numBytes + ofs];
    }
    /*!
    \endGroup
    */
};

namespace impl {
template <>
struct InitListType<String> {
    using Type = StringView;
};
} // namespace impl

template <typename Derived>
PLY_INLINE String StringMixin<Derived>::operator+(StringView other) const {
    return static_cast<const Derived*>(this)->view() + other;
}

template <typename Derived>
PLY_INLINE String StringMixin<Derived>::operator*(u32 count) const {
    return static_cast<const Derived*>(this)->view() * count;
}

template <typename Derived>
PLY_INLINE String StringMixin<Derived>::join(ArrayView<const StringView> comps) const {
    return static_cast<const Derived*>(this)->view().join(comps);
}

template <typename Derived>
PLY_INLINE String StringMixin<Derived>::upperAsc() const {
    return static_cast<const Derived*>(this)->view().upperAsc();
}

template <typename Derived>
PLY_INLINE String StringMixin<Derived>::lowerAsc() const {
    return static_cast<const Derived*>(this)->view().lowerAsc();
}

template <typename Derived>
PLY_INLINE String StringMixin<Derived>::reversedBytes() const {
    return static_cast<const Derived*>(this)->view().reversedBytes();
}

template <typename Derived>
PLY_INLINE String StringMixin<Derived>::filterBytes(char (*filterFunc)(char)) const {
    return static_cast<const Derived*>(this)->view().filterBytes(filterFunc);
}

//------------------------------------------------------------------------------------------------
/*!
A `HybridString` is a cross between a `String` and a `StringView`. It references a range of memory
that is generally intended (but not required) to contain UTF-8-encoded text. The `HybridString` may
or may not own the memory it points to, as determined by the `isOwner` flag. If `isOwner` is
non-zero, the memory block owned by the `HybridString` will be freed from heap when the
`HybridString` is destroyed. If `isOwner` is zero, the `HybridString` does not own the memory it
points to, and the caller must ensure that the memory remains valid for the lifetime of the
`HybridString`.
*/
struct HybridString : StringMixin<HybridString> {
    /*!
    The bytes. Should be treated as const if isOwner == 0.
    */
    char* bytes;

    /*!
    \beginGroup
    Number of bytes and owner flag.
    */
    u32 isOwner : 1;
    u32 numBytes : 31;
    /*!
    \endGroup
    */

    /*!
    Constructs an empty `HybridString`.
    */
    PLY_INLINE HybridString() : bytes{nullptr}, isOwner{0}, numBytes{0} {
    }

    /*!
    Constructs a `HybridString` that views the same memory block as `view`. `view` is expected to
    remain valid for the lifetime of the `HybridString`. No new memory is allocated.
    */
    PLY_INLINE HybridString(StringView view)
        : bytes{const_cast<char*>(view.bytes)}, isOwner{0}, numBytes{view.numBytes} {
        PLY_ASSERT(view.numBytes < (1u << 30));
    }

    PLY_INLINE HybridString(const String& str) : HybridString{str.view()} {
    }

    /*!
    Constructs a `HybridString` that takes ownership of the memory block owned by `str`. `str` is
    reset to an empty string.
    */
    PLY_INLINE HybridString(String&& str) {
        this->isOwner = 1;
        PLY_ASSERT(str.numBytes < (1u << 30));
        this->numBytes = str.numBytes;
        this->bytes = str.release();
    }

    /*!
    Move constructor.
    */
    PLY_INLINE HybridString(HybridString&& other)
        : bytes{other.bytes}, isOwner{other.isOwner}, numBytes{other.numBytes} {
        other.bytes = nullptr;
        other.isOwner = 0;
        other.numBytes = 0;
    }

    /*!
    Copy constructor.
    */
    PLY_NO_INLINE HybridString(const HybridString& other);

    /*!
    Construct a `HybridString` from a string literal. Compilers seem able to calculate its length at
    compile time (if optimization is enabled).
    */
    PLY_INLINE HybridString(const char* s)
        : bytes{const_cast<char*>(s)}, isOwner{0}, numBytes{
                                                       (u32) std::char_traits<char>::length(s)} {
        PLY_ASSERT(s[this->numBytes] == 0); // Sanity check; numBytes must fit in 31-bit field
    }

    PLY_INLINE ~HybridString() {
        if (this->isOwner) {
            Heap.free(this->bytes);
        }
    }

    /*!
    Move assignment operator.
    */
    PLY_INLINE void operator=(HybridString&& other) {
        this->~HybridString();
        new (this) HybridString(std::move(other));
    }

    /*!
    Copy assignment operator.
    */
    PLY_INLINE void operator=(const HybridString& other) {
        this->~HybridString();
        new (this) HybridString(other);
    }

    /*!
    Conversion operator. Makes `HybridString` implicitly convertible to `StringView`.
    */
    PLY_INLINE operator StringView() const {
        return {this->bytes, this->numBytes};
    }

    /*!
    Explicitly creates a `StringView` into the `HybridString`. No new memory is allocated by this
    function.
    */
    PLY_INLINE StringView view() const {
        return {this->bytes, this->numBytes};
    }
};

template <typename Derived>
PLY_INLINE HybridString StringMixin<Derived>::withNullTerminator() const {
    return static_cast<const Derived*>(this)->view().withNullTerminator();
}

} // namespace ply
