/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ChunkList.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/container/Owned.h>
#include <ply-runtime/string/String.h>

namespace ply {

struct StringWriter;

//------------------------------------------------------------------------------------------------
/*!
`OutStream` performs buffered output to an arbitrary output destination. The output can be sent to
an `OutPipe` or to memory.

`OutStream` is similar to `FILE` in C or `std::ostream` in C++. It maintains an internal output
buffer on the heap, and provides functions to quickly write small amounts of data incrementally,
making it suitable for application-level serialization of text and binary formats. `OutPipe`, on the
other hand, is a lower-level class more akin to a Unix file descriptor.

Every `OutStream` is also a `StringWriter`, and you can cast an `OutStream` to a `StringWriter` at
any time by calling `strWriter()`. The main reason why `OutStream` and `StringWriter` are separate
classes is to help express intention in the code. `OutStream` is mainly intended to write binary
data and `StringWriter` is mainly intended to write text, but the two classes are interchangeable.
*/
struct OutStream {
    static constexpr u32 DefaultChunkSizeExp = 12;

    enum class Type : u32 {
        View = 0,
        Pipe,
        Mem,
    };

    struct Status {
        u32 chunkSizeExp : 28;
        u32 type : 2;
        u32 isPipeOwner : 1;
        u32 eof : 1;

        PLY_INLINE Status(Type type, u32 chunkSizeExp = DefaultChunkSizeExp)
            : chunkSizeExp{chunkSizeExp}, type{(u32) type}, isPipeOwner{0}, eof{0} {
        }
    };

    /*!
    A pointer to the next byte in the output buffer. If this pointer is equal to `endByte`, it is
    not safe to write to the pointer. Use member functions such as `tryMakeBytesAvailable()` to
    ensure that this pointer can be written to safely.
    */
    u8* curByte = nullptr;

    /*!
    A pointer to the last byte in the output buffer. This pointer does not necessarily represent the
    end of the output stream; for example, it still might be possible to write more data to the
    underlying `OutPipe`, or to allocate a new chunk in a `ChunkList`, by calling
    `tryMakeBytesAvailable()`.
    */
    u8* endByte = nullptr;
    union {
        u8* startByte;                  // if Type::View
        Reference<ChunkListNode> chunk; // if Type::Pipe or Type::Mem
    };
    union {
        OutPipe* outPipe;                   // only if Type::Pipe
        Reference<ChunkListNode> headChunk; // only if Type::Mem
        void* reserved;                     // only if Type::View
    };
    Status status;

protected:
    void initFirstChunk();
    PLY_DLL_ENTRY void destructInternal();
    PLY_DLL_ENTRY u32 tryMakeBytesAvailableInternal(s32 numBytes);
    bool flushInternal();

    PLY_INLINE OutStream(Type type, u32 chunkSizeExp = DefaultChunkSizeExp)
        : status{type, chunkSizeExp} {
    }

public:
    /*!
    Move constructor. `other` is reset to a null stream. This constructor is safe to call even when
    `other` is an instance of `MemOutStream` or `ViewOutStream`.
    */
    PLY_DLL_ENTRY OutStream(OutStream&& other);

    /*!
    Constructs an `OutStream` that writes to an `OutPipe`. If `outPipe` is an owned pointer, the
    `OutStream` takes ownership of the `OutPipe` and will automatically destroy it in its
    destructor. If `outPipe` is a borrowed pointer, the `OutStream` does not take ownership of the
    `OutPipe`. See `OptionallyOwned`.
    */
    PLY_DLL_ENTRY OutStream(OptionallyOwned<OutPipe>&& outPipe,
                            u32 chunkSizeExp = DefaultChunkSizeExp);

