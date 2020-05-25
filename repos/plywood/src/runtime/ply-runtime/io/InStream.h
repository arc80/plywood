/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/ChunkList.h>
#include <ply-runtime/io/Pipe.h>
#include <ply-runtime/container/Owned.h>

namespace ply {

struct ViewInStream;
struct StringReader;
struct StringViewReader;

//------------------------------------------------------------------------------------------------
/*!
`InStream` performs buffered input from an arbitrary input source. The underlying input can either
come from an `InPipe` or from memory.

`InStream` is similar to `FILE` in C or `std::istream` in C++. It maintains an internal input buffer
on the heap, and provides functions to quickly read small amounts of data incrementally, making it
suitable for application-level parsing of text and binary formats. `InPipe`, on the other hand, is a
lower-level class more akin to a Unix file descriptor.

If the `InStream`'s underlying input comes from an `InPipe`, the internal input buffer is managed by
a `ChunkListNode`, and you can call `getCursor()` at any time to create a `ChunkCursor`. You can
then rewind the `InStream` to an earlier point using `rewind()`, or copy some data between two
`ChunkCursor`s into a single contiguous memory buffer.

Every `InStream` is also a `StringReader`, and you can cast an `InStream` to a `StringReader` at any
time by calling `strReader()`. The main reason why `InStream` and `StringReader` are separate
classes is to help express intention in the code. `InStream` is mainly intended to read binary data
and `StringReader` is mainly intended to read text, but the two classes are interchangeable.
*/
struct InStream {
    static const u32 DefaultChunkSizeExp = 12;

    enum class Type : u32 {
        View = 0,
        Pipe,
        Mem,
    };

    struct Status {
        u32 chunkSizeExp : 27;
        u32 type : 2;
        u32 isPipeOwner : 1;
        u32 eof : 1;
        u32 parseError : 1;

        PLY_INLINE Status(Type type, u32 chunkSizeExp = InStream::DefaultChunkSizeExp)
            : chunkSizeExp{chunkSizeExp}, type{(u32) type}, isPipeOwner{0}, eof{0}, parseError{0} {
        }
    };

    /*!
    A pointer to the next byte in the input buffer. If this pointer is equal to `endByte`, it is not
    safe to read from the pointer. Use member functions such as `tryMakeBytesAvailable()` to ensure
    that this pointer can be read safely.
    */
    u8* curByte = nullptr;

    /*!
    A pointer to the last byte in the input buffer. This pointer does not necessarily represent the
    end of the input stream; for example, it still might be possible to read more data from the
    underlying `InPipe`, or to advance to the next chunk in a `ChunkList`, by calling
    `tryMakeBytesAvailable()`.
    */
    u8* endByte = nullptr;

    union {
        u8* startByte;                  // if Type::View
        Reference<ChunkListNode> chunk; // if Type::Pipe or Type::Mem
    };
    union {
        InPipe* inPipe; // only if Type::Pipe
        void* reserved; // if Type::Mem or Type::View
    };
    Status status;

protected:
    PLY_DLL_ENTRY void destructInternal();
    PLY_DLL_ENTRY u32 tryMakeBytesAvailableInternal(u32 numBytes);

    PLY_INLINE InStream(Type type, u32 chunkSizeExp = DefaultChunkSizeExp)
        : status{type, chunkSizeExp} {
    }

public:
    /*!
    Constructs an empty `InStream`. You can replace it with a valid `InStream` later using move
    assignment.
    */
    PLY_INLINE InStream() : startByte{nullptr}, reserved{nullptr}, status{Type::View, 0} {
    }

    /*!
    Move constructor.
    */
    PLY_DLL_ENTRY InStream(InStream&& other);

    /*!
    Constructs an `InStream` from an `InPipe`. If `inPipe` is an owned pointer, the
    `InStream` takes ownership of the `InPipe` and will automatically destroy it in its
    destructor. If `inPipe` is a borrowed pointer, the `InStream` does not take ownership of the
    `InPipe`.
    */
    PLY_DLL_ENTRY InStream(OptionallyOwned<InPipe>&& inPipe,
                           u32 chunkSizeExp = DefaultChunkSizeExp);

    PLY_INLINE ~InStream() {
        if (this->status.type != (u32) Type::View) {
            this->destructInternal();
        }
    }

