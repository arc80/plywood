/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Reference.h>
#include <ply-runtime/string/StringView.h>
#include <ply-runtime/container/LambdaView.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/LambdaView.h>

namespace ply {

struct ChunkCursor;
struct OutStream;

//-----------------------------------------------------------------
// ChunkListNode
// A building block for manipulating chunks of memory, possibly (but not necessarily) chained
// together.
//-----------------------------------------------------------------
struct ChunkListNode {
    static const u32 DefaultAlignment = 16;
    static const u32 DefaultChunkSize = 2048;

    u64 fileOffset = 0; // Offset of the first byte in this chunk relative to underlying file
    char* bytes = nullptr;
    Reference<ChunkListNode> next = nullptr;
    u32 numBytes = 0;            // Number of bytes allocated in this chunk
    u32 writePos = 0;            // Write position within this chunk
    u32 offsetIntoNextChunk = 0; // Where to continue in the next chunk
    mutable s32 refCount = 0;

    static PLY_INLINE u32 getAlignedSize() {
        return alignPowerOf2((u32) sizeof(ChunkListNode), DefaultAlignment);
    }

    PLY_INLINE MutableStringView viewAll() {
        return {this->bytes, this->numBytes};
    }
    PLY_INLINE StringView viewAll() const {
        return {this->bytes, this->numBytes};
    }
    PLY_INLINE StringView viewUsedBytes() const {
        return {this->bytes, this->writePos};
    }
    PLY_INLINE MutableStringView viewRemainingBytes() {
        return {this->bytes + this->writePos, this->numBytes - this->writePos};
    }
    PLY_INLINE char* end() {
        return this->bytes + this->numBytes;
    }

    PLY_INLINE void incRef() {
        this->refCount++;
    }
    PLY_DLL_ENTRY void decRef();

    static PLY_DLL_ENTRY Reference<ChunkListNode> allocate(u64 fileOffset, u32 numBytes);
    static PLY_DLL_ENTRY void addChunkToTail(Reference<ChunkListNode>& nodeRef, u32 numBytes);
};

//-----------------------------------------------------------------
// ChunkCursor
//-----------------------------------------------------------------
struct ChunkCursor {
    Reference<ChunkListNode> chunk; // allowed to be null, in which case it's a raw pointer
    char* curByte = nullptr;

    PLY_INLINE ChunkCursor() {
    }
    PLY_INLINE ChunkCursor(const ChunkCursor& other) : chunk{other.chunk}, curByte{other.curByte} {
    }
    PLY_INLINE ChunkCursor(ChunkCursor&& other)
        : chunk{std::move(other.chunk)}, curByte{other.curByte} {
        other.curByte = nullptr;
    }
    PLY_INLINE ChunkCursor(ChunkListNode* chunk, char* curByte) : chunk{chunk}, curByte{curByte} {
    }
    PLY_INLINE ChunkCursor(Reference<ChunkListNode>&& chunk, char* curByte)
        : chunk{std::move(chunk)}, curByte{curByte} {
    }
    PLY_INLINE void operator=(const ChunkCursor& other) {
        this->chunk = other.chunk;
        this->curByte = other.curByte;
    }
    PLY_INLINE void operator=(ChunkCursor&& other) {
        this->chunk = std::move(other.chunk);
        this->curByte = other.curByte;
        other.curByte = nullptr;
    }
    PLY_INLINE bool operator!=(const ChunkCursor& other) const {
        return this->chunk != other.chunk || this->curByte != other.curByte;
    }
    PLY_INLINE StringView viewAvailable() const {
        return StringView::fromRange(this->curByte, this->chunk->bytes + this->chunk->writePos);
    }
    PLY_DLL_ENTRY void advanceToNextChunk();
    PLY_INLINE void advanceBytes(u32 numBytes) {
        this->curByte += numBytes;
        PLY_ASSERT(this->chunk->viewUsedBytes().contains(this->curByte));
        if (this->curByte >= this->chunk->bytes + this->chunk->writePos) {
            this->advanceToNextChunk();
        }
    }
    PLY_DLL_ENTRY void iterateOverViews(LambdaView<void(StringView)> callback,
                                        const ChunkCursor& end) const;
    /*!
    Returns a `String` that owns a newly allocated block of memory containing a copy of the data
    contained in some (possible multiple) chunks between the `start` and `end` cursors. If `end` is not specified,
    the returned string contains a copy of the data from `start` up to the end of the chunk list. 

    The first argument must be an rvalue reference to a `ChunkCursor` which might be moved from
    (reset to an empty state) as a result of this call. This requirement enables an optimization: If
    `start` is the sole reference to the start of a single `ChunkListNode`, no data is copied;
    instead, the chunk's memory block is truncated and ownership is passed directly to the `String`.

    This function is used internally by `StringOutStream::moveToString()`.
    */
    static PLY_DLL_ENTRY String toString(ChunkCursor&& start, const ChunkCursor& end = {});
    PLY_DLL_ENTRY void writeToStream(OutStream* outs, const ChunkCursor& end = {}) const;
};

} // namespace ply
