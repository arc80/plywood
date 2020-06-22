/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Buffer.h>
#include <ply-runtime/string/StringMixin.h>

namespace ply {

struct HybridString;
struct ChunkCursor;

//------------------------------------------------------------------------------------------------
/*!
A `String` object owns a single block of memory, allocated on the heap, that is generally intended
(but not required) to contain UTF-8-encoded text. Many `String` functions also work with text in any
8-bit format compatible with ASCII, such as ISO 8859-1 or Windows-1252. The memory block is freed
when the `String` is destroyed.

The `String` class uses the same memory representation as `Buffer` but provides member functions
suitable for string manipulation, such as `trim()`, `split()` and concatenation using the `+`
operator.

`String` objects are not automatically null-terminated. If you need a null-terminated string (for
example, to pass to a third-party library), you must add a null terminator byte to the string
yourself. The null terminator then counts towards the number of bytes in `numBytes`. A convenience
function `withNullTerminator()` is provided for this.

Strictly speaking, `String` objects are not _required_ to contain text, though many member functions
expect it. Internally, a `String` just contains a sequence of bytes. The main reason to prefer a
variable of type `String` over `Buffer` is to express the intention for the allocated memory to
contain text and/or for the convenience of having the `String` member functions available.

For more information, see [Unicode Support](Unicode).
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
    PLY_INLINE String(char u) : String{StringView{&u, 1}} {
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
            PLY_HEAP.free(this->bytes);
        }
    }

    /*!
    Copy assignment operator. If this `String` already owns a memory block, it is destroyed. A new
    memory block is allocated on the heap and the contents of `other` are copied into it.
    */
    PLY_INLINE void operator=(const String& other) {
        this->~String();
        new (this) String{other.view()};
    }

    /*!
    Move assignment operator. If this `String` already owns a memory block, it is destroyed. `other`
    is reset to an empty string.
    */
    PLY_INLINE void operator=(String&& other) {
        this->~String();
        new (this) String{std::move(other)};
    }

    /*!
    Converts a `Buffer` to a `String`. No new memory is allocated by this function. The returned
    `String` takes ownership of the memory block previously owned by `buffer`, and `buffer` is reset
    to an empty buffer.

        String str = String::moveFromBuffer(std::move(buffer));
    */
    PLY_INLINE static String moveFromBuffer(Buffer&& buffer) {
        return reinterpret_cast<String&&>(buffer);
    }

    /*!
    Conversion operator. Makes `String` implicitly convertible to `StringView`.
    */
    PLY_INLINE operator const StringView&() const {
        return reinterpret_cast<const StringView&>(*this);
    }

    /*!
    \beginGroup
    Explicitly creates a `StringView`, `BufferView` or `ConstBufferView` into the given string. No
    new memory is allocated by these functions.
    */
    PLY_INLINE const StringView& view() const {
        return reinterpret_cast<const StringView&>(*this);
    }
    PLY_INLINE const ConstBufferView& bufferView() const {
        return reinterpret_cast<const ConstBufferView&>(*this);
    }
    PLY_INLINE const BufferView& bufferView() {
        return reinterpret_cast<const BufferView&>(*this);
    }
    /*!
    \endGroup
    */

    /*!
    If this `String` owns a memory block, it is destroyed and the `String` is reset to an empty
    string.
    */
    PLY_INLINE void clear() {
        if (this->bytes) {
            PLY_HEAP.free(this->bytes);
        }
        this->bytes = nullptr;
        this->numBytes = 0;
    }

    /*!
    Appends the contents of `other` to this string. This function performs a heap reallocation each
    time it is called. If you wish to concatenate several strings together, consider using a
    `StringWriter` instead, then convert to `String` at the final step by calling
    `StringWriter::moveToString()`.
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
    Returns a `String` that owns a newly allocated block of memory containing a copy of the data
    contained in some (possible multiple) chunks. The first form of `fromChunks` copies all the data
    from `start` up to the end of the chunk list. The second form copies all the data between
    `start` and `end`.

    The first argument must be an rvalue reference to a `ChunkCursor` which might be moved from
    (reset to an empty state) as a result of this call. This requirement enables an optimization: If
    `start` is the sole reference to the start of a single `ChunkListNode`, no data is copied;
    instead, the chunk's memory block is truncated and ownership is passed directly to the `Buffer`.
    To satisfy this requirement, either use `std::move()` or pass a temporary copy of a
    `ChunkCursor`.

    This function is used internally by `StringOutStream::moveToString()`.
    */
    static PLY_INLINE String fromChunks(ChunkCursor&& start);
    static PLY_INLINE String fromChunks(ChunkCursor&& start, const ChunkCursor& end);
    /*!
    \endGroup
    */

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
PLY_INLINE String StringMixin<Derived>::reversedUTF8() const {
    return static_cast<const Derived*>(this)->view().reversedUTF();
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
            PLY_HEAP.free(this->bytes);
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