    /*!
    Move assignment operator.
    */
    PLY_DLL_ENTRY void operator=(InStream&& other);

    /*!
    Return `true` if the `InStream` is reading from a fixed memory buffer. Typically this means that
    the `InStream` was created from a derived class such as `ViewInStream` or `StringViewReader`.
    */
    PLY_INLINE bool isView() const {
        return this->status.type == (u32) Type::View;
    }

    PLY_INLINE u32 getChunkSize() {
        return 1 << this->status.chunkSizeExp;
    }

    /*!
    Returns `true` if no further data can be read from the stream, such as when the end of-file is
    encountered. This function also returns `true` if there's an error in the underlying `InPipe`
    that prevents further reading, such as when a network socket is closed prematurely.
    */
    PLY_INLINE bool atEOF() const {
        return this->status.eof != 0;
    }

    /*!
    Returns the number of bytes between `curByte` and `endByte`. Equivalent to
    `viewAvailable().numBytes`.
    */
    PLY_INLINE s32 numBytesAvailable() const {
        return safeDemote<s32>(this->endByte - this->curByte);
    }

    /*!
    Returns the memory region between `curByte` and `endByte` as a `ConstBufferView`.
    */
    PLY_INLINE ConstBufferView viewAvailable() const {
        return {this->curByte, safeDemote<u32>(this->endByte - this->curByte)};
    }

    /*!
    Returns a number that increases each time a byte is read from the `InStream`. If the underlying
    `InPipe` is a file, this number typically corresponds to the file offset.
    */
    PLY_DLL_ENTRY u64 getSeekPos() const;

    /*!
    Returns a `ChunkCursor` at the current input position. The `ChunkCursor` increments the
    reference count of the `InStream`'s internal `ChunkListNode`, preventing it from being destroyed
    when reading past the end of the chunk. This function is used internally by
    `StringReader::readString` in order to copy some region of the input to a `String`. In
    particular, `StringReader::readString<fmt::Line>` returns a single line of input as a `String`
    even if it originally spanned multiple `ChunkListNode`s.
    */
    PLY_DLL_ENTRY ChunkCursor getCursor() const;

    /*!
    Rewinds the `InStream` back to a previously saved position. This also clears the `InStream`'s
    end-of-file status.
    */
    PLY_DLL_ENTRY void rewind(ChunkCursor cursor);

    /*!
    Attempts to make at least `numBytes` available to read contiguously at `curByte`. Returns the
    number of bytes actually made available. If the underlying `InPipe` is waiting for data, this
    function will block until at least `numBytes` bytes arrive. If EOF/error is encountered, the
    return value will be less than `numBytes`; otherwise, it will be greater than or equal to
    `numBytes`.
    */
    PLY_INLINE u32 tryMakeBytesAvailable(u32 numBytes = 1) {
        if (uptr(this->endByte - this->curByte) < numBytes)
            return this->tryMakeBytesAvailableInternal(numBytes);
        return numBytes;
    }

    /*!
    Equivalent to `curByte[index]`, with bounds checking performed on `index` at runtime. The caller
    is responsible for ensuring that it's safe to read from the input buffer at this index by
    calling `tryMakeBytesAvailable()` beforehand.

        if (ins->tryMakeBytesAvailable() && ins->peekByte() == '.') {
            // Handle this input character...
            ins->advanceByte();
        }
    */
    PLY_INLINE char peekByte(u32 index = 0) const {
        PLY_ASSERT(index < (uptr)(this->endByte - this->curByte));
        return this->curByte[index];
    }

    /*!
    Equivalent to `curByte += numBytes`, with bounds checking performed on `numBytes` at runtime.
    The caller is responsible for ensuring that there are actually `numBytes` available in the input
    buffer by calling `tryMakeBytesAvailable()` beforehand.
    */
    PLY_INLINE void advanceByte(u32 numBytes = 1) {
        PLY_ASSERT(numBytes <= (uptr)(this->endByte - this->curByte));
        this->curByte += numBytes;
    }

    /*!
    Reads and returns the next byte from the input stream, or `0` if no more bytes are available.
    Use `atEOF()` to determine whether the read was actually successful.
    */
    PLY_INLINE u8 readByte() {
        if (!tryMakeBytesAvailable())
            return 0;
        return *this->curByte++;
    }

