/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/BufferView.h>
#include <ply-runtime/memory/Heap.h>
#include <ply-runtime/container/Hash.h>

namespace ply {

struct ChunkCursor;

//------------------------------------------------------------------------------------------------
/*!
A `Buffer` object owns a single block of memory allocated on the heap. The memory block is freed
when the `Buffer` is destroyed.

`Buffer` is implicitly convertible to `BufferView` and `ConstBufferView` and can be passed as an
argument to any function that expects one of those types.

`Buffer` provides a `resize()` function, but resizing is a expensive operation that potentially
involves copying the entire memory block, so it's not recommended to call it too many times. If you
wish to create a `Buffer` whose size is not known ahead of time, consider creating a `MemOutStream`
instead, then convert to `Buffer` at the final step by calling `MemOutStream::moveToBuffer()`.
*/
struct Buffer {
    /*!
    Pointer to the first byte.
    */
    u8* bytes = nullptr;

    /*!
    The number of bytes allocated.
    */
    u32 numBytes = 0;

    /*!
    Constructs an empty `Buffer`.
    */
    PLY_INLINE Buffer() = default;

    /*!
    Constructs a copy of `other`. A new memory block is allocated on the heap and the contents of
    `other` are copied into it.
    */
    PLY_DLL_ENTRY Buffer(const ConstBufferView other);

    /*!
    Move constructor. `other` is reset to an empty `Buffer`.
    */
    PLY_INLINE Buffer(Buffer&& other) : bytes(other.bytes), numBytes(other.numBytes) {
        other.bytes = nullptr;
        other.numBytes = 0;
    }

    PLY_INLINE ~Buffer() {
        PLY_HEAP.free(this->bytes);
    }

    /*!
    \beginGroup
    Copy assignment operator. If this `Buffer` already owns a memory block, it is destroyed. A new
    memory block is allocated on the heap and the contents of `other` are copied into it.
    */
    PLY_INLINE void operator=(const ConstBufferView other) {
        this->~Buffer();
        new (this) Buffer{other};
    }
    PLY_INLINE void operator=(const Buffer& other) {
        *this = other.view();
    }
    /*!
    \endGroup
    */

    /*!
    Move assignment operator. If this `Buffer` already owns a memory block, it is destroyed. `other`
    is reset to an empty state.
    */
    PLY_INLINE void operator=(Buffer&& other) {
        this->bytes = other.bytes;
        this->numBytes = other.numBytes;
        other.bytes = nullptr;
        other.numBytes = 0;
    }

    /*!
    Returns a `Buffer` that owns a newly allocated block of memory. The contents of the memory block
    are uninitialized.
    */
    static PLY_INLINE Buffer allocate(u32 numBytes) {
        Buffer bin;
        bin.bytes = (u8*) PLY_HEAP.alloc(numBytes);
        bin.numBytes = numBytes;
        return bin;
    }

    /*!
    Returns a `Buffer` that takes ownership of the memory block at `bytes`. This memory block must
    have been allocated on the heap.
    */
    static PLY_INLINE Buffer adopt(void* bytes, u32 numBytes) {
        Buffer bin;
        bin.bytes = (u8*) bytes;
        bin.numBytes = numBytes;
        return bin;
    }

    /*!
    \beginGroup
    Returns a `Buffer` that owns a newly allocated block of memory containing a copy of the data
    contained in some (possible multiple) chunks. The first form of `fromChunks` copies all the data
    from `start` up to the end of the chunk list. The second form copies all the data between
    `start` and `end`.

    The first argument must be an rvalue reference to a `ChunkCursor` which might be moved from
    (reset to an empty state) as a result of this call. This requirement enables an optimization: If
    `start` is the sole reference to the start of a single `ChunkListNode`, no data is copied;
    instead, the chunk's memory block is truncated and ownership is passed directly to the `Buffer`.
    To satisfy this requirement, either use `std::move()` or pass a temporary copy of a
    `ChunkCursor`.

    This function is used internally by `MemOutStream::moveToBuffer()`.
    */
    static PLY_INLINE Buffer fromChunks(ChunkCursor&& start);
    static PLY_INLINE Buffer fromChunks(ChunkCursor&& start, const ChunkCursor& end);
    /*!
    \endGroup
    */

    /*!
    \beginGroup
    Conversion operators. Makes `Buffer` implicitly convertible to `BufferView` and
    `ConstBufferView`.
    */
    PLY_INLINE operator const ConstBufferView&() const {
        return reinterpret_cast<const ConstBufferView&>(*this);
    }
    PLY_INLINE operator const BufferView&() {
        return reinterpret_cast<const BufferView&>(*this);
    }
    /*!
    \endGroup
    */

    /*!
    \beginGroup
    Explicitly create a `BufferView` or `ConstBufferView` to the owned memory block.
    */
    PLY_INLINE const ConstBufferView& view() const {
        return reinterpret_cast<const ConstBufferView&>(*this);
    }
    PLY_INLINE const BufferView& view() {
        return reinterpret_cast<const BufferView&>(*this);
    }
    /*!
    \endGroup
    */

    /*!
    Explicit conversion to `bool`. Returns `true` if the length of the memory block is greater than
    0. Allows you to use a `Buffer` in an `if` condition:

        if (bin) {
            ...
        }
    */
    PLY_INLINE explicit operator bool() const {
        return this->numBytes != 0;
    }

    /*!
    Returns `true` if the length of the memory block is 0.
    */
    PLY_INLINE bool isEmpty() const {
        return this->numBytes == 0;
    }

    /*!
    Resize the owned memory block. If `numBytes` is greater than the current size, new space added
    to the memory block is left uninitialized.
    */
    PLY_INLINE void resize(u32 numBytes) {
        this->bytes = (u8*) PLY_HEAP.realloc(this->bytes, numBytes);
        this->numBytes = numBytes;
    }

    /*!
    Returns `bytes + numBytes`. This pointer is considered to point to the first byte _after_ the
    memory block.
    */
    PLY_INLINE u8* end() const {
        return this->bytes + this->numBytes;
    }

    /*!
    Tests whether the owned memory block exactly matches the contents of `other`.
    */
    PLY_INLINE bool operator==(const ConstBufferView other) const {
        return this->view() == other;
    }

    /*!
    Returns a pointer to the owned memory block while releasing ownership. The `Buffer` is reset to
    an empty state.
    */
    PLY_INLINE void* release() {
        void* bytes = this->bytes;
        this->bytes = nullptr;
        this->numBytes = 0;
        return bytes;
    }

    /*!
    If this `Buffer` owns a memory block, it is destroyed and the `Buffer` is reset to an empty
    state.
    */
    PLY_INLINE void clear() {
        PLY_HEAP.free(this->bytes);
        this->bytes = nullptr;
    }

    /*!
    Feeds the contents of the memory block to a hash function.
    */
    template <typename Hasher>
    PLY_INLINE void appendTo(Hasher& hasher) const {
        hasher.append(this->numBytes);
        hasher.appendBuffer(this->bytes, this->numBytes);
    }
};

} // namespace ply