    /*!
    Destructor. `flushMem()` is called before the `OutStream` is destroyed. If the `OutStream` owns
    an `OutPipe`, the `OutPipe` is also destroyed. If the `OutStream` borrows an `OutPipe` but does
    not own it, the `OutPipe` is not destroyed. If the `OutStream` writes to a chunk list in memory,
    the chunk list is destroyed.
    */
    PLY_INLINE ~OutStream() {
        if (this->status.type != (u32) Type::View) {
            this->destructInternal();
        }
    }

    /*!
    Move assignment operator. `other` is reset to a null stream. This operator is safe to call even
    when `other` is an instance of `MemOutStream` or `ViewOutStream`.
    */
    PLY_INLINE void operator=(OutStream&& other) {
        this->~OutStream();
        new (this) OutStream{std::move(other)};
    }

    PLY_INLINE u32 getChunkSize() {
        return 1 << this->status.chunkSizeExp;
    }

    /*!
    Returns `true` if no further data can be written to the stream, such as at the end of a
    fixed-size `ViewOutStream`. This function also returns `true` if there's an error in the
    underlying `OutPipe` that prevents further writing, such as when a network socket is closed
    prematurely.
    */
    PLY_INLINE bool atEOF() const {
        return this->status.eof != 0;
    }

    /*!
    Returns the number of bytes between `curByte` and `endByte`. Equivalent to
    `viewAvailable().numBytes`.
    */
    PLY_INLINE u32 numBytesAvailable() const {
        return safeDemote<u32>(this->endByte - this->curByte);
    }

    /*!
    Returns the memory region between `curByte` and `endByte` as a `BufferView`.
    */
    PLY_INLINE BufferView viewAvailable() {
        return {curByte, safeDemote<u32>(endByte - curByte)};
    }

    /*!
    Returns `true` if `curByte < endByte`.
    */
    PLY_INLINE bool anyBytesAvailable() const {
        return curByte < endByte;
    }

    /*!
    Returns a number that increases each time a byte is written to the `OutStream`. If the
    underlying `OutPipe` is a file, this number typically corresponds to the file offset.
    */
    PLY_DLL_ENTRY u64 getSeekPos() const;

    /*!
    Flushes the internal memory buffer in the same manner as `flushMem()`, then performs an
    implementation-specific device flush if writing to an `OutPipe` and `toDevice` is `true`. For
    example, if `toDevice` is `true`, this will call `fsync()` on when writing to a POSIX file
    descriptor or `FlushFileBuffers()` when writing to a Windows file handle. Returns `true` unless
    there's an error in the implementation-specific device flush.
    */
    PLY_DLL_ENTRY bool flush(bool toDevice = true);

    /*!
    Flushes the internal memory buffer to the underlying `OutPipe` or chunk list. If writing to an
    `OutPipe`, also flushes any application-level memory buffers maintained by the `OutPipe`, such
    as when the `OutPipe` is a conversion, compression or encryption filter.
    */
    PLY_INLINE void flushMem() {
        flush(false);
    }

    /*!
    Attempts to make at least `numBytes` available to write contiguously at `curByte`. Returns the
    number of bytes actually made available. If EOF/error is encountered, the return value will be
    less than `numBytes`; otherwise, it will be greater than or equal to `numBytes`.
    */
    PLY_INLINE u32 tryMakeBytesAvailable(u32 numBytes = 1) {
        if (uptr(this->endByte - this->curByte) < numBytes)
            return this->tryMakeBytesAvailableInternal(numBytes);
        return numBytes;
    }

    /*!
    Always makes at least `numBytes` available to write contiguously at `curByte`. If EOF/error is
    encountered, this function will still make at least `numBytes` available for write; they just
    won't be flushed to the underlying `OutPipe` later.
    */
    PLY_INLINE void makeBytesAvailable(u32 numBytes = 1) {
        // It is currently illegal to call this function on a ViewInStream, but support is possible
        // to add later if needed.
        if (uptr(this->endByte - this->curByte) < numBytes)
            this->tryMakeBytesAvailableInternal(-(s32) numBytes);
    }