    PLY_DLL_ENTRY bool readSlowPath(BufferView dst);

    /*!
    Attempts to fill `dst` with data from the input stream. If the underlying `InPipe` is waiting
    for data, this function will block until `dst` is filled. Returns `true` if the buffer is filled
    successfully. If EOF/error is encountered before `dst` can be filled, the remainder of `dst` is
    filled with zeros and `false` is returned.
    */
    PLY_INLINE bool read(BufferView dst) {
        if (dst.numBytes > safeDemote<u32>(this->endByte - this->curByte)) {
            return this->readSlowPath(dst);
        }
        // If dst.numBytes is small and known at compile time, we count on the compiler to
        // implement memcpy() as a MOV:
        memcpy(dst.bytes, this->curByte, dst.numBytes);
        this->curByte += dst.numBytes;
        return this->status.eof == 0;
    }

    /*!
    Reads all the remaining data from the input stream and returns the contents as a `Buffer`.
    */
    PLY_DLL_ENTRY Buffer readRemainingContents();

    /*!
    \beginGroup
    Casts the `InStream` to various other types. It's always legal to call `asStringReader()`, but
    it's only legal to call the other functions if `isView()` returns `true`.
    */
    PLY_INLINE ViewInStream* asViewInStream();
    PLY_INLINE const ViewInStream* asViewInStream() const;
    PLY_INLINE StringReader* asStringReader();
    PLY_INLINE StringViewReader* asStringViewReader();
    /*!
    \endGroup
    */
};

//------------------------------------------------------------------
// ViewInStream
//------------------------------------------------------------------
struct ViewInStream : InStream {
    PLY_INLINE ViewInStream() = default;

    PLY_INLINE ViewInStream(ConstBufferView view) : InStream{Type::View, 0} {
        this->startByte = (u8*) view.bytes;
        this->curByte = (u8*) view.bytes;
        this->endByte = (u8*) view.bytes + view.numBytes;
        this->reserved = nullptr;
    }

    PLY_INLINE ~ViewInStream() {
        PLY_ASSERT(this->isView());
        // This lets the compiler optimize away the call to destructInternal():
        this->status.type = (u32) Type::View;
    }

    PLY_INLINE void operator=(const ViewInStream& other) {
        PLY_ASSERT(this->isView() && other.isView());
        this->startByte = other.startByte;
        this->curByte = other.curByte;
        this->endByte = other.endByte;
        this->status = other.status;
    }

    struct SavePoint {
        u8* startByte = nullptr;

        PLY_INLINE SavePoint(const InStream* ins) : startByte{ins->curByte} {
        }
    };

    PLY_INLINE SavePoint savePoint() const {
        PLY_ASSERT(this->isView());
        return {this};
    }

    PLY_INLINE ConstBufferView getViewFrom(const SavePoint& savePoint) const {
        PLY_ASSERT(this->isView());
        PLY_ASSERT(uptr(this->curByte - savePoint.startByte) <=
                   uptr(this->endByte - this->startByte));
        return ConstBufferView::fromRange(savePoint.startByte, this->curByte);
    }

    PLY_INLINE void restore(const SavePoint& savePoint) {
        PLY_ASSERT(this->isView());
        PLY_ASSERT(uptr(this->curByte - savePoint.startByte) <=
                   uptr(this->endByte - this->startByte));
        this->curByte = savePoint.startByte;
    }

    PLY_INLINE const u8* getStartByte() const {
        PLY_ASSERT(this->isView());
        return this->startByte;
    }
};

PLY_INLINE ViewInStream* InStream::asViewInStream() {
    PLY_ASSERT(this->isView());
    return static_cast<ViewInStream*>(this);
}

PLY_INLINE const ViewInStream* InStream::asViewInStream() const {
    PLY_ASSERT(this->isView());
    return static_cast<const ViewInStream*>(this);
}

//------------------------------------------------------------------
// NativeEndianReader
//------------------------------------------------------------------
class NativeEndianReader {
public:
    InStream* ins;

    PLY_INLINE NativeEndianReader(InStream* ins) : ins{ins} {
    }

    template <typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value>>
    PLY_INLINE T read() {
        T value;
        ins->read({&value, sizeof(value)});
        return value;
    }
};

} // namespace ply
