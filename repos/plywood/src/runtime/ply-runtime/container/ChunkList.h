/*------------------------------------
  ///\  Plywood C++ Framework
  \\\/  https://plywood.arc80.com/
------------------------------------*/
#pragma once
#include <ply-runtime/Core.h>
#include <ply-runtime/container/Reference.h>
#include <ply-runtime/container/BufferView.h>
#include <ply-runtime/container/LambdaView.h>
#include <ply-runtime/string/String.h>
#include <ply-runtime/container/LambdaView.h>

namespace ply {

struct ChunkCursor;
struct Buffer;
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
    u8* bytes = nullptr;
    Reference<ChunkListNode> next = nullptr;
    u32 numBytes = 0;            // Number of bytes allocated in this chunk
    u32 writePos = 0;            // Write position within this chunk
    u32 offsetIntoNextChunk = 0; // Where to continue in the next chunk
    mutable s32 refCount = 0;

    static PLY_INLINE u32 getAlignedSize() {
        return alignPowerOf2((u32) sizeof(ChunkListNode), DefaultAlignment);
    }

    PLY_INLINE BufferView viewAll() {
        return {this->bytes, this->numBytes};
    }
    PLY_INLINE ConstBufferView viewAll() const {
        return {this->bytes, this->numBytes};
    }
    PLY_INLINE ConstBufferView viewUsedBytes() const {
        return {this->bytes, this->writePos};
    }
    PLY_INLINE BufferView viewRemainingBytes() {
        return {this->bytes + this->writePos, this->numBytes - this->writePos};
    }
    PLY_INLINE u8* end() {
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
    u8* curByte = nullptr;

    PLY_INLINE ChunkCursor() {
    }
    PLY_INLINE ChunkCursor(const ChunkCursor& other) : chunk{other.chunk}, curByte{other.curByte} {
    }
    PLY_INLINE ChunkCursor(ChunkCursor&& other)
        : chunk{std::move(other.chunk)}, curByte{other.curByte} {
        other.curByte = nullptr;
    }
    PLY_INLINE ChunkCursor(ChunkListNode* chunk, u8* curByte) : chunk{chunk}, curByte{curByte} {
    }
    PLY_INLINE ChunkCursor(Reference<ChunkListNode>&& chunk, u8* curByte)
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
    PLY_INLINE ConstBufferView viewAvailable() const {
        return ConstBufferView::fromRange(this->curByte,
                                          this->chunk->bytes + this->chunk->writePos);
    }
    PLY_DLL_ENTRY void advanceToNextChunk();
    PLY_INLINE void advanceBytes(u32 numBytes) {
        this->curByte += numBytes;
        PLY_ASSERT(this->chunk->viewUsedBytes().contains(this->curByte));
        if (this->curByte >= this->chunk->bytes + this->chunk->writePos) {
            this->advanceToNextChunk();
        }
    }
    PLY_DLL_ENTRY void iterateOverViews(LambdaView<void(ConstBufferView)> callback,
                                        const ChunkCursor& end) const;
    static PLY_DLL_ENTRY Buffer toBuffer(ChunkCursor&& start, const ChunkCursor& end = {});
    PLY_DLL_ENTRY void writeToStream(OutStream* outs, const ChunkCursor& end = {}) const;
};

PLY_INLINE Buffer Buffer::fromChunks(ChunkCursor&& start) {
    return ChunkCursor::toBuffer(std::move(start));
}

PLY_INLINE Buffer Buffer::fromChunks(ChunkCursor&& start, const ChunkCursor& end) {
    return ChunkCursor::toBuffer(std::move(start), end);
}

PLY_INLINE String String::fromChunks(ChunkCursor&& start) {
    return String::moveFromBuffer(ChunkCursor::toBuffer(std::move(start)));
}

PLY_INLINE String String::fromChunks(ChunkCursor&& start, const ChunkCursor& end) {
    return String::moveFromBuffer(ChunkCursor::toBuffer(std::move(start), end));
}

} // namespace ply