    /*!
    Writes a single byte to the output buffer in memory. If the output buffer is full, this function
    flushes the internal memory buffer to the underlying `OutPipe` or chunk list first. Returns
    `true` if the write was successful. The return value of this function is equivalent to
    `!atEOF()`.
    */
    PLY_INLINE bool writeByte(u8 byte) {
        if (!this->tryMakeBytesAvailable())
            return false;
        *this->curByte = byte;
        this->curByte++;
        return true;
    }

    PLY_DLL_ENTRY bool writeSlowPath(ConstBufferView src);

    /*!
    Attempts to write `src` to the output stream in its entirety. Returns `true` if the write was
    successful. The return value of this function is equivalent to `!atEOF()`.
    */
    PLY_INLINE bool write(ConstBufferView src) {
        // FIXME: Re-add race detection
        if ((s32) src.numBytes > this->endByte - this->curByte) {
            return writeSlowPath(src);
        }
        memcpy(this->curByte, src.bytes, src.numBytes);
        this->curByte += src.numBytes;
        return this->status.eof == 0;
    }

    /*!
    Returns a `StringWriter` interface for the output stream.
    */
    PLY_INLINE StringWriter* strWriter();
};

//------------------------------------------------------------------------------------------------
/*!
A `MemOutStream` object is an `OutStream` that writes to a memory.

The amount of data that can be written to `MemOutStream` is unbounded. Internally, the data is
stored in a list of `ChunkListNode`s. As each `ChunkListNode` fills up, a new one is allocated and
added to the end of the list.

Once you've finished writing data to a `MemOutStream`, you can convert it to a single contiguous
memory block by calling `moveToBuffer()`.
*/
struct MemOutStream : OutStream {
    /*!
    Constructs a new `MemOutStream` object.
    */
    PLY_DLL_ENTRY MemOutStream(u32 chunkSizeExp = DefaultChunkSizeExp);

    /*!
    Returns a `ChunkCursor` to the start of the internal list of `ChunkListNode`s. It's safe to call
    this function even before data is written to the `MemOutStream`.
    */
    PLY_INLINE ChunkCursor getHeadCursor() {
        PLY_ASSERT(this->status.type == (u32) Type::Mem);
        PLY_ASSERT(this->headChunk);
        return {this->headChunk, this->headChunk->bytes};
    }

    /*!
    Returns a `Buffer` containing all the data that was written. The `MemOutStream` is reset to a
    null stream as result of this call, which means that no further data can be written.

    If all the data written fits in a single `ChunkListNode` (default fewer than 4096 bytes), no new
    memory is allocated and no data is copied as a result of this call; instead, the memory
    allocation containing the `ChunkListNode` is resized and the returned `Buffer` takes ownership
    of it directly.
    */
    PLY_DLL_ENTRY Buffer moveToBuffer();
    PLY_INLINE String moveToString() {
        return String::moveFromBuffer(this->moveToBuffer());
    }
};

//------------------------------------------------------------------
// ViewOutStream
//------------------------------------------------------------------
struct ViewOutStream : OutStream {
    PLY_INLINE ViewOutStream(BufferView binView) : OutStream{Type::View, 0} {
        this->startByte = binView.bytes;
        this->curByte = binView.bytes;
        this->endByte = binView.bytes + binView.numBytes;
        this->reserved = nullptr;
    }

    PLY_INLINE ~ViewOutStream() {
        PLY_ASSERT(this->status.type == (u32) Type::View);
        // This lets the compiler optimize away the call to destructInternal():
        this->status.type = (u32) Type::View;
    }
};

//------------------------------------------------------------------
// NativeEndianWriter
//------------------------------------------------------------------
class NativeEndianWriter {
public:
    OutStream* outs;

    PLY_INLINE NativeEndianWriter(OutStream* s) : outs(s) {
    }

    template <typename T>
    PLY_INLINE void write(const T& value) {
        outs->write({&value, sizeof(value)});
    }
};

} // namespace ply
