/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>

namespace ply {

//------------------------------------------------------------------------------------------------
/*!
A `ConstBufferView` references an immutable range of memory. It consists of a pointer `bytes` and an
integer `numBytes`. It is used to . A `ConstBufferView` does not own the memory it points to, and no
heap memory is freed when the `ConstBufferView` is destroyed.

`ConstBufferView` is similar to `BufferView` except that the memory referenced by `ConstBufferView`
is `const`, so it's illegal to modify its contents. `ConstBufferView` is used as the sole argument
to `OutPipe::write()` and `OutStream::write()`.

`BufferView` is implicitly convertible to `ConstBufferView` and can be passed as an argument to any
function that expects a `ConstBufferView`.
*/
struct ConstBufferView {
    /*!
    The first byte in the immutable memory range.
    */
    const u8* bytes = nullptr;
    /*!
    The number of bytes in the immutable memory range.
    */
    u32 numBytes = 0;

    /*!
    Constructs an empty `ConstBufferView`.
    */
    PLY_INLINE ConstBufferView() = default;
    /*!
    Constructs a `ConstBufferView` from an explicit pointer and number of bytes.
    */
    PLY_INLINE ConstBufferView(const void* bytes, u32 numBytes)
        : bytes{(const u8*) bytes}, numBytes{numBytes} {
    }
    /*!
    FIXME: Make this show up
    Constructs a `ConstBufferView` from a string literal. (?)
    */
    template <u32 N>
    PLY_INLINE ConstBufferView(const u8 (&bytes)[N]) : bytes{bytes}, numBytes{N} {
    }
    /*!
    Returns a `ConstBufferView` referencing a range of memory between two pointers. The number of
    bytes in the memory range is given by `endByte` - `startByte`, and `endByte` is considered a
    pointer to the first byte _after_ the memory range.
    */
    static PLY_INLINE ConstBufferView fromRange(const u8* startByte, const u8* endByte) {
        return {startByte, safeDemote<u32>(endByte - startByte)};
    }
    /*!
    Subscript operator that performs runtime bounds checking.
    */
    PLY_INLINE u8 operator[](u32 ofs) const {
        PLY_ASSERT(ofs < numBytes);
        return bytes[ofs];
    }
    /*!
    Advances the start of the memory range by `ofs` bytes while keeping the end of the memory range
    unchanged.
    */
    PLY_INLINE void offsetHead(u32 ofs) {
        PLY_ASSERT(ofs <= numBytes);
        bytes += ofs;
        numBytes -= ofs;
    }
    /*!
    Advances the end of the memory range by `ofs` bytes while keeping the start of the memory range
    unchanged.
    */
    PLY_INLINE void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= numBytes);
        bytes += ofs;
    }
    /*!
    Returns a new, smaller `ConstBufferView` that references a subrange of this `ConstBufferView`,
    with runtime bounds checking. `start` is the offset of the new `ConstBufferView` relative to the
    start of this one, and `numBytes` is the number of bytes in the new `ConstBufferView`.
    */
    PLY_INLINE ConstBufferView subView(u32 start, u32 numBytes) const {
        PLY_ASSERT(start <= this->numBytes); // FIXME: Support different end parameters
        PLY_ASSERT(start + numBytes <= this->numBytes);
        return {this->bytes + start, numBytes};
    }
    /*!
    Returns `true` if `curByte` points to a byte inside the memory range.
    */
    PLY_INLINE bool contains(const u8* curByte) const {
        return uptr(curByte - this->bytes) <= this->numBytes;
    }
    /*!
    Returns `true` if the contents of this `ConstBufferView` exactly match the contents of `other`.
    */
    PLY_DLL_ENTRY bool operator==(ConstBufferView other) const;
    /*!
    Returns `true` if the contents of this `ConstBufferView` do not exactly match the contents of
    `other`.
    */
    PLY_INLINE bool operator!=(ConstBufferView other) {
        return !(*this == other);
    }
};

//------------------------------------------------------------------------------------------------
/*!
A `BufferView` references a mutable range of memory. It consists of a pointer `bytes` and an integer
`numBytes`. A `BufferView` does not own the memory it points to, and no heap memory is freed when
the `BufferView` is destroyed.

`BufferView` is similar to `ConstBufferView` except that the memory referenced by `BufferView` is
mutable (not `const`), so it's legal to modify its contents. `BufferView` is used as the sole
argument to `InPipe::read()` and `InStream::read()`.

`BufferView` is implicitly convertible to `ConstBufferView` and can be passed as an argument to any
function that expects a `ConstBufferView`.
*/
struct BufferView {
    /*!
    The first byte in the mutable memory range.
    */
    u8* bytes = nullptr;
    /*!
    The number of bytes in the mutable memory range.
    */
    u32 numBytes = 0;

    /*!
    Constructs an empty `BufferView`.
    */
    PLY_INLINE BufferView() = default;
    /*!
    Constructs a `BufferView` from an explicit pointer and number of bytes.
    */
    PLY_INLINE BufferView(void* bytes, u32 numBytes) : bytes{(u8*) bytes}, numBytes{numBytes} {
    }
    /*!
    FIXME: Make this show up
    Constructs a `ConstBufferView` from a string literal. (?)
    */
    template <u32 N>
    PLY_INLINE BufferView(u8 (&bytes)[N]) : bytes{bytes}, numBytes{N} {
    }
    /*!
    Returns a `BufferView` referencing a mutable range of memory between two pointers. The number of
    bytes in the memory range is given by `endByte` - `startByte`, and `endByte` is considered a
    pointer to the first byte _after_ the memory range.
    */
    static PLY_INLINE BufferView fromRange(u8* startByte, u8* endByte) {
        return {startByte, safeDemote<u32>(endByte - startByte)};
    }
    /*!
    Conversion operator. Makes `BufferView` implicitly convertible to `ConstBufferView`.
    */
    PLY_INLINE operator const ConstBufferView&() const {
        return reinterpret_cast<const ConstBufferView&>(*this);
    }
    /*!
    Subscript operator with runtime bounds checking.
    */
    PLY_INLINE u8& operator[](u32 ofs) const {
        PLY_ASSERT(ofs < numBytes);
        return bytes[ofs];
    }
    /*!
    Advances the start of the memory range by `ofs` bytes while keeping the end of the memory range
    unchanged.
    */
    PLY_INLINE void offsetHead(u32 ofs) {
        PLY_ASSERT(ofs <= numBytes);
        bytes += ofs;
        numBytes -= ofs;
    }
    /*!
    Advances the end of the memory range by `ofs` bytes while keeping the start of the memory range
    unchanged.
    */
    PLY_INLINE void offsetBack(s32 ofs) {
        PLY_ASSERT((u32) -ofs <= numBytes);
        bytes += ofs;
    }
    /*!
    Returns a new, smaller `BufferView` that references a subrange of this `BufferView`, with
    runtime bounds checking. `start` is the offset of the new `BufferView` relative to the start of
    this one, and `numBytes` is the number of bytes in the new `BufferView`.
    */
    PLY_INLINE BufferView subView(u32 start, u32 numBytes) const {
        PLY_ASSERT(start <= this->numBytes); // FIXME: Support different end parameters
        PLY_ASSERT(start + numBytes <= this->numBytes);
        return {this->bytes + start, numBytes};
    }
    /*!
    Returns `true` if `curByte` points to a byte inside the memory range.
    */
    PLY_INLINE bool contains(const u8* curByte) const {
        return uptr(curByte - this->bytes) <= this->numBytes;
    }
    // FIXME: Should implement == and other ConstBufferView member functions here
};

} // namespace ply
